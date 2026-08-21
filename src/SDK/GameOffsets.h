#pragma once
#include <cstdint>
#include "Core/Memory.h"

// ============================================================
// 香肠派对 (Sausage Man) — 偏移常量表
// 来源：dump.cs / il2cpp.h (2026-08-21 dump) + IDA 反编译验证
// ============================================================
namespace Offsets {

// ---- GameWorldClientManager (静态类) ----
inline constexpr uintptr_t GWCM_MyGameWorld   = 0x8;   // BattleWorld*

// ---- BattleWorld ----
inline constexpr uintptr_t BW_startGame       = 0x498; // StartGame*

// ---- StartGame ----
inline constexpr uintptr_t SG_RoleList        = 0x50;  // List<BattleRoleLogic>
inline constexpr uintptr_t SG_RolePlayerIdDic = 0x60;  // Dictionary<long, BattleRoleLogic>
inline constexpr uintptr_t SG_RoleNetList     = 0x98;  // List<RoleNet>

// ---- RoleNet ----
inline constexpr uintptr_t RN_Transform       = 0x40;  // Transform*
inline constexpr uintptr_t RN_RoleNetClient   = 0x58;  // RoleNetClient*
inline constexpr uintptr_t RN_MyRoleLogic     = 0x68;  // BattleRoleLogic*

// ---- RoleNetClient ----
inline constexpr uintptr_t RNC_isLocalPlayer  = 0x1D;  // bool

// ---- BattleRoleLogic ----
inline constexpr uintptr_t BRL_Hp             = 0x224; // float
inline constexpr uintptr_t BRL_MaxHp          = 0x228; // float
inline constexpr uintptr_t BRL_NickName       = 0x768; // string*
inline constexpr uintptr_t BRL_PlayerId       = 0x770; // int64_t
inline constexpr uintptr_t BRL_TeamNum        = 0x7A8; // int64_t

// ---- BattleRoleLogic → AnimatorControl 链路 ----
inline constexpr uintptr_t BRL_RLC_Offset     = 0xAE0; // → RoleLogicComponent
inline constexpr uintptr_t RLC_BR_Offset      = 0x80;  // → BattleRole
inline constexpr uintptr_t BR_RC_Offset       = 0x240; // → RoleControl
inline constexpr uintptr_t RC_AC_Offset       = 0x48;  // → AnimatorControl

// ---- GameData (静态类) ----
inline constexpr uintptr_t GD_WarCamera       = 0x1E0; // CameraController*
inline constexpr uintptr_t GD_LocalRole       = 0x230; // BattleRole*
inline constexpr uintptr_t GD_LocalRoleNet    = 0x238; // RoleNet*

// ---- CameraController ----
inline constexpr uintptr_t CC_MyCamera        = 0x30;  // Camera* (managed)
inline constexpr uintptr_t CC_MyCameraRotationX = 0xA0; // Quaternion (Pitch)
inline constexpr uintptr_t CC_MyCameraRotationY = 0xB0; // Quaternion (Yaw)

// ---- managed Camera → native Camera ----
inline constexpr uintptr_t CAM_CachedPtr      = 0x10;  // native Camera*

// ---- native Camera 矩阵偏移 ----
inline constexpr uintptr_t CAM_ViewMatrix     = 0x80;  // worldToCameraMatrix
inline constexpr uintptr_t CAM_ProjMatrix     = 0xC0;  // projectionMatrix

// ---- RoleAnimatorControlPool (静态类) ----
inline constexpr uintptr_t RACP_animData      = 0x8;   // Dictionary<long, AnimatorControl>*

// ---- AnimatorControl 骨骼字段 ----
inline constexpr uintptr_t AC_LeftHand        = 0x80;  // Transform*
inline constexpr uintptr_t AC_RightHand       = 0x88;
inline constexpr uintptr_t AC_Head            = 0xE8;
inline constexpr uintptr_t AC_Hip             = 0xF8;
inline constexpr uintptr_t AC_SkinBody        = 0x108;
inline constexpr uintptr_t AC_RightFoot       = 0x118;
inline constexpr uintptr_t AC_LeftFoot        = 0x120;
inline constexpr uintptr_t AC_Spine           = 0x190;
inline constexpr uintptr_t AC_MyRole          = 0x168; // BattleRole* 反向引用

// ---- IL2CPP 运行时结构 ----
inline constexpr uintptr_t List_ItemsPtr      = 0x10;  // void** items
inline constexpr uintptr_t List_Count         = 0x18;  // int count
inline constexpr uintptr_t Array_ItemsStart   = 0x20;  // 引用数组元素起始
inline constexpr uintptr_t Str_Length         = 0x10;  // int32
inline constexpr uintptr_t Str_Chars          = 0x14;  // UTF-16 字符

} // namespace Offsets