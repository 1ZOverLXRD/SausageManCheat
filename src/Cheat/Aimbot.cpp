#include "Cheat/Aimbot.h"
#include "Cheat/Config.h"
#include "SDK/Game.h"
#include "SDK/PlayerManager.h"
#include "Core/Log.h"
#include <cmath>
#include <windows.h>

namespace Aimbot {

int g_targetIndex = -1;
int g_boneTarget = BONE_HEAD;  // 默认瞄准头部

// 次像素累积
static float g_accumX = 0, g_accumY = 0;

// 上一帧目标（用于锁定切换检测）
static int g_prevTarget = -1;

int FindTarget() {
    auto& players = Game::GetPlayers();
    if (players.empty()) return -1;

    // 本地队伍
    int64_t localTeam = 0;
    for (auto& p : players) {
        if (p.isLocal) { localTeam = p.team; break; }
    }

    g_boneTarget = Config::Aimbot::AimBone;
    if (g_boneTarget < 0 || g_boneTarget >= BONE_COUNT) g_boneTarget = BONE_HEAD;  // 越界保护

    int bestIdx = -1;
    float bestDist = 999999.0f;

    for (size_t i = 0; i < players.size(); i++) {
        auto& p = players[i];
        if (p.isLocal) continue;
        if (!p.valid || !p.alive) continue;
        if (!p.onScreen) continue;
        if (Config::Aimbot::TeamCheck && localTeam != 0 && p.team == localTeam) continue;

        // 获取瞄准目标点（骨骼或位置）
        float targetX = p.screenX, targetY = p.screenY;
        int bone = g_boneTarget;
        if (p.boneValid[bone]) {
            targetX = p.boneScreen[bone][0];
            targetY = p.boneScreen[bone][1];
        }

        // 只在屏幕中心附近搜索（FOV 限制）
        int sw, sh;
        CameraManager::GetScreenSize(sw, sh);
        float cx = sw * 0.5f, cy = sh * 0.5f;
        float dist = std::sqrt((targetX - cx) * (targetX - cx) + (targetY - cy) * (targetY - cy));

        // FOV 限制（默认屏幕对角线的一半）
        float maxFov = std::sqrt(sw * sw + sh * sh) * 0.5f;
        if (dist > maxFov) continue;

        if (dist < bestDist) {
            bestDist = dist;
            bestIdx = (int)i;
        }
    }

    return bestIdx;
}

void AimAt(float targetX, float targetY, float centerX, float centerY) {
    float deltaX = targetX - centerX;
    float deltaY = targetY - centerY;
    float dist = std::sqrt(deltaX * deltaX + deltaY * deltaY);

    if (dist < Config::Aimbot::DeadZone) {
        g_accumX = 0;
        g_accumY = 0;
        return;
    }

    // 方向归一化
    float dirX = deltaX / dist;
    float dirY = deltaY / dist;

    // 速度曲线
    float speed = 0;
    if (dist >= Config::Aimbot::RampDist) {
        speed = Config::Aimbot::MaxSpeed;
    } else {
        float t = dist / Config::Aimbot::RampDist;
        speed = Config::Aimbot::MinSpeed + (Config::Aimbot::MaxSpeed - Config::Aimbot::MinSpeed) * t * t;
    }

    // 次像素累积
    g_accumX += dirX * speed;
    g_accumY += dirY * speed;
    int mickeyX = (int)g_accumX;
    int mickeyY = (int)g_accumY;
    g_accumX -= (float)mickeyX;
    g_accumY -= (float)mickeyY;

    if (mickeyX != 0 || mickeyY != 0) {
        mouse_event(MOUSEEVENTF_MOVE, (DWORD)(short)mickeyX, (DWORD)(short)mickeyY, 0, 0);
    }
}

void Update() {
    if (!Config::Aimbot::Enabled) return;

    // 检测按键
    bool keyDown = (GetAsyncKeyState(Config::Aimbot::Key) & 0x8000) != 0;
    if (!keyDown) {
        g_targetIndex = -1;
        g_accumX = 0;
        g_accumY = 0;
        return;
    }

    // 找目标
    int target = FindTarget();
    if (target < 0) return;

    g_targetIndex = target;

    // 目标切换日志（限流：只在切换且间隔 > 1 秒时打）
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
    if (!p.valid || !p.alive) return;

    // 瞄准点
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

    AimAt(targetX, targetY, cx, cy);
}

} // namespace Aimbot