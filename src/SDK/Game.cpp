#include "SDK/Game.h"
#include "SDK/IL2CPP.h"
#include "Core/Log.h"
#include <cmath>

namespace Game {

static bool s_initialized = false;
static uintptr_t s_nativeCam = 0;

bool IsInitialized() { return s_initialized; }

void Init() {
    if (s_initialized) return;
    Log::Printf("[Game] 初始化...");
    if (!IL2CPP::Init()) {
        Log::Printf("[Game] IL2CPP::Init 失败");
        return;
    }
    // 线程附加
    IL2CPP::ThreadAttach();
    s_initialized = true;
    Log::Printf("[Game] 初始化完成");
}

void Update() {
    if (!s_initialized) return;

    // PlayerManager::Update() 已做：每帧全量读取 + ReadPlayerData + ReadSkeleton
    PlayerManager::Update();

    auto& players = PlayerManager::GetPlayers();

    // 更新相机
    CameraManager::Update();

    // W2S 所有玩家
    float camPos[3];
    CameraManager::GetCamPos(camPos);
    for (auto& p : players) {
        if (!p.valid) continue;
        float dx = p.pos[0] - camPos[0];
        float dy = p.pos[1] - camPos[1];
        float dz = p.pos[2] - camPos[2];
        p.distToCam = sqrtf(dx*dx + dy*dy + dz*dz);

        p.onScreen = CameraManager::WorldToScreen(p.pos[0], p.pos[1], p.pos[2], p.screenX, p.screenY);

        for (int b = 0; b < BONE_COUNT; b++) {
            if (p.boneValid[b]) {
                float sx, sy;
                if (CameraManager::WorldToScreen(p.bonePos[b][0], p.bonePos[b][1], p.bonePos[b][2], sx, sy)) {
                    p.boneScreen[b][0] = sx;
                    p.boneScreen[b][1] = sy;
                } else {
                    p.boneValid[b] = false;
                }
            }
        }
    }
}

std::vector<PlayerInfo>& GetPlayers() {
    return PlayerManager::GetPlayers();
}

float* GetCamPos() {
    static float pos[3] = {0};
    CameraManager::GetCamPos(pos);
    return pos;
}

bool IsLocalPlayer(uintptr_t roleLogic) {
    if (!roleLogic) return false;
    auto& players = PlayerManager::GetPlayers();
    for (auto& p : players) {
        if (p.roleLogic == roleLogic) return p.isLocal;
    }
    return false;
}

void DumpState() {
    PlayerManager::DumpPlayersToLog();
    Log::Printf("[Game] Camera valid=%d", CameraManager::IsValid() ? 1 : 0);
    int sw, sh;
    CameraManager::GetScreenSize(sw, sh);
    Log::Printf("[Game] Screen=%dx%d", sw, sh);
}

} // namespace Game