#pragma once

#include "win32_common.h"

bool DialogsShowConfirm(HWND owner, LPCWSTR title, LPCWSTR message, LPCWSTR primary, LPCWSTR secondary, bool topmost = false);
void DialogsShowError(HWND owner, LPCWSTR title, LPCWSTR message, bool topmost = false);
void DialogsShowAbout(HWND owner);
