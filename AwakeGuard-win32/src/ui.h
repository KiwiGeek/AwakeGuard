#pragma once

#include "win32_common.h"
#include <uxtheme.h>

void UiInitialize();
void UiShutdown();

HFONT UiBodyFont();
HFONT UiTitleFont();
HFONT UiSectionFont();

// Layout is authored in 96-DPI pixels; UiScale converts to the DPI the UI fonts were
// created at so control geometry and text stay in proportion.
UINT UiDpi();
int UiScale(int value);
// Returns the outer window size needed for a client area of clientWidth x clientHeight
// (both already scaled) with the given styles.
SIZE UiWindowSizeForClient(int clientWidth, int clientHeight, DWORD style, DWORD exStyle);

void UiApplyFont(HWND hwnd, HFONT font);
void UiApplyFontToChildren(HWND parent, HFONT font);

void UiApplyModernChrome(HWND hwnd);

COLORREF UiTextColor();
COLORREF UiSecondaryTextColor();
HBRUSH UiBackgroundBrush();
HBRUSH UiPanelBrush();

INT_PTR UiRunModalLoop(HWND dialog, HWND owner);
