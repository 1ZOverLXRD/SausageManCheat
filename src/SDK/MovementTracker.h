#pragma once
#include <cstdint>
#include "SDK/PlayerManager.h"

namespace GameSDK {

// ============================================================
// 移动追踪器 — 基于手掌位置变化的存活检测
// 纯数学，零 Unity API 调用（热路径安全）
// 30帧(0.5s)未动 → Suspect，60帧(1s)未动 → Inactive
// ============================================================
class MovementTracker {
public:
    // 更新并返回新状态（传入左右手位置）
    PlayerState update(float leftHand[3], float rightHand[3], bool hasLeft, bool hasRight);

    // 直接返回状态（不更新）
    PlayerState state() const { return m_state; }
    void reset() { m_state = STATE_ACTIVE; m_stillFrames = 0; }

private:
    PlayerState m_state = STATE_ACTIVE;
    int m_stillFrames = 0;
    float m_lastHand[3] = {0};
};

} // namespace GameSDK