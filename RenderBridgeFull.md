# Minecraft Java 多后端渲染桥接总文档（合并版）
包名统一：`com.dongge0210.renderbridge`  
Mod ID：`renderbridge`  
目标：Forge 1.20.1 (Java 17) 下实现多后端渲染桥接：OpenGL / D3D11 / Vulkan / 后续 D3D12。  
当前阶段：已规划整体 + 已给出“步骤1：D3D11 直接窗口覆盖验证实现”。下一阶段将转“步骤2（方案3）：D3D11 离屏渲染与 OpenGL 纹理叠加”。

---

## 目录
1. 总体目标与阶段路线  
2. 架构概览  
3. 已回答的选择 & 影响  
4. 项目目录结构（规范化）  
5. Java 层设计（统一接口 + 自动加载本地库）  
6. JNI 头文件生成与命名变化  
7. C++ 层结构与后端实现（步骤1版本）  
8. 构建方式（VS 2022 与 CMake）  
9. 资源打包与自动解压加载本地库  
10. 运行验证步骤（步骤1：D3D11覆盖）  
11. 常见问题与排错表  
12. 下一步：方案3（离屏 + 合成）规划说明  
13. 后续扩展路线图（含 Vulkan / D3D12）  
14. （原计划第15节）关键问题与当前状态  
15. 你需要反馈的点  

---

## 1. 总体目标与阶段路线
阶段细分：
- 阶段 1（已进行）：验证 D3D11 能获取 HWND，创建 SwapChain，绘制三角形并 Present（覆盖原画面，允许闪烁）。
- 阶段 2（待执行）：转“离屏渲染”方案（D3D11 渲染到纹理，再在 OpenGL 中显示，避免覆盖）。先用 CPU 拷贝低性能版本，后优化共享。
- 阶段 3：Vulkan 引入（Windows + macOS MoltenVK），基本清屏/三角形。
- 阶段 4：OpenGL / Vulkan / D3D11 统一资源抽象（Buffer/Texture/Shader）。
- 阶段 5：加入 D3D12（仅 Windows）。
- 阶段 6：性能优化（异步、命令队列、共享句柄）。
- 阶段 7：可选引入 bgfx 或保持自研。

---

## 2. 架构概览

```
Minecraft Forge (OpenGL via LWJGL)
        |
        v
Java 接口层 (Renderer / NativeBridge / NativeLoader)
        |
        v
JNI 桥
        |
        v
C++ 动态库 RendererNative
   | OpenGLBackend  (利用现有上下文)
   | D3D11Backend   (步骤1: 直接窗口 SwapChain)
   | VulkanBackend  (占位，后续扩展)
   | (未来) D3D12Backend / Offscreen 合成 / Vulkan SwapChain
```

---

## 3. 已回答的选择 & 影响
- Forge 先做，Fabric 以后补。
- 平台：Windows 优先（macOS 回退 OpenGL/Vulkan，不能原生 D3D11/12）。
- 先 D3D11（验证），后 Vulkan，再 D3D12。
- 本地库自动解压加载（无需用户复制）。
- 窗口句柄（HWND）通过 LWJGL -> JNI 已实现传递。
- 方案 1 验证成功后，会做方案 3（离屏纹理合成）。

---

## 4. 项目目录结构（推荐）
```
RenderBridgeMod/
├── build.gradle / settings.gradle
├── gradle/ & gradlew
├── src/main/java/com/dongge0210/renderbridge/
│   ├── RenderBridgeMod.java
│   ├── Renderer.java
│   ├── RendererType.java
│   ├── NativeBridge.java
│   ├── NativeLoader.java
│   └── (未来：Config / VulkanBridge / OffscreenManager 等)
├── src/main/resources/
│   ├── META-INF/mods.toml
│   └── natives/win64/RendererNative.dll   (构建后复制进入)
├── native/
│   ├── CMakeLists.txt
│   ├── include/
│   │   ├── com_dongge0210_renderbridge_NativeBridge.h (由 javac -h 生成)
│   │   ├── RendererNative.h
│   │   └── backend/
│   │       ├── OpenGLBackend.h
│   │       ├── D3D11Backend.h
│   │       ├── VulkanBackend.h
│   ├── src/
│   │   ├── RendererNative.cpp
│   │   ├── OpenGLBackend.cpp
│   │   ├── D3D11Backend.cpp
│   │   ├── VulkanBackend.cpp
│   └── build/ (CMake 或 VS 输出目录)
└── README.md / RenderBridgeFull.md
```

