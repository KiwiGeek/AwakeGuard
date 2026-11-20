#include "app.h"

#include "os_compat.h"
#include "paths.h"
#include "ui.h"
#include "util.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, PSTR, int) {
    OsCompatInit();
    UiInitialize();

    const PathStr cleanupSource = paths::TryGetCleanupSourceExecutable();

    if (!AppInitialize(g_app, cleanupSource.c_str())) {
        MessageBoxA(nullptr, "Failed to initialize AwakeGuard 9x ROFLMAO.", "AwakeGuard 9x ROFLMAO", MB_ICONERROR | MB_OK);
        return 1;
    }

    MSG msg {};
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    AppShutdown(g_app);
    UiShutdown();
    return static_cast<int>(msg.wParam);
}
