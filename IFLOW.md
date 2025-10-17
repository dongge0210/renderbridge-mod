# RenderBridge Mod - iFlow CLI 上下文文档

## 项目概述

RenderBridge Mod 是一个 Minecraft Forge 1.20.1 Mod，实现多后端渲染桥接功能，支持 OpenGL/D3D11/Vulkan 等多种图形 API。项目采用 Java 17 开发，通过 JNI 连接 C++ 本地库实现高性能渲染。

**主要特性**：
- 多后端渲染支持（OpenGL/D3D11/Vulkan）
- bgfx 风格的渲染器抽象架构
- 跨平台兼容（Windows/macOS/Linux）
- 自动本地库加载机制
- 统一的渲染器工厂模式

## 项目详细规划

**详细规划文档**：请参考 `RenderBridgeFull.md` 获取完整的项目规划和技术规格。该文档包含：

### 阶段化开发路线
1. **阶段 1**：D3D11 直接窗口覆盖验证（当前实现）
2. **阶段 2**：D3D11 离屏渲染与 OpenGL 纹理叠加
3. **阶段 3**：Vulkan 引入（Windows + macOS MoltenVK）
4. **阶段 4**：统一资源抽象（Buffer/Texture/Shader）
5. **阶段 5**：D3D12 后端支持
6. **阶段 6**：性能优化（异步、命令队列、共享句柄）
7. **阶段 7**：bgfx 集成或保持自研

### 技术架构决策
- **Forge 优先**，Fabric 后续补充
- **Windows 优先**，macOS 回退到 OpenGL/Vulkan
- **先 D3D11 验证**，后扩展 Vulkan，再 D3D12
- **自动本地库解压加载**，无需用户手动配置
- **窗口句柄传递**通过 LWJGL → JNI 已实现

### 核心实现细节
- **Java 层设计**：统一接口 + 自动加载本地库
- **JNI 桥接**：`NativeBridge` 类实现 Java-C++ 通信
- **C++ 架构**：多后端渲染器 + 工厂模式 + 平台抽象
- **构建系统**：Gradle（Java）+ CMake（C++）

### 验证步骤
1. 确保 DLL 正确打包到资源目录
2. 运行 `gradlew runClient` 进行测试
3. 观察控制台日志确认初始化状态
4. 验证 D3D11 渲染输出（三角形/颜色渐变）

### 常见问题排错
| 问题 | 原因 | 解决方案 |
|------|------|----------|
| UnsatisfiedLinkError | DLL 未解压或路径错误 | 检查 NativeLoader 路径输出 |
| D3D11 "HWND 未设置" | GLFW 上下文未就绪 | 在 AFTER_SKY 事件中重试 |
| 画面闪烁 | OpenGL 与 D3D11 竞争 | 阶段2离屏渲染解决 |
| 黑屏/崩溃 | ABI 不一致 | 确认 Release x64 构建 |

## 项目结构

```
RenderBridgeMod/
├── src/main/java/com/dongge0210/renderbridge/  # Java层代码
│   ├── RenderBridgeMod.java                    # 主Mod类
│   ├── NativeBridge.java                       # JNI桥接层
│   ├── NativeLoader.java                       # 本地库加载器
│   ├── Renderer.java                           # 渲染器接口
│   └── RendererType.java                       # 渲染器类型枚举
├── src/main/resources/META-INF/                # Mod配置
│   └── mods.toml                               # Forge Mod元数据
├── native/                                     # C++本地库
│   ├── include/                                # 头文件
│   │   ├── Renderer.h                          # 渲染器抽象接口
│   │   ├── Platform.h                          # 平台抽象层
│   │   └── backend/                            # 后端特定头文件
│   ├── src/                                    # 源文件
│   │   ├── Renderer.cpp                        # 渲染器管理
│   │   ├── RendererBackend.cpp                 # 后端实现
│   │   └── backend/                            # 具体后端实现
│   └── CMakeLists.txt                          # CMake构建配置
├── build.gradle                                # Gradle构建配置
├── settings.gradle                             # Gradle设置
├── IFLOW.md                                    # 本文档
└── RenderBridgeFull.md                         # 详细规划文档
```