---

## 5. Java 层设计（统一接口 + 自动加载）

### 5.1 Renderer 接口与类型

```java
public interface Renderer {
    void init(RendererType type);
    void render(float partialTicks, long frame);
    void resize(int width, int height);
    void shutdown();
}
```

### 5.2 包内全部 Java 文件（步骤1版本）

```java name=src/main/java/com/dongge0210/renderbridge/NativeLoader.java
package com.dongge0210.renderbridge;

import java.io.IOException;
import java.io.InputStream;
import java.nio.file.*;

public final class NativeLoader {
    private static boolean loaded = false;
    public static void load() {
        if (loaded) return;
        String os = System.getProperty("os.name").toLowerCase();
        boolean isWindows = os.contains("win");
        boolean isMac     = os.contains("mac");
        String resourcePath;
        if (isWindows) {
            resourcePath = "/natives/win64/RendererNative.dll";
        } else if (isMac) {
            System.out.println("[RenderBridge] macOS 暂无 D3D11，本地库不加载（后续 Vulkan/Metal）。");
            return;
        } else {
            System.out.println("[RenderBridge] 非 Windows 平台暂不加载 D3D11，本地库缺少。");
            return;
        }
        try (InputStream in = NativeLoader.class.getResourceAsStream(resourcePath)) {
            if (in == null) {
                System.err.println("[RenderBridge] 资源未找到: " + resourcePath);
                return;
            }
            Path target = Paths.get(System.getProperty("user.home"),
                                    ".minecraft", "renderbridge", "RendererNative.dll");
            Files.createDirectories(target.getParent());
            Files.copy(in, target, StandardCopyOption.REPLACE_EXISTING);
            System.load(target.toAbsolutePath().toString());
            loaded = true;
            System.out.println("[RenderBridge] 已加载本地库: " + target);
        } catch (IOException e) {
            throw new RuntimeException("加载本地库失败: " + e.getMessage(), e);
        }
    }
}
```

```java name=src/main/java/com/dongge0210/renderbridge/RendererType.java
package com.dongge0210.renderbridge;
public enum RendererType { OPENGL(0), VULKAN(1), D3D11(2);
    public final int id;
    RendererType(int id){ this.id = id; }
}
```

```java name=src/main/java/com/dongge0210/renderbridge/Renderer.java
package com.dongge0210.renderbridge;
public interface Renderer {
    void init(RendererType type);
    void render(float partialTicks, long frameIndex);
    void resize(int width, int height);
    void shutdown();
}
```

```java name=src/main/java/com/dongge0210/renderbridge/NativeBridge.java
package com.dongge0210.renderbridge;

import org.lwjgl.glfw.GLFW;
import org.lwjgl.glfw.GLFWNativeWin32;
import org.lwjgl.glfw.GLFWNativeCocoa;

public class NativeBridge implements Renderer {
    private boolean windowHandleSent = false;
    private boolean initialized = false;
    private boolean d3dAttempted = false;

    static { NativeLoader.load(); }

    private native void nInit(int apiType);
    private native void nRender(float partialTicks, long frameIndex);
    private native void nResize(int w, int h);
    private native void nShutdown();
    private native void nSetWindowHandle(long handle, int platform); // 0=WIN32,1=MACOS

    @Override
    public void init(RendererType type) {
        String os = System.getProperty("os.name").toLowerCase();
        boolean isWindows = os.contains("win");
        if (type == RendererType.D3D11 && !isWindows) {
            System.out.println("[RenderBridge] 非 Windows D3D11 回退 OpenGL。");
            type = RendererType.OPENGL;
        }
        nInit(type.id);
        initialized = true;
        if (type == RendererType.D3D11 && isWindows) {
            d3dAttempted = true;
            pushWindowHandleIfNeeded();
        }
    }
    private void pushWindowHandleIfNeeded() {
        if (windowHandleSent) return;
        long glfwWindow = GLFW.glfwGetCurrentContext();
        if (glfwWindow == 0L) {
            System.err.println("[RenderBridge] GLFW 窗口句柄获取失败（可能时机过早）");
            return;
        }
        String os = System.getProperty("os.name").toLowerCase();
        if (os.contains("win")) {
            long hwnd = GLFWNativeWin32.glfwGetWin32Window(glfwWindow);
            if (hwnd == 0L) {
                System.err.println("[RenderBridge] 获取 HWND 失败");
                return;
            }
            nSetWindowHandle(hwnd, 0);
            windowHandleSent = true;
            System.out.println("[RenderBridge] 已传递 HWND");
        } else if (os.contains("mac")) {
            long nsWindow = GLFWNativeCocoa.glfwGetCocoaWindow(glfwWindow);
            nSetWindowHandle(nsWindow, 1);
            windowHandleSent = true;
            System.out.println("[RenderBridge] 已传递 NSWindow（暂未使用）");
        }
    }
    @Override
    public void render(float partialTicks, long frameIndex) {
        if (!initialized) return;
        if (d3dAttempted && !windowHandleSent) pushWindowHandleIfNeeded();
        nRender(partialTicks, frameIndex);
    }
    @Override
    public void resize(int width, int height) {
        if (!initialized) return;
        nResize(width, height);
    }
    @Override
    public void shutdown() {
        if (!initialized) return;
        nShutdown();
        initialized = false;
    }
}
```

