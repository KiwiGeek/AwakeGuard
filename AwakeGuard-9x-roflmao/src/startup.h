#pragma once

#include "win32_common.h"

struct AppState;

enum class StartupEnableResult {
    Enabled,
    Relocating,
    Failed,
};

bool StartupIsRegistered();
StartupEnableResult StartupTryEnable(AppState& app, bool offerRelocate);
void StartupDisable();
