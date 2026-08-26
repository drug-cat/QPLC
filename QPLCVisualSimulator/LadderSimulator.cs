using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;

namespace QPLCVisualSimulator
{
    public class LadderSimulator
    {
        public Dictionary<string, bool> GetBoolVariables() => boolVars;
        public Dictionary<string, double> GetNumericVariables() => numVars;

        private readonly List<LadderNetwork> networks;
        private readonly Dictionary<string, bool> boolVars = new();
        private readonly Dictionary<string, double> numVars = new();
        private readonly Dictionary<string, TimerInstance> timers = new();
        private readonly Dictionary<string, CounterInstance> counters = new();

        // وضعیت لبه هر کنتاکت (کلید = مرجع المان)
        private readonly Dictionary<object, bool> edgePrev = new();
        // آخرین وضعیت هدایت هر المان در اسکن — منبع واحد حقیقت برای رنگ‌آمیزی UI
        private readonly Dictionary<object, bool> lastConduction = new();

        public double CurrentTime { get; private set; } = 0.0;
        public long ScanCount { get; private set; } = 0;
        public string LastWarning { get; private set; } = "";
        public int MaxStepsPerScan { get; set; } = 100000;

        public LadderSimulator(List<LadderNetwork> networks)
        {
            this.networks = networks;
            InitializeVariables();
        }

        private void InitializeVariables()
        {
            foreach (var network in networks)
            {
                foreach (var rung in network.Rungs)
                {
                    foreach (var branch in rung.Branches)
                    {
                        foreach (var elem in branch)
                        {
                            if (elem.Type == "contact" && elem.ContactType != "comparison")
                                boolVars.TryAdd(elem.Address, false);
                            if (elem.Type == "contact" && elem.ContactType == "comparison")
                            {
                                AddComparisonVariable(elem.Left);
                                AddComparisonVariable(elem.Right);
                            }
                        }
                    }

                    if (rung.Coil != null && rung.Coil.Type == "coil")
                        boolVars.TryAdd(rung.Coil.Address, false);

                    if (rung.Move != null)
                    {
                        AddUnknownVariable(rung.Move.Dest);
                        AddUnknownVariable(rung.Move.Source);
                    }

                    if (rung.Timer != null)
                        boolVars.TryAdd(rung.Timer.Address, false);

                    if (rung.Counter != null)
                        boolVars.TryAdd(rung.Counter.Address, false);
                }
            }
        }

        private void AddComparisonVariable(string? expr)
        {
            if (string.IsNullOrWhiteSpace(expr)) return;
            if (!IsPlainIdentifier(expr)) return;
            if (IsNumericLiteral(expr)) return;
            if (boolVars.ContainsKey(expr)) return;
            if (!numVars.ContainsKey(expr)) numVars[expr] = 0.0;
        }

        private void AddUnknownVariable(string? expr)
        {
            if (string.IsNullOrWhiteSpace(expr)) return;
            if (!IsPlainIdentifier(expr)) return;
            if (boolVars.ContainsKey(expr) || numVars.ContainsKey(expr)) return;
            if (IsNumericLiteral(expr)) return;
            numVars[expr] = 0.0;
        }

        private static bool IsNumericLiteral(string s) => double.TryParse(s, out _);

        // فقط شناسه‌های ساده (متغیر/آدرس) متغیر ثبت می‌شوند، نه عبارت‌هایی مثل "(a + 1)"
        private static bool IsPlainIdentifier(string? s) =>
            !string.IsNullOrWhiteSpace(s) && Regex.IsMatch(s, "^[A-Za-z_][A-Za-z0-9_.]*$");

        public void SetBool(string name, bool value) => boolVars[name] = value;
        public void SetNumeric(string name, double value) => numVars[name] = value;
        public void AdvanceTime(double ms) => CurrentTime += ms;

