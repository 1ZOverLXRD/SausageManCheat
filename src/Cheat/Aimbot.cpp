#include "Cheat/Aimbot.h"
#include "Cheat/Config.h"
#include "SDK/Game.h"
#include "SDK/CameraManager.h"
#include "SDK/PlayerManager.h"
#include "Core/Log.h"
#include <cmath>
#include <windows.h>

namespace Aimbot {

int g_targetIndex = -1;
int g_boneTarget = BONE_HEAD;

static int g_prevTarget = -1;

int FindTarget() {
    auto& players = Game::GetPlayers();
    if (players.empty()) return -1;

    int64_t localTeam = 0;
    for (auto& p : players) {
        if (p.isLocal) { localTeam = p.team; break; }
    }

    g_boneTarget = Config::Aimbot::AimBone;
    if (g_boneTarget < 0 || g_boneTarget >= BONE_COUNT) g_boneTarget = BONE_HEAD;

    int bestIdx = -1;
    float bestScore = 999999.0f;

    int sw, sh;
    CameraManager::GetScreenSize(sw, sh);
    float centerX = sw * 0.5f, centerY = sh * 0.5f;

    for (size_t i = 0; i < players.size(); i++) {
        auto& p = players[i];
        if (p.isLocal) continue;
        if (!p.valid) continue;
        if (p.state == STATE_INACTIVE) continue;
        if (!p.onScreen) continue;
        if (Config::Aimbot::TeamCheck && localTeam != 0 && p.team == localTeam) continue;

        float targetX = p.screenX, targetY = p.screenY;
        int bone = g_boneTarget;
        if (p.boneValid[bone]) {
            targetX = p.boneScreen[bone][0];
            targetY = p.boneScreen[bone][1];
        }

        float dist = std::sqrt((targetX - centerX) * (targetX - centerX) + (targetY - centerY) * (targetY - centerY));
        if (dist > Config::Aimbot::FovRadius) continue;

        float score = 0;
        switch (Config::Aimbot::TargetMode) {
            case 0: score = p.distToCam; break;
            case 1: score = dist; break;
            case 2: score = p.hp > 0 ? p.hp : 9999; break;
            default: score = dist;
        }

        if (score < bestScore) {
            bestScore = score;
            bestIdx = (int)i;
        }
    }

    return bestIdx;
}

void Update() {
    if (!Config::Aimbot::Enabled) return;

    bool keyDown = (GetAsyncKeyState(Config::Aimbot::Key) & 0x8000) != 0;
    if (!keyDown) {
        g_targetIndex = -1;
        return;
    }

    int target = FindTarget();
    if (target < 0) return;

    g_targetIndex = target;

    if (g_targetIndex != g_prevTarget) {
        auto now = GetTickCount64();
        static ULONGLONG s_lastLog = 0;
        if (now - s_lastLog > 1000) {
            auto& p = Game::GetPlayers()[g_targetIndex];
            Log::Printf("[Aimbot] 锁定目标 %s", p.name);
            s_lastLog = now;
        }
        g_prevTarget = g_targetIndex;
    }

    auto& p = Game::GetPlayers()[target];
    if (!p.valid) return;
    if (p.state == STATE_INACTIVE) return;

    // 瞄准点屏幕坐标
    float targetX = p.screenX, targetY = p.screenY;
    int bone = g_boneTarget;
    if (p.boneValid[bone]) {
        targetX = p.boneScreen[bone][0];
        targetY = p.boneScreen[bone][1];
    }

    // 屏幕中心
    int sw, sh;
    CameraManager::GetScreenSize(sw, sh);
    float cx = sw * 0.5f, cy = sh * 0.5f;

    // 像素差
    float deltaX = targetX - cx;
    float deltaY = targetY - cy;
    float dist = std::sqrt(deltaX * deltaX + deltaY * deltaY);

    if (dist < Config::Aimbot::DeadZone) return;

    // delta / smooth — 距离越远移动越快，自然收敛
    // smooth 越大越慢（防抖）
    float smooth = Config::Aimbot::Smooth;
    float moveX = deltaX / smooth;
    float moveY = deltaY / smooth;

    // 限幅，防止单帧跳太大（被检测）
    if (moveX > 30.0f) moveX = 30.0f;
    if (moveX < -30.0f) moveX = -30.0f;
    if (moveY > 30.0f) moveY = 30.0f;
    if (moveY < -30.0f) moveY = -30.0f;

    mouse_event(MOUSEEVENTF_MOVE, (DWORD)(short)moveX, (DWORD)(short)moveY, 0, 0);
}

} // namespace Aimbot