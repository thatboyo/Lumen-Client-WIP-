package dev.lumin.bridge;

import java.util.ArrayList;
import java.util.List;
import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.atomic.AtomicInteger;

import net.minecraft.client.MinecraftClient;
import net.minecraft.client.network.ClientPlayerEntity;
import net.minecraft.client.option.SimpleOption;
import net.minecraft.entity.attribute.EntityAttributes;

import dev.lumin.bridge.mixin.MinecraftClientAccessor;
import dev.lumin.bridge.mixin.SimpleOptionAccessor;
import net.minecraft.client.render.Camera;
import net.minecraft.entity.Entity;
import net.minecraft.entity.LivingEntity;
import net.minecraft.entity.player.PlayerEntity;
import net.minecraft.util.math.Vec3d;

/**
 * The only class the native client ever touches through JNI.
 *
 * <p>Rules that make this safe to call from an arbitrary native thread at any time:
 * <ul>
 *   <li>Every getter reads a {@code volatile} field written on the client thread in
 *       {@link #onClientTick(MinecraftClient)}. No game object is touched off-thread.</li>
 *   <li>Every setter only enqueues a {@link Runnable} that runs on the client thread in the
 *       same tick callback, so nothing mutates the world from the render/overlay thread.</li>
 *   <li>Every entry point swallows its own {@link Throwable}: an exception must never unwind
 *       into JNI, where it would abort the process.</li>
 * </ul>
 */
public final class Bridge {
    private Bridge() {
    }

    // ------------------------------------------------------------------
    // Snapshot state: written on the client thread, read from any thread.
    // ------------------------------------------------------------------
    private static volatile boolean ready;
    private static volatile boolean inWorld;

    private static volatile double posX, posY, posZ;
    private static volatile double eyeX, eyeY, eyeZ;
    private static volatile double motX, motY, motZ;
    private static volatile float yaw, pitch;
    private static volatile boolean onGround;
    private static volatile boolean sprinting;

    private static volatile float health, maxHealth;
    private static volatile int food;

    private static volatile double camX, camY, camZ;
    private static volatile float camYaw, camPitch;
    private static volatile double fov;
    private static volatile double gamma = 1.0;

    private static volatile int screenW, screenH;
    private static volatile String dimension = "";

    private static volatile int tickCount;
    private static volatile long frameStamp;

    /** 8 doubles per entity: x, y, z, width, height, health, maxHealth, flags. */
    private static final int STRIDE = 8;
    private static volatile double[] entityData = new double[0];
    private static volatile String[] entityNames = new String[0];
    private static volatile int entityVersion;

    // ------------------------------------------------------------------
    // Command channel: filled from any thread, drained on the client thread.
    // ------------------------------------------------------------------
    private static final Queue<Runnable> IMMEDIATE = new ConcurrentLinkedQueue<>();
    private static final List<Delayed> DELAYED = new ArrayList<>();
    private static final AtomicInteger QUEUE_DEPTH = new AtomicInteger();

    private record Delayed(int atTick, Runnable action) {
    }

    // ==================================================================
    // Called by the mixin every client tick (client thread).
    // ==================================================================
    public static void onClientTick(MinecraftClient mc) {
        try {
            tickCount++;
            drainDelayed(tickCount);
            runQueued();
            snapshot(mc);
        } catch (Throwable t) {
            report("tick", t);
        }
    }

    private static void runQueued() {
        for (int i = 0; i < 512; i++) {
            Runnable r = IMMEDIATE.poll();
            if (r == null) {
                return;
            }
            QUEUE_DEPTH.decrementAndGet();
            try {
                r.run();
            } catch (Throwable t) {
                report("command", t);
            }
        }
    }

    private static void drainDelayed(int now) {
        for (int i = DELAYED.size() - 1; i >= 0; i--) {
            Delayed d = DELAYED.get(i);
            if (d.atTick() <= now) {
                DELAYED.remove(i);
                try {
                    d.action().run();
                } catch (Throwable t) {
                    report("delayed", t);
                }
            }
        }
    }

