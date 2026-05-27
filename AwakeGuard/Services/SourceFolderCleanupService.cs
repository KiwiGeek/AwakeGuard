using System.IO;
using System.Security.Cryptography;
using System.Windows;
using AwakeGuard.Dialogs;

namespace AwakeGuard.Services;

internal static class SourceFolderCleanupService
{
    public static void OfferCleanup(string sourceDirectory)
    {
        if (!AppPaths.IsRunningFromInstallLocation())
        {
            return;
        }

        sourceDirectory = Path.GetFullPath(sourceDirectory);
        if (!Directory.Exists(sourceDirectory) || !AppPaths.IsTransientDirectory(sourceDirectory))
        {
            return;
        }

        var installDirectory = Path.GetFullPath(AppPaths.InstallDirectory);
        if (PathsReferToSameDirectory(sourceDirectory, installDirectory))
        {
            return;
        }

        var delete = FluentMessageBox.Confirm(
            owner: Application.Current.MainWindow,
            title: "Delete original copy?",
            message: $"AwakeGuard was copied from:\n{sourceDirectory}\n\n"
            + "Delete the original files that match your AppData copy?",
            primaryText: "Delete",
            secondaryText: "Keep",
            topmost: true);

        if (!delete)
        {
            return;
        }

        try
        {
            CleanupMatchingFiles(sourceDirectory, installDirectory);
            RemoveDirectoryIfEmpty(sourceDirectory);
        }
        catch (Exception ex)
        {
            FluentMessageBox.ShowError(Application.Current.MainWindow, "Cleanup failed", $"Could not remove the original copy:\n{ex.Message}", topmost: true);
        }
    }

    private static void CleanupMatchingFiles(string sourceDirectory, string installDirectory)
    {
        foreach (var sourceFile in Directory.EnumerateFiles(sourceDirectory))
        {
            var fileName = Path.GetFileName(sourceFile);
            var installFile = Path.Combine(installDirectory, fileName);

            if (!File.Exists(installFile) || !FilesAreIdentical(sourceFile, installFile))
            {
                continue;
            }

            File.Delete(sourceFile);
        }
    }

    private static void RemoveDirectoryIfEmpty(string directory)
    {
        if (!Directory.Exists(directory))
        {
            return;
        }

        if (Directory.EnumerateFileSystemEntries(directory).Any())
        {
            return;
        }

        Directory.Delete(directory);
    }

    private static bool FilesAreIdentical(string firstPath, string secondPath)
    {
        var firstInfo = new FileInfo(firstPath);
        var secondInfo = new FileInfo(secondPath);

        if (firstInfo.Length != secondInfo.Length)
        {
            return false;
        }

        return string.Equals(ComputeSha256(firstPath), ComputeSha256(secondPath), StringComparison.OrdinalIgnoreCase);
    }

    private static string ComputeSha256(string path)
    {
        using var stream = File.OpenRead(path);
        var hash = SHA256.HashData(stream);
        return Convert.ToHexString(hash);
    }

    private static bool PathsReferToSameDirectory(string first, string second)
    {
        var normalizedFirst = Path.GetFullPath(first.TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar);
        var normalizedSecond = Path.GetFullPath(second.TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar);
        return string.Equals(normalizedFirst, normalizedSecond, StringComparison.OrdinalIgnoreCase);
    }
}
