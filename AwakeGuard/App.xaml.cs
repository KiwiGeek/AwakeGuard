using System.Windows;
using AwakeGuard.Services;
using AwakeGuard.Tray;
using AwakeGuard.ViewModels;

namespace AwakeGuard;

public partial class App : System.Windows.Application
{
    private AwakeSessionManager? _session;
    private TrayController? _tray;

    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        var cleanupSourceDirectory = StartupArguments.TryGetCleanupSourceDirectory(e.Args);

        _session = new AwakeSessionManager();
        var settingsStore = new AppSettingsStore();
        var startup = new WindowsStartupService(settingsStore);

        var viewModel = new MainViewModel(_session, settingsStore, startup);

        var mainWindow = new MainWindow(viewModel);
        viewModel.SetDialogOwner(mainWindow);
        MainWindow = mainWindow;
        _tray = new TrayController(_session, viewModel, mainWindow);

        if (cleanupSourceDirectory is not null)
        {
            mainWindow.ShowSettings(refreshStartupState: false);
            CompleteStartup(viewModel, cleanupSourceDirectory);
            return;
        }

        mainWindow.ShowInTaskbar = false;
        mainWindow.Hide();
        viewModel.FinishStartupInitialization();
    }

    private void CompleteStartup(MainViewModel viewModel, string? cleanupSourceDirectory)
    {
        try
        {
            if (cleanupSourceDirectory is not null)
            {
                SourceFolderCleanupService.OfferCleanup(cleanupSourceDirectory);
            }
        }
        finally
        {
            viewModel.FinishStartupInitialization();
        }
    }

    protected override void OnExit(ExitEventArgs e)
    {
        _tray?.Dispose();
        _session?.Dispose();
        base.OnExit(e);
    }
}