    private static void snapshot(MinecraftClient mc) {
        var player = mc.player;
        var world = mc.world;

        inWorld = player != null && world != null;
        ready = true;

        if (mc.getWindow() != null) {
            screenW = mc.getWindow().getFramebufferWidth();
            screenH = mc.getWindow().getFramebufferHeight();
        }

        if (!inWorld) {
            entityData = new double[0];
            entityNames = new String[0];
            entityVersion++;
            return;
        }

        posX = player.getX();
        posY = player.getY();
        posZ = player.getZ();

        Vec3d eye = player.getEyePos();
        eyeX = eye.x;
        eyeY = eye.y;
        eyeZ = eye.z;

        Vec3d vel = player.getVelocity();
        motX = vel.x;
        motY = vel.y;
        motZ = vel.z;

        yaw = player.getYaw();
        pitch = player.getPitch();
        onGround = player.isOnGround();
        sprinting = player.isSprinting();

        health = player.getHealth();
        maxHealth = player.getMaxHealth();
        food = player.getHungerManager().getFoodLevel();

        dimension = world.getRegistryKey().getValue().toString();

        Camera cam = mc.gameRenderer.getCamera();
        Vec3d camPos = cam.getCameraPos();
        camX = camPos.x;
        camY = camPos.y;
        camZ = camPos.z;
        camYaw = cam.getYaw();
        camPitch = cam.getPitch();

        // The rendered FOV is the option value with the sprint multiplier applied.
        // Good enough for projecting overlays onto entity boxes.
        double base = mc.options.getFov().getValue();
        fov = sprinting ? base * 1.1 : base;
        gamma = mc.options.getGamma().getValue();

        buildEntities(mc, player);
        frameStamp = System.nanoTime();
    }

    private static final double ESP_RANGE_SQ = 128.0 * 128.0;

    private static void buildEntities(MinecraftClient mc, ClientPlayerEntity self) {
        List<double[]> data = new ArrayList<>();
        List<String> names = new ArrayList<>();

        for (PlayerEntity p : mc.world.getPlayers()) {
            if (p == self) {
                continue;
            }
            if (addEntity(p, data, names, 1)) {
                continue;
            }
        }

        for (Entity e : mc.world.getEntities()) {
            if (!(e instanceof LivingEntity) || e instanceof PlayerEntity) {
                continue;
            }
            addEntity(e, data, names, 0);
        }

        double[] flat = new double[data.size() * STRIDE];
        for (int i = 0; i < data.size(); i++) {
            System.arraycopy(data.get(i), 0, flat, i * STRIDE, STRIDE);
        }

        entityData = flat;
        entityNames = names.toArray(new String[0]);
        entityVersion++;
    }

    private static boolean addEntity(Entity e, List<double[]> data, List<String> names, int playerFlag) {
        if (e.isRemoved()) {
            return false;
        }
        double dx = e.getX() - posX;
        double dy = e.getY() - posY;
        double dz = e.getZ() - posZ;
        if (dx * dx + dy * dy + dz * dz > ESP_RANGE_SQ) {
            return false;
        }

        float hp = 0.0f;
        float maxHp = 0.0f;
        int flag = playerFlag;
        if (e instanceof LivingEntity living) {
            hp = living.getHealth();
            maxHp = living.getMaxHealth();
        }
        if (e instanceof net.minecraft.entity.mob.HostileEntity) {
            flag |= 2;
        }

        data.add(new double[] {
                e.getX(), e.getY(), e.getZ(),
                e.getBoundingBox().getLengthX(),
                e.getBoundingBox().getLengthY(),
                hp, maxHp, flag
        });

        String name;
        try {
            name = e.getName().getString();
        } catch (Throwable t) {
            name = e.getType().getName().getString();
        }
        names.add(name);
        return true;
    }

    // ==================================================================
    // Getters (any thread).
    // ==================================================================
    public static boolean isReady() {
        return ready;
    }

    public static boolean isInWorld() {
        return inWorld;
    }

    public static double getPosX() {
        return posX;
    }

    public static double getPosY() {
        return posY;
    }

    public static double getPosZ() {
        return posZ;
    }

    public static double getEyeX() {
        return eyeX;
    }

    public static double getEyeY() {
        return eyeY;
    }

    public static double getEyeZ() {
        return eyeZ;
    }

    public static double getMotionX() {
        return motX;
    }

    public static double getMotionY() {
        return motY;
    }

    public static double getMotionZ() {
        return motZ;
    }

    public static float getYaw() {
        return yaw;
    }

    public static float getPitch() {
        return pitch;
    }

    public static boolean isOnGround() {
        return onGround;
    }

    public static boolean isSprinting() {
        return sprinting;
    }

    public static float getHealth() {
        return health;
    }

    public static float getMaxHealth() {
        return maxHealth;
    }

    public static int getFood() {
        return food;
    }

    public static double getCamX() {
        return camX;
    }

    public static double getCamY() {
        return camY;
    }

    public static double getCamZ() {
        return camZ;
    }

    public static float getCamYaw() {
        return camYaw;
    }

    public static float getCamPitch() {
        return camPitch;
    }

    public static double getFov() {
        return fov;
    }

    public static double getGamma() {
        return gamma;
    }

    public static int getScreenWidth() {
        return screenW;
    }

    public static int getScreenHeight() {
        return screenH;
    }

    public static String getDimension() {
        return dimension;
    }

