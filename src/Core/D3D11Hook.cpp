#include "Core/D3D11Hook.h"
#include "Core/Log.h"
#include "SDK/Game.h"
#include "SDK/IL2CPP.h"
#include "Cheat/Config.h"
#include "Cheat/ESP.h"
#include "Cheat/Aimbot.h"

#include <imgui.h>
// 确保 IMGUI_IMPL_API 可用（DLL 内部编译时定义为空）
#ifndef IMGUI_IMPL_API
#define IMGUI_IMPL_API
#endif
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
// imgui_impl_win32.h 的 WndProcHandler 声明在 #if 0 里（防止拖入 windows.h 依赖）
// 官方要求使用者自己声明。我们已 include windows.h（D3D11Hook.h 间接），手动声明：
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace D3D11Hook {

// VTable 索引
constexpr int PRESENT_INDEX = 8;
constexpr int RESIZE_INDEX = 13;

// 原始函数指针
HRESULT (STDMETHODCALLTYPE* oPresent)(IDXGISwapChain*, UINT, UINT) = nullptr;
static void** g_origVTable = nullptr;
static IDXGISwapChain* g_swapChain = nullptr;

static bool g_initialized = false;
static WNDPROC g_origWndProc = nullptr;
static HWND g_hwnd = nullptr;

// ---- 前向声明 ----
static HRESULT STDMETHODCALLTYPE hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
static LRESULT CALLBACK hkWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 创建临时设备扫描 VTable
static void** GetSwapChainVTable() {
    // 创建临时 D3D11 设备 + SwapChain 来读 VTable
    ID3D11Device* dev = nullptr;
    IDXGISwapChain* sc = nullptr;
    D3D_FEATURE_LEVEL featureLevel;
    ID3D11DeviceContext* ctx = nullptr;

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.Width = 1;
    sd.BufferDesc.Height = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = GetDesktopWindow();
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION, &sd, &sc, &dev, &featureLevel, &ctx);

    if (FAILED(hr)) {
        // 回退 WARP
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION, &sd, &sc, &dev, &featureLevel, &ctx);
        if (FAILED(hr)) {
            Log::Printf("[D3D11] 创建临时设备失败 hr=0x%X", hr);
            return nullptr;
        }
    }

    void** vtable = *(void***)sc;

    // 释放临时设备
    if (sc) sc->Release();
    if (ctx) ctx->Release();
    if (dev) dev->Release();

    return vtable;
}

// 初始化 ImGui
static void InitImGui(IDXGISwapChain* pSwapChain) {
    // 获取设备
    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    HRESULT hr = pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&dev);
    if (FAILED(hr) || !dev) {
        Log::Printf("[D3D11] GetDevice 失败 hr=0x%X", hr);
        return;
    }
    dev->GetImmediateContext(&ctx);
    if (!ctx) {
        Log::Printf("[D3D11] GetImmediateContext 失败");
        dev->Release();
        return;
    }

    // 获取窗口句柄
    DXGI_SWAP_CHAIN_DESC sd;
    pSwapChain->GetDesc(&sd);
    g_hwnd = sd.OutputWindow;

    // 设置 WndProc
    g_origWndProc = (WNDPROC)SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, (LONG_PTR)hkWndProc);

    // ImGui 初始化
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.IniFilename = nullptr;  // 不保存 ini

    // 字体（大字体，用户偏好）
    ImFontConfig fontCfg;
    fontCfg.SizePixels = 16.0f;
    io.Fonts->AddFontDefault(&fontCfg);

    // 中文 fallback
    const char* fontPaths[] = {
        "C:\\Windows\\Fonts\\msyh.ttc",
        "C:\\Windows\\Fonts\\simsun.ttc",
        "C:\\Windows\\Fonts\\simhei.ttf"
    };
    for (auto fp : fontPaths) {
        if (FILE* f = fopen(fp, "rb")) {
            fclose(f);
            ImFontConfig cfg;
            cfg.MergeMode = true;
            cfg.FontNo = 0; // TTC 集合索引
            cfg.GlyphMinAdvanceX = 16.0f;
            static const ImWchar range[] = {0x4E00, 0x9FFF, 0x3000, 0x303F, 0};
            io.Fonts->AddFontFromFileTTF(fp, 16.0f, &cfg, range);
            break;
        }
    }

    ImGui::StyleColorsLight();  // 亮色主题

    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(dev, ctx);

    dev->Release();
    ctx->Release();

    Log::Printf("[D3D11] ImGui 初始化完成 hwnd=0x%p", g_hwnd);
}

// ---- Hook Present ----
bool Init() {
    if (g_initialized) return true;

    // 获取 VTable
    void** vtable = GetSwapChainVTable();
    if (!vtable) {
        Log::Printf("[D3D11] 获取 VTable 失败");
        return false;
    }

    // 保存原始 Present
    oPresent = (HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT))vtable[PRESENT_INDEX];
    Log::Printf("[D3D11] oPresent=0x%p", oPresent);

    // 保存原始 VTable 指针用于替换
    g_origVTable = vtable;

    // 替换 VTable 槽
    DWORD oldProtect;
    VirtualProtect(&vtable[PRESENT_INDEX], 8, PAGE_READWRITE, &oldProtect);
    vtable[PRESENT_INDEX] = &hkPresent;
    VirtualProtect(&vtable[PRESENT_INDEX], 8, oldProtect, &oldProtect);

    g_initialized = true;
    Log::Printf("[D3D11] Hook 安装成功");
    return true;
}

