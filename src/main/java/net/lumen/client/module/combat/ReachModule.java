package net.lumen.client.module.combat;

import net.lumen.client.event.EventPacket;
import net.lumen.client.module.Category;
import net.lumen.client.module.Module;
import net.lumen.client.setting.ChanceSetting;
import net.lumen.client.setting.SliderSetting;
import net.minecraft.client.MinecraftClient;
import net.minecraft.util.math.Vec3d;

public class ReachModule extends Module {
    private final SliderSetting reach;
    private final ChanceSetting chance;

    public ReachModule() {
        super("Reach", "Extends attack reach when chance allows.", Category.COMBAT, 0);
        this.isGhost = true;
        reach = new SliderSetting("Reach", "Extended reach value.", 4.5, 3.0, 6.0, 0.1);
        chance = new ChanceSetting("Chance", "Chance to use extended reach.", 65.0);
        registerSetting(reach);
        registerSetting(chance);
    }

    @Override
    protected void onEnable() {
        subscribe(EventPacket.class, this::onPacket);
    }

    private void onPacket(EventPacket event) {
        MinecraftClient client = MinecraftClient.getInstance();
        if (client == null || client.player == null) {
            return;
        }

        double random = Math.random();
        if (random <= (chance.getValue() / 100.0)) {
            double reachVal = reach.getValue();
            Vec3d playerPos = client.player.getEntityPos();
            
            if (client.targetedEntity != null) {
                Vec3d targetPos = client.targetedEntity.getEntityPos();
                double distance = playerPos.distanceTo(targetPos);
                
                if (distance <= reachVal) {
                    // Allow the attack
                }
            }
        }
    }
}
