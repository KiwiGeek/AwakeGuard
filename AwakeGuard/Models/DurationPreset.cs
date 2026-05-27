namespace AwakeGuard.Models;

public sealed class DurationPreset
{
    public required string Label { get; init; }

    public required DurationPresetKind Kind { get; init; }

    public TimeSpan? FixedDuration { get; init; }

    public static IReadOnlyList<DurationPreset> All { get; } =
    [
        new() { Label = "30 minutes", Kind = DurationPresetKind.Fixed, FixedDuration = TimeSpan.FromMinutes(30) },
        new() { Label = "1 hour", Kind = DurationPresetKind.Fixed, FixedDuration = TimeSpan.FromHours(1) },
        new() { Label = "2 hours", Kind = DurationPresetKind.Fixed, FixedDuration = TimeSpan.FromHours(2) },
        new() { Label = "4 hours", Kind = DurationPresetKind.Fixed, FixedDuration = TimeSpan.FromHours(4) },
        new() { Label = "8 hours", Kind = DurationPresetKind.Fixed, FixedDuration = TimeSpan.FromHours(8) },
        new() { Label = "Rest of today", Kind = DurationPresetKind.RestOfDay },
        new() { Label = "Until turned off", Kind = DurationPresetKind.Indefinite },
    ];
}

public enum DurationPresetKind
{
    Fixed,
    RestOfDay,
    Indefinite,
}