    public static int getTickCount() {
        return tickCount;
    }

    public static int getEntityCount() {
        return entityData.length / STRIDE;
    }

    public static int getEntityVersion() {
        return entityVersion;
    }

    public static double[] getEntityData() {
        return entityData;
    }

    public static String[] getEntityNames() {
        return entityNames;
    }

    public static int getQueueDepth() {
        return QUEUE_DEPTH.get();
    }

    // ==================================================================
    // Commands (any thread -> client thread).
    // ==================================================================
    private static void enqueue(Runnable r) {
        if (QUEUE_DEPTH.get() > 4096) {
            return; // never let a stalled game thread balloon memory
        }
        QUEUE_DEPTH.incrementAndGet();
        IMMEDIATE.add(r);
    }

    private static void enqueueDelayed(int ticks, Runnable r) {
        if (QUEUE_DEPTH.get() > 4096) {
            return;
        }
        QUEUE_DEPTH.incrementAndGet();
        synchronized (DELAYED) {
            DELAYED.add(new Delayed(tickCount + Math.max(1, ticks), () -> {
                QUEUE_DEPTH.decrementAndGet();
                r.run();
            }));
        }
    }

    private static MinecraftClient mc() {
        return MinecraftClient.getInstance();
    }

    public static void setSprinting(final boolean value) {
        enqueue(() -> {
            MinecraftClient c = mc();
            if (c.player != null) {
                c.player.setSprinting(value);
            }
        });
    }

    /**
     * Brightness above 1.0 is what makes a real fullbright, but the vanilla option clamps to
     * [0, 1], so the accessor mixin writes the backing value instead.
     */
    public static void setGamma(final double value) {
        enqueue(() -> {
            SimpleOption<Double> option = mc().options.getGamma();
            ((SimpleOptionAccessor) (Object) option).lumin$setValue(value);
        });
    }

    /** Step height is an attribute since 1.20.5; there is no setter on the entity itself. */
    public static void setStepHeight(final double value) {
        enqueue(() -> {
            MinecraftClient c = mc();
            if (c.player == null) {
                return;
            }
            var attribute = c.player.getAttributeInstance(EntityAttributes.STEP_HEIGHT);
            if (attribute != null) {
                attribute.setBaseValue(value);
            }
        });
    }

    public static void setVelocity(final double x, final double y, final double z) {
        enqueue(() -> {
            MinecraftClient c = mc();
            if (c.player != null) {
                c.player.setVelocity(new Vec3d(x, y, z));
            }
        });
    }

    public static void setRotation(final float newYaw, final float newPitch) {
        enqueue(() -> {
            MinecraftClient c = mc();
            if (c.player != null) {
                c.player.setYaw(newYaw);
                c.player.setPitch(Math.max(-90.0f, Math.min(90.0f, newPitch)));
            }
        });
    }

    public static void setFly(final boolean value, final double speed) {
        enqueue(() -> {
            MinecraftClient c = mc();
            if (c.player == null) {
                return;
            }
            var abilities = c.player.getAbilities();
            abilities.allowFlying = value || abilities.allowFlying;
            abilities.flying = value;
            abilities.setFlySpeed((float) speed);
        });
    }

    public static void setFallDistance(final double value) {
        enqueue(() -> {
            MinecraftClient c = mc();
            if (c.player != null) {
                c.player.fallDistance = (float) value;
            }
        });
    }

    /** Written through the accessor mixin; the field is private and has no setter. */
    public static void setItemUseCooldown(final int value) {
        enqueue(() -> ((MinecraftClientAccessor) (Object) mc()).lumin$setItemUseCooldown(value));
    }

    /** Presses the vanilla attack key for one tick so {@code wasPressed()} fires exactly once. */
    public static void clickAttack() {
        enqueue(() -> {
            MinecraftClient c = mc();
            c.options.attackKey.setPressed(true);
            enqueueDelayed(1, () -> c.options.attackKey.setPressed(false));
        });
    }

    /** Presses the vanilla use/place key for one tick, so block placement runs exactly once. */
    public static void clickUse() {
        enqueue(() -> {
            MinecraftClient c = mc();
            c.options.useKey.setPressed(true);
            enqueueDelayed(1, () -> c.options.useKey.setPressed(false));
        });
    }

    public static void sendChat(final String message) {
        if (message == null || message.isEmpty()) {
            return;
        }
        enqueue(() -> {
            MinecraftClient c = mc();
            if (c.getNetworkHandler() != null) {
                c.getNetworkHandler().sendChatMessage(message);
            }
        });
    }

    private static void report(String where, Throwable t) {
        // Deliberately quiet: the native overlay logs its own state. Never rethrow.
        System.err.println("[lumin-bridge] " + where + ": " + t);
    }
}