```java name=src/main/java/com/dongge0210/renderbridge/RenderBridgeMod.java
package com.dongge0210.renderbridge;

import net.minecraftforge.fml.common.Mod;
import net.minecraftforge.eventbus.api.SubscribeEvent;
import net.minecraftforge.client.event.RenderLevelStageEvent;
import net.minecraftforge.client.event.ClientPlayerNetworkEvent;
import net.minecraftforge.fml.event.lifecycle.FMLClientSetupEvent;
import net.minecraftforge.api.distmarker.Dist;
import net.minecraftforge.fml.common.Mod.EventBusSubscriber;

@Mod("renderbridge")
@EventBusSubscriber(modid="renderbridge", value=Dist.CLIENT)
public class RenderBridgeMod {
    private static NativeBridge bridge;
    private static long frameCounter = 0;

    @SubscribeEvent
    public static void onClientSetup(FMLClientSetupEvent e){
        bridge = new NativeBridge();
        bridge.init(RendererType.D3D11);
        System.out.println("[RenderBridge] 初始化完成（尝试 D3D11）");
    }

    @SubscribeEvent
    public static void onRenderStage(RenderLevelStageEvent e){
        if (bridge == null) return;
        if (e.getStage() == RenderLevelStageEvent.Stage.AFTER_SKY) {
            frameCounter++;
            bridge.render(e.getPartialTick(), frameCounter);
        }
    }

    @SubscribeEvent
    public static void onLogout(ClientPlayerNetworkEvent.LoggingOut e){
        if (bridge != null){
            bridge.shutdown();
            bridge = null;
        }
    }
}
```

---

## 6. JNI 头文件生成与命名
执行：
```bash
javac -h native/include src/main/java/com/dongge0210/renderbridge/NativeBridge.java
```
生成：`native/include/com_dongge0210_renderbridge_NativeBridge.h`  
函数名示例：
- `Java_com_dongge0210_renderbridge_NativeBridge_nInit`
- `Java_com_dongge0210_renderbridge_NativeBridge_nRender`
- `Java_com_dongge0210_renderbridge_NativeBridge_nResize`
- `Java_com_dongge0210_renderbridge_NativeBridge_nShutdown`
- `Java_com_dongge0210_renderbridge_NativeBridge_nSetWindowHandle`

如与你看到的不一致，复制给我核对。

---

## 7. C++ 层（步骤1）文件

### 7.1 JNI 头（参考手写，最终以生成版为准）

```cpp name=native/include/com_dongge0210_renderbridge_NativeBridge.h
#ifndef _Included_com_dongge0210_renderbridge_NativeBridge
#define _Included_com_dongge0210_renderbridge_NativeBridge
#include <jni.h>
#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT void JNICALL Java_com_dongge0210_renderbridge_NativeBridge_nInit(JNIEnv*, jobject, jint);
JNIEXPORT void JNICALL Java_com_dongge0210_renderbridge_NativeBridge_nRender(JNIEnv*, jobject, jfloat, jlong);
JNIEXPORT void JNICALL Java_com_dongge0210_renderbridge_NativeBridge_nResize(JNIEnv*, jobject, jint, jint);
JNIEXPORT void JNICALL Java_com_dongge0210_renderbridge_NativeBridge_nShutdown(JNIEnv*, jobject);
JNIEXPORT void JNICALL Java_com_dongge0210_renderbridge_NativeBridge_nSetWindowHandle(JNIEnv*, jobject, jlong, jint);
#ifdef __cplusplus
}
#endif
#endif
```

