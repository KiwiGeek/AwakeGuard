using System.Windows;
using Wpf.Ui.Controls;

namespace AwakeGuard.Dialogs;

public partial class ConfirmDialog : FluentWindow
{
    private bool _confirmed;

    private ConfirmDialog()
    {
        InitializeComponent();
    }

    public static bool ShowConfirm(
        Window? owner,
        string title,
        string message,
        string primaryText,
        string secondaryText,
        bool topmost = false)
    {
        var dialog = new ConfirmDialog
        {
            Title = title,
            Topmost = topmost,
            WindowStartupLocation = owner is null
                ? WindowStartupLocation.CenterScreen
                : WindowStartupLocation.CenterOwner,
        };

        if (owner is not null)
        {
            dialog.Owner = owner;
        }

        return dialog.ShowForResult(title, message, primaryText, secondaryText, showSecondary: true);
    }

    public static void ShowError(Window? owner, string title, string message, bool topmost = false)
    {
        var dialog = new ConfirmDialog
        {
            Title = title,
            Topmost = topmost,
            WindowStartupLocation = owner is null
                ? WindowStartupLocation.CenterScreen
                : WindowStartupLocation.CenterOwner,
        };

        if (owner is not null)
        {
            dialog.Owner = owner;
        }

        dialog.ShowForResult(title, message, "OK", secondaryText: string.Empty, showSecondary: false);
    }

    private bool ShowForResult(
        string title,
        string message,
        string primaryText,
        string secondaryText,
        bool showSecondary)
    {
        DialogTitleBar.Title = title;
        MessageText.Text = message;
        PrimaryButton.Content = primaryText;
        SecondaryButton.Content = secondaryText;
        SecondaryButton.Visibility = showSecondary ? Visibility.Visible : Visibility.Collapsed;
        PrimaryButton.IsDefault = true;

        AdjustSizeForContent();
        ShowDialog();
        return _confirmed;
    }

    private void TitleBar_CloseClicked(object sender, RoutedEventArgs e)
    {
        _confirmed = false;
        Close();
    }

    private void AdjustSizeForContent()
    {
        UpdateLayout();
        SizeToContent = SizeToContent.Height;
        MinHeight = 0;
        MaxHeight = double.PositiveInfinity;
    }

    private void PrimaryButton_Click(object sender, RoutedEventArgs e)
    {
        _confirmed = true;
        Close();
    }

    private void SecondaryButton_Click(object sender, RoutedEventArgs e)
    {
        _confirmed = false;
        Close();
    }
}
