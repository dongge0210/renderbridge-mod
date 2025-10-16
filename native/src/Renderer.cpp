#include "Renderer.h"
#include "Platform.h"
#include <cstdio>

namespace RenderBridge {

// D3D11渲染器声明
class D3D11Renderer : public IRenderer {
public:
    bool init(const RendererInit& init) override;
    void shutdown() override;
    void beginFrame() override;
    void endFrame() override;
    void setViewRect(uint8_t view, uint16_t x, uint16_t y, uint16_t width, uint16_t height) override;
    void touch(uint8_t view) override;
    void submit(uint8_t view, const void* data, uint32_t size) override;
    void submit(uint8_t view, const void* data, uint32_t size, bool discard) override;
    void frame() override;
    void updateViewName(uint8_t view, const char* name) override;
    void setMarker(const char* marker) override;
    RendererType getType() const override { return RendererType::Direct3D11; }
    const char* getName() const override { return "Direct3D 11"; }

private:
    // D3D11具体实现将在D3D11Backend中
};

// OpenGL渲染器声明
class OpenGLRenderer : public IRenderer {
public:
    bool init(const RendererInit& init) override;
    void shutdown() override;
    void beginFrame() override;
    void endFrame() override;
    void setViewRect(uint8_t view, uint16_t x, uint16_t y, uint16_t width, uint16_t height) override;
    void touch(uint8_t view) override;
    void submit(uint8_t view, const void* data, uint32_t size) override;
    void submit(uint8_t view, const void* data, uint32_t size, bool discard) override;
    void frame() override;
    void updateViewName(uint8_t view, const char* name) override;
    void setMarker(const char* marker) override;
    RendererType getType() const override { return RendererType::OpenGL; }
    const char* getName() const override { return "OpenGL"; }

private:
    // OpenGL具体实现将在OpenGLBackend中
};

// Vulkan渲染器声明
class VulkanRenderer : public IRenderer {
public:
    bool init(const RendererInit& init) override;
    void shutdown() override;
    void beginFrame() override;
    void endFrame() override;
    void setViewRect(uint8_t view, uint16_t x, uint16_t y, uint16_t width, uint16_t height) override;
    void touch(uint8_t view) override;
    void submit(uint8_t view, const void* data, uint32_t size) override;
    void submit(uint8_t view, const void* data, uint32_t size, bool discard) override;
    void frame() override;
    void updateViewName(uint8_t view, const char* name) override;
    void setMarker(const char* marker) override;
    RendererType getType() const override { return RendererType::Vulkan; }
    const char* getName() const override { return "Vulkan"; }

private:
    // Vulkan具体实现将在VulkanBackend中
};

// 渲染器工厂实现
std::unique_ptr<IRenderer> RendererFactory::createRenderer(RendererType type) {
    switch (type) {
        case RendererType::Direct3D11:
            return std::make_unique<D3D11Renderer>();
        case RendererType::OpenGL:
            return std::make_unique<OpenGLRenderer>();
        case RendererType::Vulkan:
            return std::make_unique<VulkanRenderer>();
        default:
            printf("[RendererFactory] Unsupported renderer type: %d\n", static_cast<int>(type));
            return nullptr;
    }
}

bool RendererFactory::isSupported(RendererType type) {
    switch (type) {
#if RB_PLATFORM_WINDOWS
        case RendererType::Direct3D11:
            return true;
#endif
        case RendererType::OpenGL:
            return true;
        case RendererType::Vulkan:
            // TODO: 检查Vulkan运行时是否可用
            return false;
        default:
            return false;
    }
}

RendererType RendererFactory::getFirstSupported() {
    RendererType preferred[] = {
#if RB_PLATFORM_WINDOWS
        RendererType::Direct3D11,
#endif
        RendererType::OpenGL,
        RendererType::Vulkan,
    };

    for (RendererType type : preferred) {
        if (isSupported(type)) {
            return type;
        }
    }

    return RendererType::Noop;
}

// 渲染器管理器实现
RendererManager& RendererManager::instance() {
    static RendererManager instance;
    return instance;
}

bool RendererManager::init(const RendererInit& init) {
    if (m_initialized) {
        printf("[RendererManager] Already initialized\n");
        return true;
    }

    printf("[RendererManager] Initializing renderer type: %d\n", static_cast<int>(init.type));
    
    m_renderer = RendererFactory::createRenderer(init.type);
    if (!m_renderer) {
        printf("[RendererManager] Failed to create renderer\n");
        return false;
    }

    if (!m_renderer->init(init)) {
        printf("[RendererManager] Failed to initialize renderer\n");
        m_renderer.reset();
        return false;
    }

    m_initialized = true;
    printf("[RendererManager] Successfully initialized %s renderer\n", m_renderer->getName());
    return true;
}

void RendererManager::shutdown() {
    if (!m_initialized) {
        return;
    }

    if (m_renderer) {
        m_renderer->shutdown();
        m_renderer.reset();
    }

    m_initialized = false;
    printf("[RendererManager] Shutdown complete\n");
}

void RendererManager::beginFrame() {
    if (m_renderer && m_initialized) {
        m_renderer->beginFrame();
    }
}

void RendererManager::endFrame() {
    if (m_renderer && m_initialized) {
        m_renderer->endFrame();
    }
}

void RendererManager::frame() {
    if (m_renderer && m_initialized) {
        m_renderer->frame();
    }
}

} // namespace RenderBridge