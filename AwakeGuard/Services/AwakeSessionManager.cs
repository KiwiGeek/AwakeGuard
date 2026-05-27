using System.Windows.Threading;
using AwakeGuard.Models;
using AwakeGuard.Native;

namespace AwakeGuard.Services;

public sealed class AwakeSessionManager : IDisposable
{
    private readonly DispatcherTimer _refreshTimer;
    private readonly DispatcherTimer _expiryTimer;

    private bool _preventSleep = true;
    private bool _preventDisplayOff = true;
    private DateTimeOffset? _expiresAt;

    public AwakeSessionManager()
    {
        _refreshTimer = new DispatcherTimer { Interval = TimeSpan.FromMinutes(1) };
        _refreshTimer.Tick += (_, _) => RefreshState();

        _expiryTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(15) };
        _expiryTimer.Tick += (_, _) => CheckExpiry();
    }

    public bool IsActive { get; private set; }

    public bool PreventSleep
    {
        get => _preventSleep;
        set
        {
            _preventSleep = value;
            if (IsActive)
            {
                RefreshState();
            }
        }
    }

    public bool PreventDisplayOff
    {
        get => _preventDisplayOff;
        set
        {
            _preventDisplayOff = value;
            if (IsActive)
            {
                RefreshState();
            }
        }
    }

    public DateTimeOffset? ExpiresAt => _expiresAt;

    public event EventHandler? SessionChanged;

    public void Start(DurationPreset preset)
    {
        if (!PreventSleep && !PreventDisplayOff)
        {
            throw new InvalidOperationException("Enable at least one protection option.");
        }

        _expiresAt = preset.Kind switch
        {
            DurationPresetKind.Fixed => DateTimeOffset.Now.Add(preset.FixedDuration!.Value),
            DurationPresetKind.RestOfDay => DateTimeOffset.Now.Date.AddDays(1),
            DurationPresetKind.Indefinite => null,
            _ => null,
        };

        IsActive = true;
        RefreshState();
        _refreshTimer.Start();
        _expiryTimer.Start();
        RaiseChanged();
    }

    public void Stop()
    {
        if (!IsActive)
        {
            return;
        }

        IsActive = false;
        _expiresAt = null;
        _refreshTimer.Stop();
        _expiryTimer.Stop();
        ExecutionStateInterop.Clear();
        RaiseChanged();
    }

    public TimeSpan? Remaining
    {
        get
        {
            if (_expiresAt is null)
            {
                return null;
            }

            var remaining = _expiresAt.Value - DateTimeOffset.Now;
            return remaining > TimeSpan.Zero ? remaining : TimeSpan.Zero;
        }
    }

    public string StatusText
    {
        get
        {
            if (!IsActive)
            {
                return "Off — normal power and lock behavior";
            }

            var parts = new List<string>();
            if (PreventSleep)
            {
                parts.Add("no sleep");
            }

            if (PreventDisplayOff)
            {
                parts.Add("no lock/screensaver");
            }

            var protection = string.Join(", ", parts);
            if (_expiresAt is null)
            {
                return $"On — {protection} (until you turn off)";
            }

            var remaining = Remaining;
            if (remaining is null || remaining <= TimeSpan.Zero)
            {
                return $"On — {protection} (expiring)";
            }

            return $"On — {protection} ({FormatRemaining(remaining.Value)} left)";
        }
    }

    private void RefreshState()
    {
        if (!IsActive)
        {
            return;
        }

        if (!PreventSleep && !PreventDisplayOff)
        {
            Stop();
            return;
        }

        ExecutionStateInterop.Apply(PreventSleep, PreventDisplayOff);
    }

    private void CheckExpiry()
    {
        if (!IsActive || _expiresAt is null)
        {
            return;
        }

        if (DateTimeOffset.Now >= _expiresAt.Value)
        {
            Stop();
        }
        else
        {
            RaiseChanged();
        }
    }

    private void RaiseChanged() => SessionChanged?.Invoke(this, EventArgs.Empty);

    private static string FormatRemaining(TimeSpan span)
    {
        if (span.TotalHours >= 1)
        {
            return $"{(int)span.TotalHours}h {span.Minutes}m";
        }

        return $"{Math.Max(1, span.Minutes)}m";
    }

    public void Dispose()
    {
        Stop();
        _refreshTimer.Stop();
        _expiryTimer.Stop();
    }
}
