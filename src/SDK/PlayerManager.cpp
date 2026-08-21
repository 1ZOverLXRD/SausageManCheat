#include "SDK/PlayerManager.h"
#include "SDK/IL2CPP.h"
#include "SDK/GameOffsets.h"
#include "Core/Log.h"
#include <cmath>

namespace PlayerManager {

static std::vector<PlayerInfo> s_players;
static int s_scanCounter = 0;  // 全量扫描计数（每 30 帧重建一次）
static uintptr_t s_localRoleLogic = 0;
static uintptr_t s_gwmStatic = 0;    // GameWorldClientManager 静态字段基址
static void* s_getPositionMethod = nullptr;
static void* s_getAnimControlMethod = nullptr;
static void* s_int64Class = nullptr;
static void* (*s_valueBoxFn)(void*, void*) = nullptr;

// ---------------- 内部工具 ----------------

// IL2CPP 类 static_fields 偏移探测
// Il2CppClass_1 = 20 指针 × 8 = 0xA0，static_fields 在 0xB8（il2cpp.h 确认）
// 直接固定偏移，不探测（避免捡到 nestedTypes/implementedInterfaces 的误判）
__declspec(noinline) static uintptr_t GetStaticFields(void* klass) {
    uintptr_t base = (uintptr_t)klass;
    if (!base) return 0;
    uintptr_t candidate = *(uintptr_t*)(base + 0xB8);
    if (candidate > 0x10000 && candidate < 0x7FFFFFFF0000) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery((LPCVOID)candidate, &mbi, sizeof(mbi)) &&
            mbi.State == MEM_COMMIT &&
            mbi.Protect != PAGE_NOACCESS) {
            return candidate;
        }
    }
    return 0;
}

// ---------------- 遍历 ----------------

