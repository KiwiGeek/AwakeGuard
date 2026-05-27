using System.ComponentModel;
using System.Windows;
using System.Windows.Input;
using AwakeGuard.Dialogs;
using AwakeGuard.ViewModels;
using Wpf.Ui.Controls;

namespace AwakeGuard;

public partial class MainWindow : FluentWindow
{
    public MainWindow(MainViewModel viewModel)
    {
        InitializeComponent();
        DataContext = viewModel;
        Closing += OnClosing;
    }

    public void HideToTray()
    {
        Hide();
        ShowInTaskbar = false;
    }

    public void ShowFromTray() => ShowSettings(refreshStartupState: true);

    public void ShowSettings(bool refreshStartupState = true)
    {
        if (refreshStartupState && DataContext is MainViewModel viewModel)
        {
            viewModel.RefreshStartupState();
        }

        ShowInTaskbar = true;
        Show();
        WindowState = WindowState.Normal;
        Activate();
    }


    private void OnClosing(object? sender, CancelEventArgs e)
    {
        e.Cancel = true;
        HideToTray();
    }

    private void TitleBar_CloseClicked(object sender, RoutedEventArgs e) => HideToTray();

    private void HideToTray_Click(object sender, RoutedEventArgs e) => HideToTray();

    private void About_Click(object sender, RoutedEventArgs e) => AboutDialog.Show();

    private void MainWindow_KeyDown(object sender, System.Windows.Input.KeyEventArgs e)
    {
        if (e.Key == Key.Escape)
        {
            HideToTray();
            e.Handled = true;
        }
    }
}
