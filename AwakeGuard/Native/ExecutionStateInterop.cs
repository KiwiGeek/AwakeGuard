using System.Runtime.InteropServices;

namespace AwakeGuard.Native;

[Flags]
internal enum ExecutionState : uint
{
    EsAwaymodeRequired = 0x00000040,
    EsContinuous = 0x80000000,
    EsDisplayRequired = 0x00000002,
    EsSystemRequired = 0x00000001,
}

internal static class ExecutionStateInterop
{
    [DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
    private static extern ExecutionState SetThreadExecutionState(ExecutionState esFlags);

    public static void Apply(bool preventSleep, bool preventDisplayOff)
    {
        var flags = ExecutionState.EsContinuous;

        if (preventSleep)
        {
            flags |= ExecutionState.EsSystemRequired;
        }

        if (preventDisplayOff)
        {
            flags |= ExecutionState.EsDisplayRequired;
        }

        SetThreadExecutionState(flags);
    }

    public static void Clear()
    {
        SetThreadExecutionState(ExecutionState.EsContinuous);
    }
}