        public void Reset()
        {
            foreach (var key in boolVars.Keys.ToList()) boolVars[key] = false;
            foreach (var key in numVars.Keys.ToList()) numVars[key] = 0.0;
            timers.Clear();
            counters.Clear();
            edgePrev.Clear();
            lastConduction.Clear();
            CurrentTime = 0.0;
            ScanCount = 0;
            LastWarning = "";
        }

        public void ShowVariables()
        {
            Console.WriteLine("Boolean variables:");
            foreach (var kvp in boolVars.OrderBy(k => k.Key))
                Console.WriteLine($"  {kvp.Key} = {kvp.Value}");
            Console.WriteLine("Numeric variables:");
            foreach (var kvp in numVars.OrderBy(k => k.Key))
                Console.WriteLine($"  {kvp.Key} = {kvp.Value}");
            Console.WriteLine($"Current simulation time: {CurrentTime} ms");
        }

        public void RunScan()
        {
            LastWarning = "";
            ScanCount++;
            foreach (var network in networks)
                ProcessNetwork(network);
        }

        // ------------------- Network execution with jumps -------------------

        private void ProcessNetwork(LadderNetwork network)
        {
            var rungs = network.Rungs;

            // جدول برچسب‌های این شبکه
            var labelIndex = new Dictionary<string, int>();
            for (int i = 0; i < rungs.Count; i++)
                if (rungs[i].Label != null)
                    labelIndex[rungs[i].Label!.Label] = i;

            int pc = 0;
            long steps = 0;
            while (pc < rungs.Count)
            {
                if (++steps > MaxStepsPerScan)
                {
                    LastWarning = $"Step limit ({MaxStepsPerScan}) exceeded in '{network.Name}' - possible infinite loop";
                    break;
                }

                var rung = rungs[pc];
                bool powerFlow = EvaluateRungCondition(rung);
                ProcessRungOutputs(rung, powerFlow);

                if (rung.Jump != null)
                {
                    // jmp: پرش همیشگی؛ jmpn: پرش فقط وقتی جریان برق قطع است
                    bool jumpNow = rung.Jump.JumpType == "jmp" || !powerFlow;
                    if (jumpNow && labelIndex.TryGetValue(rung.Jump.Label, out int target))
                    {
                        pc = target;
                        continue;
                    }
                }
                pc++;
            }
        }

        private void ProcessRungOutputs(LadderRung rung, bool powerFlow)
        {
            if (rung.Coil != null)
            {
                string coilVar = rung.Coil.Address;
                if (rung.Coil.ContactType == "reset")
                {
                    if (powerFlow) boolVars[coilVar] = false;
                }
                else if (rung.Coil.ContactType == "set")
                {
                    if (powerFlow) boolVars[coilVar] = true;
                }
                else
                {
                    boolVars[coilVar] = powerFlow;
                }
            }

            if (rung.Move != null && powerFlow)
            {
                string dest = rung.Move.Dest;
                string source = rung.Move.Source;
                double value = GetNumericValue(source);
                SetNumeric(dest, value);
            }

            if (rung.Timer != null)
                ProcessTimer(rung.Timer, powerFlow);

            if (rung.Counter != null)
                ProcessCounter(rung.Counter);
        }

        // ------------------- Timers -------------------

        private class TimerInstance
        {
            public string Type { get; set; } = "";
            public double DurationMs { get; set; }
            public bool InputState { get; set; }
            public bool PrevInput { get; set; }
            public double StartTime { get; set; }
            public bool Output { get; set; }
        }

