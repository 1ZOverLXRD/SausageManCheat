#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include "Core/Memory.h"

// 骨骼枚举
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

// 玩家状态
enum PlayerState : int {
    STATE_ACTIVE = 0,
    STATE_SUSPECT = 1,
    STATE_INACTIVE = 2,
};

// 单个玩家信息（数据采集后由渲染侧读取）
struct PlayerInfo {
    uintptr_t roleNet = 0;
    uintptr_t roleLogic = 0;
    uintptr_t trans = 0;
    bool isLocal = false;

    float pos[3] = {0};
    float hp = 0, maxHp = 0;
    int64_t team = 0;
    char name[32] = {0};
    bool valid = false;

    // 骨骼
    uintptr_t animCtrl = 0;
    bool boneValid[BONE_COUNT] = {false};
    float bonePos[BONE_COUNT][3] = {0};
    float boneScreen[BONE_COUNT][2] = {0};

    // 屏幕坐标
    float screenX = 0, screenY = 0;
    bool onScreen = false;
    float distToCam = 0;

    // 存活状态
    PlayerState state = STATE_ACTIVE;
    int stillFrames = 0;
    float lastHandPos[3] = {0};

    void Reset() {
        valid = false;
        hp = maxHp = 0;
        team = 0;
        name[0] = 0;
        onScreen = false;
        screenX = screenY = 0;
        distToCam = 0;
        state = STATE_ACTIVE;
        stillFrames = 0;
    }
};

namespace PlayerManager {

// 每帧全量更新
void Update();

// 读取玩家数据（位置/血量/名字）
void ReadPlayerData(PlayerInfo& p);

// 读取骨骼
void ReadSkeleton(PlayerInfo& p);

// 调 Transform.get_position
bool ReadTransformPosition(uintptr_t trans, float* out);

// 只读访问
std::vector<PlayerInfo>& GetPlayers();
uintptr_t GetLocalRoleLogic();
bool IsLocalPlayer(const PlayerInfo& p);

// 调试
void DumpPlayersToLog();

} // namespace PlayerManager