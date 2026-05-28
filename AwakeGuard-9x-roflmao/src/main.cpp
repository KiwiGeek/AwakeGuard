#include "app.h"

#include "os_compat.h"
#include "paths.h"
#include "ui.h"
#include "util.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, PSTR, int) {
    OsCompatInit();
    UiInitialize();

    const std::vector<std::string> args = ParseCommandLineArgs();
    const std::string cleanupSource = paths::TryGetCleanupSourceExecutable(args);

    if (!AppInitialize(g_app, cleanupSource)) {
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
