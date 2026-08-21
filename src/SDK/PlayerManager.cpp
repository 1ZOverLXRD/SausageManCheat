#include "SDK/PlayerManager.h"
#include "SDK/GameObjects.h"
#include "SDK/IL2CPP.h"
#include "Core/Log.h"
#include <cmath>

namespace PlayerManager {

// 静态字段基址
static uintptr_t s_gwmStatic = 0;
static uintptr_t s_localRoleLogic = 0;
static std::vector<PlayerInfo> s_players;

// ---------------- 内部工具 ----------------

// il2cpp 类 static_fields 偏移固定 0xB8（il2cpp.h 确认）
__declspec(noinline) static uintptr_t GetStaticFields(void* klass) {
    uintptr_t base = (uintptr_t)klass;
    if (!base) return 0;
    return Memory::ReadPtr(base + 0xB8);
}

// 读取链：GWM → MyGameWorld → startGame → RoleNetList
__declspec(noinline) static bool SafeReadGameData(uintptr_t gwmStatic,
    uintptr_t* outRoleNetList, uintptr_t* outItems, int* outCount)
{
    __try {
        uintptr_t gameWorld = Memory::ReadPtr(gwmStatic + Offsets::GWCM_MyGameWorld);
        if (!gameWorld) return false;
        uintptr_t startGame = Memory::ReadPtr(gameWorld + Offsets::BW_startGame);
        if (!startGame) return false;
        *outRoleNetList = Memory::ReadPtr(startGame + Offsets::SG_RoleNetList);
        if (!*outRoleNetList) return false;
        *outItems = Memory::ReadPtr(*outRoleNetList + Offsets::List_ItemsPtr);
        *outCount = Memory::ReadInt32(*outRoleNetList + Offsets::List_Count);
        if (!*outItems || *outCount <= 0 || *outCount > 128) return false;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        Log::Printf("[Player] 读取异常 code=0x%X", GetExceptionCode());
        return false;
    }
}

// ---------------- 主更新 ----------------

void Update() {
    if (!s_gwmStatic) {
        void* gwcmClass = IL2CPP::FindClass("Assembly-CSharp.dll", "", "GameWorldClientManager");
        if (!gwcmClass) return;
        s_gwmStatic = GetStaticFields(gwcmClass);
        if (!s_gwmStatic) return;
        Log::Printf("[Player] GWM static_fields = 0x%llX", (unsigned long long)s_gwmStatic);
    }

    uintptr_t roleNetList = 0, items = 0;
    int count = 0;
    if (!SafeReadGameData(s_gwmStatic, &roleNetList, &items, &count)) {
        if (!s_players.empty()) s_players.clear();
        return;
    }

    s_players.clear();
    s_players.reserve(count);

    for (int i = 0; i < count; i++) {
        // 用 GameSDK 对象模型读取
        GameSDK::RoleNet roleNet(Memory::ReadPtr(items + Offsets::Array_ItemsStart + (uintptr_t)i * 8));
        if (!roleNet.valid()) continue;

        GameSDK::RoleNetClient client = roleNet.client();
        GameSDK::Transform trans = roleNet.transform();

        PlayerInfo p;
        p.roleNet = roleNet.raw();
        p.roleLogic = roleNet.roleLogic();
        p.trans = trans.raw();
        p.isLocal = client.valid() && client.isLocalPlayer();

        ReadPlayerData(p);
        ReadSkeleton(p);
        s_players.push_back(p);
    }

    // 本地玩家指针
    void* gdClass = IL2CPP::FindClass("Assembly-CSharp.dll", "", "GameData");
    if (gdClass) {
        uintptr_t gdStatic = GetStaticFields(gdClass);
        if (gdStatic) {
            s_localRoleLogic = Memory::ReadPtr(gdStatic + Offsets::GD_LocalRole);
        }
    }
}

// ---------------- 读取玩家数据 ----------------

static bool ReadIl2CppString(uintptr_t strObj, char* out, int outSize) {
    if (!strObj || !out || outSize <= 0) return false;
    int len = Memory::ReadInt32(strObj + Offsets::Str_Length);
    if (len <= 0 || len >= outSize) return false;
    wchar_t wbuf[64] = {0};
    if (len >= 64) len = 63;
    if (!Memory::ReadBytes(strObj + Offsets::Str_Chars, wbuf, (size_t)len * 2)) return false;
    wbuf[len] = 0;
    WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, out, outSize, nullptr, nullptr);
    return true;
}

