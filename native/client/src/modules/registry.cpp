#include "modules/modules.hpp"

namespace lumin::modules {

void registerAll() {
    registerCombat();
    registerMovement();
    registerRender();
    registerWorld();
    registerClient();
}

} // namespace lumin::modules
