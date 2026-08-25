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

        public double CurrentTime { get; private set; } = 0.0;

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
            if (IsNumericLiteral(expr)) return;
            if (boolVars.ContainsKey(expr)) return;
            if (!numVars.ContainsKey(expr)) numVars[expr] = 0.0;
        }

        private void AddUnknownVariable(string? expr)
        {
            if (string.IsNullOrWhiteSpace(expr)) return;
            if (boolVars.ContainsKey(expr) || numVars.ContainsKey(expr)) return;
            if (IsNumericLiteral(expr)) return;
            numVars[expr] = 0.0;
        }

        private static bool IsNumericLiteral(string s) => double.TryParse(s, out _);

        public void SetBool(string name, bool value) => boolVars[name] = value;
        public void SetNumeric(string name, double value) => numVars[name] = value;
        public void AdvanceTime(double ms) => CurrentTime += ms;

        public void Reset()
        {
            foreach (var key in boolVars.Keys.ToList()) boolVars[key] = false;
            foreach (var key in numVars.Keys.ToList()) numVars[key] = 0.0;
            timers.Clear();
            counters.Clear();
            CurrentTime = 0.0;
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
            foreach (var network in networks)
                ProcessNetwork(network);
        }

        private void ProcessNetwork(LadderNetwork network)
        {
            foreach (var rung in network.Rungs)
            {
                bool powerFlow = EvaluateRungCondition(rung);

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
        }

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

        private bool EvaluateRungCondition(LadderRung rung)
        {
            if (rung.Branches.Count == 0) return true;

            foreach (var branch in rung.Branches)
            {
                bool branchResult = true;
                foreach (var elem in branch)
                {
                    if (!EvaluateElement(elem))
                    {
                        branchResult = false;
                        break;
                    }
                }
                if (branchResult) return true;
            }
            return false;
        }

        private bool EvaluateElement(LadderElement elem)
        {
            if (elem.Type == "contact")
            {
                if (elem.ContactType == "comparison")
                    return EvaluateComparison(elem.Op, elem.Left, elem.Right);
                else
                {
                    bool state = boolVars.TryGetValue(elem.Address, out bool val) && val;
                    return elem.ContactType == "NO" ? state : !state;
                }
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

        private double GetNumericValue(string expr)
        {
            if (double.TryParse(expr, out double literal)) return literal;
            if (numVars.TryGetValue(expr, out double val)) return val;
            if (boolVars.TryGetValue(expr, out bool b)) return b ? 1.0 : 0.0;
            return 0.0;
        }

        private bool GetBoolValue(string expr)
        {
            if (boolVars.TryGetValue(expr, out bool val)) return val;
            if (double.TryParse(expr, out double d)) return d != 0;
            return false;
        }
    }
}