void ReadPlayerData(PlayerInfo& p) {
    p.Reset();
    if (!p.trans) return;

    GameSDK::Transform trans(p.trans);
    if (!trans.getPosition(p.pos)) return;
    if (!p.roleLogic) return;

    GameSDK::RoleLogic logic(p.roleLogic);
    p.hp = logic.hp();
    p.maxHp = logic.maxHp();
    p.team = logic.team();
    ReadIl2CppString(logic.nickName(), p.name, sizeof(p.name));
    p.valid = true;
}

bool ReadTransformPosition(uintptr_t trans, float* out) {
    return GameSDK::Transform(trans).getPosition(out);
}

void ReadSkeleton(PlayerInfo& p) {
    if (!p.roleLogic) return;
    p.animCtrl = 0;

    // 用 GameSDK 获取 AnimatorControl
    GameSDK::RoleLogic logic(p.roleLogic);
    uintptr_t ac = logic.animatorControl();
    if (!ac) return;
    p.animCtrl = ac;

    GameSDK::AnimatorControl anim(ac);
    for (int b = 0; b < BONE_COUNT; b++) {
        GameSDK::Transform boneTrans = anim.bone(b);
        if (!boneTrans.valid()) { p.boneValid[b] = false; continue; }
        if (boneTrans.getPosition(p.bonePos[b])) {
            p.boneValid[b] = true;
        } else {
            p.boneValid[b] = false;
        }
    }

    // === 存活检测：手掌位置变化 ===
    if (p.boneValid[BONE_LEFT_HAND] || p.boneValid[BONE_RIGHT_HAND]) {
        float avgHand[3] = {0};
        int handCount = 0;
        if (p.boneValid[BONE_LEFT_HAND]) {
            avgHand[0] += p.bonePos[BONE_LEFT_HAND][0];
            avgHand[1] += p.bonePos[BONE_LEFT_HAND][1];
            avgHand[2] += p.bonePos[BONE_LEFT_HAND][2];
            handCount++;
        }
        if (p.boneValid[BONE_RIGHT_HAND]) {
            avgHand[0] += p.bonePos[BONE_RIGHT_HAND][0];
            avgHand[1] += p.bonePos[BONE_RIGHT_HAND][1];
            avgHand[2] += p.bonePos[BONE_RIGHT_HAND][2];
            handCount++;
        }
        if (handCount > 0) {
            avgHand[0] /= handCount;
            avgHand[1] /= handCount;
            avgHand[2] /= handCount;

            float dx = avgHand[0] - p.lastHandPos[0];
            float dy = avgHand[1] - p.lastHandPos[1];
            float dz = avgHand[2] - p.lastHandPos[2];
            float dist = sqrtf(dx*dx + dy*dy + dz*dz);

            p.lastHandPos[0] = avgHand[0];
            p.lastHandPos[1] = avgHand[1];
            p.lastHandPos[2] = avgHand[2];

            if (dist < 0.01f) {
                p.stillFrames++;
                if (p.stillFrames >= 60) {
                    if (p.state != STATE_INACTIVE)
                        Log::Printf("[Player] %s 判定死亡/挂机(1s未动), 移除", p.name);
                    p.state = STATE_INACTIVE;
                } else if (p.stillFrames >= 30) {
                    if (p.state == STATE_ACTIVE)
                        Log::Printf("[Player] %s 疑似死亡(0.5s未动)", p.name);
                    p.state = STATE_SUSPECT;
                }
            } else {
                if (p.state != STATE_ACTIVE)
                    Log::Printf("[Player] %s 恢复活跃", p.name);
                p.stillFrames = 0;
                p.state = STATE_ACTIVE;
            }
        }
    }
}

// ---------------- 对外接口 ----------------

std::vector<PlayerInfo>& GetPlayers() { return s_players; }
uintptr_t GetLocalRoleLogic() { return s_localRoleLogic; }
bool IsLocalPlayer(const PlayerInfo& p) { return p.isLocal; }

void DumpPlayersToLog() {
    Log::Printf("=== 玩家列表 dump ===");
    for (auto& p : s_players) {
        Log::Printf("  RoleNet=0x%llX Logic=0x%llX name=%s hp=%.1f/%.1f local=%d anim=0x%llX",
            (unsigned long long)p.roleNet, (unsigned long long)p.roleLogic,
            p.name, p.hp, p.maxHp, p.isLocal ? 1 : 0,
            (unsigned long long)p.animCtrl);
    }
}

} // namespace PlayerManager