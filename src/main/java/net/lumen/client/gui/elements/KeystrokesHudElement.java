package net.lumen.client.gui.elements;

import net.lumen.client.hud.HudElement;
import net.lumen.client.theme.LumenTheme;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.gui.DrawContext;
import org.lwjgl.glfw.GLFW;

public class KeystrokesHudElement extends HudElement {
    public KeystrokesHudElement(float x, float y, float scale) {
        super(x, y, scale);
    }

    @Override
    public void render(DrawContext context, float tickDelta) {
        MinecraftClient client = MinecraftClient.getInstance();
        if (client == null) {
            return;
        }
        int activeColor = LumenTheme.active.accent;
        int inactiveColor = LumenTheme.active.muted;
        boolean forward = GLFW.glfwGetKey(client.getWindow().getHandle(), GLFW.GLFW_KEY_W) == GLFW.GLFW_PRESS;
        boolean left = GLFW.glfwGetKey(client.getWindow().getHandle(), GLFW.GLFW_KEY_A) == GLFW.GLFW_PRESS;
        boolean back = GLFW.glfwGetKey(client.getWindow().getHandle(), GLFW.GLFW_KEY_S) == GLFW.GLFW_PRESS;
        boolean right = GLFW.glfwGetKey(client.getWindow().getHandle(), GLFW.GLFW_KEY_D) == GLFW.GLFW_PRESS;

        context.drawTextWithShadow(MinecraftClient.getInstance().textRenderer, "W", (int)x + 14, (int)y, forward ? activeColor : inactiveColor);
        context.drawTextWithShadow(MinecraftClient.getInstance().textRenderer, "A", (int)x, (int)y + 12, left ? activeColor : inactiveColor);
        context.drawTextWithShadow(MinecraftClient.getInstance().textRenderer, "S", (int)x + 14, (int)y + 12, back ? activeColor : inactiveColor);
        context.drawTextWithShadow(MinecraftClient.getInstance().textRenderer, "D", (int)x + 28, (int)y + 12, right ? activeColor : inactiveColor);
    }
}
