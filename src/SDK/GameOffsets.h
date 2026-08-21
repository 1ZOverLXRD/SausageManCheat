#pragma once
#include <cstdint>

// ============================================================
// 香肠派对 (Sausage Man) — 偏移常量表
// 来源：dump.cs / il2cpp.h (2026-08-21 dump) + IDA 反编译验证
// 版本：PC (TapTap 58881)
// ============================================================
namespace Offsets {

// ---- GameWorldClientManager (静态类) ----
// 静态字段区布局（il2cpp.h GameWorldClientManager_StaticFields）
inline constexpr uintptr_t GWCM_MyGameWorld   = 0x8;   // BattleWorld*

// ---- BattleWorld ----
inline constexpr uintptr_t BW_startGame       = 0x498; // StartGame*

// ---- StartGame ----
inline constexpr uintptr_t SG_RoleList        = 0x50;  // List<BattleRoleLogic>
inline constexpr uintptr_t SG_RolePlayerIdDic = 0x60;  // Dictionary<long, BattleRoleLogic>
inline constexpr uintptr_t SG_RoleNetList     = 0x98;  // List<RoleNet>

// ---- RoleNet ----
inline constexpr uintptr_t RN_Transform       = 0x40;  // Transform* (private $a)
inline constexpr uintptr_t RN_RoleNetClient   = 0x58;  // RoleNetClient*
inline constexpr uintptr_t RN_MyRoleLogic     = 0x68;  // BattleRoleLogic*

// ---- RoleNetClient ----
inline constexpr uintptr_t RNC_isLocalPlayer  = 0x1D;  // bool

// ---- BattleRoleLogic ----
inline constexpr uintptr_t BRL_Hp             = 0x224; // float
inline constexpr uintptr_t BRL_MaxHp          = 0x228; // float
inline constexpr uintptr_t BRL_NickName       = 0x768; // string*
inline constexpr uintptr_t BRL_PlayerId       = 0x770; // int64_t (il2cpp.h _q)
inline constexpr uintptr_t BRL_TeamNum        = 0x7A8; // int64_t

// ---- GameData (静态类) ----
inline constexpr uintptr_t GD_WarCamera       = 0x1E0; // CameraController*
inline constexpr uintptr_t GD_LocalRole       = 0x230; // BattleRole*
inline constexpr uintptr_t GD_LocalRoleNet    = 0x238; // RoleNet*

// ---- CameraController ----
inline constexpr uintptr_t CC_MyCamera        = 0x30;  // Camera* (managed)

// ---- managed Camera → native Camera ----
inline constexpr uintptr_t CAM_CachedPtr      = 0x10;  // native Camera*

// ---- native Camera 矩阵偏移 (IDA 反编译确认) ----
inline constexpr uintptr_t CAM_ViewMatrix     = 0x80;  // worldToCameraMatrix (128)
inline constexpr uintptr_t CAM_ProjMatrix     = 0xC0;  // projectionMatrix (192)

// ---- RoleAnimatorControlPool (静态类) ----
inline constexpr uintptr_t RACP_animData      = 0x8;   // Dictionary<long, AnimatorControl>*

// ---- AnimatorControl 骨骼字段 ----
inline constexpr uintptr_t AC_LeftHand        = 0x80;  // Transform*
inline constexpr uintptr_t AC_RightHand       = 0x88;  // Transform*
inline constexpr uintptr_t AC_Head            = 0xE8;  // Transform*
inline constexpr uintptr_t AC_Hip             = 0xF8;  // Transform*
inline constexpr uintptr_t AC_SkinBody        = 0x108; // Transform*
inline constexpr uintptr_t AC_RightFoot       = 0x118; // Transform*
inline constexpr uintptr_t AC_LeftFoot        = 0x120; // Transform*
inline constexpr uintptr_t AC_Spine           = 0x190; // Transform*
inline constexpr uintptr_t AC_MyRole          = 0x168; // BattleRole* 反向引用

// ---- IL2CPP List<T> 布局 ----
inline constexpr uintptr_t List_ItemsPtr      = 0x10;  // void** items 数组
inline constexpr uintptr_t List_Count         = 0x18;  // int count
// IL2CPP 数组 (Il2CppArray) 布局：
// +0x10 = max_length (int)  → 实际用 +0x18 的 items 起始（引用类型）
// 对 List<T>：items 数组是 Object[]，元素从 +0x20 开始（每个 8 字节）
inline constexpr uintptr_t Array_ItemsStart   = 0x20;  // 引用数组元素起始

// ---- IL2CPP string 布局 ----
inline constexpr uintptr_t Str_Length         = 0x10;  // int32
inline constexpr uintptr_t Str_Chars          = 0x14;  // UTF-16 字符

} // namespace Offsets