#pragma once
#include <cstdint>

namespace Aimbot {

// 每帧更新（在 Present 中调用）
void Update();

// 当前锁定目标（调试用）
extern int g_targetIndex;

} // namespace Aimbot