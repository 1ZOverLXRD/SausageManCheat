#include "Core/Log.h"
#include <ctime>
#include <filesystem>

namespace Log {

static FILE* g_file = nullptr;
static HANDLE g_console = nullptr;
static bool g_hasConsole = false;

bool CreateConsoleWindow() {
    // 如果已有控制台（调试器附加），直接复用
    if (GetConsoleWindow()) {
        g_hasConsole = true;
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        return true;
    }
    // 创建新控制台
    if (AllocConsole()) {
        SetConsoleTitleW(L"SausageMan Cheat Log");
        // 大字体（用户偏好）
        CONSOLE_FONT_INFOEX cfi = {};
        cfi.cbSize = sizeof(cfi);
        cfi.nFont = 0;
        cfi.dwFontSize = {0, 16};
        cfi.FontFamily = FF_DONTCARE;
        wcscpy_s(cfi.FaceName, L"Consolas");
        SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);

        g_console = GetStdHandle(STD_OUTPUT_HANDLE);
        if (g_console && g_console != INVALID_HANDLE_VALUE) {
            freopen("CONOUT$", "w", stdout);
            freopen("CONOUT$", "w", stderr);
            g_hasConsole = true;
        }
    }
    return g_hasConsole;
}

void Init() {
    g_hasConsole = CreateConsoleWindow();

    // 文件日志（放 DLL 同目录旁，方便找）
    char path[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    std::string dir = std::filesystem::path(path).parent_path().string();
    std::string logPath = dir + "\\SausageMan_Cheat.log";
    g_file = fopen(logPath.c_str(), "a");
    if (g_file) {
        time_t now = time(nullptr);
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        fprintf(g_file, "\n==== [%s] DLL attached ====\n", buf);
        fflush(g_file);
    }

    if (g_hasConsole)
        printf("[Log] 控制台初始化完成, 日志文件: %s\n", logPath.c_str());
}

void Shutdown() {
    if (g_file) {
        fclose(g_file);
        g_file = nullptr;
    }
    if (g_hasConsole && GetConsoleWindow()) {
        FreeConsole();
        g_hasConsole = false;
    }
}

static void PrintTimePrefix(FILE* out) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    fprintf(out, "[%02d:%02d:%02d.%03d] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

void Printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if (g_console && g_hasConsole) {
        PrintTimePrefix(stdout);
        vprintf(fmt, args);
        printf("\n");
        fflush(stdout);
    }
    if (g_file) {
        PrintTimePrefix(g_file);
        vfprintf(g_file, fmt, args);
        fprintf(g_file, "\n");
        fflush(g_file);
    }

    va_end(args);
}

void Debug(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (g_console && g_hasConsole) {
        vprintf(fmt, args);
        printf("\n");
        fflush(stdout);
    }
    va_end(args);
}

} // namespace Log