        private void ProcessTimer(LadderElement timerElem, bool inputState)
        {
            string timerType = timerElem.ContactType;
            string outputVar = timerElem.Address;
            double durationMs = ParseTimeLiteral(timerElem.Source);

            if (!timers.TryGetValue(outputVar, out var timer))
            {
                timer = new TimerInstance
                {
                    Type = timerType,
                    DurationMs = durationMs,
                    InputState = inputState,
                    PrevInput = inputState,
                    StartTime = CurrentTime,
                    Output = false
                };
                timers[outputVar] = timer;
            }
            else
            {
                timer.Type = timerType;
                timer.DurationMs = durationMs;
                timer.PrevInput = timer.InputState;
                timer.InputState = inputState;
            }

            bool prev = timer.PrevInput;
            bool current = inputState;

            switch (timer.Type)
            {
                case "on_delay":
                    if (current)
                    {
                        if (!prev) timer.StartTime = CurrentTime;
                        timer.Output = (CurrentTime - timer.StartTime) >= timer.DurationMs;
                    }
                    else
                    {
                        timer.Output = false;
                        timer.StartTime = CurrentTime;
                    }
                    break;

                case "off_delay":
                    if (current)
                    {
                        timer.Output = true;
                        timer.StartTime = CurrentTime;
                    }
                    else
                    {
                        if (prev) timer.StartTime = CurrentTime;
                        timer.Output = (CurrentTime - timer.StartTime) < timer.DurationMs;
                    }
                    break;

                case "pulse":
                    if (current)
                    {
                        if (!prev) timer.StartTime = CurrentTime;
                        timer.Output = (CurrentTime - timer.StartTime) < timer.DurationMs;
                    }
                    else
                    {
                        timer.Output = false;
                    }
                    break;

                default:
                    timer.Output = false;
                    break;
            }

            boolVars[outputVar] = timer.Output;
        }

        // مقدار زمان سپری‌شده تایمر برای نمایش زنده در UI
        public double GetTimerElapsedMs(string outputVar)
        {
            return timers.TryGetValue(outputVar, out var t) ? Math.Max(0.0, CurrentTime - t.StartTime) : 0.0;
        }

        // ------------------- Counters -------------------

        private class CounterInstance
        {
            public string Type { get; set; } = "";
            public int Count { get; set; }
            public int Preset { get; set; }
            public bool PrevInput1 { get; set; }
            public bool PrevInput2 { get; set; }
            public bool PrevInput3 { get; set; }
            public bool PrevInput4 { get; set; }
            public bool Output { get; set; }
        }

        private void ProcessCounter(LadderElement counterElem)
        {
            string counterType = counterElem.ContactType;
            string outputVar = counterElem.Address;
            int preset = (int)GetNumericValue(counterElem.Preset);

            if (!counters.TryGetValue(outputVar, out var counter))
            {
                counter = new CounterInstance
                {
                    Type = counterType,
                    Count = 0,
                    Preset = preset,
                    PrevInput1 = false,
                    PrevInput2 = false,
                    PrevInput3 = false,
                    PrevInput4 = false,
                    Output = false
                };
                counters[outputVar] = counter;
            }
            else
            {
                counter.Type = counterType;
                counter.Preset = preset;
            }

            bool in1 = GetBoolValue(counterElem.Input1);
            bool in2 = GetBoolValue(counterElem.Input2);
            bool in3 = GetBoolValue(counterElem.Input3);
            bool in4 = GetBoolValue(counterElem.Input4);

            bool edge1 = in1 && !counter.PrevInput1;
            bool edge2 = in2 && !counter.PrevInput2;
            bool edge3 = in3 && !counter.PrevInput3;
            bool edge4 = in4 && !counter.PrevInput4;

            switch (counter.Type)
            {
                case "count_up":
                    if (in2) counter.Count = 0; // reset
                    else if (edge1) counter.Count++;
                    break;

                case "count_down":
                    if (in2) counter.Count = counter.Preset; // load
                    else if (edge1) counter.Count--;
                    break;

                case "count_updown":
                    if (in3) counter.Count = 0; // reset
                    else if (in4) counter.Count = counter.Preset; // load
                    else
                    {
                        if (edge1) counter.Count++;
                        if (edge2) counter.Count--;
                    }
                    break;
            }

            // محدودیت شمارنده
            if (counter.Count < 0) counter.Count = 0;
            if (counter.Count > 32767) counter.Count = 32767;

            counter.Output = counter.Count >= counter.Preset;

            counter.PrevInput1 = in1;
            counter.PrevInput2 = in2;
            counter.PrevInput3 = in3;
            counter.PrevInput4 = in4;

            boolVars[outputVar] = counter.Output;
        }

