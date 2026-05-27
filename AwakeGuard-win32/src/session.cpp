#include "session.h"

#include "win32_common.h"

#include <sstream>
#include <vector>

namespace {

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

std::wstring FormatRemainingMs(ULONGLONG remainingMs) {
    const ULONGLONG totalMinutes = remainingMs / 60000ULL;
    if (totalMinutes >= 60) {
        const ULONGLONG hours = totalMinutes / 60;
        const ULONGLONG minutes = totalMinutes % 60;
        std::wstringstream ss;
        ss << hours << L"h " << minutes << L"m";
        return ss.str();
    }
    const ULONGLONG minutes = totalMinutes == 0 ? 1 : totalMinutes;
    return std::to_wstring(minutes) + L"m";
}

std::wstring ProtectionPhrase(const SessionState& session) {
    std::vector<std::wstring> parts;
    if (session.preventSleep) {
        parts.emplace_back(L"no sleep");
    }
    if (session.preventDisplayOff) {
        parts.emplace_back(L"no lock/screensaver");
    }
    std::wstring result;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            result += L", ";
        }
        result += parts[i];
    }
    return result;
}

}  // namespace

void SessionApplyExecutionState(const SessionState& session) {
    DWORD flags = ES_CONTINUOUS;
    if (session.preventSleep) {
        flags |= ES_SYSTEM_REQUIRED;
    }
    if (session.preventDisplayOff) {
        flags |= ES_DISPLAY_REQUIRED;
    }
    SetThreadExecutionState(flags);
}

void SessionClearExecutionState() {
    SetThreadExecutionState(ES_CONTINUOUS);
}

void SessionStart(SessionState& session, int presetIndex) {
    if (!session.preventSleep && !session.preventDisplayOff) {
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
    if (!session.preventSleep && !session.preventDisplayOff) {
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

std::wstring SessionStatusText(const SessionState& session) {
    if (!session.isActive) {
        return L"Off \x2014 normal power and lock behavior";
    }

    const std::wstring protection = ProtectionPhrase(session);
    if (!session.hasExpiry) {
        return L"On \x2014 " + protection + L" (until you turn off)";
    }

    const ULONGLONG remaining = session.expiresAtMs > NowMs() ? session.expiresAtMs - NowMs() : 0;
    if (remaining == 0) {
        return L"On \x2014 " + protection + L" (expiring)";
    }
    return L"On \x2014 " + protection + L" (" + FormatRemainingMs(remaining) + L" left)";
}
