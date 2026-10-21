#include "modules/modules.hpp"

#include "api/module_manager.hpp"
#include "core/bridge.hpp"

#include <windows.h>

#include <algorithm>

namespace lumin::modules {
namespace {

bool rightMouseDown() {
    return (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
}

class FastPlace : public Module {
public:
    FastPlace() : Module("FastPlace", "Removes the block placement delay", Category::World) {
        addInt("Delay", 0, 0, 4, "Ticks between placements (0 = every tick)");
    }

    void onTick() override {
        if (!runtime().game.inWorld) return;

        Bridge::setItemUseCooldown(std::max(0, intSetting("Delay")));
    }
};

class FastUse : public Module {
public:
    FastUse()
        : Module("FastUse", "Removes the cooldown while you hold right click", Category::World) {
        addInt("Delay", 0, 0, 4, "Ticks between uses (0 = every tick)");
    }

    void onTick() override {
        if (!runtime().game.inWorld) return;
        if (!rightMouseDown()) return;

        Bridge::setItemUseCooldown(std::max(0, intSetting("Delay")));
    }
};

} // namespace

void registerWorld() {
    ModuleManager& manager = ModuleManager::instance();
    manager.add<FastPlace>();
    manager.add<FastUse>();
}

} // namespace lumin::modules
