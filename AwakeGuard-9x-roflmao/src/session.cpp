#include "session.h"

#include "os_compat.h"
#include "win32_common.h"

#include <sstream>
#include <vector>

namespace {

BOOL g_savedScreenSaveActive = TRUE;
bool g_screenSaveCaptured = false;

ULONGLONG FileTimeToMs(const FILETIME& ft) {
    ULARGE_INTEGER ui {};
    ui.LowPart = ft.dwLowDateTime;
    ui.HighPart = ft.dwHighDateTime;
    return ui.QuadPart / 10000ULL;
}

ULONGLONG NowMs() {
    FILETIME ft {};
    GetSystemTimeAsFileTime(&ft);
    return FileTimeToMs(ft);
}

SYSTEMTIME LocalMidnightTomorrow() {
    SYSTEMTIME now {};
    GetLocalTime(&now);
    SYSTEMTIME midnight = now;
    midnight.wHour = 0;
    midnight.wMinute = 0;
    midnight.wSecond = 0;
    midnight.wMilliseconds = 0;

    FILETIME ft {};
    SystemTimeToFileTime(&midnight, &ft);
    ULARGE_INTEGER ui {};
    ui.LowPart = ft.dwLowDateTime;
    ui.HighPart = ft.dwHighDateTime;
    ui.QuadPart += 24ULL * 60 * 60 * 10000000ULL;

    FILETIME next {};
    next.dwLowDateTime = ui.LowPart;
    next.dwHighDateTime = ui.HighPart;
    SYSTEMTIME out {};
    FileTimeToSystemTime(&next, &out);
    return out;
}

ULONGLONG SystemTimeToMsLocal(const SYSTEMTIME& st) {
    SYSTEMTIME local = st;
    FILETIME ftLocal {};
    SystemTimeToFileTime(&local, &ftLocal);
    FILETIME ftUtc {};
    LocalFileTimeToFileTime(&ftLocal, &ftUtc);
    return FileTimeToMs(ftUtc);
}

std::string FormatRemainingMs(ULONGLONG remainingMs) {
    const ULONGLONG totalMinutes = remainingMs / 60000ULL;
    if (totalMinutes >= 60) {
        const ULONGLONG hours = totalMinutes / 60;
        const ULONGLONG minutes = totalMinutes % 60;
        std::stringstream ss;
        ss << hours << "h " << minutes << "m";
        return ss.str();
    }
    const ULONGLONG minutes = totalMinutes == 0 ? 1 : totalMinutes;
    std::stringstream ss;
    ss << minutes << "m";
    return ss.str();
}

std::string ProtectionPhrase(const SessionState& session) {
    std::vector<std::string> parts;
    if (session.preventSleep && OsHasSetThreadExecutionState()) {
        parts.push_back("no sleep");
    }
    if (session.preventDisplayOff) {
        parts.push_back("no screensaver");
    }
    std::string result;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            result += ", ";
        }
        result += parts[i];
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
        session.expiresAtMs = NowMs() + static_cast<ULONGLONG>(preset.fixedMinutes) * 60ULL * 1000ULL;
        break;
    case DurationKind::RestOfDay:
        session.hasExpiry = true;
        session.expiresAtMs = SystemTimeToMsLocal(LocalMidnightTomorrow());
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
    if (NowMs() >= session.expiresAtMs) {
        SessionStop(session);
        expiredNow = true;
    }
}

std::string SessionStatusText(const SessionState& session) {
    if (!session.isActive) {
        return "Off - normal power and screensaver";
    }

    const std::string protection = ProtectionPhrase(session);
    if (protection.empty()) {
        return "On - (no protection active on this OS)";
    }
    if (!session.hasExpiry) {
        return "On - " + protection + " (until you turn off)";
    }

    const ULONGLONG remaining = session.expiresAtMs > NowMs() ? session.expiresAtMs - NowMs() : 0;
    if (remaining == 0) {
        return "On - " + protection + " (expiring)";
    }
    return "On - " + protection + " (" + FormatRemainingMs(remaining) + " left)";
}
