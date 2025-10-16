#pragma once
bool D3D11_SetWindowHandle(void* hwnd);
bool D3D11_Init(int w,int h);
void D3D11_Resize(int w,int h);
void D3D11_Render(float partial, uint64_t frame);
void D3D11_Shutdown();