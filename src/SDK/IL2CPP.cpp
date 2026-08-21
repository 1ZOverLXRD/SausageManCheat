#include "SDK/IL2CPP.h"
#include "Core/Log.h"

namespace IL2CPP {

static HMODULE g_gameAssembly = nullptr;
static HMODULE g_unityPlayer = nullptr;

static DomainGetFn            fn_domain_get = nullptr;
static ThreadAttachFn        fn_thread_attach = nullptr;
static ClassFromNameFn       fn_class_from_name = nullptr;
static ImageFromNameFn       fn_image_from_name = nullptr;
static ClassGetMethodFromNameFn fn_class_get_method = nullptr;
static RuntimeInvokeFn       fn_runtime_invoke = nullptr;
static StringNewFn           fn_string_new = nullptr;
static StringGetLengthFn     fn_string_get_length = nullptr;
static TypeGetObjectFn       fn_type_get_object = nullptr;
static ValueBoxFn            fn_value_box = nullptr;
static GCHandleNewFn         fn_gchandle_new = nullptr;
static GCHandleGetTargetFn   fn_gchandle_get_target = nullptr;
static GCHandleFreeFn        fn_gchandle_free = nullptr;
static GCDisableFn           fn_gc_disable = nullptr;
static GCEnableFn            fn_gc_enable = nullptr;

// 通用导出获取宏
#define LOAD_EXPORT(mod, name, target) \
    do { \
        target = (decltype(target))GetProcAddress(mod, name); \
        if (!target) { Log::Printf("[IL2CPP] 缺少导出: %s", name); return false; } \
    } while(0)

bool Init() {
    g_gameAssembly = GetModuleHandleW(L"GameAssembly.dll");
    g_unityPlayer = GetModuleHandleW(L"UnityPlayer.dll");
    if (!g_gameAssembly || !g_unityPlayer) {
        Log::Printf("[IL2CPP] 模块未加载: GameAssembly=0x%p UnityPlayer=0x%p", g_gameAssembly, g_unityPlayer);
        return false;
    }
    Log::Printf("[IL2CPP] GameAssembly=0x%p UnityPlayer=0x%p", g_gameAssembly, g_unityPlayer);

    // 必须的三个
    LOAD_EXPORT(g_gameAssembly, "il2cpp_domain_get", fn_domain_get);
    LOAD_EXPORT(g_gameAssembly, "il2cpp_thread_attach", fn_thread_attach);
    LOAD_EXPORT(g_gameAssembly, "il2cpp_class_from_name", fn_class_from_name);

    // 可选（有的版本导出名不同）
    fn_image_from_name = (ImageFromNameFn)GetProcAddress(g_gameAssembly, "il2cpp_image_from_name");
    fn_class_get_method = (ClassGetMethodFromNameFn)GetProcAddress(g_gameAssembly, "il2cpp_class_get_method_from_name");
    fn_runtime_invoke = (RuntimeInvokeFn)GetProcAddress(g_gameAssembly, "il2cpp_runtime_invoke");
    fn_string_new = (StringNewFn)GetProcAddress(g_gameAssembly, "il2cpp_string_new");
    fn_string_get_length = (StringGetLengthFn)GetProcAddress(g_gameAssembly, "il2cpp_string_length");
    fn_type_get_object = (TypeGetObjectFn)GetProcAddress(g_gameAssembly, "il2cpp_type_get_object");
    fn_value_box = (ValueBoxFn)GetProcAddress(g_gameAssembly, "il2cpp_value_box");
    fn_gchandle_new = (GCHandleNewFn)GetProcAddress(g_gameAssembly, "il2cpp_gchandle_new");
    fn_gchandle_get_target = (GCHandleGetTargetFn)GetProcAddress(g_gameAssembly, "il2cpp_gchandle_get_target");
    fn_gchandle_free = (GCHandleFreeFn)GetProcAddress(g_gameAssembly, "il2cpp_gchandle_free");
    fn_gc_disable = (GCDisableFn)GetProcAddress(g_gameAssembly, "il2cpp_gc_disable");
    fn_gc_enable = (GCEnableFn)GetProcAddress(g_gameAssembly, "il2cpp_gc_enable");

    Log::Printf("[IL2CPP] Init 完成: runtime_invoke=0x%p string_new=0x%p gc_disable=0x%p",
        fn_runtime_invoke, fn_string_new, fn_gc_disable);
    return true;
}

HMODULE GameAssembly() { return g_gameAssembly; }
HMODULE UnityPlayer() { return g_unityPlayer; }

uintptr_t ModuleBase(const wchar_t* name) {
    HMODULE h = GetModuleHandleW(name);
    return (uintptr_t)h;
}

uintptr_t ModuleSize(const wchar_t* name) {
    HMODULE h = GetModuleHandleW(name);
    if (!h) return 0;
    IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)h;
    IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)((uintptr_t)h + dos->e_lfanew);
    return nt->OptionalHeader.SizeOfImage;
}

void* DomainGet() { return fn_domain_get ? fn_domain_get() : nullptr; }
void* ThreadAttach() { return fn_thread_attach ? fn_thread_attach(fn_domain_get()) : nullptr; }

void GCDisable() { if (fn_gc_disable) fn_gc_disable(); }
void GCEnable()  { if (fn_gc_enable) fn_gc_enable(); }

void* FindClass(const char* image, const char* ns, const char* name) {
    if (!fn_image_from_name || !fn_class_from_name) return nullptr;
    void* img = fn_image_from_name(image);
    if (!img) return nullptr;
    return fn_class_from_name(img, ns, name);
}

void* GetMethodFromName(void* klass, const char* name, int paramCount) {
    if (!fn_class_get_method || !klass) return nullptr;
    return fn_class_get_method(klass, name, paramCount);
}

void* RuntimeInvoke(void* method, void* obj, void** params, void** exc) {
    if (!fn_runtime_invoke || !method) return nullptr;
    return fn_runtime_invoke(method, obj, params, exc);
}

void* StringNew(const char* str) {
    return fn_string_new ? fn_string_new(str) : nullptr;
}

int StringGetLength(void* str) {
    return fn_string_get_length ? fn_string_get_length(str) : 0;
}

void* ValueBox(void* klass, void* value) {
    return fn_value_box ? fn_value_box(klass, value) : nullptr;
}

int32_t GCHandleNew(void* obj) {
    return fn_gchandle_new ? fn_gchandle_new(obj, false) : 0;
}

void* GCHandleGetTarget(int32_t handle) {
    return handle ? (fn_gchandle_get_target ? fn_gchandle_get_target(handle) : nullptr) : nullptr;
}

void GCHandleFree(int32_t handle) {
    if (handle && fn_gchandle_free) fn_gchandle_free(handle);
}

} // namespace IL2CPP