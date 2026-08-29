using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Media;
using Avalonia.Threading;
using QPLC.Core;

namespace QPLC.Studio;

public partial class MainWindow : Window
{
    private LadderSimulator? sim;
    private Config? config;
    private ModbusServer? modbus;
    private DispatcherTimer? runTimer;
    private readonly ObservableCollection<VarRow> vars = new();
    private bool isReplaying;

    public MainWindow()
    {
        InitializeComponent();
        varList.ItemsSource = vars;
        UpdateStatus("QPLC Studio — open a .qplc config and a ladder .xml to start");
    }

    private void UpdateStatus(string msg) => statusBar.Text = msg;
    private void UpdateFooter()
    {
        if (sim == null) return;
        scanText.Text = sim.ScanCount.ToString();
        timeText.Text = ((int)sim.CurrentTime).ToString();
        modbusText.Text = modbus == null ? "off" : $"on :{modbus.Port}";
    }

    private void OnOpenConfig(object? sender, RoutedEventArgs e)
    {
        var dlg = new OpenFileDialog { Title = "Open config.qplc", Filters = { new FileDialogFilter { Name = "QPLC config", Extensions = new List<string> { "qplc" } } } };
        if (dlg.ShowAsync(this) is { } task)
        {
            task.ContinueWith(t =>
            {
                if (t.Result?.Length > 0) LoadConfig(t.Result[0]);
            });
        }
    }

    private void OnOpenLadder(object? sender, RoutedEventArgs e)
    {
        var dlg = new OpenFileDialog { Title = "Open ladder XML", Filters = { new FileDialogFilter { Name = "Ladder XML", Extensions = new List<string> { "xml" } } } };
        if (dlg.ShowAsync(this) is { } task)
        {
            task.ContinueWith(t =>
            {
                if (t.Result?.Length > 0) LoadLadder(t.Result[0]);
            });
        }
    }

    private void LoadConfig(string path)
    {
        try
        {
            config = ConfigParser.Parse(path);
            UpdateStatus($"Config loaded: {config.Io.Count} IO, hardware={config.Hardware.Cpu}");
        }
        catch (Exception ex) { UpdateStatus($"Config error: {ex.Message}"); }
    }

    private void LoadLadder(string path)
    {
        try
        {
            var networks = LadderXmlParser.Parse(path);
            sim = new LadderSimulator(networks);
            RebuildVarList();
            RenderLadder(networks);
            UpdateStatus($"Ladder loaded: {networks.Count} networks, {CountRungs(networks)} rungs");
            UpdateFooter();
        }
        catch (Exception ex) { UpdateStatus($"Ladder error: {ex.Message}"); }
    }

    private int CountRungs(List<LadderNetwork> n) => n.Sum(x => x.Rungs.Count);

    private void RebuildVarList()
    {
        vars.Clear();
        if (sim == null) return;
        foreach (var k in sim.BoolVars.Keys.OrderBy(k => k))
            vars.Add(new VarRow(k, "BOOL", sim.BoolVars[k].ToString()));
        foreach (var k in sim.NumVars.Keys.OrderBy(k => k))
            vars.Add(new VarRow(k, "NUM", sim.NumVars[k].ToString("0.##")));
    }