### 7.2 RendererNative.h

```cpp name=native/include/RendererNative.h
#pragma once
#include <cstdint>
enum class APIType : int { OPENGL=0, VULKAN=1, D3D11=2 };
bool Backend_Init(APIType type, int w, int h);
void Backend_Render(float partialTicks, uint64_t frame);
void Backend_Resize(int w, int h);
void Backend_Shutdown();
void Backend_SetWindowHandle(void* handle, int platform);
```

### 7.3 OpenGL 后端

```cpp name=native/include/backend/OpenGLBackend.h
#pragma once
bool GL_Init(int w,int h);
void GL_Render(float partialTicks, uint64_t frame);
void GL_Resize(int w,int h);
void GL_Shutdown();
```

```cpp name=native/src/OpenGLBackend.cpp
#include "backend/OpenGLBackend.h"
#include <GL/gl.h>
static bool g_inited=false;
bool GL_Init(int w,int h){ glViewport(0,0,w,h); g_inited=true; return true; }
void GL_Render(float partialTicks, uint64_t frame){
    if(!g_inited) return;
    float t=(frame%300)/300.0f;
    glDisable(GL_DEPTH_TEST);
    glClearColor(t,0.1f,0.3f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_TRIANGLES);
      glColor3f(1,0,0); glVertex2f(-0.5f,-0.5f);
      glColor3f(0,1,0); glVertex2f(0.5f,-0.5f);
      glColor3f(0,0,1); glVertex2f(0,0.5f);
    glEnd();
}
void GL_Resize(int w,int h){ if(g_inited) glViewport(0,0,w,h); }
void GL_Shutdown(){ g_inited=false; }
```

### 7.4 D3D11 后端（SwapChain + 三角形）

```cpp name=native/include/backend/D3D11Backend.h
#pragma once
bool D3D11_SetWindowHandle(void* hwnd);
bool D3D11_Init(int w,int h);
void D3D11_Resize(int w,int h);
void D3D11_Render(float partial, uint64_t frame);
void D3D11_Shutdown();
```

