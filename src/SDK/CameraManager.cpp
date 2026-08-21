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
    // 探测 static_fields 偏移：Il2CppClass_1=0xA0, static_fields 在 0xB8
    // 直接固定偏移，不探测范围（避免捡到 nestedTypes 等误判）
    uintptr_t candidate = 0;
    __try { candidate = *(uintptr_t*)((uintptr_t)gdClass + 0xB8); } __except(1) { return 0; }
    if (candidate > 0x10000 && candidate < 0x7FFFFFFF0000) {
        __try { volatile uint8_t b = *(volatile uint8_t*)candidate; (void)b; s_gdStatic = candidate; }
        __except(1) { return 0; }
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

        // 从 view 矩阵提取相机位置
        // view = [R^T | -R^T * pos]，列主序存储：
        //   m[0..2]   = R^T 第0列 = R 第0行 (即 R 的 row0)
        //   m[4..6]   = R^T 第1列 = R 第1行
        //   m[8..10]  = R^T 第2列 = R 第2行
        //   m[12..14] = -R^T * pos
        // 相机位置 = -(R^T)^-1 * [m12,m13,m14] = -R * [m12,m13,m14]  （R^T 正交）
        // 所以：
        //   cam.x = -(R.row0)·t = -(m[0]*t.x + m[1]*t.y + m[2]*t.z)
        //   cam.y = -(R.row1)·t = -(m[4]*t.x + m[5]*t.y + m[6]*t.z)
        //   cam.z = -(R.row2)·t = -(m[8]*t.x + m[9]*t.y + m[10]*t.z)
        float* m = s_viewMatrix;
        float tx = m[12], ty = m[13], tz = m[14];
        s_camPos[0] = -(m[0]*tx + m[1]*ty + m[2]*tz);   // R 第0行 · t
        s_camPos[1] = -(m[4]*tx + m[5]*ty + m[6]*tz);   // R 第1行 · t
        s_camPos[2] = -(m[8]*tx + m[9]*ty + m[10]*tz);  // R 第2行 · t

        // 屏幕尺寸：用 GetForegroundWindow 验证其类名含 Unity（避免切窗口时误判）
        {
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