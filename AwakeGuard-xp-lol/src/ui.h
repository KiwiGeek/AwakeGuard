#pragma once

#include "win32_common.h"

void UiInitialize();
void UiShutdown();

HFONT UiBodyFont();
HFONT UiTitleFont();
HFONT UiSectionFont();

void UiApplyFont(HWND hwnd, HFONT font);
void UiApplyFontToChildren(HWND parent, HFONT font);

void UiApplyModernChrome(HWND hwnd);

COLORREF UiTextColor();
COLORREF UiSecondaryTextColor();
HBRUSH UiBackgroundBrush();
HBRUSH UiPanelBrush();

INT_PTR UiRunModalLoop(HWND dialog, HWND owner);
