#pragma once
#include <cstdint>
#include "Core/Memory.h"
#include "SDK/IL2CPP.h"
#include "SDK/GameOffsets.h"

namespace GameSDK {

// ============================================================
// 游戏对象抽象层 — 每个类封装一个游戏对象，提供类型化字段访问
// 全部纯内存读，零 Unity API 调用（热路径安全）
// ============================================================

// 玩家状态（由 MovementTracker 更新）
enum class PlayerState : int {
    Active = 0,
    Suspect = 1,   // 0.5s 未动
    Inactive = 2,  // 1s 未动
};

// ---- Transform（managed）----
// managed Transform 内存布局：klass(8) + monitor(8) + m_CachedPtr(8)
// m_CachedPtr → native Transform
class Transform {
    uintptr_t m_ptr = 0;
public:
    explicit Transform(uintptr_t p) : m_ptr(p) {}
    bool valid() const { return m_ptr != 0; }
    uintptr_t raw() const { return m_ptr; }

    // managed +0x10 = m_CachedPtr → native Transform*
    uintptr_t native() const {
        return Memory::ReadPtr(m_ptr + 0x10);
    }

    // 读取世界坐标（调 Transform.get_position RuntimeInvoke）
    // 返回 false 表示失败，out 不变
    bool getPosition(float out[3]) const {
        if (!m_ptr) return false;
        static void* s_method = nullptr;
        if (!s_method) {
            void* cls = IL2CPP::FindClass("UnityEngine.CoreModule.dll", "UnityEngine", "Transform");
            if (!cls) return false;
            s_method = IL2CPP::GetMethodFromName(cls, "get_position", 0);
            if (!s_method) return false;
        }
        void* exc = nullptr;
        void* result = IL2CPP::RuntimeInvoke(s_method, (void*)m_ptr, nullptr, &exc);
        if (!result || exc) return false;
        float* v = (float*)((uintptr_t)result + 0x10);
        out[0] = v[0]; out[1] = v[1]; out[2] = v[2];
        return true;
    }
};

// ---- RoleNetClient ----
class RoleNetClient {
    uintptr_t m_ptr = 0;
public:
    explicit RoleNetClient(uintptr_t p) : m_ptr(p) {}
    bool valid() const { return m_ptr != 0; }

    // RNC +0x1D = 本地玩家标记
    bool isLocalPlayer() const {
        return Memory::ReadBool(m_ptr + Offsets::RNC_isLocalPlayer);
    }
};

// ---- RoleNet ----
class RoleNet {
    uintptr_t m_ptr = 0;
public:
    explicit RoleNet(uintptr_t p) : m_ptr(p) {}
    bool valid() const { return m_ptr != 0; }
    uintptr_t raw() const { return m_ptr; }

    // RN +0x40 = Transform
    Transform transform() const {
        return Transform(Memory::ReadPtr(m_ptr + Offsets::RN_Transform));
    }

    // RN +0x58 = RoleNetClient
    RoleNetClient client() const {
        return RoleNetClient(Memory::ReadPtr(m_ptr + Offsets::RN_RoleNetClient));
    }

    // RN +0x68 = BattleRoleLogic
    uintptr_t roleLogic() const {
        return Memory::ReadPtr(m_ptr + Offsets::RN_MyRoleLogic);
    }
};

// ---- BattleRoleLogic ----
class RoleLogic {
    uintptr_t m_ptr = 0;
public:
    explicit RoleLogic(uintptr_t p) : m_ptr(p) {}
    bool valid() const { return m_ptr != 0; }
    uintptr_t raw() const { return m_ptr; }

    float hp() const      { return Memory::ReadFloat(m_ptr + Offsets::BRL_Hp); }
    float maxHp() const   { return Memory::ReadFloat(m_ptr + Offsets::BRL_MaxHp); }
    int64_t team() const  { return Memory::ReadInt64(m_ptr + Offsets::BRL_TeamNum); }
    uintptr_t nickName() const { return Memory::ReadPtr(m_ptr + Offsets::BRL_NickName); }

    // 链路：BattleRoleLogic → RLC → BR → RC → AnimatorControl
    uintptr_t animatorControl() const {
        uintptr_t rlc = Memory::ReadPtr(m_ptr + Offsets::BRL_RLC_Offset);
        if (!rlc) return 0;
        uintptr_t br = Memory::ReadPtr(rlc + Offsets::RLC_BR_Offset);
        if (!br) return 0;
        uintptr_t rc = Memory::ReadPtr(br + Offsets::BR_RC_Offset);
        if (!rc) return 0;
        return Memory::ReadPtr(rc + Offsets::RC_AC_Offset);
    }
};

// ---- AnimatorControl（骨骼容器）----
class AnimatorControl {
    uintptr_t m_ptr = 0;
public:
    explicit AnimatorControl(uintptr_t p) : m_ptr(p) {}
    bool valid() const { return m_ptr != 0; }
    uintptr_t raw() const { return m_ptr; }

    // 骨骼字段 → Transform
    Transform bone(int boneIndex) const {
        static const uintptr_t offsets[8] = {
            Offsets::AC_LeftHand, Offsets::AC_RightHand, Offsets::AC_Head,
            Offsets::AC_Hip, Offsets::AC_SkinBody,
            Offsets::AC_RightFoot, Offsets::AC_LeftFoot, Offsets::AC_Spine
        };
        if (boneIndex < 0 || boneIndex >= 8) return Transform(0);
        return Transform(Memory::ReadPtr(m_ptr + offsets[boneIndex]));
    }
};

// ---- 静态基址访问 ----
// Il2CppClass* +0xB8 = static_fields（il2cpp.h 确认，禁止探测范围）
class StaticClass {
    uintptr_t m_staticFields = 0;
public:
    explicit StaticClass(uintptr_t staticFields) : m_staticFields(staticFields) {}
    bool valid() const { return m_staticFields != 0; }
    uintptr_t fields() const { return m_staticFields; }
};

} // namespace GameSDK