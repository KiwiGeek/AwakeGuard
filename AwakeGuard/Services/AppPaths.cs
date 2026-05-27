using System.IO;

namespace AwakeGuard.Services;

internal static class AppPaths
{
    public const string AppFolderName = "AwakeGuard";
    public const string RunRegistryValueName = "AwakeGuard";

    public static string DataDirectory =>
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), AppFolderName);

    public static string SettingsFilePath => Path.Combine(DataDirectory, "settings.json");

    public static string InstallDirectory => DataDirectory;

    public static string GetInstallExecutablePath(string executableFileName) =>
        Path.Combine(InstallDirectory, executableFileName);

    public static string CurrentExecutablePath =>
        Environment.ProcessPath ?? Path.Combine(AppContext.BaseDirectory, "AwakeGuard.exe");

    public static bool IsRunningFromInstallLocation()
    {
        var current = Path.GetFullPath(CurrentExecutablePath);
        var installDir = Path.GetFullPath(InstallDirectory.TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar);
        return current.StartsWith(installDir, StringComparison.OrdinalIgnoreCase);
    }

    public static bool IsOnDesktopOrDownloads(string? filePath = null)
    {
        var path = filePath ?? CurrentExecutablePath;
        var directory = Path.GetDirectoryName(path);
        if (string.IsNullOrEmpty(directory))
        {
            return false;
        }

        return IsTransientDirectory(Path.GetFullPath(directory));
    }

    public static bool IsTransientDirectory(string directory)
    {
        if (string.IsNullOrWhiteSpace(directory))
        {
            return false;
        }

        directory = Path.GetFullPath(directory);

        foreach (var transientRoot in GetTransientInstallRoots())
        {
            if (IsUnderDirectory(directory, transientRoot))
            {
                return true;
            }
        }

        return false;
    }

    private static IEnumerable<string> GetTransientInstallRoots()
    {
        yield return Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory);

        var userProfile = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
        yield return Path.Combine(userProfile, "Downloads");

        var desktop = Environment.GetFolderPath(Environment.SpecialFolder.Desktop);
        if (!string.IsNullOrEmpty(desktop))
        {
            yield return desktop;
        }
    }

    private static bool IsUnderDirectory(string path, string parent)
    {
        var normalizedPath = Path.GetFullPath(path.TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar);
        var normalizedParent = Path.GetFullPath(parent.TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar);
        return normalizedPath.StartsWith(normalizedParent, StringComparison.OrdinalIgnoreCase);
    }
}
