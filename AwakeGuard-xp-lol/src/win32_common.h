#pragma once

// Windows XP (5.01) target for the MinGW "museum edition" build.
#ifndef WINVER
#define WINVER 0x0501
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>

#ifndef ES_DISPLAY_REQUIRED
#define ES_DISPLAY_REQUIRED 0x00000002L
#endif
