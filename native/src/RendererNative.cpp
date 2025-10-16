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
        case APIType::D3D11:  return D3D11_Render(partial,frame); break;
    }
}
void Backend_Resize(int w,int h){
    g_w=w; g_h=h;
    switch(g_api){
        case APIType::OPENGL: GL_Resize(w,h); break;
        case APIType::VULKAN: VK_Resize(w,h); break;
        case APIType::D3D11:  return D3D11_Resize(w,h); break;
    }
}
void Backend_Shutdown(){
    switch(g_api){
        case APIType::OPENGL: GL_Shutdown(); break;
        case APIType::VULKAN: VK_Shutdown(); break;
        case APIType::D3D11:  return D3D11_Shutdown(); break;
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