```cpp name=native/src/D3D11Backend.cpp
#include "backend/D3D11Backend.h"
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <cstdio>
#include <vector>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

static HWND g_hwnd=nullptr;
static ComPtr<ID3D11Device> g_device;
static ComPtr<ID3D11DeviceContext> g_context;
static ComPtr<IDXGISwapChain> g_swapChain;
static ComPtr<ID3D11RenderTargetView> g_rtv;
static ComPtr<ID3D11Buffer> g_vertexBuffer;
static ComPtr<ID3D11InputLayout> g_inputLayout;
static ComPtr<ID3D11VertexShader> g_vs;
static ComPtr<ID3D11PixelShader> g_ps;

struct Vertex { float pos[3]; float color[3]; };

bool D3D11_SetWindowHandle(void* hwnd){ g_hwnd=(HWND)hwnd; return g_hwnd!=nullptr; }

static bool CreateSwapChainRTV(int w,int h){
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount=2;
    sd.BufferDesc.Width=w; sd.BufferDesc.Height=h;
    sd.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator=60; sd.BufferDesc.RefreshRate.Denominator=1;
    sd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow=g_hwnd;
    sd.SampleDesc.Count=1;
    sd.Windowed=TRUE;
    sd.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;

    ComPtr<IDXGIDevice> dxgiDevice;
    if(FAILED(g_device->QueryInterface(__uuidof(IDXGIDevice),(void**)&dxgiDevice))){
        std::fprintf(stderr,"[D3D11] Query IDXGIDevice 失败\n"); return false;
    }
    ComPtr<IDXGIAdapter> adapter;
    dxgiDevice->GetAdapter(&adapter);
    ComPtr<IDXGIFactory> factory;
    adapter->GetParent(__uuidof(IDXGIFactory),(void**)&factory);

    if(FAILED(factory->CreateSwapChain(g_device.Get(), &sd, &g_swapChain))){
        std::fprintf(stderr,"[D3D11] 创建 SwapChain 失败\n"); return false;
    }
    ComPtr<ID3D11Texture2D> back;
    if(FAILED(g_swapChain->GetBuffer(0,__uuidof(ID3D11Texture2D),(void**)&back))){
        std::fprintf(stderr,"[D3D11] 获取 BackBuffer 失败\n"); return false;
    }
    if(FAILED(g_device->CreateRenderTargetView(back.Get(), nullptr, &g_rtv))){
        std::fprintf(stderr,"[D3D11] 创建 RTV 失败\n"); return false;
    }
    return true;
}

static bool CompilePipeline(){
    const char* vsSrc=
      "struct VSIn{float3 pos:POSITION;float3 color:COLOR;};"
      "struct VSOut{float4 pos:SV_POSITION;float3 color:COLOR;};"
      "VSOut main(VSIn i){VSOut o;o.pos=float4(i.pos,1);o.color=i.color;return o;}";
    const char* psSrc=
      "struct PSIn{float4 pos:SV_POSITION;float3 color:COLOR;};"
      "float4 main(PSIn i):SV_TARGET{return float4(i.color,1);}";

    ComPtr<ID3DBlob> vsBlob, psBlob, err;
    if(FAILED(D3DCompile(vsSrc, strlen(vsSrc), nullptr,nullptr,nullptr,"main","vs_5_0",0,0,&vsBlob,&err))){
        if(err) std::fprintf(stderr,"%s\n",(char*)err->GetBufferPointer()); return false;
    }
    if(FAILED(D3DCompile(psSrc, strlen(psSrc), nullptr,nullptr,nullptr,"main","ps_5_0",0,0,&psBlob,&err))){
        if(err) std::fprintf(stderr,"%s\n",(char*)err->GetBufferPointer()); return false;
    }
    if(FAILED(g_device->CreateVertexShader(vsBlob->GetBufferPointer(),vsBlob->GetBufferSize(),nullptr,&g_vs))) return false;
    if(FAILED(g_device->CreatePixelShader(psBlob->GetBufferPointer(),psBlob->GetBufferSize(),nullptr,&g_ps))) return false;

    D3D11_INPUT_ELEMENT_DESC layout[2]={
        {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"COLOR",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}
    };
    if(FAILED(g_device->CreateInputLayout(layout,2,vsBlob->GetBufferPointer(),vsBlob->GetBufferSize(),&g_inputLayout))) return false;

    Vertex vertices[3]={
        {{-0.5f,-0.5f,0.0f},{1,0,0}},
        {{ 0.5f,-0.5f,0.0f},{0,1,0}},
        {{ 0.0f, 0.5f,0.0f},{0,0,1}}
    };
    D3D11_BUFFER_DESC bd{}; bd.Usage=D3D11_USAGE_DEFAULT; bd.ByteWidth=sizeof(vertices); bd.BindFlags=D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA init{}; init.pSysMem=vertices;
    if(FAILED(g_device->CreateBuffer(&bd,&init,&g_vertexBuffer))) return false;
    return true;
}

bool D3D11_Init(int w,int h){
    if(!g_hwnd){ std::fprintf(stderr,"[D3D11] HWND 未设置\n"); return false; }
    UINT flags=0;
#ifdef _DEBUG
    flags|=D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL levels[]={
        D3D_FEATURE_LEVEL_11_1,D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,D3D_FEATURE_LEVEL_10_0
    };
    D3D_FEATURE_LEVEL out;
    if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,flags,
                                levels,_countof(levels),D3D11_SDK_VERSION,
                                &g_device,&out,&g_context))){
        std::fprintf(stderr,"[D3D11] 硬件失败，尝试 WARP\n");
        if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,flags,
                                    levels,_countof(levels),D3D11_SDK_VERSION,
                                    &g_device,&out,&g_context))){
            std::fprintf(stderr,"[D3D11] WARP 失败\n");
            return false;
        }
    }
    if(!CreateSwapChainRTV(w,h)) return false;
    if(!CompilePipeline()) return false;
    std::fprintf(stdout,"[D3D11] 初始化成功\n");
    return true;
}

void D3D11_Resize(int w,int h){
    if(!g_swapChain) return;
    g_context->OMSetRenderTargets(0,nullptr,nullptr);
    g_rtv.Reset();
    g_swapChain->ResizeBuffers(0,w,h,DXGI_FORMAT_UNKNOWN,0);
    ComPtr<ID3D11Texture2D> back;
    g_swapChain->GetBuffer(0,__uuidof(ID3D11Texture2D),(void**)&back);
    g_device->CreateRenderTargetView(back.Get(),nullptr,&g_rtv);
}

void D3D11_Render(float partial,uint64_t frame){
    if(!g_context||!g_swapChain||!g_rtv) return;
    float t=(frame%240)/240.0f;
    float clear[4]={t,0.2f,0.5f,1.0f};
    g_context->OMSetRenderTargets(1,g_rtv.GetAddressOf(),nullptr);
    g_context->ClearRenderTargetView(g_rtv.Get(),clear);
    UINT stride=sizeof(Vertex), offset=0;
    g_context->IASetInputLayout(g_inputLayout.Get());
    g_context->IASetVertexBuffers(0,1,g_vertexBuffer.GetAddressOf(),&stride,&offset);
    g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_context->VSSetShader(g_vs.Get(),nullptr,0);
    g_context->PSSetShader(g_ps.Get(),nullptr,0);
    g_context->Draw(3,0);
    g_swapChain->Present(1,0);
}

void D3D11_Shutdown(){
    g_ps.Reset(); g_vs.Reset(); g_inputLayout.Reset();
    g_vertexBuffer.Reset(); g_rtv.Reset(); g_swapChain.Reset();
    g_context.Reset(); g_device.Reset(); g_hwnd=nullptr;
    std::fprintf(stdout,"[D3D11] 已释放\n");
}
```

