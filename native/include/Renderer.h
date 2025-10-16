#pragma once
#include <cstdint>
#include <memory>

namespace RenderBridge {

// 渲染器类型枚举
enum class RendererType : uint32_t {
    Noop = 0,
    Direct3D11 = 1,
    Direct3D12 = 2,
    Metal = 3,
    OpenGL = 4,
    OpenGLES = 5,
    Vulkan = 6,
    Count
};

// 渲染器配置
struct RendererInit {
    RendererType type;
    void* ndt;          // Native display type
    void* nwh;          // Native window handle
    uint16_t width;
    uint16_t height;
    bool debug;
    bool multithreaded;
    
    RendererInit() 
        : type(RendererType::OpenGL)
        , ndt(nullptr)
        , nwh(nullptr)
        , width(1280)
        , height(720)
        , debug(false)
        , multithreaded(true)
    {}
};

// 渲染器接口基类
class IRenderer {
public:
    virtual ~IRenderer() = default;
    
    virtual bool init(const RendererInit& init) = 0;
    virtual void shutdown() = 0;
    
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    
    virtual void setViewRect(uint8_t view, uint16_t x, uint16_t y, uint16_t width, uint16_t height) = 0;
    virtual void touch(uint8_t view) = 0;
    
    virtual void submit(uint8_t view, const void* data, uint32_t size) = 0;
    virtual void submit(uint8_t view, const void* data, uint32_t size, bool discard) = 0;
    
    virtual void frame() = 0;
    
    virtual void updateViewName(uint8_t view, const char* name) = 0;
    virtual void setMarker(const char* marker) = 0;
    
    virtual RendererType getType() const = 0;
    virtual const char* getName() const = 0;
};

// 渲染器工厂
class RendererFactory {
public:
    static std::unique_ptr<IRenderer> createRenderer(RendererType type);
    static bool isSupported(RendererType type);
    static RendererType getFirstSupported();
};

// 全局渲染器管理
class RendererManager {
public:
    static RendererManager& instance();
    
    bool init(const RendererInit& init);
    void shutdown();
    
    IRenderer* getRenderer() const { return m_renderer.get(); }
    
    void beginFrame();
    void endFrame();
    void frame();
    
private:
    std::unique_ptr<IRenderer> m_renderer;
    bool m_initialized = false;
};

} // namespace RenderBridge