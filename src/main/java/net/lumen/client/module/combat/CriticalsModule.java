package net.lumen.client.module.combat;

import net.lumen.client.event.EventMotion;
import net.lumen.client.module.Category;
import net.lumen.client.module.Module;
import net.lumen.client.setting.EnumSetting;
import net.minecraft.client.MinecraftClient;

public class CriticalsModule extends Module {
    public enum Mode {
        PACKET,
        JUMP
    }

    private final EnumSetting<Mode> mode;

    public CriticalsModule() {
        super("Criticals", "Forces critical hits by injecting small upward motion.", Category.COMBAT, 0);
        mode = new EnumSetting<>("Mode", "Critical hit mode.", Mode.PACKET, Mode.class);
        registerSetting(mode);
    }

    @Override
    protected void onEnable() {
        subscribe(EventMotion.class, this::onMotion);
    }

    private void onMotion(EventMotion event) {
        MinecraftClient client = MinecraftClient.getInstance();
        if (client.player == null) return;

        switch (mode.getValue()) {
            case PACKET:
                // Inject small upward velocity to trigger critical
                if (client.player.isOnGround()) {
                    client.player.setVelocity(client.player.getVelocity().x, 0.1, client.player.getVelocity().z);
                }
                break;
            case JUMP:
                // Jump-based critical
                if (client.player.isOnGround() && client.options.jumpKey.isPressed()) {
                    client.player.jump();
                }
                break;
        }
    }
}
