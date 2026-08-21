#pragma once

namespace Aimbot {

// 每帧更新（在 Present 中调用）
void Update();

// 当前锁定目标（调试用）
extern int g_targetIndex;

// 获取目标玩家索引
int FindTarget();

// 瞄准移动
void AimAt(float targetX, float targetY, float centerX, float centerY);

} // namespace Aimbot