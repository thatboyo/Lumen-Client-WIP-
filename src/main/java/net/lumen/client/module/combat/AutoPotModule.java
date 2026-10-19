package net.lumen.client.module.combat;

import net.lumen.client.event.EventTick;
import net.lumen.client.module.Category;
import net.lumen.client.module.Module;
import net.lumen.client.setting.BooleanSetting;
import net.lumen.client.setting.SliderSetting;
import net.minecraft.client.MinecraftClient;
import net.minecraft.item.ItemStack;
import net.minecraft.item.Items;
import net.minecraft.util.Hand;
import net.minecraft.entity.effect.StatusEffects;

public class AutoPotModule extends Module {
    private final SliderSetting healthThreshold;
    private final BooleanSetting preferSplash;

    public AutoPotModule() {
        super("AutoPot", "Automatically uses health or splash potions when health drops below threshold.", Category.COMBAT, 0);
        healthThreshold = new SliderSetting("Health Threshold", "Health level to trigger potion.", 8.0, 1.0, 19.0, 0.5);
        preferSplash = new BooleanSetting("Prefer Splash", "Use splash potions if available.", true);
        registerSetting(healthThreshold);
        registerSetting(preferSplash);
    }

    @Override
    protected void onEnable() {
        subscribe(EventTick.class, this::onTick);
    }

    private void onTick(EventTick event) {
        MinecraftClient client = MinecraftClient.getInstance();
        if (client.player == null || client.world == null) return;

        // Check if player health is below threshold
        if (client.player.getHealth() > healthThreshold.getValue()) {
            return;
        }

        // Check for healing effects already active
        if (client.player.hasStatusEffect(StatusEffects.INSTANT_HEALTH) || 
            client.player.hasStatusEffect(StatusEffects.REGENERATION)) {
            return;
        }

        // Find potion in hotbar
        int potionSlot = -1;
        for (int i = 0; i < 9; i++) {
            ItemStack stack = client.player.getInventory().getStack(i);
            if (stack.getItem() == Items.POTION || 
                (preferSplash.getValue() && stack.getItem() == Items.SPLASH_POTION)) {
                potionSlot = i;
                break;
            }
        }

        if (potionSlot != -1) {
            // Switch to potion
            int previousSlot = client.player.getInventory().getSelectedSlot();
            client.player.getInventory().setSelectedSlot(potionSlot);

            // Use potion
            if (client.player.getInventory().getStack(potionSlot).getItem() == Items.SPLASH_POTION) {
                client.interactionManager.interactItem(client.player, Hand.MAIN_HAND);
            } else {
                client.interactionManager.interactItem(client.player, Hand.MAIN_HAND);
            }

            // Switch back
            client.player.getInventory().setSelectedSlot(previousSlot);
        }
    }
}
