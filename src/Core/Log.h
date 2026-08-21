#pragma once
#include <cstdio>
#include <cstdarg>
#include <windows.h>

namespace Log {

// 初始化：创建控制台窗口 + 文件日志
void Init();

// 关闭：释放控制台 + 文件
void Shutdown();

// 带时间戳输出（控制台 + 文件）
void Printf(const char* fmt, ...);

// 调试级别（控制台只输出）
void Debug(const char* fmt, ...);

} // namespace Log