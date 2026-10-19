package net.lumen.client.module.combat;

import net.lumen.client.event.EventKey;
import net.lumen.client.event.EventMotion;
import net.lumen.client.event.EventTick;
import net.lumen.client.module.Category;
import net.lumen.client.module.Module;
import net.lumen.client.setting.BooleanSetting;
import net.lumen.client.setting.ChanceSetting;
import net.lumen.client.setting.EnumSetting;
import net.lumen.client.setting.SliderSetting;
import net.minecraft.client.MinecraftClient;
import net.minecraft.entity.Entity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.math.Vec3d;

import java.util.List;
import java.util.function.Consumer;

public class SilentAuraModule extends Module {
    private final SliderSetting range;
    private final SliderSetting smoothness;
    private final SliderSetting fov;
    private final SliderSetting cooldownThreshold;
    private final ChanceSetting attackChance;
    private final EnumSetting<Priority> priority;
    private final BooleanSetting targetMobs;
    private LivingEntity currentTarget;

    public SilentAuraModule() {
        super("Silent Aura", "Smooth visible aim assist with ghost-style attack timing.", Category.COMBAT, 0);
        this.isGhost = true;
        range = new SliderSetting("Range", "Maximum target range.", 4.0, 2.0, 6.0, 0.1);
        smoothness = new SliderSetting("Smoothness", "Higher values make the aim slower.", 10.0, 1.0, 20.0, 0.5);
        fov = new SliderSetting("FOV", "Field of view cone.", 90.0, 30.0, 180.0, 1.0);
        cooldownThreshold = new SliderSetting("Cooldown", "Attack wait threshold.", 75.0, 50.0, 100.0, 1.0);
        attackChance = new ChanceSetting("Attack Chance", "Chance to perform an attack.", 85.0);
        priority = new EnumSetting<>("Priority", "Target priority selection.", Priority.CLOSEST, Priority.class);
        targetMobs = new BooleanSetting("Target Mobs", "Whether to target mobs.", false);
        registerSetting(range);
        registerSetting(smoothness);
        registerSetting(fov);
        registerSetting(cooldownThreshold);
        registerSetting(attackChance);
        registerSetting(priority);
        registerSetting(targetMobs);
    }

    @Override
    protected void onEnable() {
        subscribe(EventTick.class, this::onTick);
        subscribe(EventMotion.class, this::onMotion);
        currentTarget = null;
    }

    private void onTick(EventTick event) {
        MinecraftClient client = MinecraftClient.getInstance();
        if (client == null || client.player == null || client.world == null) {
            return;
        }

        currentTarget = findTarget(client);
    }

    private void onMotion(EventMotion event) {
        MinecraftClient client = MinecraftClient.getInstance();
        if (client == null || client.player == null || currentTarget == null) {
            return;
        }

        double random = Math.random();
        if (random <= (attackChance.getValue() / 100.0)) {
            if (client.player.getAttackCooldownProgress((float) (cooldownThreshold.getValue() / 100.0)) >= 1.0f) {
                client.interactionManager.attackEntity(client.player, currentTarget);
            }
        }
    }

    private LivingEntity findTarget(MinecraftClient client) {
        List<Entity> entities = client.world.getOtherEntities(client.player, client.player.getBoundingBox().expand(range.getValue()));
        LivingEntity bestTarget = null;
        double bestScore = Double.MAX_VALUE;
        double rangeVal = range.getValue();
        double fovVal = fov.getValue();

        for (Entity entity : entities) {
            if (!(entity instanceof LivingEntity) || entity == client.player) {
                continue;
            }

            if (entity instanceof PlayerEntity && !targetMobs.getValue()) {
                continue;
            }

            double distance = client.player.distanceTo(entity);
            if (distance > rangeVal) {
                continue;
            }

            Vec3d playerToEntity = entity.getEntityPos().subtract(client.player.getEntityPos()).normalize();
            Vec3d playerLook = Vec3d.fromPolar(0, client.player.getYaw()).normalize();
            double angleToTarget = Math.toDegrees(Math.acos(playerToEntity.dotProduct(playerLook)));

            if (angleToTarget > fovVal / 2.0) {
                continue;
            }

            double score = getTargetScore((LivingEntity) entity, distance);
            if (score < bestScore) {
                bestScore = score;
                bestTarget = (LivingEntity) entity;
            }
        }

        return bestTarget;
    }

    private double getTargetScore(LivingEntity target, double distance) {
        Priority priorityMode = priority.getValue();

        switch (priorityMode) {
            case CLOSEST:
                return distance;
            case LOWEST_HEALTH:
                return -target.getHealth();
            case ARMOR_VALUE:
                return -target.getArmor();
            default:
                return distance;
        }
    }

    private enum Priority {
        CLOSEST,
        LOWEST_HEALTH,
        ARMOR_VALUE
    }
}
