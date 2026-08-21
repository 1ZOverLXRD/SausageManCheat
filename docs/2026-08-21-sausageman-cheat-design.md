# 香肠派对 SausageMan 内部作弊 — 设计文档

- 日期：2026-08-21
- 项目：`F:\Project\Reverse\UnityReverse\unityi2cpp\SausageMan`
- 类型：Unity IL2CPP 内部 DLL（x64）
- 注入：用户已有注入器，只需产出 DLL

## 1. 目标

- ESP：2D Box / 3D Box / 骨骼 / 血量 / 名字 / 距离 / 队伍区分
- Aimbot：固定速度追逐算法，支持骨骼瞄准
- D3D11 Present Hook + ImGui 渲染
- 关键路径日志输出（控制台 + 文件）

## 2. 架构

```
src/
├── dllmain.cpp              # 入口，创建控制台，主线程
├── Core/
│   ├── Memory.h             # 安全内存读写（__try/__except）
│   ├── Log.h/.cpp           # 控制台 + 文件日志
│   └── D3D11Hook.h/.cpp     # Present Hook + ImGui + 渲染循环
├── SDK/
│   ├── IL2CPP.h/.cpp        # IL2CPP API 封装
│   ├── GameOffsets.h        # 所有偏移常量（dump.cs/il2cpp.h/IDA 验证）
│   ├── PlayerManager.h/.cpp # 玩家枚举 + 数据读取 + 骨骼
│   ├── CameraManager.h/.cpp # Camera 矩阵 + 手写 W2S
│   └── Game.h/.cpp          # 薄封装层
├── Cheat/
│   ├── Config.h/.cpp        # ImGui 菜单 + ini 持久化
│   ├── ESP.h/.cpp           # 绘制
│   └── Aimbot.h/.cpp        # 瞄准
└── Render/                  # 预留
```

## 3. 偏移表（全部来自 dump.cs + il2cpp.h + IDA 反编译）

### 玩家枚举
```
GameWorldClientManager 静态 +0x8 → BattleWorld
  → +0x498 → StartGame
    → +0x50 → RoleList (List<BattleRoleLogic>)
    → +0x60 → RolePlayerIdListDic (Dictionary<long, BattleRoleLogic>)
    → +0x98 → RoleNetList (List<RoleNet>)
```

### RoleNet
| 偏移 | 字段 | 类型 |
|------|------|------|
| +0x40 | Transform | Transform* |
| +0x58 | RoleNetClient | RoleNetClient* |
| +0x68 | MyRole | BattleRoleLogic* |
| +0x90 | mirror | RoleNetMirror* |

### RoleNetClient
| 偏移 | 字段 | 类型 |
|------|------|------|
| +0x1D | isLocalPlayer | bool |

### BattleRoleLogic
| 偏移 | 字段 | 类型 |
|------|------|------|
| +0x224 | hp | float |
| +0x228 | maxHp | float |
| +0x768 | NickName | string* |
| +0x770 | playerId | int64 |
| +0x7A8 | TeamNum | int64 |

### GameData 静态
| 偏移 | 字段 | 类型 |
|------|------|------|
| +0x1E0 | WarCamera | CameraController* |
| +0x230 | LocalRole | BattleRole* |
| +0x238 | LocalRoleNet | RoleNet* |

### Camera 矩阵（IDA 反编译确认）
```
managed Camera → +0x10 → native Camera
worldToCameraMatrix = +0x80
projectionMatrix    = +0xC0
```

### AnimatorControl（骨骼）
```
+0x80 = leftHand   +0x88 = rightHand  +0xE8 = Head
+0xF8 = Hip        +0x108 = SkinBody  +0x118 = RightFoot
+0x120 = LeftFoot  +0x190 = Spine     +0x1A8 = animator
+0x168 = MyRole (BattleRole 反向引用)
```

### RoleAnimatorControlPool 静态字典
```
+0x00 = animatorPoolLayer
+0x08 = animData (Dictionary<long, AnimatorControl>*)
+0x10 = initStageLoadEvent
+0x11 = UsePool
+0x18 = aiControllerMap
```
方法：`GetAnimatorControl(long playerId)` — 通过 `RuntimeInvoke` 调用

### IL2CPP 内存布局
```
List<T>: +0x10 = items, +0x18 = count
数组: +0x20 = 元素起始（引用类型）
string: +0x10 = length, +0x14 = UTF-16 chars
```

## 4. 数据流

每帧（Present Hook，单线程）：
1. `IL2CPP::GCDisable()` — 防 GC 堆压缩
2. `Game::Update()`:
   - `PlayerManager::Update()` — 扫 RoleNetList（每30帧全量）
   - 每玩家：`ReadPlayerData()`（位置/血量/名字）+ `ReadSkeleton()`（骨骼）
   - `CameraManager::Update()` — 读矩阵
   - W2S 所有玩家 + 骨骼
3. `Aimbot::Update()` — 目标选择 + mouse_event
4. `RenderFrame()` — ImGui 菜单 + ESP 绘制
5. `IL2CPP::GCEnable()`

## 5. 手写 W2S

```cpp
// 列主序矩阵乘法
// view: worldToCameraMatrix (+0x80)
// proj: projectionMatrix (+0xC0)
v = view * worldPos
clip = proj * v
if (cw < 0.001f) return false
sx = (clip.x/clip.w + 1) * 0.5 * screenW
sy = (1 - clip.y/clip.w) * 0.5 * screenH   // ImGui 顶部原点
```

## 6. 骨骼方案（方案 A）

- 从 `BattleRoleLogic +0x770` 读 playerId
- 用 `RuntimeInvoke` 调 `RoleAnimatorControlPool.GetAnimatorControl(playerId)`
- 得到 AnimatorControl* → 读 8 个骨骼 Transform
- 每帧通过 `Transform.get_position`（RuntimeInvoke）得到世界坐标
- 失败标记 `-1` 不重试，60 帧后输出日志

## 7. 日志输出

| 事件 | 内容 |
|------|------|
| 注入 | DLL 附加成功，模块基址 |
| IL2CPP Init | 导出函数地址 |
| 静态字段 | GWM/GameData static_fields 地址 |
| 玩家扫描 | 数量变化 + 玩家名 + 位置 + 血量 |
| 骨骼 | 获取 AnimatorControl 成功/失败 |
| Camera | 场景切换检测 |
| 异常 | __except 捕获代码 + 上下文 |

## 8. 已知待验证点

1. **static_fields 偏移探测**（0xB8→0xD0 范围）— 运行时需确认，IL2CPP 版本不同会变
2. **BattleRoleLogic +0x770 是 playerId** — il2cpp.h 强信号，需运行时验证
3. **RoleAnimatorControlPool.GetAnimatorControl 静态方法 obj 参数** — 有些版本传 nullptr 会崩，需验证是传 null 还是装箱
4. **Transform.get_position 需要 RuntimeInvoke** — 无法用内存偏移替代（首次调用后缓存）
5. **W2S 的 Y 轴翻转** — ImGui 顶部原点已用 (1-ndcY)，需实机确认
6. **BONE_LEFT_HAND/RightHand 骨骼链路** — 需实机确认 AnimatorControl 字段有效

## 9. 构建

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
# 输出: build/bin/Release/SausageManCheat.dll
```

## 10. 使用

1. 打开香肠派对（TapTap PC 58881）
2. 用注入器注入 SausageManCheat.dll
3. 控制台窗口自动弹出（标题 SausageMan Cheat Log）
4. 按 Insert 开关菜单
5. 状态 Tab → Dump 玩家到日志 检查枚举是否正常