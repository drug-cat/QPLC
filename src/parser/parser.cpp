using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Shapes;

namespace QPLCVisualSimulator
{
    public partial class LadderView : UserControl
    {
        private const double RailLeftX = 30;
        private const double RailRightX = 850;
        private const double ElementWidth = 50;      // کاهش از 60 به 50
        private const double RungHeight = 50;        // کاهش از 60 به 50
        private const double ContactWidth = 8;
        private const double ContactHeight = 30;
        private const double VerticalGap = 20;       // فاصله بین رانگ‌ها

        private List<LadderNetwork> networks = new();
        private LadderSimulator simulator = null!;

        public LadderView()
        {
            InitializeComponent();
        }

        public void SetSimulator(LadderSimulator sim)
        {
            simulator = sim;
        }

        public void DrawNetworks(List<LadderNetwork> nets)
        {
            networks = nets;
            LadderCanvas.Children.Clear();

            double y = 10;
            foreach (var network in networks)
            {
                // عنوان شبکه
                var title = new TextBlock
                {
                    Text = network.Name,
                    FontWeight = FontWeights.Bold,
                    FontSize = 12,
                    Foreground = Brushes.DarkSlateBlue
                };
                Canvas.SetLeft(title, RailLeftX);
                Canvas.SetTop(title, y);
                LadderCanvas.Children.Add(title);
                y += 20;

                foreach (var rung in network.Rungs)
                {
                    DrawRung(rung, y);
                    y += RungHeight + VerticalGap;
                }
                y += 10; // فاصله اضافی بین شبکه‌ها
            }

            // تنظیم اندازه Canvas بر اساس محتوا
            double totalHeight = y + 20;
            double totalWidth = RailRightX + 100;
            LadderCanvas.Width = totalWidth;
            LadderCanvas.Height = totalHeight;
        }

        // ---------------------- Helper evaluation ----------------------
        private bool IsContactConducting(LadderElement elem)
        {
            if (simulator == null) return false;

            if (elem.Type == "contact")
            {
                if (elem.ContactType == "comparison")
                    return EvaluateComparison(elem);
                else
                {
                    var bools = simulator.GetBoolVariables();
                    bool state = bools.TryGetValue(elem.Address, out bool val) && val;
                    return elem.ContactType == "NO" ? state : !state;
                }
            }
            return false;
        }

        private bool EvaluateComparison(LadderElement elem)
        {
            double l = GetNumericValue(elem.Left);
            double r = GetNumericValue(elem.Right);
            return elem.Op switch
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
            var numVars = simulator.GetNumericVariables();
            if (numVars.TryGetValue(expr, out double val)) return val;
            var boolVars = simulator.GetBoolVariables();
            if (boolVars.TryGetValue(expr, out bool b)) return b ? 1.0 : 0.0;
            return 0.0;
        }

        private bool IsBranchTrue(List<LadderElement> branch)
        {
            foreach (var elem in branch)
            {
                if (!IsContactConducting(elem))
                    return false;
            }
            return true;
        }

        private Brush ActiveBrush => Brushes.Green;
        private Brush InactiveBrush => Brushes.Gray;

        // ---------------------- Drawing methods ----------------------
        private void DrawRung(LadderRung rung, double y)
        {
            // ریل‌ها
            DrawRail(RailLeftX, y, RungHeight);
            DrawRail(RailRightX, y, RungHeight);

            double centerY = y + RungHeight / 2;
            double startX = RailLeftX + 15;
            double outputLeft;

            // تعیین محل شروع المان خروجی
            if (rung.Coil != null)
                outputLeft = RailRightX - 70;
            else if (rung.Timer != null)
                outputLeft = RailRightX - 110;
            else if (rung.Counter != null)
                outputLeft = RailRightX - 130;
            else
                outputLeft = RailRightX - 40;

            // وضعیت شاخه‌ها
            var branchStates = rung.Branches.Select(b => IsBranchTrue(b)).ToList();
            bool rungTrue = branchStates.Any(b => b);

            // اتصال از ریل چپ به شروع شاخه‌ها
            DrawHorizontalLine(RailLeftX, centerY, startX, centerY, rungTrue);

            // رسم شاخه‌ها تا خروجی
            DrawBranches(rung, startX, centerY, branchStates, rungTrue, outputLeft);

            // رسم المان خروجی
            if (rung.Coil != null)
                DrawCoil(rung.Coil, outputLeft, centerY, rungTrue);
            else if (rung.Timer != null)
                DrawTimer(rung.Timer, outputLeft, centerY, rungTrue);
            else if (rung.Counter != null)
                DrawCounter(rung.Counter, outputLeft, centerY, rungTrue);

            // اتصال از خروجی به ریل راست
            double outputRight = outputLeft + GetOutputElementWidth(rung);
            DrawHorizontalLine(outputRight, centerY, RailRightX, centerY, rungTrue);
        }