### 7.5 VulkanBackend（占位）

```cpp name=native/include/backend/VulkanBackend.h
#pragma once
bool VK_Init(int w,int h);
void VK_Render(float partial,uint64_t frame);
void VK_Resize(int w,int h);
void VK_Shutdown();
```

```cpp name=native/src/VulkanBackend.cpp
#include "backend/VulkanBackend.h"
#include <vulkan/vulkan.h>
#include <cstdio>
static VkInstance g_instance=VK_NULL_HANDLE;
bool VK_Init(int w,int h){
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName="RenderBridge"; app.apiVersion=VK_API_VERSION_1_2;
    VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ci.pApplicationInfo=&app;
    if(vkCreateInstance(&ci,nullptr,&g_instance)!=VK_SUCCESS){
        std::fprintf(stderr,"[Vulkan] Instance 创建失败\n"); return false;
    }
    std::fprintf(stdout,"[Vulkan] Instance OK %dx%d\n",w,h); return true;
}
void VK_Render(float partial,uint64_t frame){
    if(!g_instance) return;
    if(frame%120==0) std::fprintf(stdout,"[Vulkan] Frame=%llu\n",(unsigned long long)frame);
}
void VK_Resize(int w,int h){ std::fprintf(stdout,"[Vulkan] Resize %d %d\n",w,h); }
void VK_Shutdown(){ if(g_instance) vkDestroyInstance(g_instance,nullptr); g_instance=VK_NULL_HANDLE; }
```

### 7.6 RendererNative.cpp

