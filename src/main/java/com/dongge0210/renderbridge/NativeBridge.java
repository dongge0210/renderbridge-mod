package com.dongge0210.renderbridge;

import org.lwjgl.glfw.GLFW;
import org.lwjgl.glfw.GLFWNativeWin32;
import org.lwjgl.glfw.GLFWNativeCocoa;

public class NativeBridge implements Renderer {
    private boolean windowHandleSent = false;
    private boolean initialized = false;
    private boolean d3dAttempted = false;

    static { NativeLoader.load(); }

    private native void nInit(int apiType);
    private native void nRender(float partialTicks, long frameIndex);
    private native void nResize(int w, int h);
    private native void nShutdown();
    private native void nSetWindowHandle(long handle, int platform); // 0=WIN32,1=MACOS

    @Override
    public void init(RendererType type) {
        String os = System.getProperty("os.name").toLowerCase();
        boolean isWindows = os.contains("win");
        if (type == RendererType.D3D11 && !isWindows) {
            System.out.println("[RenderBridge] 非 Windows D3D11 回退 OpenGL。");
            type = RendererType.OPENGL;
        }
        nInit(type.id);
        initialized = true;
        if (type == RendererType.D3D11 && isWindows) {
            d3dAttempted = true;
            pushWindowHandleIfNeeded();
        }
    }
    private void pushWindowHandleIfNeeded() {
        if (windowHandleSent) return;
        long glfwWindow = GLFW.glfwGetCurrentContext();
        if (glfwWindow == 0L) {
            System.err.println("[RenderBridge] GLFW 窗口句柄获取失败（可能时机过早）");
            return;
        }
        String os = System.getProperty("os.name").toLowerCase();
        if (os.contains("win")) {
            long hwnd = GLFWNativeWin32.glfwGetWin32Window(glfwWindow);
            if (hwnd == 0L) {
                System.err.println("[RenderBridge] 获取 HWND 失败");
                return;
            }
            nSetWindowHandle(hwnd, 0);
            windowHandleSent = true;
            System.out.println("[RenderBridge] 已传递 HWND");
        } else if (os.contains("mac")) {
            long nsWindow = GLFWNativeCocoa.glfwGetCocoaWindow(glfwWindow);
            nSetWindowHandle(nsWindow, 1);
            windowHandleSent = true;
            System.out.println("[RenderBridge] 已传递 NSWindow（暂未使用）");
        }
    }
    @Override
    public void render(float partialTicks, long frameIndex) {
        if (!initialized) return;
        if (d3dAttempted && !windowHandleSent) pushWindowHandleIfNeeded();
        nRender(partialTicks, frameIndex);
    }
    @Override
    public void resize(int width, int height) {
        if (!initialized) return;
        nResize(width, height);
    }
    @Override
    public void shutdown() {
        if (!initialized) return;
        nShutdown();
        initialized = false;
    }
}