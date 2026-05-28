#pragma once

#include "win32_common.h"

bool DialogsShowConfirm(HWND owner, LPCSTR title, LPCSTR message, LPCSTR primary, LPCSTR secondary, bool topmost = false);
void DialogsShowError(HWND owner, LPCSTR title, LPCSTR message, bool topmost = false);
void DialogsShowAbout(HWND owner);