```cpp name=native/src/RendererNative.cpp
#include <jni.h>
#include <cstdio>
#include "com_dongge0210_renderbridge_NativeBridge.h"
#include "RendererNative.h"
#include "backend/OpenGLBackend.h"
#include "backend/VulkanBackend.h"
#include "backend/D3D11Backend.h"

static APIType g_api=APIType::OPENGL;
static int g_w=1280, g_h=720;
static bool g_inited=false;

void Backend_SetWindowHandle(void* handle,int platform){
    if(g_api==APIType::D3D11 && platform==0){
        D3D11_SetWindowHandle(handle);
    }
}

bool Backend_Init(APIType t,int w,int h){
    switch(t){
        case APIType::OPENGL: return GL_Init(w,h);
        case APIType::VULKAN: return VK_Init(w,h);
        case APIType::D3D11:  return D3D11_Init(w,h);
    }
    return false;
}
void Backend_Render(float partial,uint64_t frame){
    switch(g_api){
        case APIType::OPENGL: GL_Render(partial,frame); break;
        case APIType::VULKAN: VK_Render(partial,frame); break;
        case APIType::D3D11:  D3D11_Render(partial,frame); break;
    }
}
void Backend_Resize(int w,int h){
    g_w=w; g_h=h;
    switch(g_api){
        case APIType::OPENGL: GL_Resize(w,h); break;
        case APIType::VULKAN: VK_Resize(w,h); break;
        case APIType::D3D11:  D3D11_Resize(w,h); break;
    }
}
void Backend_Shutdown(){
    switch(g_api){
        case APIType::OPENGL: GL_Shutdown(); break;
        case APIType::VULKAN: VK_Shutdown(); break;
        case APIType::D3D11:  D3D11_Shutdown(); break;
    }
    g_inited=false;
}

extern "C" {
JNIEXPORT void JNICALL Java_com_dongge0210_renderbridge_NativeBridge_nInit
  (JNIEnv*, jobject, jint apiType){
    if(g_inited) Backend_Shutdown();
    g_api=static_cast<APIType>(apiType);
    if(Backend_Init(g_api,g_w,g_h)){ g_inited=true;
        std::fprintf(stdout,"[RendererNative] Init success api=%d\n",apiType);
    } else {
        std::fprintf(stderr,"[RendererNative] Init failed api=%d\n",apiType);
    }
}
JNIEXPORT void JNICALL Java_com_dongge0210_renderbridge_NativeBridge_nRender
  (JNIEnv*, jobject, jfloat partial, jlong frame){
    if(!g_inited) return;
    Backend_Render(partial,(uint64_t)frame);
}
JNIEXPORT void JNICALL Java_com_dongge0210_renderbridge_NativeBridge_nResize
  (JNIEnv*, jobject, jint w, jint h){
    if(!g_inited) return;
    Backend_Resize(w,h);
}
JNIEXPORT void JNICALL Java_com_dongge0210_renderbridge_NativeBridge_nShutdown
  (JNIEnv*, jobject){
    Backend_Shutdown();
}
JNIEXPORT void JNICALL Java_com_dongge0210_renderbridge_NativeBridge_nSetWindowHandle
  (JNIEnv*, jobject, jlong handle, jint platform){
    Backend_SetWindowHandle((void*)handle, platform);
}
}
```

---

## 8. 构建（VS / CMake）

### VS 2022
- 创建 DLL 项目，添加上述源文件。
- 包含：`native/include`、`<JDK>/include`、`<JDK>/include/win32`
- 链接：`d3d11.lib`, `d3dcompiler.lib`（可选 `vulkan-1.lib`）
- 编译输出 `RendererNative.dll`，复制到 `src/main/resources/natives/win64/`.

### CMake
```cmake
cmake_minimum_required(VERSION 3.24)
project(RendererNative LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(JAVA_HOME "C:/Program Files/Java/jdk-17")
include_directories("${JAVA_HOME}/include" "${JAVA_HOME}/include/win32" "include")
if(DEFINED ENV{VULKAN_SDK})
    include_directories("$ENV{VULKAN_SDK}/Include")
    link_directories("$ENV{VULKAN_SDK}/Lib")
endif()

add_library(RendererNative SHARED
    src/RendererNative.cpp
    src/OpenGLBackend.cpp
    src/VulkanBackend.cpp
    src/D3D11Backend.cpp
)
target_link_libraries(RendererNative d3d11 d3dcompiler $<$<BOOL:$ENV{VULKAN_SDK}>:vulkan-1>)
set_target_properties(RendererNative PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build")
```

构建：
```bash
cd native
cmake -B build -S .
cmake --build build --config Release
# 复制 build/RendererNative.dll 到资源目录
```

---

## 9. 资源打包与自动加载
DLL 放：`src/main/resources/natives/win64/RendererNative.dll`  
启动加载顺序：
1. Forge Init -> `NativeLoader.load()` 解压到 `~/.minecraft/renderbridge/`
2. `System.load(绝对路径)`  
macOS 不加载 DLL，保留 OpenGL 后端。

---

## 10. 运行验证步骤（步骤1）
1. 确保 DLL 已放入资源。
2. `gradlew runClient` 或 IDE RunClient。
3. 控制台日志应出现：
   - `[RenderBridge] 已加载本地库: ...`
   - `[RendererNative] Init success api=2`
   - `[D3D11] 初始化成功`
4. 窗口里可能出现 D3D11 清屏色渐变 + 三角形，原版画面可能被覆盖或闪烁（预期）。
5. 退出看到 `[D3D11] 已释放`。

如果没有三角形或颜色：
- 查看是否出现 “HWND 未设置” → 可能时机过早，检查是否在 `AFTER_SKY` 事件。
- 确认 DLL 构建架构与 JDK/JVM 一致（64位）。
- 打印更多日志或暂时禁用 OpenGL 后端调用。

---

## 11. 常见问题与排错

