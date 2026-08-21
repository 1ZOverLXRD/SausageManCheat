#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <cstdint>

namespace D3D11Hook {

// 初始化：扫描 VTable 并 Hook Present
bool Init();

// 卸载
void Shutdown();

// 是否已初始化
bool IsInitialized();

// 原始 Present 函数指针
extern HRESULT (STDMETHODCALLTYPE* oPresent)(IDXGISwapChain*, UINT, UINT);

} // namespace D3D11Hook