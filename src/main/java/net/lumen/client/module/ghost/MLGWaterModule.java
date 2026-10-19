package net.lumen.client.module.ghost;

import net.lumen.client.event.EventTick;
import net.lumen.client.module.Category;
import net.lumen.client.module.Module;
import net.lumen.client.setting.BooleanSetting;
import net.lumen.client.setting.SliderSetting;
import net.minecraft.client.MinecraftClient;
import net.minecraft.item.Items;
import net.minecraft.util.Hand;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Direction;
import net.minecraft.util.hit.BlockHitResult;
import net.minecraft.util.math.Vec3d;

public class MLGWaterModule extends Module {
    private final SliderSetting fallThreshold;
    private final BooleanSetting autoReequip;

    private int previousSlot = -1;

    public MLGWaterModule() {
        super("MLG Water", "Automatically places a water bucket to break fall damage.", Category.GHOST, 0);
        fallThreshold = new SliderSetting("Fall Threshold", "Minimum fall distance to trigger.", 10.0, 2.0, 30.0, 0.5);
        autoReequip = new BooleanSetting("Auto Re-equip", "Return to the previous item after placement.", true);
        registerSetting(fallThreshold);
        registerSetting(autoReequip);
    }

    @Override
    protected void onEnable() {
        subscribe(EventTick.class, this::onTick);
    }

    private void onTick(EventTick event) {
        MinecraftClient client = MinecraftClient.getInstance();
        if (client.player == null || client.world == null || client.interactionManager == null) return;

        if (client.player.fallDistance > fallThreshold.getValue() && !client.player.isOnGround()) {
            // Find water bucket in hotbar
            int waterSlot = -1;
            for (int i = 0; i < 9; i++) {
                if (client.player.getInventory().getStack(i).getItem() == Items.WATER_BUCKET) {
                    waterSlot = i;
                    break;
                }
            }

            if (waterSlot != -1) {
                // Switch to water bucket
                previousSlot = client.player.getInventory().getSelectedSlot();
                client.player.getInventory().selectedSlot = waterSlot;

                // Find position to place water (below player)
                BlockPos placePos = client.player.getBlockPos().down();
                if (client.world.getBlockState(placePos).isAir()) {
                    // Create block hit result for placement
                    Vec3d hitPos = new Vec3d(placePos.getX() + 0.5, placePos.getY(), placePos.getZ() + 0.5);
                    BlockHitResult hitResult = new BlockHitResult(hitPos, Direction.UP, placePos, false);

                    // Attempt to place water
                    client.interactionManager.interactBlock(client.player, Hand.MAIN_HAND, hitResult);

                    // Switch back if auto re-equip
                    if (autoReequip.getValue() && previousSlot != -1) {
                        client.player.getInventory().selectedSlot = previousSlot;
                    }
                }
            }
        }
    }
}