| 问题 | 原因 | 解决 |
|------|------|------|
| UnsatisfiedLinkError | DLL 未解压或路径错 | 打印 `NativeLoader` 路径，确认文件存在 |
| D3D11 “HWND 未设置” | GLFW 上下文未就绪 | 在首次 render 重试（已实现），或延后 init |
| 画面闪烁 | OpenGL 与 D3D11 同窗口竞争 | 属于方案1预期，进入方案3后解决 |
| 只有黑屏 | D3D11 清屏覆盖后未绘制成功 | 检查 VS/PS 编译日志 |
| 崩溃且无日志 | 可能 ABI 不一致 | 确认使用 Release x64，JDK x64 |
| Vulkan Instance 失败 | 未安装 Vulkan SDK/驱动 | 暂可忽略（当前重点 D3D11） |

---

## 12. 下一步：方案3（离屏 + OpenGL 合成）规划
目标：不再覆盖主画面，在原版渲染后叠加你的内容。

子步骤（低性能版本优先）：
1. D3D11 创建离屏 RenderTarget（Texture2D + RTV + SRV）。
2. 每帧：渲染三角形到离屏。
3. CPU 拷贝：`CopyResource` -> staging → `Map` 取像素。
4. JNI 返回字节数组或直接在 C++ 调用 `glTexSubImage2D`（需要共享 OpenGL context，注意跨 API 调用风险）。
5. Java OpenGL：创建一个纹理，更新数据，在渲染事件最后阶段绘制一个屏幕空间四边形（GLSL 简单着色器）叠加。
6. 验证：原画面 + 你的三角形（作为图片贴图）。  
后续优化：
- 用 DXGI Shared Handle + WGL_NV_DX_interop（减少 CPU 复制）。
- Vulkan 路径同步设计。

你跑通步骤1后告诉我，我再给方案3的全部文件（新增 OffscreenManager / 更新 NativeBridge / OpenGL 纹理渲染代码）。

---

## 13. 后续扩展路线图（含 Vulkan / D3D12）

阶段 | 内容 | 要点
-----|------|-----
2 | 离屏 + 合成 | CPU 拷贝验证路径
3 | Vulkan Surface + SwapChain | 窗口句柄传递，MoltenVK macOS
4 | 统一资源抽象 | Buffer/Texture/Shader ID 映射
5 | D3D12 后端 | Command List / Fence / Descriptor Heap
6 | 高性能共享 | WGL_NV_DX_interop / Vulkan 外部内存
7 | 着色器统一格式 | GLSL/HLSL → 中间层（SPIR-V）
8 | 动态后端切换 | 运行时迁移资源
9 | 性能调优 | 分帧上传 / 异步加载
10 | 全管线替换 | 拦截 Minecraft 原生渲染
11 | Metal 后端（可选） | macOS 原生
12 | 引入 bgfx（可选） | 降低维护复杂度

---

## 14. 原计划第15节关键问题 & 当前状态
你已选择：
- Forge 优先 ✅
- Windows/macOS ✅（macOS 回退 OpenGL）
- 先 D3D11，再 Vulkan，再 D3D12 ✅
- 自动解压 DLL ✅
- 已介绍窗口句柄传递 ✅

未完成部分：
- 方案3实现 → 待你确认步骤1是否跑通
- 配置系统（后端选择文件/命令行） → 暂未加入
- Fabric 支持 → 后续单独补

---

## 15. 你需要反馈的点
请现在反馈：
1. 步骤1是否已运行成功（日志 + 你看到的现象）？
2. DLL 是否正确加载（给我控制台关键行）？
3. 是否需要我直接跳过 CPU 版本，先给共享句柄 / WGL_NV_DX_interop（还是按计划低性能→高性能）？
4. 是否希望方案3里同时给一个最简 OpenGL Shader 文件（顶点 + 片元）？
5. 你的 VS 编译环境是否使用 `/MD`（防止运行库不匹配）？

回复格式示例：
```
1: 成功（看到三角形覆盖）
2: 已加载
3: 先低性能
4: 要
5: /MD
```

我收到后立即输出“方案3（离屏合成）”全套代码与步骤。若遇到问题请直接粘贴错误或日志，不要概述。

---

## 结语
以上为合并后的完整文档（规划 + 当前实现 + 下一步计划）。请先验证步骤1，再决定方案3代码生成。需要我补 VS 工程示例或 Gradle 任务（复制 DLL）说明也可以直接说。

等待你的反馈。  