using System.Diagnostics;
using System.Reflection;
using System.Windows;
using System.Windows.Navigation;
using Wpf.Ui.Controls;

namespace AwakeGuard.Dialogs;

public partial class AboutWindow : FluentWindow
{
    public AboutWindow()
    {
        InitializeComponent();
        DescriptionText.Text =
            "A system-tray utility that temporarily keeps your PC awake and prevents automatic sleep, screensaver, and display lock.";
        VersionText.Text = $"Version {GetVersionText()}";
    }

    private void Ok_Click(object sender, RoutedEventArgs e) => Close();

    private void Hyperlink_RequestNavigate(object sender, RequestNavigateEventArgs e)
    {
        Process.Start(new ProcessStartInfo(e.Uri.AbsoluteUri) { UseShellExecute = true });
        e.Handled = true;
    }

    private static string GetVersionText()
    {
        var informational = Assembly.GetExecutingAssembly()
            .GetCustomAttribute<AssemblyInformationalVersionAttribute>()
            ?.InformationalVersion;
        if (!string.IsNullOrWhiteSpace(informational))
        {
            var plus = informational.IndexOf('+');
            return plus >= 0 ? informational[..plus] : informational;
        }

        var version = Assembly.GetExecutingAssembly().GetName().Version;
        if (version is null)
            return "dev";

        var patch = version.Build >= 0 ? version.Build : 0;
        return $"{version.Major}.{version.Minor}.{patch}";
    }
}