// 把 __try 块独立到无 C++ 对象的函数
__declspec(noinline) static bool SafeReadGameData(uintptr_t gwmStatic,
    uintptr_t* outGameWorld, uintptr_t* outStartGame,
    uintptr_t* outRoleNetList, uintptr_t* outItems, int* outCount)
{
    __try {
        *outGameWorld = Memory::ReadPtr(gwmStatic + Offsets::GWCM_MyGameWorld);
        if (!*outGameWorld) return false;
        *outStartGame = Memory::ReadPtr(*outGameWorld + Offsets::BW_startGame);
        if (!*outStartGame) return false;
        *outRoleNetList = Memory::ReadPtr(*outStartGame + Offsets::SG_RoleNetList);
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

void Update() {
    // 懒初始化：拿 GameWorldClientManager 静态基址
    if (!s_gwmStatic) {
        void* gwcmClass = IL2CPP::FindClass("Assembly-CSharp.dll", "", "GameWorldClientManager");
        if (!gwcmClass) { return; }
        s_gwmStatic = GetStaticFields(gwcmClass);
        if (!s_gwmStatic) { return; }
        Log::Printf("[Player] GWM static_fields = 0x%llX", (unsigned long long)s_gwmStatic);
    }

    uintptr_t gameWorld = 0, startGame = 0, roleNetList = 0, items = 0;
    int count = 0;
    if (!SafeReadGameData(s_gwmStatic, &gameWorld, &startGame, &roleNetList, &items, &count)) {
        return;
    }

    // === 第二步：vector 操作（在 __try 外） ===
    if ((int)s_players.size() != count || s_scanCounter % 30 == 0) {
        // 全量重建（保留 animCtrl 缓存）
        Log::Printf("[Player] 全量扫描: %d 玩家", count);
        std::vector<PlayerInfo> oldList = s_players;
        std::vector<uintptr_t> oldRoleNets;
        for (auto& o : oldList) oldRoleNets.push_back(o.roleNet);

        s_players.clear();
        s_players.reserve(count);

        for (int i = 0; i < count; i++) {
            uintptr_t roleNet = Memory::ReadPtr(items + Offsets::Array_ItemsStart + (uintptr_t)i * 8);
            if (!roleNet) continue;

            PlayerInfo p;
            p.roleNet = roleNet;
            p.trans = Memory::ReadPtr(roleNet + Offsets::RN_Transform);
            p.roleLogic = Memory::ReadPtr(roleNet + Offsets::RN_MyRoleLogic);
            p.roleNetClient = Memory::ReadPtr(roleNet + Offsets::RN_RoleNetClient);
            p.isLocal = p.roleNetClient && Memory::ReadBool(p.roleNetClient + Offsets::RNC_isLocalPlayer);

            // 恢复缓存
            for (size_t j = 0; j < oldRoleNets.size(); j++) {
                if (oldRoleNets[j] == roleNet) {
                    p.animCtrl = oldList[j].animCtrl;
                    p.playerId = oldList[j].playerId;
                    break;
                }
            }
            s_players.push_back(p);
        }
    }
    s_scanCounter++;

    // 本地玩家
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

    if (!ReadTransformPosition(p.trans, p.pos)) return;
    if (!p.roleLogic) return;

    p.hp = Memory::ReadFloat(p.roleLogic + Offsets::BRL_Hp);
    p.maxHp = Memory::ReadFloat(p.roleLogic + Offsets::BRL_MaxHp);
    p.team = Memory::ReadInt64(p.roleLogic + Offsets::BRL_TeamNum);
    p.playerId = Memory::ReadInt64(p.roleLogic + Offsets::BRL_PlayerId);
    ReadIl2CppString(Memory::ReadPtr(p.roleLogic + Offsets::BRL_NickName), p.name, sizeof(p.name));
    p.alive = p.hp > 0.1f && p.maxHp > 0.1f;
    p.valid = true;
}

bool ReadTransformPosition(uintptr_t trans, float* out) {
    if (!trans || !out) return false;

    if (!s_getPositionMethod) {
        void* transClass = IL2CPP::FindClass("UnityEngine.CoreModule.dll", "UnityEngine", "Transform");
        if (!transClass) { s_getPositionMethod = (void*)-1; return false; }
        s_getPositionMethod = IL2CPP::GetMethodFromName(transClass, "get_position", 0);
        if (!s_getPositionMethod) { s_getPositionMethod = (void*)-1; return false; }
        Log::Printf("[Player] get_position method = 0x%p", s_getPositionMethod);
    }
    if (s_getPositionMethod == (void*)-1) return false;

    void* exc = nullptr;
    void* result = IL2CPP::RuntimeInvoke(s_getPositionMethod, (void*)trans, nullptr, &exc);
    if (!result || exc) return false;
    float* v = (float*)((uintptr_t)result + 0x10);
    out[0] = v[0]; out[1] = v[1]; out[2] = v[2];
    return true;
}

void ReadSkeleton(PlayerInfo& p) {
    if (!p.roleLogic || p.playerId == 0) return;

    if (p.animCtrl == 0) {
        // 尝试通过 RoleAnimatorControlPool.GetAnimatorControl(playerId) 获取
        if (!s_getAnimControlMethod) {
            void* poolClass = IL2CPP::FindClass("Assembly-CSharp.dll", "", "RoleAnimatorControlPool");
            if (!poolClass) { s_getAnimControlMethod = (void*)-1; return; }
            s_getAnimControlMethod = IL2CPP::GetMethodFromName(poolClass, "GetAnimatorControl", 1);
            if (!s_getAnimControlMethod) { s_getAnimControlMethod = (void*)-1; return; }
            Log::Printf("[Player] GetAnimatorControl method = 0x%p", s_getAnimControlMethod);
        }
        if (s_getAnimControlMethod == (void*)-1) return;

        if (!s_int64Class) {
            s_int64Class = IL2CPP::FindClass("System.Private.CoreLib.dll", "System", "Int64");
            if (!s_int64Class) s_int64Class = IL2CPP::FindClass("mscorlib.dll", "System", "Int64");
        }
        if (!s_valueBoxFn) {
            s_valueBoxFn = IL2CPP::GetExport<void* (*)(void*, void*)>("il2cpp_value_box");
        }
        if (!s_valueBoxFn || !s_int64Class) { p.animCtrl = (uintptr_t)-1; return; }

        void* boxed = s_valueBoxFn(s_int64Class, &p.playerId);
        void* params[1] = { boxed };
        void* exc = nullptr;
        void* animCtrl = IL2CPP::RuntimeInvoke(s_getAnimControlMethod, nullptr, params, &exc);
        if (animCtrl && !exc) {
            p.animCtrl = (uintptr_t)animCtrl;
            Log::Printf("[Player] %s (id=%lld) AnimatorControl=0x%llX", p.name, (long long)p.playerId, (unsigned long long)animCtrl);
        } else {
            p.animCtrl = (uintptr_t)-1;
            p.boneFailCount++;
            if (p.boneFailCount % 60 == 1)
                Log::Printf("[Player] %s 获取 AnimatorControl 失败", p.name);
        }
        return;
    }
    if (p.animCtrl == (uintptr_t)-1) return;

    // 已缓存 AnimatorControl，读骨骼
    static const int boneOffsets[BONE_COUNT] = {
        Offsets::AC_LeftHand, Offsets::AC_RightHand, Offsets::AC_Head, Offsets::AC_Hip,
        Offsets::AC_SkinBody, Offsets::AC_RightFoot, Offsets::AC_LeftFoot, Offsets::AC_Spine
    };
    for (int b = 0; b < BONE_COUNT; b++) {
        uintptr_t boneTrans = Memory::ReadPtr(p.animCtrl + boneOffsets[b]);
        if (!boneTrans) { p.boneValid[b] = false; continue; }
        if (s_getPositionMethod && s_getPositionMethod != (void*)-1) {
            __try {
                void* exc = nullptr;
                void* result = IL2CPP::RuntimeInvoke(s_getPositionMethod, (void*)boneTrans, nullptr, &exc);
                if (result && !exc) {
                    float* v = (float*)((uintptr_t)result + 0x10);
                    p.bonePos[b][0] = v[0]; p.bonePos[b][1] = v[1]; p.bonePos[b][2] = v[2];
                    p.boneValid[b] = true;
                } else { p.boneValid[b] = false; }
            } __except(EXCEPTION_EXECUTE_HANDLER) { p.boneValid[b] = false; }
        } else { p.boneValid[b] = false; }
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