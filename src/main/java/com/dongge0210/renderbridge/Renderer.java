package com.dongge0210.renderbridge;
public interface Renderer {
    void init(RendererType type);
    void render(float partialTicks, long frameIndex);
    void resize(int width, int height);
    void shutdown();
}