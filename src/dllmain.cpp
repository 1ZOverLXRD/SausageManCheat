#include <windows.h>
#include "Core/Log.h"
#include "Core/D3D11Hook.h"
#include "SDK/Game.h"
#include "Cheat/Config.h"

// 外部函数声明
extern void GameDumpState();  // Config.cpp 中引用
void GameDumpState() {
    Game::DumpState();
}

static bool g_exiting = false;
static HMODULE g_hModule = nullptr;

static void RequestUnloadImpl() {
    g_exiting = true;
    Log::Printf("[DLL] 请求卸载...");
}
void RequestUnload() { RequestUnloadImpl(); }

DWORD WINAPI MainThread(LPVOID) {
    // 初始化控制台
    Log::Init();
    Log::Printf("[DLL] SausageMan Cheat 注入成功");

    // 加载配置
    Config::Load();
    Log::Printf("[DLL] 配置加载完成");

    // 初始化 Hook 和游戏 SDK
    if (!D3D11Hook::Init()) {
        Log::Printf("[DLL] D3D11 Hook 初始化失败，3秒后卸载");
        Sleep(3000);
        g_exiting = true;
    }

    // 主循环
    while (!g_exiting) {
        Sleep(100);
    }

    // 清理
    Log::Printf("[DLL] 开始清理...");
    D3D11Hook::Shutdown();
    Log::Shutdown();
    FreeLibraryAndExitThread(g_hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        HANDLE hThread = CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
        if (hThread) CloseHandle(hThread);
    }
    return TRUE;
}