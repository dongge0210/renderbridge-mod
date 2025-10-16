package com.dongge0210.renderbridge;

import net.minecraftforge.fml.common.Mod;
import net.minecraftforge.eventbus.api.SubscribeEvent;
import net.minecraftforge.client.event.RenderLevelStageEvent;
import net.minecraftforge.client.event.ClientPlayerNetworkEvent;
import net.minecraftforge.fml.event.lifecycle.FMLClientSetupEvent;
import net.minecraftforge.api.distmarker.Dist;
import net.minecraftforge.fml.common.Mod.EventBusSubscriber;

@Mod("renderbridge")
@EventBusSubscriber(modid="renderbridge", value=Dist.CLIENT)
public class RenderBridgeMod {
    private static NativeBridge bridge;
    private static long frameCounter = 0;

    @SubscribeEvent
    public static void onClientSetup(FMLClientSetupEvent e){
        bridge = new NativeBridge();
        bridge.init(RendererType.D3D11);
        System.out.println("[RenderBridge] 初始化完成（尝试 D3D11）");
    }

    @SubscribeEvent
    public static void onRenderStage(RenderLevelStageEvent e){
        if (bridge == null) return;
        if (e.getStage() == RenderLevelStageEvent.Stage.AFTER_SKY) {
            frameCounter++;
            bridge.render(e.getPartialTick(), frameCounter);
        }
    }

    @SubscribeEvent
    public static void onLogout(ClientPlayerNetworkEvent.LoggingOut e){
        if (bridge != null){
            bridge.shutdown();
            bridge = null;
        }
    }
}