        // مقدار فعلی شمارنده برای نمایش زنده در UI
        public int GetCounterValue(string outputVar)
        {
            return counters.TryGetValue(outputVar, out var c) ? c.Count : 0;
        }

        private static double ParseTimeLiteral(string timeStr)
        {
            if (string.IsNullOrWhiteSpace(timeStr) || !timeStr.StartsWith("T#"))
                return 0;

            string body = timeStr.Substring(2).Trim();
            var match = Regex.Match(body, @"(?<num>[\d\.]+)(?<unit>ms|s|m|h)");
            if (!match.Success) return 0;

            double num = double.Parse(match.Groups["num"].Value);
            string unit = match.Groups["unit"].Value.ToLower();

            return unit switch
            {
                "ms" => num,
                "s" => num * 1000.0,
                "m" => num * 60_000.0,
                "h" => num * 3_600_000.0,
                _ => 0
            };
        }

        // ------------------- Rung / contact evaluation -------------------

        private bool EvaluateRungCondition(LadderRung rung)
        {
            if (rung.Branches.Count == 0) return true;

            bool anyTrue = false;
            foreach (var branch in rung.Branches)
            {
                bool branchResult = true;
                foreach (var elem in branch)
                {
                    if (!EvaluateElement(elem))
                    {
                        branchResult = false;
                    }
                }
                if (branchResult) anyTrue = true;
            }
            return anyTrue;
        }

        // ارزیابی بدون اثر جانبی — فقط برای پرس‌وجوی UI خارج از اسکن
        private bool EvaluateContactStateless(LadderElement elem)
        {
            if (elem.Type != "contact") return false;
            if (elem.ContactType == "comparison")
                return EvaluateComparison(elem.Op, elem.Left, elem.Right);
            if (elem.ContactType == "rising" || elem.ContactType == "falling")
                return false; // لبه خارج از اسکن وضعیت ندارد
            bool state = boolVars.TryGetValue(elem.Address, out bool val) && val;
            return elem.ContactType == "NO" ? state : !state;
        }

        // منبع واحد حقیقت برای رنگ‌آمیزی نمای گرافیکی
        public bool IsContactActive(LadderElement elem)
        {
            if (lastConduction.TryGetValue(elem, out bool cached)) return cached;
            return EvaluateContactStateless(elem);
        }

        private bool CacheConduction(LadderElement elem, bool value)
        {
            lastConduction[elem] = value;
            return value;
        }

        private bool EvaluateElement(LadderElement elem)
        {
            if (elem.Type == "contact")
            {
                if (elem.ContactType == "comparison")
                    return CacheConduction(elem, EvaluateComparison(elem.Op, elem.Left, elem.Right));

                if (elem.ContactType == "rising" || elem.ContactType == "falling")
                {
                    // تشخیص لبه با حافظه وضعیت قبلی (کلید = مرجع المان)
                    bool cur = boolVars.TryGetValue(elem.Address, out bool cv) && cv;
                    bool prev = edgePrev.TryGetValue(elem, out bool pv) && pv;
                    bool result = elem.ContactType == "rising" ? (cur && !prev) : (!cur && prev);
                    edgePrev[elem] = cur;
                    return CacheConduction(elem, result);
                }

                bool state = boolVars.TryGetValue(elem.Address, out bool val) && val;
                return CacheConduction(elem, elem.ContactType == "NO" ? state : !state);
            }
            return true;
        }

