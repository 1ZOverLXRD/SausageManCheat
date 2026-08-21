#include "SDK/CameraManager.h"
#include "SDK/IL2CPP.h"
#include "SDK/GameOffsets.h"
#include "Core/Log.h"
#include "Core/Memory.h"
#include <cmath>

namespace CameraManager {

static uintptr_t s_nativeCamera = 0;
static float s_viewMatrix[16] = {0};
static float s_projMatrix[16] = {0};
static float s_camPos[3] = {0};
static int s_screenW = 1920, s_screenH = 1080;
static bool s_valid = false;

// GameData 静态区
static uintptr_t s_gdStatic = 0;

static uintptr_t GetGameDataStatic() {
    if (s_gdStatic) return s_gdStatic;
    void* gdClass = IL2CPP::FindClass("Assembly-CSharp.dll", "", "GameData");
    if (!gdClass) return 0;
    uintptr_t candidate = Memory::ReadPtr((uintptr_t)gdClass + 0xB8);
    if (candidate > 0x10000 && candidate < 0x7FFFFFFF0000) {
        s_gdStatic = candidate;
        Log::Printf("[Camera] GameData static_fields = 0x%llX", (unsigned long long)s_gdStatic);
    }
    return s_gdStatic;
}

void Update() {
    uintptr_t gdStatic = GetGameDataStatic();
    if (!gdStatic) { s_valid = false; return; }

    __try {
        // GameData.WarCamera → CameraController → managed Camera → native Camera
        uintptr_t camCtrl = Memory::ReadPtr(gdStatic + Offsets::GD_WarCamera);
        if (!camCtrl) { s_valid = false; return; }

        uintptr_t managedCam = Memory::ReadPtr(camCtrl + Offsets::CC_MyCamera);
        if (!managedCam) { s_valid = false; return; }

        uintptr_t nativeCam = Memory::ReadPtr(managedCam + Offsets::CAM_CachedPtr);
        if (!nativeCam) { s_valid = false; return; }

        // 场景切换检测
        if (nativeCam != s_nativeCamera) {
            Log::Printf("[Camera] 场景切换: 0x%llX → 0x%llX",
                (unsigned long long)s_nativeCamera, (unsigned long long)nativeCam);
            s_nativeCamera = nativeCam;
        }

        // 读矩阵
        if (!Memory::ReadBytes(nativeCam + Offsets::CAM_ViewMatrix, s_viewMatrix, 64)) return;
        if (!Memory::ReadBytes(nativeCam + Offsets::CAM_ProjMatrix, s_projMatrix, 64)) return;

        // 从 view 矩阵提取相机位置
        // view = [R^T | -R^T * pos]，列主序
        float* m = s_viewMatrix;
        float tx = m[12], ty = m[13], tz = m[14];
        s_camPos[0] = -(m[0]*tx + m[1]*ty + m[2]*tz);
        s_camPos[1] = -(m[4]*tx + m[5]*ty + m[6]*tz);
        s_camPos[2] = -(m[8]*tx + m[9]*ty + m[10]*tz);

        // 屏幕尺寸
        HWND hwnd = GetForegroundWindow();
        char cls[64] = {};
        if (hwnd && GetClassNameA(hwnd, cls, sizeof(cls)) > 0 && strstr(cls, "Unity")) {
            RECT rect;
            if (GetClientRect(hwnd, &rect)) {
                s_screenW = rect.right - rect.left;
                s_screenH = rect.bottom - rect.top;
            }
        }
        if (s_screenW <= 0) s_screenW = 1920;
        if (s_screenH <= 0) s_screenH = 1080;

        s_valid = true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        Log::Printf("[Camera] Update 异常 code=0x%X", GetExceptionCode());
        s_valid = false;
    }
}

float GetVerticalFOV() {
    if (!s_valid) return 90.0f;
    float cotHalfFov = s_projMatrix[5];
    if (cotHalfFov < 0.001f) return 90.0f;
    return 2.0f * atan2f(1.0f, cotHalfFov) * 180.0f / 3.14159265f;
}

bool WorldToScreen(float wx, float wy, float wz, float& sx, float& sy) {
    if (!s_valid) return false;

    float* w2c = s_viewMatrix;
    float* proj = s_projMatrix;

    // 世界 → 视图（列主序）
    float vx = wx*w2c[0] + wy*w2c[4] + wz*w2c[8]  + w2c[12];
    float vy = wx*w2c[1] + wy*w2c[5] + wz*w2c[9]  + w2c[13];
    float vz = wx*w2c[2] + wy*w2c[6] + wz*w2c[10] + w2c[14];
    float vw = wx*w2c[3] + wy*w2c[7] + wz*w2c[11] + w2c[15];

    // 视图 → 裁剪
    float cx = vx*proj[0] + vy*proj[4] + vz*proj[8]  + vw*proj[12];
    float cy = vx*proj[1] + vy*proj[5] + vz*proj[9]  + vw*proj[13];
    float cz = vx*proj[2] + vy*proj[6] + vz*proj[10] + vw*proj[14];
    float cw = vx*proj[3] + vy*proj[7] + vz*proj[11] + vw*proj[15];

    if (cw < 0.001f) return false;

    float ndcX = cx / cw;
    float ndcY = cy / cw;

    sx = (ndcX + 1.0f) * 0.5f * s_screenW;
    sy = (1.0f - ndcY) * 0.5f * s_screenH;

    return sx > -100 && sx < s_screenW + 100 && sy > -100 && sy < s_screenH + 100;
}

void GetCamPos(float* out) {
    out[0] = s_camPos[0]; out[1] = s_camPos[1]; out[2] = s_camPos[2];
}

void GetScreenSize(int& w, int& h) { w = s_screenW; h = s_screenH; }
bool IsValid() { return s_valid; }

} // namespace CameraManager