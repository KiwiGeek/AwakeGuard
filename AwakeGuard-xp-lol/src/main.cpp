#include "app.h"

#include "paths.h"
#include "ui.h"
#include "util.h"

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    UiInitialize();

    const std::vector<std::wstring> args = ParseCommandLineArgs();
    const std::wstring cleanupSource = paths::TryGetCleanupSourceExecutable(args);

    if (!AppInitialize(g_app, cleanupSource)) {
        MessageBoxW(nullptr, L"Failed to initialize AwakeGuard XP LOL.", L"AwakeGuard XP LOL", MB_ICONERROR | MB_OK);
        return 1;
    }

    MSG msg {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    AppShutdown(g_app);
    UiShutdown();
    return static_cast<int>(msg.wParam);
}
