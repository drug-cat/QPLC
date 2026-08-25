using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace QPLCVisualSimulator
{
    public partial class MainWindow : Window
    {
        private LadderSimulator simulator = null!;
        private List<LadderNetwork> networks = null!;
        private ViewMode currentView = ViewMode.Ladder;

        public MainWindow()
        {
            InitializeComponent();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            try
            {
                string configPath = @"E:\QPLC\examples\conf.qplc";
                string ladderPath = @"E:\QPLC\output.xml";

                if (!System.IO.File.Exists(configPath) || !System.IO.File.Exists(ladderPath))
                {
                    MessageBox.Show("Config or Ladder file not found. Adjust paths.");
                    return;
                }

                var config = ConfigParser.Parse(configPath);
                networks = LadderXmlParser.Parse(ladderPath);
                simulator = new LadderSimulator(networks);

                LadderViewControl.SetSimulator(simulator);
                LadderViewControl.SetViewMode(currentView);
                LadderViewControl.DrawNetworks(networks);

                RefreshAll();
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error loading project: {ex.Message}");
            }
        }

        private void RefreshAll()
        {
            RefreshVariables();
            RefreshNetworks();
        }

        private void RefreshVariables()
        {
            BoolListBox.Items.Clear();
            NumericListBox.Items.Clear();

            var boolVars = simulator.GetBoolVariables();
            foreach (var kvp in boolVars.OrderBy(k => k.Key))
            {
                BoolListBox.Items.Add($"{kvp.Key} = {kvp.Value}");
            }

            var numVars = simulator.GetNumericVariables();
            foreach (var kvp in numVars.OrderBy(k => k.Key))
            {
                NumericListBox.Items.Add($"{kvp.Key} = {kvp.Value}");
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
                simulator.RunScan();   // auto-run after setting variable
            }
            RefreshAll();
        }

        private void ToggleView_Click(object sender, RoutedEventArgs e)
        {
            currentView = currentView == ViewMode.Ladder ? ViewMode.Fbd : ViewMode.Ladder;
            LadderViewControl.SetViewMode(currentView);
            RefreshNetworks();
        }
    }
}