#include <jni.h>
#include <cstdio>
#include "com_dongge0210_renderbridge_NativeBridge.h"
#include "Renderer.h"
#include "Platform.h"

using namespace RenderBridge;

extern "C" {
JNIEXPORT void JNICALL Java_com_dongge0210_renderbridge_NativeBridge_nInit
  (JNIEnv*, jobject, jint apiType){
    RendererInit init;
    init.type = static_cast<RendererType>(apiType);
    init.width = 1280;
    init.height = 720;
    init.debug = true;
    init.multithreaded = true;
    
    printf("[JNI] Initializing renderer type: %d\n", apiType);
    
    if (RendererManager::instance().init(init)) {
        printf("[JNI] Renderer initialized successfully\n");
    } else {
        printf("[JNI] Failed to initialize renderer\n");
    }
}

JNIEXPORT void JNICALL Java_com_dongge0210_renderbridge_NativeBridge_nRender
  (JNIEnv*, jobject, jfloat partial, jlong frame){
    RendererManager& manager = RendererManager::instance();
    manager.beginFrame();
    manager.frame();
    manager.endFrame();
}

JNIEXPORT void JNICALL Java_com_dongge0210_renderbridge_NativeBridge_nResize
  (JNIEnv*, jobject, jint w, jint h){
    // 通过渲染器管理器设置视口
    RendererManager& manager = RendererManager::instance();
    IRenderer* renderer = manager.getRenderer();
    if (renderer) {
        renderer->setViewRect(0, 0, 0, static_cast<uint16_t>(w), static_cast<uint16_t>(h));
    }
}

JNIEXPORT void JNICALL Java_com_dongge0210_renderbridge_NativeBridge_nShutdown
  (JNIEnv*, jobject){
    printf("[JNI] Shutting down renderer\n");
    RendererManager::instance().shutdown();
}

JNIEXPORT void JNICALL Java_com_dongge0210_renderbridge_NativeBridge_nSetWindowHandle
  (JNIEnv*, jobject, jlong handle, jint platform){
    printf("[JNI] Setting window handle: %lld, platform: %d\n", handle, platform);
    
    // 重新初始化渲染器，这次带着窗口句柄
    RendererManager::instance().shutdown();
    
    RendererInit init;
    init.nwh = reinterpret_cast<void*>(handle);
    init.width = 1280;
    init.height = 720;
    init.debug = true;
    init.multithreaded = true;
    
    // 根据平台选择合适的渲染器
    switch (platform) {
        case 0: // Windows
            init.type = RendererType::Direct3D11;
            break;
        case 1: // macOS
            init.type = RendererType::OpenGL;
            break;
        default:
            init.type = RendererType::OpenGL;
            break;
    }
    
    if (RendererManager::instance().init(init)) {
        printf("[JNI] Renderer re-initialized with window handle\n");
    } else {
        printf("[JNI] Failed to re-initialize renderer with window handle\n");
    }
}
}