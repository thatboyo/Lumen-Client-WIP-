#pragma once

#include <string>
#include <vector>

namespace lumin {

// One row of the bridge's entity snapshot.
struct EntitySnapshot {
    double x = 0, y = 0, z = 0;
    double width = 0.6, height = 1.8;
    float health = 0, maxHealth = 0;
    int flags = 0; // bit0 = player, bit1 = hostile

    bool isPlayer() const { return (flags & 1) != 0; }
    bool isHostile() const { return (flags & 2) != 0; }
    std::string name;
};

// Everything the overlay needs, read once per frame. All fields come from
// volatile Java fields, so this is safe from the render thread.
struct GameState {
    bool ready = false;
    bool inWorld = false;

    double px = 0, py = 0, pz = 0;
    double eyeX = 0, eyeY = 0, eyeZ = 0;
    double mx = 0, my = 0, mz = 0;
    float yaw = 0, pitch = 0;
    bool onGround = false;
    bool sprinting = false;

    float health = 0, maxHealth = 0;
    int food = 0;

    double camX = 0, camY = 0, camZ = 0;
    float camYaw = 0, camPitch = 0;
    double fov = 70;
    double gamma = 1.0;

    int screenW = 0, screenH = 0;
    int tick = 0;

    std::string dimension;

    double horizontalSpeed() const;
};

// The single boundary between native code and Minecraft.
//
// Resolution goes through the thread context classloaders because the game
// classes (and this bridge) live in Fabric's Knot loader, which is not the
// system loader an attached native thread would otherwise search.
class Bridge {
public:
    static bool resolve();

    static bool available();

    static GameState state();

    // Entity rows, refetched only when the bridge's snapshot version changes.
    static const std::vector<EntitySnapshot>& entities();

    // ---- commands ---------------------------------------------------
    static void setSprinting(bool value);
    static void setGamma(double value);
    static void setStepHeight(double value);
    static void setVelocity(double x, double y, double z);
    static void setRotation(float yaw, float pitch);
    static void setFly(bool value, double speed);
    static void setFallDistance(double value);
    static void setItemUseCooldown(int value);
    static void clickAttack();
    static void clickUse();
    static void sendChat(const std::string& message);

private:
    static bool resolveLocked();
};

} // namespace lumin
