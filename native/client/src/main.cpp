#include "api/module_manager.hpp"
#include "core/bridge.hpp"
#include "core/jvm.hpp"
#include "core/log.hpp"
#include "hook/swap.hpp"
#include "modules/modules.hpp"
#include "render/theme.hpp"

#include <windows.h>

#include <string>
#include <thread>

namespace lumin {
namespace {

HMODULE g_module = nullptr;
volatile bool g_running = false;

std::string moduleDirectory() {
    char path[MAX_PATH] = {0};
    if (GetModuleFileNameA(g_module, path, MAX_PATH) == 0) return ".";

    std::string full(path);
    const size_t slash = full.find_last_of("\\/");
    return slash == std::string::npos ? std::string(".") : full.substr(0, slash);
}

// Runs on its own thread: the DLL is injected while the game is still starting,
// so every dependency (JVM, bridge mod, OpenGL) is awaited instead of assumed.
void bootstrap() {
    const std::string directory = moduleDirectory();
    Log::init(directory + "\\lumin.log");
    VLOG_INFO("lumin injected from %s", directory.c_str());

    // A theme.json next to the DLL wins over the saved preset; otherwise the
    // palette comes from whatever the Appearance module last stored.
    const bool externalTheme = theme::loadJson(directory + "\\theme.json");

    modules::registerAll();
    ModuleManager::instance().loadConfig();
    if (!externalTheme) modules::applySavedTheme();

    int waited = 0;
    while (!Jvm::acquire()) {
        if (!g_running) return;
        Sleep(200);
        if (++waited % 25 == 0) VLOG_INFO("waiting for the JVM (%d s)", waited / 5);
    }

    waited = 0;
    while (!Bridge::resolve()) {
        if (!g_running) return;
        Sleep(500);
        if (++waited % 10 == 0) {
            VLOG_INFO("waiting for the lumin bridge mod (is it in mods/?)");
        }
    }

    waited = 0;
    while (!hook::install()) {
        if (!g_running) return;
        Sleep(500);
        if (++waited % 10 == 0) VLOG_INFO("waiting for opengl32!wglSwapBuffers");
    }

    VLOG_INFO("lumin is up");
}

} // namespace
} // namespace lumin

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)reserved;

    if (reason == DLL_PROCESS_ATTACH) {
        lumin::g_module = instance;
        lumin::g_running = true;
        DisableThreadLibraryCalls(instance);

        std::thread(lumin::bootstrap).detach();
    } else if (reason == DLL_PROCESS_DETACH) {
        lumin::g_running = false;

        if (reserved == nullptr) {
            // Process is exiting normally: unhook so the game is not left with a
            // trampoline pointing into unmapped memory while it shuts down.
            lumin::hook::uninstall();
        }
    }

    return TRUE;
}
