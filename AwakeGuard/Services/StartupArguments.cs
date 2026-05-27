using System.IO;

namespace AwakeGuard.Services;

internal static class StartupArguments
{
    public const string CleanupSourceFlag = "--cleanup-source";

    public static string? TryGetCleanupSourceDirectory(IReadOnlyList<string> args)
    {
        for (var i = 0; i < args.Count; i++)
        {
            if (string.Equals(args[i], CleanupSourceFlag, StringComparison.OrdinalIgnoreCase) && i + 1 < args.Count)
            {
                return NormalizeDirectory(args[i + 1]);
            }

            const string prefix = CleanupSourceFlag + "=";
            if (args[i].StartsWith(prefix, StringComparison.OrdinalIgnoreCase))
            {
                return NormalizeDirectory(args[i][prefix.Length..]);
            }
        }

        return null;
    }

    public static string FormatCleanupSourceArgument(string sourceDirectory) =>
        $"{CleanupSourceFlag} {Quote(sourceDirectory)}";

    private static string? NormalizeDirectory(string path)
    {
        if (string.IsNullOrWhiteSpace(path))
        {
            return null;
        }

        try
        {
            return Path.GetFullPath(path.Trim().Trim('"'));
        }
        catch
        {
            return null;
        }
    }

    private static string Quote(string value)
    {
        value = value.Trim().TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
        return $"\"{value.Replace("\"", "\\\"")}\"";
    }
}
