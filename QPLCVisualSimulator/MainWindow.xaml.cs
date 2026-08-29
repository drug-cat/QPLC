using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using Microsoft.Win32;
using QPLC.Core;

namespace QPLCVisualSimulator
{
    public partial class MainWindow : Window
    {
        private LadderSimulator simulator = null!;
        private List<LadderNetwork> networks = null!;
        private ViewMode currentView = ViewMode.Ladder;
        private string? currentConfigPath;
        private string? currentLadderPath;

        public MainWindow()
        {
            InitializeComponent();
            KeyDown += MainWindow_KeyDown;
        }

        private void MainWindow_KeyDown(object sender, System.Windows.Input.KeyEventArgs e)
        {
            if (e.Key == System.Windows.Input.Key.F5) RunScan_Click(null, null);
            else if (e.Key == System.Windows.Input.Key.F6) ToggleView_Click(null, null);
            else if (e.Key == System.Windows.Input.Key.R && (Keyboard.Modifiers & ModifierKeys.Control) != 0) Reset_Click(null, null);
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            // Try to load a default relative to the executable path
            TryLoadDefaultFiles();
        }

        private void TryLoadDefaultFiles()
        {
            try
            {
                string exeDir = AppDomain.CurrentDomain.BaseDirectory;
                string configPath = Path.Combine(exeDir, "..", "..", "..", "examples", "conf.qplc");
                string ladderPath = Path.Combine(exeDir, "..", "..", "..", "output.xml");
                configPath = Path.GetFullPath(configPath);
                ladderPath = Path.GetFullPath(ladderPath);

                if (File.Exists(configPath) && File.Exists(ladderPath))
                {
                    LoadProject(configPath, ladderPath);
                }
                else
                {
                    UpdateStatus("Ready - Use toolbar to load config & ladder files");
                }
            }
            catch (Exception ex)
            {
                UpdateStatus($"Load error: {ex.Message}");
            }
        }

        private void LoadConfig_Click(object sender, RoutedEventArgs e)
        {
            var dlg = new OpenFileDialog
            {
                Title = "Select Config File (conf.qplc)",
                Filter = "QPLC Config (*.qplc)|*.qplc|All Files (*.*)|*.*",
                InitialDirectory = Path.GetFullPath(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "..", "..", "..", "examples"))
            };
            if (dlg.ShowDialog() == true)
            {
                currentConfigPath = dlg.FileName;
                TryReloadProject();
            }
        }

