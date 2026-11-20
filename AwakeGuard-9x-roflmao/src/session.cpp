#include "session.h"

#include "os_compat.h"
#include "win32_common.h"

namespace {

BOOL g_savedScreenSaveActive = TRUE;
bool g_screenSaveCaptured = false;

// Times are kept in FILETIME ticks (100 ns) so only 64-bit add/compare is
// needed; 64-bit division would pull in libgcc.
constexpr ULONGLONG kTicksPerMinute = 60ULL * 10000000ULL;
constexpr ULONGLONG kTicksPerDay = 24ULL * 60ULL * kTicksPerMinute;

ULONGLONG FileTimeToTicks(const FILETIME& ft) {
    return (static_cast<ULONGLONG>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}

FILETIME TicksToFileTime(ULONGLONG ticks) {
    FILETIME ft;
    ft.dwLowDateTime = static_cast<DWORD>(ticks);
    ft.dwHighDateTime = static_cast<DWORD>(ticks >> 32);
    return ft;
}

ULONGLONG NowTicks() {
    FILETIME ft {};
    GetSystemTimeAsFileTime(&ft);
    return FileTimeToTicks(ft);
}

// Next local midnight, as UTC ticks.
ULONGLONG LocalMidnightTomorrowTicks() {
    SYSTEMTIME midnight {};
    GetLocalTime(&midnight);
    midnight.wHour = 0;
    midnight.wMinute = 0;
    midnight.wSecond = 0;
    midnight.wMilliseconds = 0;

    FILETIME localToday {};
    SystemTimeToFileTime(&midnight, &localToday);
    const FILETIME localTomorrow = TicksToFileTime(FileTimeToTicks(localToday) + kTicksPerDay);

    FILETIME utc {};
    LocalFileTimeToFileTime(&localTomorrow, &utc);
    return FileTimeToTicks(utc);
}

void AppendUnsigned(StatusStr& out, DWORD value) {
    char digits[16];
    wsprintfA(digits, "%lu", value);
    out += digits;
}

StatusStr FormatRemaining(ULONGLONG remainingTicks) {
    const DWORD totalMinutes = UDiv64To32(remainingTicks, static_cast<DWORD>(kTicksPerMinute));
    StatusStr out;
    if (totalMinutes >= 60) {
        AppendUnsigned(out, totalMinutes / 60);
        out += "h ";
        AppendUnsigned(out, totalMinutes % 60);
        out += "m";
        return out;
    }
    AppendUnsigned(out, totalMinutes == 0 ? 1 : totalMinutes);
    out += "m";
    return out;
}

StatusStr ProtectionPhrase(const SessionState& session) {
    StatusStr result;
    if (session.preventSleep && OsHasSetThreadExecutionState()) {
        result += "no sleep";
    }
    if (session.preventDisplayOff) {
        if (!result.empty()) {
            result += ", ";
        }
        result += "no screensaver";
    }
    return result;
}

void ApplyScreensaverState(bool prevent) {
    if (prevent) {
        if (!g_screenSaveCaptured) {
            SystemParametersInfoA(SPI_GETSCREENSAVEACTIVE, 0, &g_savedScreenSaveActive, 0);
            g_screenSaveCaptured = true;
        }
        SystemParametersInfoA(SPI_SETSCREENSAVEACTIVE, FALSE, nullptr, SPIF_SENDCHANGE);
    } else if (g_screenSaveCaptured) {
        SystemParametersInfoA(SPI_SETSCREENSAVEACTIVE, g_savedScreenSaveActive, nullptr, SPIF_SENDCHANGE);
        g_screenSaveCaptured = false;
    }
}

}  // namespace

void SessionApplyExecutionState(const SessionState& session) {
    if (OsHasSetThreadExecutionState() && session.preventSleep) {
        DWORD flags = ES_CONTINUOUS | ES_SYSTEM_REQUIRED;
        OsSetThreadExecutionState(flags);
    } else if (OsHasSetThreadExecutionState()) {
        OsSetThreadExecutionState(ES_CONTINUOUS);
    }
    ApplyScreensaverState(session.preventDisplayOff);
}

void SessionClearExecutionState() {
    if (OsHasSetThreadExecutionState()) {
        OsSetThreadExecutionState(ES_CONTINUOUS);
    }
    ApplyScreensaverState(false);
}

void SessionStart(SessionState& session, int presetIndex) {
    const bool canSleep = OsHasSetThreadExecutionState() && session.preventSleep;
    if (!canSleep && !session.preventDisplayOff) {
        return;
    }
    if (presetIndex < 0 || presetIndex >= kDurationPresetCount) {
        return;
    }

    const DurationPreset& preset = kDurationPresets[presetIndex];
    session.selectedDurationIndex = presetIndex;
    session.isActive = true;

    switch (preset.kind) {
    case DurationKind::FixedMinutes:
        session.hasExpiry = true;
        session.expiresAt = NowTicks() + static_cast<ULONGLONG>(preset.fixedMinutes) * kTicksPerMinute;
        break;
    case DurationKind::RestOfDay:
        session.hasExpiry = true;
        session.expiresAt = LocalMidnightTomorrowTicks();
        break;
    case DurationKind::Indefinite:
        session.hasExpiry = false;
        break;
    }

    SessionRefresh(session);
}

void SessionStop(SessionState& session) {
    if (!session.isActive) {
        return;
    }
    session.isActive = false;
    session.hasExpiry = false;
    SessionClearExecutionState();
}

void SessionRefresh(SessionState& session) {
    if (!session.isActive) {
        return;
    }
    const bool canSleep = OsHasSetThreadExecutionState() && session.preventSleep;
    if (!canSleep && !session.preventDisplayOff) {
        SessionStop(session);
        return;
    }
    SessionApplyExecutionState(session);
}

void SessionCheckExpiry(SessionState& session, bool& expiredNow) {
    expiredNow = false;
    if (!session.isActive || !session.hasExpiry) {
        return;
    }
    if (NowTicks() >= session.expiresAt) {
        SessionStop(session);
        expiredNow = true;
    }
}

StatusStr SessionStatusText(const SessionState& session) {
    if (!session.isActive) {
        return StatusStr("Off - normal power and screensaver");
    }

    const StatusStr protection = ProtectionPhrase(session);
    if (protection.empty()) {
        return StatusStr("On - (no protection active on this OS)");
    }

    StatusStr text = "On - ";
    text += protection;
    if (!session.hasExpiry) {
        text += " (until you turn off)";
        return text;
    }

    const ULONGLONG now = NowTicks();
    if (session.expiresAt <= now) {
        text += " (expiring)";
        return text;
    }
    text += " (";
    text += FormatRemaining(session.expiresAt - now);
    text += " left)";
    return text;
}