        private double GetOutputElementWidth(LadderRung rung)
        {
            if (rung.Coil != null) return 40;
            if (rung.Timer != null) return 80;
            if (rung.Counter != null) return 100;
            return 30;
        }

        private void DrawRail(double x, double y, double height)
        {
            var rail = new Line
            {
                X1 = x, Y1 = y,
                X2 = x, Y2 = y + height,
                Stroke = Brushes.Black,
                StrokeThickness = 4
            };
            LadderCanvas.Children.Add(rail);
        }

        private void DrawBranches(LadderRung rung, double startX, double centerY,
            List<bool> branchStates, bool rungTrue, double outputLeft)
        {
            if (rung.Branches.Count == 0)
            {
                DrawHorizontalLine(startX, centerY, outputLeft, centerY, rungTrue);
                return;
            }

            if (rung.Branches.Count == 1)
            {
                // سری ساده
                var branch = rung.Branches[0];
                bool branchTrue = branchStates[0];
                double x = startX;

                DrawHorizontalLine(x, centerY, x + 8, centerY, branchTrue);
                x += 8;

                for (int i = 0; i < branch.Count; i++)
                {
                    DrawElement(branch[i], x, centerY - ContactHeight / 2, branchTrue);
                    x += ElementWidth;
                    DrawHorizontalLine(x, centerY, x + 8, centerY, branchTrue);
                    x += 8;
                }

                DrawHorizontalLine(x, centerY, outputLeft, centerY, branchTrue);
            }
            else
            {
                // شاخه‌های موازی (OR)
                double branchSpacing = (RungHeight) / (rung.Branches.Count + 1);
                List<double> branchYs = new();
                for (int i = 0; i < rung.Branches.Count; i++)
                    branchYs.Add(centerY - RungHeight / 2 + (i + 1) * branchSpacing);

                double commonStartX = startX;
                double commonEndX = startX + ElementWidth * rung.Branches.Max(b => b.Count) + 16;

                // اتصال عمودی از خط اصلی به شاخه‌ها
                for (int i = 0; i < rung.Branches.Count; i++)
                {
                    DrawVerticalLine(commonStartX, branchYs[i], centerY, branchStates[i]);
                }

                // رسم هر شاخه
                for (int i = 0; i < rung.Branches.Count; i++)
                {
                    double by = branchYs[i];
                    bool branchTrue = branchStates[i];
                    double x = commonStartX;

                    DrawHorizontalLine(x, by, x + 8, by, branchTrue);
                    x += 8;

                    for (int j = 0; j < rung.Branches[i].Count; j++)
                    {
                        DrawElement(rung.Branches[i][j], x, by - ContactHeight / 2, branchTrue);
                        x += ElementWidth;
                        DrawHorizontalLine(x, by, x + 8, by, branchTrue);
                        x += 8;
                    }

                    // اتصال عمودی از انتهای شاخه به خط مشترک
                    DrawVerticalLine(x, by, centerY, branchTrue);
                }

                // خط افقی مشترک تا خروجی
                DrawHorizontalLine(commonEndX, centerY, outputLeft, centerY, rungTrue);
            }
        }

        private void DrawElement(LadderElement elem, double x, double y, bool conducting)
        {
            if (elem.Type == "contact")
                DrawContact(elem, x, y, conducting);
            else if (elem.Type == "comparison")
                DrawComparison(elem, x, y, conducting);
        }

