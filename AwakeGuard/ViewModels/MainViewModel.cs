using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows;
using System.Windows.Input;
using AwakeGuard.Models;
using AwakeGuard.Services;

namespace AwakeGuard.ViewModels;

public sealed class MainViewModel : INotifyPropertyChanged
{
    private readonly AwakeSessionManager _session;
    private readonly WindowsStartupService _startup;
    private readonly AppSettingsStore _settingsStore;
    private DurationPreset _selectedDuration;
    private bool _preventSleep = true;
    private bool _preventDisplayOff = true;
    private bool _startWithWindows;
    private bool _suppressStartupChange = true;
    private Window? _dialogOwner;

    public MainViewModel(
        AwakeSessionManager session,
        AppSettingsStore settingsStore,
        WindowsStartupService startup)
    {
        _session = session;
        _settingsStore = settingsStore;
        _startup = startup;
        _selectedDuration = DurationPreset.All[1];
        Durations = new ObservableCollection<DurationPreset>(DurationPreset.All);

        TurnOnCommand = new RelayCommand(TurnOn, () => CanTurnOn);
        TurnOffCommand = new RelayCommand(TurnOff, () => IsActive);
        ToggleCommand = new RelayCommand(Toggle);

        _session.SessionChanged += (_, _) =>
        {
            OnPropertyChanged(nameof(IsActive));
            OnPropertyChanged(nameof(StatusText));
            OnPropertyChanged(nameof(CanTurnOn));
            TurnOnCommand.RaiseCanExecuteChanged();
            TurnOffCommand.RaiseCanExecuteChanged();
        };

    }

    public void SetDialogOwner(Window owner) => _dialogOwner = owner;

    public void FinishStartupInitialization()
    {
        ApplyStartupState();
        _suppressStartupChange = false;
    }

    public void RefreshStartupState() => ApplyStartupState();

    public ObservableCollection<DurationPreset> Durations { get; }

    public bool StartWithWindows
    {
        get => _startWithWindows;
        set
        {
            if (!SetField(ref _startWithWindows, value) || _suppressStartupChange)
            {
                return;
            }

            if (value)
            {
                var result = _startup.TryEnable(_dialogOwner, offerRelocate: true);
                if (result is StartupEnableResult.Failed)
                {
                    RevertStartWithWindows(false);
                }
            }
            else
            {
                _startup.Disable();
            }
        }
    }

    public DurationPreset SelectedDuration
    {
        get => _selectedDuration;
        set
        {
            if (SetField(ref _selectedDuration, value))
            {
                TurnOnCommand.RaiseCanExecuteChanged();
            }
        }
    }

    public bool PreventSleep
    {
        get => _preventSleep;
        set
        {
            if (SetField(ref _preventSleep, value))
            {
                _session.PreventSleep = value;
                OnPropertyChanged(nameof(CanTurnOn));
                TurnOnCommand.RaiseCanExecuteChanged();
            }
        }
    }

    public bool PreventDisplayOff
    {
        get => _preventDisplayOff;
        set
        {
            if (SetField(ref _preventDisplayOff, value))
            {
                _session.PreventDisplayOff = value;
                OnPropertyChanged(nameof(CanTurnOn));
                TurnOnCommand.RaiseCanExecuteChanged();
            }
        }
    }

    public bool IsActive => _session.IsActive;

    public string StatusText => _session.StatusText;

    public bool CanTurnOn => !IsActive && (PreventSleep || PreventDisplayOff) && SelectedDuration is not null;

    public RelayCommand TurnOnCommand { get; }

    public RelayCommand TurnOffCommand { get; }

    public RelayCommand ToggleCommand { get; }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void TurnOn()
    {
        _session.PreventSleep = PreventSleep;
        _session.PreventDisplayOff = PreventDisplayOff;
        _session.Start(SelectedDuration);
    }

    private void TurnOff() => _session.Stop();

    private void Toggle()
    {
        if (IsActive)
        {
            TurnOff();
        }
        else
        {
            TurnOn();
        }
    }

    private bool SetField<T>(ref T field, T value, [CallerMemberName] string? propertyName = null)
    {
        if (EqualityComparer<T>.Default.Equals(field, value))
        {
            return false;
        }

        field = value;
        OnPropertyChanged(propertyName);
        return true;
    }

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));

    private void ApplyStartupState()
    {
        var registryEnabled = _startup.IsRegistered();

        var settings = _settingsStore.Load();
        if (!registryEnabled && settings.StartWithWindows)
        {
            settings.StartWithWindows = false;
            _settingsStore.Save(settings);
        }

        _suppressStartupChange = true;
        _startWithWindows = registryEnabled;
        OnPropertyChanged(nameof(StartWithWindows));
        _suppressStartupChange = false;
    }

    private void RevertStartWithWindows(bool enabled)
    {
        _suppressStartupChange = true;
        _startWithWindows = enabled;
        OnPropertyChanged(nameof(StartWithWindows));
        _suppressStartupChange = false;
    }
}

public sealed class RelayCommand : ICommand
{
    private readonly Action _execute;
    private readonly Func<bool>? _canExecute;

    public RelayCommand(Action execute, Func<bool>? canExecute = null)
    {
        _execute = execute;
        _canExecute = canExecute;
    }

    public event EventHandler? CanExecuteChanged;

    public bool CanExecute(object? parameter) => _canExecute?.Invoke() ?? true;

    public void Execute(object? parameter) => _execute();

    public void RaiseCanExecuteChanged() => CanExecuteChanged?.Invoke(this, EventArgs.Empty);
}
