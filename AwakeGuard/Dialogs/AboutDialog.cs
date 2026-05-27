using System.Windows;

namespace AwakeGuard.Dialogs;

internal static class AboutDialog
{
    public static void Show()
    {
        var about = new AboutWindow
        {
            WindowStartupLocation = WindowStartupLocation.CenterScreen,
        };
        about.ShowDialog();
    }
}
