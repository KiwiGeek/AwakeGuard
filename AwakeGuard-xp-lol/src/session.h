#pragma once

#include "win32_common.h"
#include <string>

enum class DurationKind {
    FixedMinutes,
    RestOfDay,
    Indefinite,
};

struct DurationPreset {
    const wchar_t* label;
    DurationKind kind;
    int fixedMinutes;
};

struct SessionState {
    bool preventSleep = true;
    bool preventDisplayOff = true;
    bool isActive = false;
    ULONGLONG expiresAtMs = 0;
    bool hasExpiry = false;
    int selectedDurationIndex = 1;
};

constexpr DurationPreset kDurationPresets[] = {
    {L"30 minutes", DurationKind::FixedMinutes, 30},
    {L"1 hour", DurationKind::FixedMinutes, 60},
    {L"2 hours", DurationKind::FixedMinutes, 120},
    {L"4 hours", DurationKind::FixedMinutes, 240},
    {L"8 hours", DurationKind::FixedMinutes, 480},
    {L"Rest of today", DurationKind::RestOfDay, 0},
    {L"Until turned off", DurationKind::Indefinite, 0},
};

constexpr int kDurationPresetCount = static_cast<int>(sizeof(kDurationPresets) / sizeof(kDurationPresets[0]));

void SessionApplyExecutionState(const SessionState& session);
void SessionClearExecutionState();
void SessionStart(SessionState& session, int presetIndex);
void SessionStop(SessionState& session);
void SessionRefresh(SessionState& session);
void SessionCheckExpiry(SessionState& session, bool& expiredNow);
std::wstring SessionStatusText(const SessionState& session);
