#pragma once
#include <cstdint>

namespace CameraManager {

// 每帧更新：读取 Camera 指针 + 矩阵
void Update();

// 手写 W2S（纯数学，零 Unity API）
bool WorldToScreen(float wx, float wy, float wz, float& sx, float& sy);

// 获取相机位置（用于距离计算）
void GetCamPos(float* out);

// 获取屏幕尺寸
void GetScreenSize(int& w, int& h);

// 日志/调试
bool IsValid();

} // namespace CameraManager