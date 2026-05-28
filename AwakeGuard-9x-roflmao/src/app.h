#pragma once

#include "win32_common.h"
#include "session.h"

struct AppState {
    HWND settingsWindow = nullptr;
    HWND chkSleep = nullptr;
    HWND chkDisplay = nullptr;
    HWND chkStartup = nullptr;
    HWND cmbDuration = nullptr;
    HWND txtStatus = nullptr;

    SessionState session {};
    NOTIFYICONDATAA trayData {};
    HMENU trayMenu = nullptr;
    HICON iconActive = nullptr;
    HICON iconInactive = nullptr;

    bool suppressStartupChange = true;
    bool startWithWindows = false;
    DWORD lastTrayClickTick = 0;
};

bool AppInitialize(AppState& app, const std::string& cleanupSourceExecutable);
void AppShutdown(AppState& app);
void AppOnSessionChanged(AppState& app, bool showExpiryBalloon);
void AppToggleSession(AppState& app);
void AppApplyStartupState(AppState& app);
void AppFinishStartupInitialization(AppState& app);

extern AppState g_app;