        private void DrawContact(LadderElement elem, double x, double y, bool conducting)
        {
            Brush brush = conducting ? ActiveBrush : InactiveBrush;

            var line1 = new Line { X1 = x, Y1 = y, X2 = x, Y2 = y + ContactHeight, Stroke = brush, StrokeThickness = 2 };
            var line2 = new Line { X1 = x + ContactWidth, Y1 = y, X2 = x + ContactWidth, Y2 = y + ContactHeight, Stroke = brush, StrokeThickness = 2 };
            LadderCanvas.Children.Add(line1);
            LadderCanvas.Children.Add(line2);

            if (elem.ContactType == "NC")
            {
                var slash = new Line { X1 = x, Y1 = y + ContactHeight, X2 = x + ContactWidth, Y2 = y, Stroke = brush, StrokeThickness = 1.5 };
                LadderCanvas.Children.Add(slash);
            }

            var label = new TextBlock { Text = elem.Address, FontSize = 9, Foreground = Brushes.Blue };
            Canvas.SetLeft(label, x - 3);
            Canvas.SetTop(label, y + ContactHeight + 2);
            LadderCanvas.Children.Add(label);

            // منطقه کلیک
            var clickArea = new Rectangle
            {
                Width = ContactWidth + 4,
                Height = ContactHeight + 4,
                Fill = Brushes.Transparent,
                Tag = elem
            };
            clickArea.MouseLeftButtonDown += (s, e) =>
            {
                if (simulator != null)
                {
                    var varName = elem.Address;
                    var bools = simulator.GetBoolVariables();
                    if (bools.ContainsKey(varName))
                    {
                        bool newVal = !bools[varName];
                        simulator.SetBool(varName, newVal);
                        DrawNetworks(networks);
                    }
                }
            };
            Canvas.SetLeft(clickArea, x - 2);
            Canvas.SetTop(clickArea, y - 2);
            LadderCanvas.Children.Add(clickArea);
        }

        private void DrawComparison(LadderElement elem, double x, double y, bool conducting)
        {
            Brush brush = conducting ? ActiveBrush : InactiveBrush;
            var box = new Rectangle { Width = 40, Height = 25, Fill = Brushes.White, Stroke = brush, StrokeThickness = 2 };
            Canvas.SetLeft(box, x);
            Canvas.SetTop(box, y + 2);
            LadderCanvas.Children.Add(box);

            var text = new TextBlock { Text = $"{elem.Left} {elem.Op} {elem.Right}", FontSize = 9, Foreground = Brushes.DarkGreen };
            Canvas.SetLeft(text, x + 2);
            Canvas.SetTop(text, y + 4);
            LadderCanvas.Children.Add(text);
        }

        private void DrawCoil(LadderElement elem, double x, double centerY, bool rungTrue)
        {
            bool output = simulator != null && simulator.GetBoolVariables().TryGetValue(elem.Address, out bool val) && val;
            Brush brush = output ? ActiveBrush : InactiveBrush;

            string symbol = elem.ContactType == "reset" ? "(R)" : elem.ContactType == "set" ? "(S)" : "( )";
            var coilText = new TextBlock
            {
                Text = $"{symbol} {elem.Address}",
                FontWeight = FontWeights.Bold,
                FontSize = 11,
                Foreground = brush
            };
            Canvas.SetLeft(coilText, x);
            Canvas.SetTop(coilText, centerY - 8);
            LadderCanvas.Children.Add(coilText);
        }

        private void DrawTimer(LadderElement elem, double x, double centerY, bool rungTrue)
        {
            bool output = simulator != null && simulator.GetBoolVariables().TryGetValue(elem.Address, out bool val) && val;
            Brush brush = output ? ActiveBrush : InactiveBrush;

            var boxWidth = 80;
            var boxHeight = 50;
            var boxY = centerY - boxHeight / 2;
            var box = new Rectangle { Width = boxWidth, Height = boxHeight, Fill = Brushes.LightYellow, Stroke = brush, StrokeThickness = 2 };
            Canvas.SetLeft(box, x);
            Canvas.SetTop(box, boxY);
            LadderCanvas.Children.Add(box);

            var title = new TextBlock { Text = elem.ContactType, FontWeight = FontWeights.Bold, FontSize = 10 };
            Canvas.SetLeft(title, x + 12);
            Canvas.SetTop(title, boxY + 18);
            LadderCanvas.Children.Add(title);

            DrawPin(x, boxY + boxHeight / 2, "IN", elem.Address, brush);
            DrawPin(x + boxWidth, boxY + boxHeight / 2, "Q", elem.Address, brush);
            DrawPin(x + boxWidth / 2, boxY, "PT", elem.Source, brush);
            DrawPin(x + boxWidth / 2, boxY + boxHeight, "ET", "", brush);
        }

