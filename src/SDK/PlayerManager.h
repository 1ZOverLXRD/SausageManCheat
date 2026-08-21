#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include "Core/Memory.h"

// 骨骼枚举（AnimatorControl 字段映射）
enum BoneIndex : int {
    BONE_LEFT_HAND = 0,
    BONE_RIGHT_HAND = 1,
    BONE_HEAD = 2,
    BONE_HIP = 3,
    BONE_SKIN_BODY = 4,
    BONE_RIGHT_FOOT = 5,
    BONE_LEFT_FOOT = 6,
    BONE_SPINE = 7,
    BONE_COUNT = 8
};

// 单个玩家信息（数据采集后由渲染侧读取）
struct PlayerInfo {
    uintptr_t roleNet;        // RoleNet*
    uintptr_t roleLogic;      // BattleRoleLogic*
    uintptr_t roleNetClient;  // RoleNetClient*
    uintptr_t trans;          // Transform*
    bool isLocal = false;     // 本地玩家

    // 每帧更新
    float pos[3] = {0};       // 世界坐标
    float hp = 0, maxHp = 0;  // 血量
    int64_t team = 0;         // 队伍
    char name[32] = {0};      // 名字 (UTF-8)
    bool valid = false;       // 有效（位置可读）
    bool alive = false;       // 存活

    // 骨骼（AnimatorControl，缓存）
    uintptr_t animCtrl = 0;   // 0=未获取, -1=不存在
    int64_t playerId = 0;     // 查表用
    bool boneValid[BONE_COUNT] = {false};
    float bonePos[BONE_COUNT][3] = {0};     // 世界坐标
    float boneScreen[BONE_COUNT][2] = {0};  // 屏幕坐标
    uint32_t boneFailCount = 0;

    // 屏幕坐标（W2S 结果）
    float screenX = 0, screenY = 0;
    bool onScreen = false;
    float distToCam = 0;

    void Reset() {
        valid = false;
        hp = maxHp = 0;
        team = 0;
        name[0] = 0;
        alive = false;
        onScreen = false;
        screenX = screenY = 0;
        distToCam = 0;
    }
};

namespace PlayerManager {

// 每 30 帧全量扫描，中间帧只更新数据
void Update();

// 读取玩家数据（位置/血量/名字）
void ReadPlayerData(PlayerInfo& p);

// 获取/读取骨骼（AnimatorControl 字典查找 + 骨骼位置读取）
void ReadSkeleton(PlayerInfo& p);

// 从 RoleNet+0x40 Transform 读位置（调 get_position）
bool ReadTransformPosition(uintptr_t trans, float* out);

// 玩家列表（渲染侧/瞄准侧读取）
std::vector<PlayerInfo>& GetPlayers();

// 本地玩家指针
uintptr_t GetLocalRoleLogic();
bool IsLocalPlayer(const PlayerInfo& p);

// 日志/调试
void DumpPlayersToLog();

} // namespace PlayerManager