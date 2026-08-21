#pragma once
#include <imgui.h>

namespace ESP {

// 每帧绘制（在 ImGui 渲染层中调用）
void Draw();

// 绘制工具（DrawList 简化封装）
ImDrawList* GetDrawList();

} // namespace ESP