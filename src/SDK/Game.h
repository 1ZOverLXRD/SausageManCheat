#pragma once
#include "SDK/PlayerManager.h"
#include "SDK/CameraManager.h"

namespace Game {

// 完整帧更新（每帧在 Present Hook 中调用）
void Update();

// 初始化
void Init();

// 是否已初始化
bool IsInitialized();

// 获取玩家列表
std::vector<PlayerInfo>& GetPlayers();

// 获取相机位置
float* GetCamPos();

// 判断是否本地玩家
bool IsLocalPlayer(uintptr_t roleLogic);

// 调试
void DumpState();

} // namespace Game