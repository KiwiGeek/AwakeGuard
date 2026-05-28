#pragma once

#include "app.h"

void TrayInitialize(AppState& app);
void TrayDispose(AppState& app);
void TrayUpdate(AppState& app);
void TrayShowBalloon(AppState& app, LPCWSTR message);
LRESULT TrayHandleMessage(AppState& app, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
