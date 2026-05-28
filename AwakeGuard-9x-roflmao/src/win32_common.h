#pragma once

// Windows 95/98/ME baseline — ANSI build (no UNICODE).
#ifdef UNICODE
#undef UNICODE
#endif
#ifdef _UNICODE
#undef _UNICODE
#endif

#ifndef WINVER
#define WINVER 0x0400
#endif
#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0400
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <shlobj.h>

#ifndef ES_CONTINUOUS
#define ES_CONTINUOUS 0x80000000L
#endif
#ifndef ES_SYSTEM_REQUIRED
#define ES_SYSTEM_REQUIRED 0x00000001L
#endif

#ifndef CSIDL_APPDATA
#define CSIDL_APPDATA 0x001a
#endif
#ifndef CSIDL_LOCAL_APPDATA
#define CSIDL_LOCAL_APPDATA 0x001c
#endif

// shell32 export (IE 4+ / Win98); load dynamically on Win95 if missing.
#ifndef CSIDL_FLAG_CREATE
#define CSIDL_FLAG_CREATE 0x8000
#endif
