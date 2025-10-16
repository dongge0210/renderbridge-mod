#pragma once
#include <cstdint>
enum class APIType : int { OPENGL=0, VULKAN=1, D3D11=2 };
bool Backend_Init(APIType type, int w, int h);
void Backend_Render(float partialTicks, uint64_t frame);
void Backend_Resize(int w, int h);
void Backend_Shutdown();
void Backend_SetWindowHandle(void* handle, int platform);