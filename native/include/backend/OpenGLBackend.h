#pragma once
bool GL_Init(int w,int h);
void GL_Render(float partialTicks, uint64_t frame);
void GL_Resize(int w,int h);
void GL_Shutdown();