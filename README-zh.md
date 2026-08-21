# SausageManCheat

香肠派对 (Sausage Man) PC 端 IL2CPP 内部作弊 DLL。

**游戏版本: 25.17.4**

本项目可作为 Unity IL2CPP FPS 作弊开发的参考示例：内部 DLL 里的完整玩家遍历、骨骼读取、ESP 与 Aimbot。

## 游戏信息

- **游戏：** 香肠派对 (Sausage Man)
- **引擎：** Unity IL2CPP（无加密，IL2CPPDumper 直接导出）
- **平台：** PC 版，从 [TapTap](https://www.taptap.cn) 下载安装
- **Dump：** `SausageMan_dump.cs`（80MB）在 [Release v1.0.0](https://github.com/1ZOverLXRD/SausageManCheat/releases/tag/v1.0.0)

## 功能

- **D3D11 Present Hook** — VTable 替换，不依赖 MinHook
- **ImGui 渲染** — 亮色主题，大字体，中文 Fallback，支持 Unity Raw Input
- **玩家遍历** — `GameWorldClientManager → BattleWorld → StartGame → RoleNetList`
- **骨骼读取** — `BattleRoleLogic → RLC → BR → RC → AnimatorControl`，8 个骨骼点
- **ESP**
  - 2D Box（骨骼高度自适应）
  - 3D Box（骨骼尺寸 + 朝向相机）
  - 骨骼绘制（无交叉线）
  - 血量条
  - 名字 + 距离
  - 队伍过滤 / 最大距离
- **Aimbot**（`mouse_event`）
  - 选敌：最近距离 / 最中心 / 最低血量
  - delta/smooth 算法（距离越远移越快，自然收敛）
  - FOV 限制 / 死区 / 平滑系数
  - 可瞄准任意骨骼
- **存活检测** — 手掌位置变化：30 帧(0.5s)不动 → Suspect，60 帧(1s) → Inactive
- **W2S** — 纯矩阵运算，零 Unity API
- **GameSDK** — Transform、RoleNet、RoleLogic、AnimatorControl 类型化封装

![SausageManCheat 功能展示](img.png)

## 项目结构

```
src/
├── dllmain.cpp          # DLL 入口
├── Core/
│   ├── D3D11Hook.cpp/h  # D3D11 Present Hook + ImGui 渲染
│   ├── Log.cpp/h        # 日志（控制台 + 文件）
│   └── Memory.h         # 安全内存读写（__try/__except 保护）
├── SDK/
│   ├── IL2CPP.cpp/h     # IL2CPP 运行时 API 封装
│   ├── GameOffsets.h    # 偏移常量表（dump.cs + il2cpp.h 验证）
│   ├── GameObjects.h    # 游戏对象模型层
│   ├── Game.cpp/h       # 主循环
│   ├── CameraManager.cpp/h  # 相机矩阵读取 + W2S
│   ├── PlayerManager.cpp/h  # 玩家遍历 + 数据采集
│   └── MovementTracker.cpp/h # 存活检测
└── Cheat/
    ├── Config.cpp/h     # 菜单 + 配置
    ├── ESP.cpp/h        # ESP 绘制
    └── Aimbot.cpp/h     # mouse_event 瞄准
```

## 玩家数据

### 遍历链路

```
GameWorldClientManager（静态类）
  └── MyGameWorld (+0x08) → BattleWorld
        └── startGame (+0x498) → StartGame
              └── RoleNetList (+0x98) → List<RoleNet>
```

通过 `il2cpp_class_from_name` 拿 `GameWorldClientManager`，沿 `static_fields → MyGameWorld → BattleWorld → StartGame → RoleNetList` 拿到所有玩家。

### 字段

```
RoleNet (+0x00)
  ├── +0x40 → Transform* (managed)     ← 位置
  ├── +0x58 → RoleNetClient*
  │     └── +0x1D → bool（是否本地玩家）
  └── +0x68 → BattleRoleLogic*
        ├── +0x224 → float HP
        ├── +0x228 → float MaxHP
        ├── +0x768 → string* 昵称
        ├── +0x770 → int64 玩家ID
        └── +0x7A8 → int64 队伍
```

**位置读取：** `Transform.get_position`（唯一 Unity API 调用，每帧每玩家 1 次）

## 骨骼

### 链路

```
BattleRoleLogic
  └── +0xAE0 → RoleLogicComponent
        └── +0x80 → BattleRole
              └── +0x240 → RoleControl
                    └── +0x48 → AnimatorControl
```

### 偏移

| 偏移 | 骨骼 | BoneIndex |
|------|------|-----------|
| +0x80 | LeftHand | 0 |
| +0x88 | RightHand | 1 |
| +0xE8 | Head | 2 |
| +0xF8 | Hip | 3 |
| +0x108 | SkinBody | 4 |
| +0x118 | RightFoot | 5 |
| +0x120 | LeftFoot | 6 |
| +0x190 | Spine | 7 |

### 存活检测

基于手掌位置变化（纯数学，无 API 调用）：
- 左右手平均位置，30 帧(0.5s)不动 → Suspect
- 60 帧(1s)不动 → Inactive
- 动了 → 恢复 Active

## 相机矩阵

```
GameData（静态类）
  └── WarCamera (+0x1E0) → CameraController
        └── MyCamera (+0x30) → managed Camera
              └── m_CachedPtr (+0x10) → native Camera
                    ├── +0x80 → worldToCameraMatrix（16 floats，列主序）
                    └── +0xC0 → projectionMatrix（16 floats，列主序）
```

## 构建

需要 VS 2022+ 和 CMake：

```bash
# 1. 下载 ImGui 到 vendor/
scripts/setup_imgui.bat

# 2. 编译
cmake -B build
cmake --build build --config Release

# 3. 产物
build/bin/Release/SausageManCheat.dll
```

## 注入

用任意 DLL 注入器注入到 `Sausage Man.exe`。

- `DEL` 开关菜单
- 默认 `右键` 激活 Aimbot

---

[English Documentation](README.md)