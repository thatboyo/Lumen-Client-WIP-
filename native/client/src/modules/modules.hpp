#pragma once

namespace lumin::modules {

// Registers every built-in module with the ModuleManager. Called once during
// bootstrap, before the overlay starts rendering.
void registerAll();

void registerCombat();
void registerMovement();
void registerRender();
void registerWorld();
void registerClient();

// Applies the theme saved in the config once, right after loadConfig(), so a
// preset choice survives a restart without opening the Appearance panel.
void applySavedTheme();

} // namespace lumin::modules
