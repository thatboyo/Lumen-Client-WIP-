#include "modules/modules.hpp"

#include "api/module_manager.hpp"
#include "core/bridge.hpp"

#include <algorithm>
#include <cmath>


namespace lumin::modules {
namespace {

class Sprint : public Module {
public:
    Sprint() : Module("Sprint", "Keeps you sprinting while you move", Category::Movement) {
        addBool("Only on ground", true, "Stop forcing sprint while airborne");
    }

    void onEnable() override { Bridge::setSprinting(true); }

    void onTick() override {
        const GameState& game = runtime().game;
        if (!game.inWorld) return;
        if (boolSetting("Only on ground") && !game.onGround) return;
        if (game.horizontalSpeed() < 0.01) return;

        if (!game.sprinting) Bridge::setSprinting(true);
    }

    void onDisable() override { Bridge::setSprinting(false); }
};

class Step : public Module {
public:
    Step() : Module("Step", "Walk up full blocks without jumping", Category::Movement) {
        addFloat("Height", 1.0f, 0.6f, 2.5f, " b", "Maximum step height");
    }

    void onEnable() override { Bridge::setStepHeight(floatSetting("Height")); }

    void onTick() override {
        if (!runtime().game.inWorld) return;
        Bridge::setStepHeight(floatSetting("Height"));
    }

    void onDisable() override { Bridge::setStepHeight(0.6); }
};

class NoFall : public Module {
public:
    NoFall() : Module("NoFall", "Cancels fall damage", Category::Movement) {}

    void onTick() override {
        const GameState& game = runtime().game;
        if (!game.inWorld || game.onGround) return;

        // The bridge only exposes a setter, so the distance is zeroed every tick
        // while airborne. The game never sees more than one tick of falling.
        Bridge::setFallDistance(0.0);
    }
};

class Fly : public Module {
public:
    Fly() : Module("Fly", "Creative-style flight", Category::Movement) {
        addFloat("Speed", 1.0f, 0.05f, 4.0f, "x", "Flight speed multiplier");
        addBool("Sprint boost", true, "Fly faster while sprinting");
    }

    void onEnable() override { Bridge::setFly(true, speed()); }

    void onTick() override {
        if (!runtime().game.inWorld) return;
        Bridge::setFly(true, speed());
    }

    void onDisable() override { Bridge::setFly(false, 0.05); }

private:
    double speed() const {
        double value = floatSetting("Speed") * 0.05;
        if (boolSetting("Sprint boost") && runtime().game.sprinting) value *= 2.0;
        return value;
    }
};

class Speed : public Module {
public:
    Speed() : Module("Speed", "Horizontal velocity multiplier on the ground", Category::Movement) {
        addFloat("Multiplier", 1.35f, 1.0f, 3.0f, "x", "Applied to your current velocity");
        addFloat("Minimum speed", 0.05f, 0.0f, 0.3f, "", "Ignore movement slower than this");
    }

    void onTick() override {
        const GameState& game = runtime().game;
        if (!game.inWorld || !game.onGround) return;

        const double horizontal = game.horizontalSpeed();
        if (horizontal < floatSetting("Minimum speed")) return;

        const double multiplier = floatSetting("Multiplier");
        Bridge::setVelocity(game.mx * multiplier, game.my, game.mz * multiplier);
    }
};

} // namespace

void registerMovement() {
    ModuleManager& manager = ModuleManager::instance();
    manager.add<Sprint>();
    manager.add<Step>();
    manager.add<NoFall>();
    manager.add<Fly>();
    manager.add<Speed>();
}

} // namespace lumin::modules
