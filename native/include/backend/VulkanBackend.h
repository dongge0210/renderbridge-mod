#pragma once
bool VK_Init(int w,int h);
void VK_Render(float partial,uint64_t frame);
void VK_Resize(int w,int h);
void VK_Shutdown();