#include "SDK/MovementTracker.h"
#include <cmath>

namespace GameSDK {

PlayerState MovementTracker::update(float leftHand[3], float rightHand[3], bool hasLeft, bool hasRight) {
    if (!hasLeft && !hasRight)
        return m_state;  // 无骨骼数据，不更新

    // 平均左右手
    float avg[3] = {0};
    int count = 0;
    if (hasLeft) { avg[0] += leftHand[0]; avg[1] += leftHand[1]; avg[2] += leftHand[2]; count++; }
    if (hasRight) { avg[0] += rightHand[0]; avg[1] += rightHand[1]; avg[2] += rightHand[2]; count++; }
    if (count > 0) {
        avg[0] /= count; avg[1] /= count; avg[2] /= count;
    }

    // 位置变化
    float dx = avg[0] - m_lastHand[0];
    float dy = avg[1] - m_lastHand[1];
    float dz = avg[2] - m_lastHand[2];
    float dist = sqrtf(dx*dx + dy*dy + dz*dz);

    m_lastHand[0] = avg[0]; m_lastHand[1] = avg[1]; m_lastHand[2] = avg[2];

    if (dist < 0.01f) {
        m_stillFrames++;
        if (m_stillFrames >= 60)      m_state = STATE_INACTIVE;
        else if (m_stillFrames >= 30) m_state = STATE_SUSPECT;
    } else {
        m_stillFrames = 0;
        m_state = STATE_ACTIVE;
    }
    return m_state;
}

} // namespace GameSDK