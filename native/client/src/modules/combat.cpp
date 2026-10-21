#include "modules/modules.hpp"

#include "api/module_manager.hpp"
#include "core/bridge.hpp"
#include "modules/esp_util.hpp"

#include <algorithm>
#include <cmath>

namespace lumin::modules {
namespace {

constexpr double kPi = 3.14159265358979323846;

// Minecraft convention: forward is (-sin(yaw), *, cos(yaw)); positive pitch looks
// down. Both are degrees, matching what the bridge's setRotation expects.
void lookAt(const GameState& state, double x, double y, double z, float& yaw, float& pitch) {
    const double dx = x - state.eyeX;
    const double dy = y - state.eyeY;
    const double dz = z - state.eyeZ;
    const double horizontal = std::sqrt(dx * dx + dz * dz);

    yaw = static_cast<float>(std::atan2(-dx, dz) * 180.0 / kPi);
    pitch = static_cast<float>(-std::atan2(dy, horizontal) * 180.0 / kPi);
}

int ticksForCps(float cps) {
    if (cps <= 0.0f) return 20;
    return std::max(1, static_cast<int>(std::round(20.0f / cps)));
}

class AutoClicker : public Module {
public:
    AutoClicker() : Module("AutoClicker", "Attacks on a fixed cadence", Category::Combat) {
        addFloat("CPS", 8.0f, 1.0f, 20.0f, " cps", "Attacks per second");
        addBool("Only with target", true, "Skip the click when nothing is in front of you");
        addFloat("Range", 3.0f, 1.0f, 6.0f, " b", "How close a target must be");
    }

    void onTick() override {
        const GameState& game = runtime().game;
        if (!game.inWorld) return;

        if (boolSetting("Only with target") && nearest(game) == nullptr) return;

        if (++elapsed_ < ticksForCps(floatSetting("CPS"))) return;
        elapsed_ = 0;
        Bridge::clickAttack();
    }

    void onDisable() override { elapsed_ = 0; }

private:
    const EntitySnapshot* nearest(const GameState& game) const {
        const double range = floatSetting("Range");
        const double rangeSq = range * range;

        const EntitySnapshot* best = nullptr;
        double bestDistance = rangeSq;

        for (const EntitySnapshot& entity : Bridge::entities()) {
            if (!entity.isPlayer() && !entity.isHostile()) continue;
            const double dx = entity.x - game.px;
            const double dy = entity.y - game.py;
            const double dz = entity.z - game.pz;
            const double distance = dx * dx + dy * dy + dz * dz;
            if (distance < bestDistance) {
                bestDistance = distance;
                best = &entity;
            }
        }
        return best;
    }

    int elapsed_ = 0;
};

class KillAura : public Module {
public:
    KillAura() : Module("KillAura", "Snaps to the closest entity and swings", Category::Combat) {
        addFloat("Range", 3.0f, 1.0f, 6.0f, " b", "Attack radius");
        addFloat("CPS", 10.0f, 1.0f, 20.0f, " cps", "Swings per second");
        addBool("Players", true, "Target other players");
        addBool("Mobs", true, "Target hostile mobs");
        addBool("Rotate", true, "Look at the target while swinging");
    }

    void onTick() override {
        const GameState& game = runtime().game;
        if (!game.inWorld) return;

        const EntitySnapshot* target = nearest(game);
        if (!target) {
            elapsed_ = 0;
            return;
        }

        if (boolSetting("Rotate")) {
            float yaw = game.yaw;
            float pitch = game.pitch;
            lookAt(game, target->x, target->y + target->height * 0.9, target->z, yaw, pitch);
            Bridge::setRotation(yaw, pitch);
        }

        if (++elapsed_ < ticksForCps(floatSetting("CPS"))) return;
        elapsed_ = 0;
        Bridge::clickAttack();
    }

    void onDisable() override { elapsed_ = 0; }

private:
    const EntitySnapshot* nearest(const GameState& game) const {
        const bool players = boolSetting("Players");
        const bool mobs = boolSetting("Mobs");
        const double range = floatSetting("Range");

        const EntitySnapshot* best = nullptr;
        double bestDistance = range * range;

        for (const EntitySnapshot& entity : Bridge::entities()) {
            if (!vis::wantEntity(entity, players, mobs)) continue;

            const double dx = entity.x - game.px;
            const double dy = entity.y - game.py;
            const double dz = entity.z - game.pz;
            const double distance = dx * dx + dy * dy + dz * dz;
            if (distance < bestDistance) {
                bestDistance = distance;
                best = &entity;
            }
        }
        return best;
    }

    int elapsed_ = 0;
};

} // namespace

void registerCombat() {
    ModuleManager& manager = ModuleManager::instance();
    manager.add<AutoClicker>();
    manager.add<KillAura>();
}

} // namespace lumin::modules
