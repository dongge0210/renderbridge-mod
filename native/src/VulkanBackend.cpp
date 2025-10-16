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