        private void DrawCounter(LadderElement elem, double x, double centerY, bool rungTrue)
        {
            bool output = simulator != null && simulator.GetBoolVariables().TryGetValue(elem.Address, out bool val) && val;
            Brush brush = output ? ActiveBrush : InactiveBrush;

            var boxWidth = 100;
            var boxHeight = 70;
            var boxY = centerY - boxHeight / 2;
            var box = new Rectangle { Width = boxWidth, Height = boxHeight, Fill = Brushes.LightCyan, Stroke = brush, StrokeThickness = 2 };
            Canvas.SetLeft(box, x);
            Canvas.SetTop(box, boxY);
            LadderCanvas.Children.Add(box);

            var title = new TextBlock { Text = elem.ContactType, FontWeight = FontWeights.Bold, FontSize = 10 };
            Canvas.SetLeft(title, x + 20);
            Canvas.SetTop(title, boxY + 25);
            LadderCanvas.Children.Add(title);

            if (elem.ContactType == "count_up")
            {
                DrawPin(x, boxY + 15, "CU", elem.Input1, brush);
                DrawPin(x, boxY + 50, "R", elem.Input2, brush);
                DrawPin(x + boxWidth, boxY + 35, "Q", elem.Address, brush);
                DrawPin(x + boxWidth / 2, boxY + boxHeight, "PV", elem.Preset, brush);
            }
            else if (elem.ContactType == "count_down")
            {
                DrawPin(x, boxY + 15, "CD", elem.Input1, brush);
                DrawPin(x, boxY + 50, "LD", elem.Input2, brush);
                DrawPin(x + boxWidth, boxY + 35, "Q", elem.Address, brush);
                DrawPin(x + boxWidth / 2, boxY + boxHeight, "PV", elem.Preset, brush);
            }
            else if (elem.ContactType == "count_updown")
            {
                DrawPin(x, boxY + 8, "CU", elem.Input1, brush);
                DrawPin(x, boxY + 28, "CD", elem.Input2, brush);
                DrawPin(x, boxY + 48, "R", elem.Input3, brush);
                DrawPin(x, boxY + 68, "LD", elem.Input4, brush);
                DrawPin(x + boxWidth, boxY + 18, "QU", elem.Address, brush);
                DrawPin(x + boxWidth, boxY + 58, "QD", elem.Address, brush);
                DrawPin(x + boxWidth / 2, boxY + boxHeight, "PV", elem.Preset, brush);
            }
        }

        private void DrawPin(double x, double y, string pinName, string value, Brush brush)
        {
            var pinText = new TextBlock { Text = $"{pinName}: {value}", FontSize = 8, Foreground = brush };
            Canvas.SetLeft(pinText, x + 2);
            Canvas.SetTop(pinText, y);
            LadderCanvas.Children.Add(pinText);
        }

        // ---------- Primitives ----------
        private void DrawHorizontalLine(double x1, double y1, double x2, double y2, bool active)
        {
            var line = new Line
            {
                X1 = x1,
                Y1 = y1,
                X2 = x2,
                Y2 = y2,
                Stroke = active ? ActiveBrush : InactiveBrush,
                StrokeThickness = 2
            };
            LadderCanvas.Children.Add(line);
        }

        private void DrawVerticalLine(double x, double y1, double y2, bool active)
        {
            var line = new Line
            {
                X1 = x,
                Y1 = y1,
                X2 = x,
                Y2 = y2,
                Stroke = active ? ActiveBrush : InactiveBrush,
                StrokeThickness = 2
            };
            LadderCanvas.Children.Add(line);
        }
    }
}