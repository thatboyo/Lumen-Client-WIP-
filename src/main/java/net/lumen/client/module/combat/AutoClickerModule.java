package net.lumen.client.module.combat;

import net.lumen.client.event.EventTick;
import net.lumen.client.module.Category;
import net.lumen.client.module.Module;
import net.lumen.client.setting.BooleanSetting;
import net.lumen.client.setting.SliderSetting;
import net.minecraft.client.MinecraftClient;
import net.minecraft.item.ItemStack;
import net.minecraft.component.DataComponentTypes;
import net.minecraft.registry.tag.ItemTags;

public class AutoClickerModule extends Module {
    private final SliderSetting minCps;
    private final SliderSetting maxCps;
    private final BooleanSetting onlyWeapon;
    private int clickTicks = 0;

    public AutoClickerModule() {
        super("AutoClicker", "Simulates mouse clicks at a configurable CPS range.", Category.COMBAT, 0);
        minCps = new SliderSetting("Min CPS", "Minimum clicks per second.", 9.0, 1.0, 20.0, 0.5);
        maxCps = new SliderSetting("Max CPS", "Maximum clicks per second.", 12.0, 1.0, 20.0, 0.5);
        onlyWeapon = new BooleanSetting("Only Weapon", "Only click while holding a weapon.", true);
        registerSetting(minCps);
        registerSetting(maxCps);
        registerSetting(onlyWeapon);
    }

    @Override
    protected void onEnable() {
        subscribe(EventTick.class, this::onTick);
        clickTicks = 0;
    }

    private void onTick(EventTick event) {
        MinecraftClient client = MinecraftClient.getInstance();
        if (client == null || client.player == null) {
            return;
        }

        if (onlyWeapon.getValue()) {
            ItemStack item = client.player.getMainHandStack();
            if (!item.isIn(ItemTags.SWORDS) && !item.contains(DataComponentTypes.TOOL)) {
                return;
            }
        }

        if (!isMouseLeftDown(client)) {
            clickTicks = 0;
            return;
        }

        double minCpsVal = minCps.getValue();
        double maxCpsVal = maxCps.getValue();
        double randomCps = minCpsVal + Math.random() * (maxCpsVal - minCpsVal);
        int ticksPerClick = (int) (20.0 / randomCps);

        clickTicks++;
        if (clickTicks >= ticksPerClick && client.targetedEntity != null) {
            client.interactionManager.attackEntity(client.player, client.targetedEntity);
            clickTicks = 0;
        }
    }

    private boolean isMouseLeftDown(MinecraftClient client) {
        if (client.getWindow() == null) {
            return false;
        }
        return client.mouse.wasLeftButtonClicked();
    }
}
