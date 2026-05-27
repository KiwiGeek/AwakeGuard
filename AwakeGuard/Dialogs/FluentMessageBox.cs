using System.Windows;

namespace AwakeGuard.Dialogs;

internal static class FluentMessageBox
{
    public static bool Confirm(
        Window? owner,
        string title,
        string message,
        string primaryText,
        string secondaryText,
        bool topmost = false) =>
        ConfirmDialog.ShowConfirm(owner, title, message, primaryText, secondaryText, topmost);

    public static void ShowError(Window? owner, string title, string message, bool topmost = false) =>
        ConfirmDialog.ShowError(owner, title, message, topmost);
}