        private bool EvaluateComparison(string op, string left, string right)
        {
            double l = GetNumericValue(left);
            double r = GetNumericValue(right);
            return op switch
            {
                "eq" => l == r,
                "ne" => l != r,
                "gt" => l > r,
                "lt" => l < r,
                "ge" => l >= r,
                "le" => l <= r,
                _ => false
            };
        }

        // ------------------- Numeric expression evaluator -------------------

        public double GetNumericValue(string expr)
        {
            if (double.TryParse(expr, out double literal)) return literal;

            string src = expr.Trim();
            int pos = 0;
            try
            {
                double value = EvalAddSub(src, ref pos);
                if (pos == src.Length) return value;
            }
            catch (FormatException)
            {
                // عبارت قابل ارزیابی نبود → جستجوی متغیر
            }

            if (numVars.TryGetValue(expr, out double val)) return val;
            if (boolVars.TryGetValue(expr, out bool b)) return b ? 1.0 : 0.0;
            return 0.0;
        }

        private static bool IsIdentChar(char c) =>
            char.IsLetterOrDigit(c) || c == '_' || c == '.';

        private double EvalAddSub(string s, ref int pos)
        {
            double left = EvalMulDiv(s, ref pos);
            while (pos < s.Length && (s[pos] == '+' || s[pos] == '-'))
            {
                char op = s[pos++];
                double right = EvalMulDiv(s, ref pos);
                left = op == '+' ? left + right : left - right;
            }
            return left;
        }

        private double EvalMulDiv(string s, ref int pos)
        {
            double left = EvalUnary(s, ref pos);
            while (pos < s.Length && (s[pos] == '*' || s[pos] == '/' || s[pos] == '%'))
            {
                char op = s[pos++];
                double right = EvalUnary(s, ref pos);
                if (op == '*') left *= right;
                else if (op == '/') left /= right;
                else left %= right;
            }
            return left;
        }

        private double EvalUnary(string s, ref int pos)
        {
            SkipSpaces(s, ref pos);
            if (pos < s.Length && s[pos] == '-')
            {
                pos++;
                return -EvalUnary(s, ref pos);
            }
            if (pos < s.Length && s[pos] == '+')
            {
                pos++;
                return EvalUnary(s, ref pos);
            }
            return EvalPrimary(s, ref pos);
        }

        private double EvalPrimary(string s, ref int pos)
        {
            SkipSpaces(s, ref pos);
            if (pos >= s.Length) throw new FormatException();

            // پرانتز
            if (s[pos] == '(')
            {
                pos++;
                double v = EvalAddSub(s, ref pos);
                SkipSpaces(s, ref pos);
                if (pos >= s.Length || s[pos] != ')') throw new FormatException();
                pos++;
                return v;
            }

            // عدد
            int numStart = pos;
            while (pos < s.Length && (char.IsDigit(s[pos]) || s[pos] == '.')) pos++;
            if (pos > numStart)
                return double.Parse(s.Substring(numStart, pos - numStart));

            // شناسه (متغیر بولی یا عددی؛ آدرس‌هایی مثل Q0.0 هم شناسه محسوب می‌شوند)
            int idStart = pos;
            while (pos < s.Length && IsIdentChar(s[pos])) pos++;
            if (pos > idStart)
            {
                string name = s.Substring(idStart, pos - idStart);
                if (numVars.TryGetValue(name, out double nv)) return nv;
                if (boolVars.TryGetValue(name, out bool bv)) return bv ? 1.0 : 0.0;
                throw new FormatException();
            }

            throw new FormatException();
        }

        private static void SkipSpaces(string s, ref int pos)
        {
            while (pos < s.Length && s[pos] == ' ') pos++;
        }

        private bool GetBoolValue(string expr)
        {
            if (boolVars.TryGetValue(expr, out bool val)) return val;
            if (double.TryParse(expr, out double d)) return d != 0;
            return false;
        }
    }
}
