#include "app.h"

#include "os_compat.h"
#include "paths.h"
#include "ui.h"
#include "util.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, PSTR, int) {
    TraceInit();
    Trace("1. WinMain entered");
    OsCompatInit();
    Trace("2. OS checks done; next: common controls, fonts");
    UiInitialize();
    Trace("3. UI init done; next: command line");

    const PathStr cleanupSource = paths::TryGetCleanupSourceExecutable();
    Trace("4. Command line parsed; next: settings window");

    if (!AppInitialize(g_app, cleanupSource.c_str())) {
        MessageBoxA(nullptr, "Failed to initialize AwakeGuard 9x ROFLMAO.", "AwakeGuard 9x ROFLMAO", MB_ICONERROR | MB_OK);
        return 1;
    }

    Trace("12. Startup complete; entering message loop");
    MSG msg {};
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    AppShutdown(g_app);
    UiShutdown();
    return static_cast<int>(msg.wParam);
}
