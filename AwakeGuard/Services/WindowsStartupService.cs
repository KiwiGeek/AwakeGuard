using System.Diagnostics;
using System.IO;
using System.Windows;
using AwakeGuard.Dialogs;
using AwakeGuard.Models;
using Microsoft.Win32;

namespace AwakeGuard.Services;

public enum StartupEnableResult
{
    Enabled,
    Relocating,
    Failed,
}

public sealed class WindowsStartupService
{
    private const string RunKeyPath = @"Software\Microsoft\Windows\CurrentVersion\Run";
    private readonly AppSettingsStore _settingsStore;

    public WindowsStartupService(AppSettingsStore settingsStore) => _settingsStore = settingsStore;

    public bool IsRegistered()
    {
        using var key = Registry.CurrentUser.OpenSubKey(RunKeyPath, writable: false);
        return key?.GetValue(AppPaths.RunRegistryValueName) is string;
    }

    public StartupEnableResult TryEnable(Window? owner, bool offerRelocate)
    {
        var currentExe = AppPaths.CurrentExecutablePath;

        if (offerRelocate
            && !AppPaths.IsRunningFromInstallLocation()
            && AppPaths.IsOnDesktopOrDownloads(currentExe))
        {
            var copy = FluentMessageBox.Confirm(
                owner,
                "Copy AwakeGuard?",
                "AwakeGuard is running from your Desktop or Downloads folder. "
                + "For reliable startup with Windows, it's best to keep a copy in your AppData folder.\n\n"
                + "Copy AwakeGuard there now?",
                "Copy",
                "Not now");

            if (copy)
            {
                return TryRelocateAndEnable(owner);
            }
        }

        return ApplyEnable(currentExe);
    }

    public void Disable()
    {
        RemoveRunEntry();
        var settings = _settingsStore.Load();
        settings.StartWithWindows = false;
        _settingsStore.Save(settings);
    }

    private StartupEnableResult TryRelocateAndEnable(Window? owner)
    {
        try
        {
            var sourceExe = AppPaths.CurrentExecutablePath;
            var sourceDir = Path.GetDirectoryName(sourceExe)
                ?? throw new InvalidOperationException("Could not determine the application folder.");

            var executableName = Path.GetFileName(sourceExe);
            var destinationDir = AppPaths.InstallDirectory;
            var destinationExe = AppPaths.GetInstallExecutablePath(executableName);

            Directory.CreateDirectory(destinationDir);

            foreach (var file in Directory.EnumerateFiles(sourceDir))
            {
                var fileName = Path.GetFileName(file);
                File.Copy(file, Path.Combine(destinationDir, fileName), overwrite: true);
            }

            var settings = _settingsStore.Load();
            settings.StartWithWindows = true;
            _settingsStore.Save(settings);

            SetRunEntry(destinationExe);

            LaunchInstalledCopy(destinationExe, destinationDir, sourceDir);
            Application.Current.Shutdown();
            return StartupEnableResult.Relocating;
        }
        catch (Exception ex)
        {
            FluentMessageBox.ShowError(owner, "Copy failed", $"Could not copy AwakeGuard:\n{ex.Message}");
            return StartupEnableResult.Failed;
        }
    }

    private StartupEnableResult ApplyEnable(string executablePath)
    {
        try
        {
            SetRunEntry(executablePath);

            var settings = _settingsStore.Load();
            settings.StartWithWindows = true;
            _settingsStore.Save(settings);

            return StartupEnableResult.Enabled;
        }
        catch (Exception ex)
        {
            FluentMessageBox.ShowError(null, "Startup failed", $"Could not enable startup with Windows:\n{ex.Message}");
            return StartupEnableResult.Failed;
        }
    }

    private static void LaunchInstalledCopy(string destinationExe, string destinationDir, string sourceDir)
    {
        if (!File.Exists(destinationExe))
        {
            throw new FileNotFoundException("The AppData copy was not found after copying.", destinationExe);
        }

        var startInfo = new ProcessStartInfo
        {
            FileName = destinationExe,
            Arguments = StartupArguments.FormatCleanupSourceArgument(sourceDir),
            WorkingDirectory = destinationDir,
            UseShellExecute = false,
        };

        var process = Process.Start(startInfo)
            ?? throw new InvalidOperationException("Windows did not start the AppData copy of AwakeGuard.");

        try
        {
            if (process.WaitForExit(1000))
            {
                throw new InvalidOperationException(
                    $"The AppData copy exited immediately (code {process.ExitCode}).");
            }
        }
        finally
        {
            process.Dispose();
        }
    }

    private static void SetRunEntry(string executablePath)
    {
        var quoted = $"\"{Path.GetFullPath(executablePath)}\"";
        using var key = Registry.CurrentUser.CreateSubKey(RunKeyPath, writable: true)
            ?? throw new InvalidOperationException("Could not open the Windows Run registry key.");
        key.SetValue(AppPaths.RunRegistryValueName, quoted);
    }

    private static void RemoveRunEntry()
    {
        using var key = Registry.CurrentUser.OpenSubKey(RunKeyPath, writable: true);
        key?.DeleteValue(AppPaths.RunRegistryValueName, throwOnMissingValue: false);
    }
}