        private void LoadLadder_Click(object sender, RoutedEventArgs e)
        {
            var dlg = new OpenFileDialog
            {
                Title = "Select Ladder XML File",
                Filter = "XML Files (*.xml)|*.xml|All Files (*.*)|*.*",
                InitialDirectory = Path.GetFullPath(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "..", "..", ".."))
            };
            if (dlg.ShowDialog() == true)
            {
                currentLadderPath = dlg.FileName;
                TryReloadProject();
            }
        }

        private void TryReloadProject()
        {
            if (string.IsNullOrEmpty(currentConfigPath) || string.IsNullOrEmpty(currentLadderPath))
            {
                UpdateStatus("Both config and ladder files must be selected");
                return;
            }
            LoadProject(currentConfigPath, currentLadderPath);
        }

        private void LoadProject(string configPath, string ladderPath)
        {
            try
            {
                if (!File.Exists(configPath) || !File.Exists(ladderPath))
                {
                    MessageBox.Show("Selected file(s) not found.");
                    return;
                }

                var config = ConfigParser.Parse(configPath);
                networks = LadderXmlParser.Parse(ladderPath);
                simulator = new LadderSimulator(networks);

                LadderViewControl.SetSimulator(simulator);
                LadderViewControl.SetViewMode(currentView);
                LadderViewControl.DrawNetworks(networks);

                currentConfigPath = configPath;
                currentLadderPath = ladderPath;

                RefreshAll();
                UpdateStatus($"Loaded: {Path.GetFileName(configPath)} + {Path.GetFileName(ladderPath)}");
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error loading project: {ex.Message}");
                UpdateStatus($"Error: {ex.Message}");
            }
        }

        private void RefreshAll()
        {
            RefreshVariables();
            RefreshNetworks();
            UpdateStatusBar();
        }

        private void RefreshVariables()
        {
            BoolListBox.Items.Clear();
            NumericListBox.Items.Clear();

            var boolVars = simulator.BoolVars;
            foreach (var kvp in boolVars.OrderBy(k => k.Key))
            {
                var item = new ListBoxItem
                {
                    Content = $"{kvp.Key} = {kvp.Value}",
                    Foreground = kvp.Value ? Brushes.DarkGreen : Brushes.DarkRed,
                    ToolTip = $"Address: {kvp.Key}"
                };
                BoolListBox.Items.Add(item);
            }

            var numVars = simulator.NumVars;
            foreach (var kvp in numVars.OrderBy(k => k.Key))
            {
                var item = new ListBoxItem
                {
                    Content = $"{kvp.Key} = {kvp.Value}",
                    ToolTip = $"Address: {kvp.Key}"
                };
                NumericListBox.Items.Add(item);
            }

            VariableComboBox.Items.Clear();
            foreach (var name in boolVars.Keys.OrderBy(n => n))
            {
                VariableComboBox.Items.Add(name);
            }
            if (VariableComboBox.Items.Count > 0)
                VariableComboBox.SelectedIndex = 0;
        }

        private void RefreshNetworks()
        {
            LadderViewControl.DrawNetworks(networks);
            UpdateStatusBar();
        }

        private void UpdateStatusBar()
        {
            StatusTime.Text = $"Time: {simulator.CurrentTime:F0} ms";
            StatusScans.Text = $"Scans: {simulator.ScanCount}";
            // Current network: simplest case - the first network that has a jump
            StatusNetwork.Text = networks.Count > 0 ? $"Networks: {networks.Count}" : "Network: -";
            if (!string.IsNullOrEmpty(simulator.LastWarning))
            {
                StatusLastOp.Text = simulator.LastWarning;
                StatusLastOp.Foreground = Brushes.Red;
            }
            else
            {
                StatusLastOp.Text = "Ready";
                StatusLastOp.Foreground = Brushes.DarkGreen;
            }
        }

        private void UpdateStatus(string msg)
        {
            StatusLastOp.Text = msg;
            StatusLastOp.Foreground = Brushes.DarkBlue;
        }

        private void RunScan_Click(object sender, RoutedEventArgs e)
        {
            simulator.RunScan();
            RefreshAll();
        }

        private void Tick100_Click(object sender, RoutedEventArgs e)
        {
            simulator.AdvanceTime(100);
            simulator.RunScan();
            RefreshAll();
        }

        private void Tick1000_Click(object sender, RoutedEventArgs e)
        {
            simulator.AdvanceTime(1000);
            simulator.RunScan();
            RefreshAll();
        }

        private void Reset_Click(object sender, RoutedEventArgs e)
        {
            simulator.Reset();
            RefreshAll();
        }

        private void ApplyValue_Click(object sender, RoutedEventArgs e)
        {
            if (VariableComboBox.SelectedItem == null) return;
            string varName = VariableComboBox.SelectedItem.ToString()!;
            string valueStr = ((ComboBoxItem)ValueComboBox.SelectedItem)?.Tag?.ToString() ?? "False";
            if (bool.TryParse(valueStr, out bool boolVal))
            {
                simulator.SetBool(varName, boolVal);
                simulator.RunScan();
            }
            RefreshAll();
        }

        private void ToggleView_Click(object sender, RoutedEventArgs e)
        {
            currentView = currentView == ViewMode.Ladder ? ViewMode.Fbd : ViewMode.Ladder;
            LadderViewControl.SetViewMode(currentView);
            RefreshNetworks();
        }

        private void ZoomSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (LadderViewbox == null) return;
            LadderViewbox.LayoutTransform = new ScaleTransform(e.NewValue, e.NewValue);
            ZoomText.Text = $"{(int)(e.NewValue * 100)}%";
        }
    }
}