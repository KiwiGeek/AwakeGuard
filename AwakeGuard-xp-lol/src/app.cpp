#include "app.h"

#include "cleanup.h"
#include "settings_store.h"
#include "settings_wnd.h"
#include "startup.h"
#include "tray.h"
#include "util.h"

AppState g_app {};

void AppApplyStartupState(AppState& app) {
    const bool registryEnabled = StartupIsRegistered();
    if (!registryEnabled && SettingsLoadStartWithWindows()) {
        SettingsSaveStartWithWindows(false);
    }

    app.suppressStartupChange = true;
    app.startWithWindows = registryEnabled;
    if (app.settingsWindow) {
        CheckDlgButton(app.settingsWindow, IDC_CHK_STARTUP, registryEnabled ? BST_CHECKED : BST_UNCHECKED);
    }
    app.suppressStartupChange = false;
}

void AppFinishStartupInitialization(AppState& app) {
    AppApplyStartupState(app);
    app.suppressStartupChange = false;
}

void AppOnSessionChanged(AppState& app, bool showExpiryBalloon) {
    SettingsWindowRefresh(app);
    if (showExpiryBalloon) {
        TrayShowBalloon(app, L"Protection ended - sleep and screensaver are allowed again.");
    }
}

void AppToggleSession(AppState& app) {
    if (app.session.isActive) {
        SessionStop(app.session);
        KillTimer(app.settingsWindow, IDT_REFRESH);
        KillTimer(app.settingsWindow, IDT_EXPIRY);
        AppOnSessionChanged(app, true);
    } else if (app.session.preventSleep || app.session.preventDisplayOff) {
        SessionStart(app.session, app.session.selectedDurationIndex);
        SetTimer(app.settingsWindow, IDT_REFRESH, REFRESH_MS, nullptr);
        SetTimer(app.settingsWindow, IDT_EXPIRY, EXPIRY_MS, nullptr);
        AppOnSessionChanged(app, false);
    }
}

bool AppInitialize(AppState& app, const std::wstring& cleanupSourceExecutable) {
    if (!SettingsWindowCreate(app)) {
        return false;
    }

    TrayInitialize(app);

    if (!cleanupSourceExecutable.empty()) {
        SettingsWindowShow(app, false);
        CleanupOfferPrompt(app.settingsWindow, cleanupSourceExecutable);
        AppFinishStartupInitialization(app);
        return true;
    }

    SettingsWindowHide(app);
    AppFinishStartupInitialization(app);
    return true;
}

void AppShutdown(AppState& app) {
    SessionStop(app.session);
    TrayDispose(app);
}
