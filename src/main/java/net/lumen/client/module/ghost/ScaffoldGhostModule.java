package net.lumen.client.module.ghost;

import net.lumen.client.event.EventTick;
import net.lumen.client.module.Category;
import net.lumen.client.module.Module;
import net.lumen.client.setting.BooleanSetting;
import net.minecraft.client.MinecraftClient;
import net.minecraft.util.math.BlockPos;
import net.minecraft.util.math.Direction;

public class ScaffoldGhostModule extends Module {
    private final BooleanSetting onlyOnBridgingBlocks;

    public ScaffoldGhostModule() {
        super("Scaffold Ghost", "Automates sneaking only while bridging at edges.", Category.GHOST, 0);
        onlyOnBridgingBlocks = new BooleanSetting("Only On Bridging Blocks", "Only activate when bridging.", true);
        registerSetting(onlyOnBridgingBlocks);
    }

    @Override
    protected void onEnable() {
        subscribe(EventTick.class, this::onTick);
    }

    private void onTick(EventTick event) {
        MinecraftClient client = MinecraftClient.getInstance();
        if (client.player == null || client.world == null) return;

        boolean shouldSneak = false;

        // Check if moving forward
        if (client.options.forwardKey.isPressed()) {
            // Get direction player is facing
            Direction facing = client.player.getHorizontalFacing();
            BlockPos currentPos = client.player.getBlockPos();
            BlockPos nextPos = currentPos.offset(facing);

            // Check if next position is air (gap to bridge)
            if (client.world.getBlockState(nextPos).isAir()) {
                BlockPos belowNext = nextPos.down();
                if (client.world.getBlockState(belowNext).isAir()) {
                    // Edge condition: no block below the next step
                    shouldSneak = true;
                }
            }
        }

        // If only on bridging blocks, additional check? For now, assume it's covered
        client.player.setSneaking(shouldSneak);
    }
}