## 构建和运行

### 前置要求

- Java 17
- CMake 3.24+
- Visual Studio 2022（Windows）或相应编译器
- Forge MDK 1.20.1

### 构建步骤

1. **构建C++本地库**：
```bash
cd native
cmake -B build -S .
cmake --build build --config Release
```

2. **构建Java Mod**：
```bash
./gradlew build
```

3. **运行测试**：
```bash
./gradlew runClient
```

### 开发命令

- `./gradlew clean` - 清理构建
- `./gradlew compileJava` - 编译Java代码
- `./gradlew genIntellijRuns` - 生成IntelliJ运行配置

## 开发约定

### 代码规范

- **Java**：遵循Google Java Style Guide
- **C++**：使用C++20标准，遵循现代C++最佳实践
- **文件编码**：UTF-8
- **日志格式**：`[组件] 消息内容`

### 架构模式

1. **渲染器抽象**：所有后端实现 `IRenderer` 接口
2. **工厂模式**：通过 `RendererFactory` 创建具体后端
3. **单例管理**：`RendererManager` 统一管理渲染器生命周期
4. **平台抽象**：`Platform.h` 提供跨平台支持

### JNI 命名规范

- Java方法：`nMethod` 前缀
- C++实现：`Java_com_dongge0210_renderbridge_NativeBridge_nMethod`
- 参数类型：使用JNI标准类型映射

### 提交规范

- feat: 新功能
- refactor: 重构
- fix: 修复bug
- docs: 文档更新
- chore: 构建/工具相关

## 关键技术点

### 多后端渲染架构

参考 bgfx 设计，实现统一的渲染器抽象层：

```cpp
// 渲染器接口
class IRenderer {
    virtual bool init(const RendererInit& init) = 0;
    virtual void frame() = 0;
    virtual void shutdown() = 0;
    // ...
};

// 工厂创建
auto renderer = RendererFactory::createRenderer(RendererType::D3D11);
```

### 窗口句柄传递

通过 LWJGL 获取原生窗口句柄：

```java
long hwnd = GLFWNativeWin32.glfwGetWin32Window(glfwWindow);
nSetWindowHandle(hwnd, 0); // 0=Windows, 1=macOS
```

### 本地库自动加载

资源文件打包到jar，运行时解压：

```java
Path target = Paths.get(System.getProperty("user.home"), 
                        ".minecraft", "renderbridge", "RendererNative.dll");
Files.copy(in, target, StandardCopyOption.REPLACE_EXISTING);
System.load(target.toAbsolutePath().toString());
```

## 调试和排错

### 常见问题

1. **UnsatisfiedLinkError**：检查本地库是否正确加载
2. **D3D11初始化失败**：确认窗口句柄传递时机
3. **编译错误**：验证Java和C++环境配置

### 日志分析

关键日志标识：
- `[RenderBridge]` - Java层日志
- `[RendererFactory]` - 渲染器工厂日志
- `[D3D11Renderer]` - D3D11后端日志
- `[JNI]` - JNI桥接日志

## 扩展指南

### 添加新渲染后端

1. 在 `RendererType` 枚举中添加新类型
2. 创建 `NewBackend.h` 和 `NewBackend.cpp`
3. 在 `RendererFactory::createRenderer` 中添加创建逻辑
4. 更新 `Platform.h` 添加平台检测

### 性能优化方向

1. **离屏渲染**：避免窗口覆盖问题（阶段2重点）
2. **命令队列**：实现异步渲染（阶段6）
3. **资源管理**：统一Buffer/Texture/Shader抽象（阶段4）
4. **共享内存**：减少CPU-GPU数据传输（阶段6）

## 相关资源

- [bgfx 官方仓库](https://github.com/bkaradzic/bgfx)
- [Forge MDK 文档](https://docs.minecraftforge.net/en/1.20.x/)
- [LWJGL 文档](https://www.lwjgl.org/customize)
- [JNI 规范](https://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/jniTOC.html)

---

**注意**：本文档提供项目概览和快速上手指南，详细的技术规格、实现方案和阶段规划请参考 `RenderBridgeFull.md` 文档。