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

// CameraController 静态基址（缓存）
static uintptr_t s_gdStatic = 0;

static uintptr_t GetGameDataStatic() {
    if (s_gdStatic) return s_gdStatic;
    void* gdClass = IL2CPP::FindClass("Assembly-CSharp.dll", "", "GameData");
    if (!gdClass) return 0;
    // 探测 static_fields 偏移
    for (uintptr_t off = 0xB8; off <= 0xD0; off += 8) {
        uintptr_t candidate = Memory::Read<uintptr_t>((uintptr_t)gdClass + off);
        if (candidate > 0x10000 && candidate < 0x7FFFFFFF0000) {
            __try { volatile uint8_t b = *(volatile uint8_t*)candidate; (void)b; s_gdStatic = candidate; break; }
            __except(1) { continue; }
        }
    }
    if (s_gdStatic) {
        Log::Printf("[Camera] GameData static_fields = 0x%llX", (unsigned long long)s_gdStatic);
    }
    return s_gdStatic;
}

void Update() {
    uintptr_t gdStatic = GetGameDataStatic();
    if (!gdStatic) { s_valid = false; return; }

    __try {
        // GameData.WarCamera (static +0x1E0) → CameraController
        uintptr_t camCtrl = Memory::ReadPtr(gdStatic + Offsets::GD_WarCamera);
        if (!camCtrl) {
            if (s_valid) { s_valid = false; Log::Printf("[Camera] CameraController 丢失"); }
            return;
        }

        // CameraController.MyCamera (+0x30) → managed Camera
        uintptr_t managedCam = Memory::ReadPtr(camCtrl + Offsets::CC_MyCamera);
        if (!managedCam) {
            if (s_valid) { s_valid = false; Log::Printf("[Camera] managed Camera 丢失"); }
            return;
        }

        // managed Camera +0x10 = m_CachedPtr → native Camera
        uintptr_t nativeCam = Memory::ReadPtr(managedCam + Offsets::CAM_CachedPtr);
        if (!nativeCam) {
            if (s_valid) { s_valid = false; Log::Printf("[Camera] native Camera 丢失"); }
            return;
        }

        // 场景切换检测
        if (nativeCam != s_nativeCamera) {
            Log::Printf("[Camera] 场景切换: 0x%llX → 0x%llX",
                (unsigned long long)s_nativeCamera, (unsigned long long)nativeCam);
            s_nativeCamera = nativeCam;
        }

        // 读矩阵（全内存读）
        // view: +0x80 (IDA 确认 GetWorldToCameraMatrix 返回 this+128)
        // proj: +0xC0 (IDA 确认 GetProjectionMatrix 返回 this+192)
        if (!Memory::ReadBytes(nativeCam + Offsets::CAM_ViewMatrix, s_viewMatrix, 64)) {
            Log::Debug("[Camera] 读 view 矩阵失败");
            return;
        }
        if (!Memory::ReadBytes(nativeCam + Offsets::CAM_ProjMatrix, s_projMatrix, 64)) {
            Log::Debug("[Camera] 读 proj 矩阵失败");
            return;
        }

        // 从 view 矩阵提取相机位置（逆矩阵第4列）
        // 对仿射矩阵 R|t，view = [R^T | -R^T*t], 相机位置 = -R*t（从逆矩阵计算）
        // 更简单的方法：view 矩阵的逆阵第4列 = 相机位置
        // 但直接读 view 矩阵的 12,13,14 不是相机位置（它们是 -R^T*t 的前3个分量）
        // 正确公式：cameraPos = -R * t = -(R^T)^T * t
        // 因为 view = [R^T | -R^T*t], 所以 R = [m[0],m[4],m[8]; m[1],m[5],m[9]; m[2],m[6],m[10]]
        // t = [m[12], m[13], m[14]]
        // cameraPos = (-R * t) = -(m[0]*t.x + m[1]*t.y + m[2]*t.z, ...) 
        // 哦不对，view = [R^T|T], 世界坐标 = R^T * (local - T) 所以 R^T = view 的上3x3
        // 相机位置在 view 空间中 = (0,0,0)，解 R^T*(world - T) = 0 → world = T
        // 所以 T = [m[12], m[13], m[14]] 就是相机位置 ❌
        // 等等，不对。Unity 的 view 矩阵 = R^T | -R^T*pos
        // 所以 m[12] = -(R^T*pos).x, 相机位置需要解
        // 相机世界位置 = -R * m[12..14]
        // 简化：m[12] = -(R^T*pos).x = -(R.col0·pos)
        // 所以 pos.x = -R^T.col0·[m12,m13,m14] = -(m[0]*m[12] + m[1]*m[13] + m[2]*m[14])
        // 但更简单的方法：直接用 get_position 读 Camera 位置或从 CameraController 拿
        // 直接从 CameraController 的 LookTarget 位置拿近似

        // 简化：用 view 矩阵的逆计算相机位置
        // 对 camToWorld = view^-1，第4列 = 相机位置
        // 但 view 矩阵是正交的（R^T），所以 R = [m0,m4,m8; m1,m5,m9; m2,m6,m10]^T
        // 逆矩阵上3x3 = R（因为 R^T 的逆 = R）
        // 逆矩阵第4列 = -R * [m12,m13,m14]
        float* m = s_viewMatrix;
        float tx = m[12], ty = m[13], tz = m[14];
        // R = [m0,m4,m8; m1,m5,m9; m2,m6,m10]
        s_camPos[0] = -(m[0]*tx + m[1]*ty + m[2]*tz);   // -R.col0·t
        s_camPos[1] = -(m[4]*tx + m[5]*ty + m[6]*tz);   // -R.col1·t
        s_camPos[2] = -(m[8]*tx + m[9]*ty + m[10]*tz);  // -R.col2·t

        // 屏幕尺寸
        HWND hwnd = GetForegroundWindow();
        RECT rect;
        if (hwnd && GetClientRect(hwnd, &rect)) {
            s_screenW = rect.right - rect.left;
            s_screenH = rect.bottom - rect.top;
            if (s_screenW <= 0) s_screenW = 1920;
            if (s_screenH <= 0) s_screenH = 1080;
        }

        s_valid = true;

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        Log::Printf("[Camera] Update 异常 code=0x%X", GetExceptionCode());
        s_valid = false;
    }
}

