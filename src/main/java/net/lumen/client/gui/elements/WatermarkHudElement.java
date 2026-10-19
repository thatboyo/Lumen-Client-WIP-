package net.lumen.client.gui.elements;

import net.lumen.client.hud.HudElement;
import net.lumen.client.theme.LumenTheme;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.gl.RenderPipelines;
import net.minecraft.client.gui.DrawContext;
import net.minecraft.util.Identifier;

public class WatermarkHudElement extends HudElement {
    public WatermarkHudElement(float x, float y, float scale) {
        super(x, y, scale);
    }

    @Override
    public void render(DrawContext context, float tickDelta) {
        Identifier watermark = LumenTheme.active.watermarkAsset();
        if (watermark != null) {
            int width = 100;
            int height = 12;
            // Minecraft 1.21.11 requires a RenderPipeline for GUI texture drawing.
            context.drawTexture(RenderPipelines.GUI_TEXTURED, watermark,
                    (int) x, (int) y, 0.0f, 0.0f, width, height, width, height);
            return;
        }

        int textColor = LumenTheme.active.accent;
        context.drawTextWithShadow(MinecraftClient.getInstance().textRenderer,
                "Lumen v1.0 | 1.21.x", (int) x, (int) y, textColor);
    }
}
