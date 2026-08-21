#pragma once
#include <cstdint>
#include <windows.h>

namespace Memory {

// 安全内存读取，__try/__except 防止野指针崩溃
template<typename T>
inline T Read(uintptr_t addr) {
    __try {
        if (!addr) return T{};
        return *(T*)addr;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return T{};
    }
}

// 读指针（含 0 检查）
inline uintptr_t ReadPtr(uintptr_t addr) {
    return Read<uintptr_t>(addr);
}

// 读 float
inline float ReadFloat(uintptr_t addr) {
    return Read<float>(addr);
}

// 读 int32
inline int32_t ReadInt32(uintptr_t addr) {
    return Read<int32_t>(addr);
}

// 读 int64
inline int64_t ReadInt64(uintptr_t addr) {
    return Read<int64_t>(addr);
}

// 读 bool
inline bool ReadBool(uintptr_t addr) {
    return Read<bool>(addr);
}

// 写内存
template<typename T>
inline bool Write(uintptr_t addr, T value) {
    __try {
        if (!addr) return false;
        *(T*)addr = value;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// 拷贝内存（带长度）
inline bool ReadBytes(uintptr_t addr, void* out, size_t len) {
    __try {
        if (!addr || !out || !len) return false;
        memcpy(out, (void*)addr, len);
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

} // namespace Memory