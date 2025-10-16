package com.dongge0210.renderbridge;

import java.io.IOException;
import java.io.InputStream;
import java.nio.file.*;

public final class NativeLoader {
    private static boolean loaded = false;
    public static void load() {
        if (loaded) return;
        String os = System.getProperty("os.name").toLowerCase();
        boolean isWindows = os.contains("win");
        boolean isMac     = os.contains("mac");
        String resourcePath;
        if (isWindows) {
            resourcePath = "/natives/win64/RendererNative.dll";
        } else if (isMac) {
            System.out.println("[RenderBridge] macOS 暂无 D3D11，本地库不加载（后续 Vulkan/Metal）。");
            return;
        } else {
            System.out.println("[RenderBridge] 非 Windows 平台暂不加载 D3D11，本地库缺少。");
            return;
        }
        try (InputStream in = NativeLoader.class.getResourceAsStream(resourcePath)) {
            if (in == null) {
                System.err.println("[RenderBridge] 资源未找到: " + resourcePath);
                return;
            }
            Path target = Paths.get(System.getProperty("user.home"),
                                    ".minecraft", "renderbridge", "RendererNative.dll");
            Files.createDirectories(target.getParent());
            Files.copy(in, target, StandardCopyOption.REPLACE_EXISTING);
            System.load(target.toAbsolutePath().toString());
            loaded = true;
            System.out.println("[RenderBridge] 已加载本地库: " + target);
        } catch (IOException e) {
            throw new RuntimeException("加载本地库失败: " + e.getMessage(), e);
        }
    }
}