void Shutdown() {
    if (!g_initialized) return;

    // 恢复 WndProc
    if (g_hwnd && g_origWndProc) {
        SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, (LONG_PTR)g_origWndProc);
    }

    // 恢复 VTable
    if (g_origVTable) {
        DWORD oldProtect;
        VirtualProtect(&g_origVTable[PRESENT_INDEX], 8, PAGE_READWRITE, &oldProtect);
        g_origVTable[PRESENT_INDEX] = oPresent;
        VirtualProtect(&g_origVTable[PRESENT_INDEX], 8, oldProtect, &oldProtect);
    }

    // ImGui 清理
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    g_initialized = false;
    Log::Printf("[D3D11] 卸载完成");
}

bool IsInitialized() { return g_initialized; }

// ---- WndProc ----
static LRESULT CALLBACK hkWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // ImGui 输入处理（键盘 + 鼠标）
    if (ImGui::GetCurrentContext()) {
        ImGuiIO& io = ImGui::GetIO();
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
            return true;

        // 菜单键（DEL）切换
        if (msg == WM_KEYDOWN && wParam == VK_DELETE && !io.WantCaptureKeyboard) {
            Config::ToggleMenu();
            return true;
        }
    }

    if (g_origWndProc)
        return CallWindowProcW(g_origWndProc, hwnd, msg, wParam, lParam);
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ---- 每帧渲染（在 hkPresent 中调用） ----
static ID3D11RenderTargetView* g_pRTV = nullptr;
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dCtx = nullptr;

static bool EnsureRenderTarget(IDXGISwapChain* pSwapChain) {
    // GetDevice 拿设备（首次）
    if (!g_pd3dDevice) {
        HRESULT hr = pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pd3dDevice);
        if (FAILED(hr) || !g_pd3dDevice) return false;
        g_pd3dDevice->GetImmediateContext(&g_pd3dCtx);
        if (!g_pd3dCtx) return false;
    }

    // 如果 RTV 已存在，检查 swapchain 尺寸是否变化（ResizeBuffers 后需重建）
    if (g_pRTV) {
        ID3D11Texture2D* pBB = nullptr;
        if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBB))) {
            D3D11_TEXTURE2D_DESC desc;
            pBB->GetDesc(&desc);
            pBB->Release();
            // 简化：不检查尺寸变化，直接用同一个 RTV（backbuffer 是同一个对象）
        }
        return true;
    }

    // 创建 RTV
    ID3D11Texture2D* pBackBuffer = nullptr;
    HRESULT hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    if (FAILED(hr) || !pBackBuffer) return false;

    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRTV);
    pBackBuffer->Release();
    if (FAILED(hr)) return false;

    Log::Printf("[D3D11] RTV 创建成功: 0x%p", g_pRTV);
    return true;
}

static void RenderFrame(IDXGISwapChain* pSwapChain) {
    if (!EnsureRenderTarget(pSwapChain)) return;

    // 保存当前 RTV/DSV/Viewport
    ID3D11RenderTargetView* oldRtv = nullptr;
    ID3D11DepthStencilView* oldDsv = nullptr;
    g_pd3dCtx->OMGetRenderTargets(1, &oldRtv, &oldDsv);

    // 设置我们的 RTV，清空深度（不绑 DSV 防止深度测试影响）
    g_pd3dCtx->OMSetRenderTargets(1, &g_pRTV, nullptr);

    // 设置 viewport（全屏）
    RECT rect;
    GetClientRect(g_hwnd, &rect);
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)(rect.right - rect.left);
    vp.Height = (FLOAT)(rect.bottom - rect.top);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    if (vp.Width < 1) vp.Width = 1920;
    if (vp.Height < 1) vp.Height = 1080;
    g_pd3dCtx->RSSetViewports(1, &vp);

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Unity Raw Input 陷阱：WM_MOUSEMOVE 不达 WndProc，需每帧手动设置鼠标
    ImGuiIO& io = ImGui::GetIO();
    if (Config::MenuOpen) {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(g_hwnd, &pt);
        io.MousePos = ImVec2((float)pt.x, (float)pt.y);
        io.MouseDown[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        io.MouseDown[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
        io.MouseDown[2] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
        io.MouseWheel = 0;  // 滚轮由 wndProc 处理
    }

    if (Config::MenuOpen) {
        Config::DrawMenu();
    }

    // ESP 绘制（在 ImGui 渲染层中）
    if (Config::ESP::Enabled) {
        ESP::Draw();
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // 恢复原 RTV/DSV
    g_pd3dCtx->OMSetRenderTargets(1, &oldRtv, oldDsv);
    if (oldRtv) oldRtv->Release();
    if (oldDsv) oldDsv->Release();
}

// ---- hkPresent ----
static HRESULT STDMETHODCALLTYPE hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    static bool imGuiInit = false;
    if (!imGuiInit) {
        InitImGui(pSwapChain);
        Log::Printf("[D3D11] 首次 Present 触发，ImGui 初始化");
        imGuiInit = true;
    }

    // 数据采集（无 GCDisable——用户确认去掉）
    __try {
        if (!Game::IsInitialized())
            Game::Init();

        Game::Update();
        Aimbot::Update();
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        DWORD code = GetExceptionCode();
        Log::Printf("[D3D11] 数据采集异常 code=0x%X", code);
    }

    // ImGui 渲染（不包在 GCDisable 里，避免渲染出错影响 GC 状态）
    __try {
        RenderFrame(pSwapChain);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        Log::Printf("[D3D11] 渲染异常 code=0x%X", GetExceptionCode());
    }

    // 调用原始 Present
    return oPresent(pSwapChain, SyncInterval, Flags);
}

} // namespace D3D11Hook