bool WorldToScreen(float wx, float wy, float wz, float& sx, float& sy) {
    if (!s_valid || !s_nativeCamera) return false;

    float* w2c = s_viewMatrix;
    float* proj = s_projMatrix;

    // 世界 → 视图空间（列主序）
    float vx = wx*w2c[0] + wy*w2c[4] + wz*w2c[8]  + w2c[12];
    float vy = wx*w2c[1] + wy*w2c[5] + wz*w2c[9]  + w2c[13];
    float vz = wx*w2c[2] + wy*w2c[6] + wz*w2c[10] + w2c[14];
    float vw = wx*w2c[3] + wy*w2c[7] + wz*w2c[11] + w2c[15];

    // 视图 → 裁剪空间
    float cx = vx*proj[0] + vy*proj[4] + vz*proj[8]  + vw*proj[12];
    float cy = vx*proj[1] + vy*proj[5] + vz*proj[9]  + vw*proj[13];
    float cz = vx*proj[2] + vy*proj[6] + vz*proj[10] + vw*proj[14];
    float cw = vx*proj[3] + vy*proj[7] + vz*proj[11] + vw*proj[15];

    if (cw < 0.001f) return false;

    float ndcX = cx / cw;
    float ndcY = cy / cw;

    // ImGui 顶部原点：翻转 Y
    sx = (ndcX + 1.0f) * 0.5f * s_screenW;
    sy = (1.0f - ndcY) * 0.5f * s_screenH;

    // 屏幕可见性检查
    return sx > -100 && sx < s_screenW + 100 && sy > -100 && sy < s_screenH + 100;
}

void GetCamPos(float* out) {
    out[0] = s_camPos[0]; out[1] = s_camPos[1]; out[2] = s_camPos[2];
}

void GetScreenSize(int& w, int& h) {
    w = s_screenW; h = s_screenH;
}

bool IsValid() { return s_valid; }

} // namespace CameraManager