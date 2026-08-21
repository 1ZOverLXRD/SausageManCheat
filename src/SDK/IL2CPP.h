#pragma once
#include <cstdint>
#include <windows.h>

namespace IL2CPP {

// ---- IL2CPP 导出函数类型 ----
typedef void* (*DomainGetFn)();
typedef void* (*ThreadAttachFn)(void* domain);
typedef void* (*ClassFromNameFn)(void* image, const char* ns, const char* name);
typedef void* (*ImageFromNameFn)(const char* name);
typedef void* (*ClassGetMethodFromNameFn)(void* klass, const char* name, int paramCount);
typedef void* (*RuntimeInvokeFn)(void* method, void* obj, void** params, void** exc);
typedef void* (*StringNewFn)(const char* str);
typedef int (*StringGetLengthFn)(void* str);
typedef void* (*ThreadDetachFn)(void* thread);
typedef void* (*DomainGetAssembliesFn)(void* domain, size_t* size);
typedef void* (*ClassGetTypeFn)(void* klass);
typedef void* (*TypeGetObjectFn)(void* type);
typedef void* (*ValueBoxFn)(void* klass, void* value);
typedef int32_t (*GCHandleNewFn)(void* obj, bool pinned);
typedef void* (*GCHandleGetTargetFn)(int32_t handle);
typedef void (*GCHandleFreeFn)(int32_t handle);
typedef void (*GCDisableFn)();
typedef void (*GCEnableFn)();
typedef void* (*GetIl2CppThreadFn)();

// 初始化：获取所有导出函数地址（在 DllMain 后调用）
bool Init();

// 获取模块基址
HMODULE GameAssembly();
HMODULE UnityPlayer();
uintptr_t ModuleBase(const wchar_t* name);
uintptr_t ModuleSize(const wchar_t* name);

// IL2CPP 基础 API（内部使用）
void* DomainGet();
void* ThreadAttach();
void* ThreadAttachEx();
void  GCDisable();
void  GCEnable();

// 查找类
void* FindClass(const char* image, const char* ns, const char* name);

// 查找方法（按名字 + 参数个数）
void* GetMethodFromName(void* klass, const char* name, int paramCount);

// RuntimeInvoke 安全调用（带异常检查）
void* RuntimeInvoke(void* method, void* obj, void** params, void** exc);

// 创建 IL2CPP 字符串
void* StringNew(const char* str);
int   StringGetLength(void* str);

// 装箱（值类型 → 对象）
void* ValueBox(void* klass, void* value);

// GC Handle
int32_t  GCHandleNew(void* obj);
void*    GCHandleGetTarget(int32_t handle);
void     GCHandleFree(int32_t handle);

// 获取函数指针（通过 RVA）
template<typename T>
inline T GetFunction(uintptr_t rva) {
    HMODULE h = GameAssembly();
    return h ? (T)((uintptr_t)h + rva) : nullptr;
}

// 获取函数指针（通过导出名）
template<typename T>
inline T GetExport(const char* name) {
    HMODULE h = GameAssembly();
    return h ? (T)GetProcAddress(h, name) : nullptr;
}

} // namespace IL2CPP