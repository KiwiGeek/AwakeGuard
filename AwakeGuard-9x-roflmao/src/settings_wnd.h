#pragma once

#include "app.h"

enum SettingsControlId : int {
    IDC_CHK_SLEEP = 2001,
    IDC_CHK_DISPLAY,
    IDC_CHK_STARTUP,
    IDC_CMB_DURATION,
    IDC_TXT_STATUS,
    IDC_BTN_ABOUT,
    IDC_BTN_TURNOFF,
    IDC_BTN_TURNON,
};

bool SettingsWindowCreate(AppState& app);
void SettingsWindowShow(AppState& app, bool refreshStartupState);
void SettingsWindowHide(AppState& app);
void SettingsWindowRefresh(AppState& app);
LRESULT CALLBACK SettingsWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
