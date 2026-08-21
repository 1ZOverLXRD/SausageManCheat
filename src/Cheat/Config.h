#pragma once
#include <cstdint>

namespace Config {

// 主开关
extern bool MenuOpen;

// ---- ESP 配置 ----
namespace ESP {
    extern bool Enabled;
    extern bool Box2D;
    extern bool Box3D;
    extern bool Skeleton;
    extern bool HealthBar;
    extern bool Name;
    extern bool Distance;
    extern float MaxDist;   // 最大绘制距离
    extern bool TeamCheck;  // 同队过滤
    extern bool IsLocal;    // 显示本地玩家
}

// ---- Aimbot 配置 ----
namespace Aimbot {
    extern bool Enabled;
    extern int  Key;          // 激活键码 (VK_*)
    extern float MaxSpeed;    // 最大速度 (mickeys/帧)
    extern float MinSpeed;    // 最小速度
    extern float RampDist;    // 减速开始距离 (px)
    extern float DeadZone;    // 死区 (px)
    extern int  AimBone;      // 瞄准骨骼 (BoneIndex)
    extern bool TeamCheck;    // 队友过滤
    extern bool VisibleOnly;  // 只瞄屏幕内
}

// 菜单操作
void ToggleMenu();
void DrawMenu();

// 配置持久化（ini）
void Save();
void Load();

} // namespace Config