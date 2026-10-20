# Lumin

A single-player utility client for **Minecraft Java 1.21.11 (Fabric)**. The client is native C++
with an ImGui menu, injected with a separate loader executable; everything that touches the game
goes through JNI into a small companion mod.

Pinned to 1.21.11 on purpose. Nothing here is written to survive the next Minecraft update.

```
lumin-loader.exe          finds javaw.exe, injects lumin.dll
      │
lumin.dll                  ImGui overlay on wglSwapBuffers, modules, config, theme
      │  JNI (snapshots in, commands out)
lumin-bridge-1.0.0.jar     Fabric mod: 20 Hz snapshot + command queue
      │
Minecraft 1.21.11            remapped to Yarn/intermediary at runtime
```

## Why there is a mod at all

Fabric remaps Minecraft at runtime (`net.minecraft.class_310` and friends), and JNI has no idea
what those names are for a given build. Keeping every game-side call inside one small Java class
means:

* no hardcoded intermediary descriptors anywhere in C++;
* no reflection against obfuscated fields;
* the mod can be rebuilt with different mappings and the DLL keeps working as long as the bridge
  API (the method names/signatures in `Bridge.java`) stays the same.

## Threading model

The overlay renders on the thread that calls `wglSwapBuffers`. It must never touch a game object
from there, so the bridge enforces two rules:

1. **Reads** are `volatile` fields refreshed once per client tick in `Bridge.snapshot()`.
   `Bridge::state()` and `Bridge::entities()` in C++ just copy those values.
2. **Writes** push a `Runnable` onto a queue that is drained on the client thread at the start of
   the next tick. `Bridge::setVelocity()` and friends therefore never race the world.

Module code only sees `runtime().game` (a plain struct), which is refreshed once per rendered
frame. That is why adding a module is a single file and never needs new plumbing.

## Requirements

* Windows 10/11 x64, Java 21 runtime (the game's own)
* Minecraft Java 1.21.11 + Fabric Loader 0.18.x
* To build: CMake 3.20+, a 64-bit C++ toolchain (MSVC 2022 or MinGW-w64), JDK 21 headers

## Build

**Bridge mod** (produces `bridge/build/libs/lumin-bridge-1.0.0.jar`):

```bash
cd bridge
JAVA_HOME=/path/to/jdk-21 ./gradlew build
```

**Native client + loader** on Windows:

```bat
set JAVA_HOME=C:\Program Files\Java\jdk-21
build.bat
```

Cross-compiling from Linux:

```bash
MINGW_ROOT=/opt/mingw64 JAVA_HOME=/opt/jdk-21 ./build.sh
```

Outputs:

```
build/client/lumin.dll            (or build-win/client/lumin.dll)
build/injector/lumin-loader.exe   (the DLL is copied next to it)
```

## Install

1. Drop `lumin-bridge-1.0.0.jar` into `.minecraft/mods`.
2. Start Minecraft, get to the title screen or into a world.
3. Run `lumin-loader.exe` (add `--wait 60` if you launch it before the game).
4. Press **INSERT** for the menu, **ESC** to close it.

`lumin-loader.exe --list` shows every running Java process with its window title in case the
automatic detection picks the wrong one; `--pid <n>` targets one explicitly. The loader needs the
same integrity level as the game: if Minecraft was started as administrator, run the loader as
administrator too.

Logs and config live next to the DLL and in `%APPDATA%\Lumin`:

```
lumin.dll
lumin.log            <- startup diagnostics
theme.json             <- optional, see below
%APPDATA%\Lumin\config.json
```

## Modules

| Category | Module | What it does |
| --- | --- | --- |
| Combat | AutoClicker | Attacks on a fixed cadence, optionally only with a target in range |
| Combat | KillAura | Snaps to the closest entity in range and swings |
| Movement | Sprint | Keeps sprint applied while moving |
| Movement | Step | Step-height attribute override |
| Movement | NoFall | Zeroes fall distance while airborne |
| Movement | Fly | Creative-style flight with a speed multiplier |
| Movement | Speed | Horizontal velocity multiplier on the ground |
| Render | ESP | Boxes, names and health bars built from the 3D bounding box |
| Render | Tracers | Lines from the crosshair to nearby entities |
| Render | Fullbright | Gamma written past the slider's clamp |
| Render | Crosshair | Custom crosshair with size/gap/thickness |
| World | FastPlace | Removes the item use cooldown |
| World | FastUse | Same, only while right click is held |
| Client | Appearance | Theme preset, UI scale, config save |

Every setting is stored by name in `config.json`, so adding a setting never breaks an existing
config.

## Themes

All visual choices live in `lumin::Theme` (`client/src/render/theme.hpp`). Two ways to bring a
theme over:

* **Data only** – drop a `theme.json` next to the DLL (start from `client/theme.example.json`).
  Colours accept `#rrggbb`, `#rrggbbaa` or `[r, g, b, a]` floats. Unknown keys are ignored, so a
  theme file from another client can be pasted in and pruned.
* **Code** – add an entry to `theme::presets()` in `client/src/render/theme.cpp` and it shows up
  in the Appearance module.

Per-category accents (the stripe on each module card and the ESP colours) are the only colours not
covered by the JSON yet: `Theme::categoryColor()`.

## Limits worth knowing

* Snapshot data is position/health/velocity only. There is no block, inventory or raycast API in
  the bridge yet, so there is nothing like Scaffold, AutoTool or block ESP.
* KillAura rotates and swings; it does not check line of sight.
* The overlay hooks `wglSwapBuffers`, so it only appears with the OpenGL renderer.
* Cursor release and grab go through `glfw.dll` (Minecraft's bundled GLFW). If that fails the menu
  still works, but the cursor stays grabbed.
* Fullbright and FastPlace write private fields (`SimpleOption.value`,
  `MinecraftClient.itemUseCooldown`) through two `@Accessor` mixins, not reflection: the accessor
  names are remapped by Loom at build time, so no mapping name is hardcoded anywhere.
