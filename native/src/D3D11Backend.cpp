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