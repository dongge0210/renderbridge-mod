package com.dongge0210.renderbridge;
public enum RendererType { OPENGL(0), VULKAN(1), D3D11(2);
    public final int id;
    RendererType(int id){ this.id = id; }
}