    private void OnRun(object? s, RoutedEventArgs e)
    {
        if (sim == null) { UpdateStatus("Load ladder first"); return; }
        if (runTimer == null)
        {
            runTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(100) };
            runTimer.Tick += (_, _) => { if (sim != null && !isReplaying) { sim.AdvanceTime(100); sim.RunScan(); UpdateVars(); } };
        }
        runTimer.IsEnabled = !runTimer.IsEnabled;
        UpdateStatus(runTimer.IsEnabled ? "Running (100ms tick)" : "Paused");
    }

    private void OnStep(object? s, RoutedEventArgs e)
    {
        if (sim == null) return;
        sim.AdvanceTime(100);
        sim.RunScan();
        UpdateVars();
        UpdateFooter();
    }

    private void OnReset(object? s, RoutedEventArgs e)
    {
        sim?.Reset();
        UpdateVars();
        UpdateFooter();
    }

    private void OnToggleModbus(object? s, RoutedEventArgs e)
    {
        if (sim == null || config == null) { UpdateStatus("Load config + ladder first"); return; }
        if (modbus != null) { modbus.Dispose(); modbus = null; UpdateStatus("Modbus server stopped"); }
        else
        {
            modbus = new ModbusServer(sim, config, config.Simulator.ModbusPort);
            modbus.Start();
            UpdateStatus($"Modbus server started on 127.0.0.1:{modbus.Port}");
        }
        UpdateFooter();
    }

    private void OnQuit(object? s, RoutedEventArgs e) => Close();
    private void OnAbout(object? s, RoutedEventArgs e) =>
        UpdateStatus("QPLC Studio — Cross-platform ladder IDE powered by QPLC.Core (Apache-2.0)");

    private void UpdateVars()
    {
        if (sim == null) return;
        for (int i = 0; i < vars.Count; i++)
        {
            var row = vars[i];
            if (sim.BoolVars.TryGetValue(row.Name, out var b)) row.Value = b.ToString();
            else if (sim.NumVars.TryGetValue(row.Name, out var n)) row.Value = n.ToString("0.##");
        }
        UpdateFooter();
    }

    private void RenderLadder(List<LadderNetwork> networks)
    {
        ladderCanvas.Children.Clear();
        double y = 20;
        foreach (var net in networks)
        {
            foreach (var rung in net.Rungs)
            {
                double x = 30;
                // left rail
                ladderCanvas.Children.Add(MakeLine(10, y + 15, 30, y + 15, Brushes.Green, 2));
                // branches
                int branchCount = rung.Branches.Count == 0 ? 1 : rung.Branches.Count;
                double branchWidth = 800.0 / branchCount;
                for (int bi = 0; bi < rung.Branches.Count; bi++)
                {
                    var branch = rung.Branches[bi];
                    double bx = 30 + bi * branchWidth;
                    foreach (var elem in branch)
                    {
                        DrawElement(elem, bx, y);
                        bx += 60;
                    }
                }
                // coil
                if (rung.Coil != null)
                    DrawElement(rung.Coil, 850, y);
                if (rung.Move != null)
                {
                    ladderCanvas.Children.Add(MakeText($"MOVE→{rung.Move.Dest}", 720, y + 5, Brushes.DarkBlue, 11));
                    ladderCanvas.Children.Add(MakeText($"←{rung.Move.Source}", 720, y + 20, Brushes.DarkBlue, 11));
                }
                if (rung.Timer != null)
                    ladderCanvas.Children.Add(MakeText($"TIMER[{rung.Timer.ContactType}]→{rung.Timer.Address} ({rung.Timer.Source})", 700, y + 5, Brushes.DarkRed, 11));
                if (rung.Counter != null)
                    ladderCanvas.Children.Add(MakeText($"COUNTER[{rung.Counter.ContactType}]→{rung.Counter.Address} (PV={rung.Counter.Preset})", 670, y + 5, Brushes.DarkGreen, 11));
                if (rung.Jump != null)
                    ladderCanvas.Children.Add(MakeText($"→{rung.Jump.JumpType} {rung.Jump.Label}", 720, y + 5, Brushes.DarkOrange, 11));
                // right rail
                ladderCanvas.Children.Add(MakeLine(870, y + 15, 990, y + 15, Brushes.Green, 2));
                y += 40;
            }
        }
    }

    private void DrawElement(LadderElement elem, double x, double y)
    {
        string label = elem.Type switch
        {
            "contact" => $"{elem.ContactType} {elem.Address}",
            "coil" => $"{elem.ContactType} {elem.Address}",
            _ => elem.Type
        };
        var color = elem.Type == "contact" ? Brushes.Black : Brushes.DarkBlue;
        ladderCanvas.Children.Add(MakeText(label, x, y, color, 10));
    }

    private static Avalonia.Controls.Shapes.Line MakeLine(double x1, double y1, double x2, double y2, IBrush stroke, double thickness) =>
        new() { StartPoint = new Avalonia.Point(x1, y1), EndPoint = new Avalonia.Point(x2, y2), Stroke = stroke, StrokeThickness = thickness };

    private static TextBlock MakeText(string text, double x, double y, IBrush color, double size) =>
        new() { Text = text, Foreground = color, FontSize = size };
}

public class VarRow
{
    public string Name { get; }
    public string Type { get; }
    public string _value;
    public string Value
    {
        get => _value;
        set
        {
            _value = value;
            // Avalonia binding notifies automatically through ObservableCollection
        }
    }
    public VarRow(string n, string t, string v) { Name = n; Type = t; _value = v; }
}
