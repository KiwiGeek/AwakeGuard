using System.Windows;
using System.Windows.Controls;
using System.Windows.Threading;
using AwakeGuard.Dialogs;
using AwakeGuard.Services;
using AwakeGuard.ViewModels;
using Hardcodet.Wpf.TaskbarNotification;

namespace AwakeGuard.Tray;

public sealed class TrayController : IDisposable
{
    private readonly AwakeSessionManager _session;
    private readonly MainViewModel _viewModel;
    private readonly TaskbarIcon _taskbarIcon;
    private bool _wasActive;

    public TrayController(AwakeSessionManager session, MainViewModel viewModel, Window mainWindow)
    {
        _session = session;
        _viewModel = viewModel;

        _taskbarIcon = new TaskbarIcon
        {
            ToolTipText = "AwakeGuard",
            ContextMenu = BuildContextMenu(mainWindow),
            LeftClickCommand = viewModel.ToggleCommand,
            DoubleClickCommand = new RelayCommand(() => ShowSettings(mainWindow)),
        };

        UpdateIcon();
        _session.SessionChanged += OnSessionChanged;
    }

    private void OnSessionChanged(object? sender, EventArgs e)
    {
        Application.Current.Dispatcher.Invoke(() =>
        {
            var hadTimedSession = _wasActive && !_session.IsActive;
            UpdateIcon();

            if (hadTimedSession)
            {
                _taskbarIcon.ShowBalloonTip(
                    "AwakeGuard",
                    "Protection ended — sleep and lock are allowed again.",
                    BalloonIcon.Info);
            }
        });
    }

    private void UpdateIcon()
    {
        _wasActive = _session.IsActive;
        _taskbarIcon.IconSource = TrayIconFactory.Create(_session.IsActive);
        _taskbarIcon.ToolTipText = _session.IsActive
            ? $"AwakeGuard — {_session.StatusText}"
            : "AwakeGuard";
    }

    private ContextMenu BuildContextMenu(Window mainWindow)
    {
        var menu = new ContextMenu();

        var status = new MenuItem { Header = "Loading...", IsEnabled = false };
        menu.Items.Add(status);

        menu.Items.Add(new Separator());

        var open = new MenuItem { Header = "Open settings…" };
        open.Click += (_, _) => ShowSettings(mainWindow);
        menu.Items.Add(open);

        var turnOff = new MenuItem { Header = "Turn off" };
        turnOff.Click += (_, _) => _viewModel.TurnOffCommand.Execute(null);
        menu.Items.Add(turnOff);

        menu.Items.Add(new Separator());

        var durations = new MenuItem { Header = "Turn on for" };
        foreach (var preset in Models.DurationPreset.All)
        {
            var item = new MenuItem { Header = preset.Label };
            var captured = preset;
            item.Click += (_, _) =>
            {
                _viewModel.SelectedDuration = captured;
                if (!_session.IsActive)
                {
                    _viewModel.TurnOnCommand.Execute(null);
                }
                else
                {
                    _session.Stop();
                    _viewModel.TurnOnCommand.Execute(null);
                }
            };
            durations.Items.Add(item);
        }

        menu.Items.Add(durations);

        menu.Items.Add(new Separator());

        var about = new MenuItem { Header = "About AwakeGuard…" };
        about.Click += (_, _) =>
        {
            Application.Current.Dispatcher.BeginInvoke(
                static () => AboutDialog.Show(),
                DispatcherPriority.ApplicationIdle);
        };
        menu.Items.Add(about);

        menu.Items.Add(new Separator());

        var exit = new MenuItem { Header = "Exit" };
        exit.Click += (_, _) => Application.Current.Shutdown();
        menu.Items.Add(exit);

        menu.Opened += (_, _) => status.Header = _session.StatusText;
        turnOff.IsEnabled = _session.IsActive;

        _session.SessionChanged += (_, _) =>
        {
            Application.Current.Dispatcher.Invoke(() =>
            {
                status.Header = _session.StatusText;
                turnOff.IsEnabled = _session.IsActive;
            });
        };

        return menu;
    }

    private static void ShowSettings(Window window)
    {
        if (window is MainWindow main)
        {
            main.ShowFromTray();
            return;
        }

        window.ShowInTaskbar = true;
        window.Show();
        window.WindowState = WindowState.Normal;
        window.Activate();
    }

    public void Dispose()
    {
        _taskbarIcon.Dispose();
    }
}
