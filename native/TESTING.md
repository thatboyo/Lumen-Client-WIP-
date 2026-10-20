# Testing Lumin

Everything you need is in `dist/`. Three files, one of which goes into `.minecraft/mods`.

```
dist/
  lumin-bridge-1.0.0.jar   -> copy into .minecraft/mods/
  lumin-loader.exe         -> run this after Minecraft is open
  lumin.dll                -> stays next to the loader, never run it directly
```

## Prerequisites

* Minecraft **Java 1.21.11** with **Fabric Loader 0.18.x** (Java 21 runtime — the vanilla launcher
  ships one).
* Windows 10/11 x64.

The bridge mod has no dependency on Fabric API, so nothing else needs to be in `mods/`.

## Steps

1. Copy `lumin-bridge-1.0.0.jar` into `%APPDATA%\.minecraft\mods\`.
2. Launch the **Fabric 1.21.11** profile and wait for the title screen. (The overlay also works on
   the title screen, not just in a world.)
3. Keep `lumin-loader.exe` and `lumin.dll` together in any folder and run the exe.
   * If you prefer to start the loader first, use `lumin-loader.exe --wait 60` — it polls for the
     game for a minute.
   * If it picks the wrong process: `lumin-loader.exe --list` shows every Java process with its
     window title, then `lumin-loader.exe --pid 12345`.
4. The loader prints `[+] injected` and exits.
5. Press **INSERT** in game for the menu, **ESC** or INSERT to close it.

First launch after injection takes a moment: the DLL waits for the JVM, then for the bridge mod to
answer, then for `opengl32!wglSwapBuffers`. Nothing appears on screen until all three succeed —
that is expected, not a hang.

## Where things land

| What | Where |
| --- | --- |
| Log | `lumin.log`, next to `lumin.dll` |
| Module config | `%APPDATA%\Lumin\config.json` (written when you close the menu) |
| Theme | Appearance module (Client tab): Lumin, Rain or Synth |
| Theme override | `theme.json` next to `lumin.dll`, optional — see `client/theme.example.json` |

Delete the config (and any `theme.json`) to get defaults back. To uninstall: remove the jar from `mods/`; the overlay dies
with the game process.

## Reading the log

In order, on a healthy run:

```
lumin injected from C:\path\to\dist
theme loaded: Lumin                     (only if theme.json exists)
attached to existing JVM at 0x...
bridge found through a thread context classloader
bridge resolved and bound
ImGui overlay initialised (GL 3.3 core)
wglSwapBuffers hooked at 0x...
lumin is up
```

What the failures mean:

| Last line | Cause | Fix |
| --- | --- | --- |
| `waiting for the JVM` | Injected into the wrong process (a launcher's `javaw.exe`, not the game) | `--list`, then `--pid` the one whose window is Minecraft |
| `waiting for the lumin bridge mod` | Jar missing, or a different profile/version was launched, or a mixin failed | Check `.minecraft/logs/latest.log` for `lumin-bridge` / mixin errors |
| `waiting for opengl32!wglSwapBuffers` | The game is not using the OpenGL renderer, or is stuck before creating a window | Remove any Vulkan renderer mod, or wait until the window exists |
| `glfw.dll not found; cursor handoff disabled` | Minecraft's GLFW module name differs | Menu still opens, but the cursor may stay grabbed |
| `overlay fault, disabling ImGui for this session` | An exception inside the overlay | Send `lumin.log` — the overlay disables itself instead of crashing the game |

Loader-side messages:

| Message | Meaning |
| --- | --- |
| `no javaw.exe found` | Game not running yet; add `--wait 60` |
| `several Java processes are running` | Use `--list` + `--pid` |
| `OpenProcess failed (5)` | Permission mismatch — run the loader at the same level as the game |
| `the game rejected the DLL` | Usually antivirus quarantining/blocking an unsigned DLL; check the av log |

## What to look at first

1. Does the menu open with INSERT and draw in the theme colours?
2. Does the sidebar switch categories and does a module's switch turn the stripe accent-coloured?
3. Enable **ESP** in a world with a few mobs — boxes should hug the entities and track them as you
   move. If boxes drift, the projection needs the real FOV; report what you see.
4. Enable **Sprint** and walk — you should sprint without holding the key.
5. Close the menu, reopen it, and confirm the enabled modules and settings persisted
   (`%APPDATA%\Lumin\config.json`).

## Reporting a problem

Send `lumin.log`, the loader's console output, and `.minecraft/logs/latest.log` around the
injection. That covers the JVM attach, the bridge handshake, the hook, and any mixin failure.
