#include "swap.hpp"

#include "api/module_manager.hpp"
#include "core/bridge.hpp"
#include "core/jvm.hpp"
#include "core/log.hpp"
#include "hook/input.hpp"
#include "render/theme.hpp"
#include "render/ui.hpp"

#include <MinHook.h>
#include <windows.h>

#include <GL/gl.h>

#include "imgui.h"
#include "backends/imgui_impl_opengl3.h"

#include <string>

namespace lumin::hook {
namespace {

// Structured exception handling is an MSVC extension. GCC builds (MinGW) still
// get a guard, just one that only catches C++ exceptions.
#if defined(_MSC_VER)
#define LUMIN_GUARDED_TRY __try
#define LUMIN_GUARDED_CATCH __except (EXCEPTION_EXECUTE_HANDLER)
#else
#define LUMIN_GUARDED_TRY try
#define LUMIN_GUARDED_CATCH catch (...)
#endif

using WglSwapBuffersFn = BOOL(WINAPI*)(HDC);

WglSwapBuffersFn g_originalSwap = nullptr;
void* g_target = nullptr;
bool g_imguiReady = false;
bool g_fontLoaded = false;
double g_lastFrameTime = 0.0;

void loadFonts() {
    ImGuiIO& io = ImGui::GetIO();

    const char* candidates[] = {
        "C:\\Windows\\Fonts\\seguisb.ttf", // Segoe UI Semibold
        "C:\\Windows\\Fonts\\segoeui.ttf",
        "C:\\Windows\\Fonts\\tahoma.ttf",
    };

    for (const char* path : candidates) {
        if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) continue;
        // Base size only: theme::scale() is applied as a global factor by applyImguiStyle().
        ImFont* font = io.Fonts->AddFontFromFileTTF(path, 17.0f);
        if (font) {
            io.FontDefault = font;
            g_fontLoaded = true;
            break;
        }
    }

    if (!g_fontLoaded) {
        io.Fonts->AddFontDefault();
        VLOG_WARN("no system font found, using ImGui default");
    }
}

void initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // we own the config file

    loadFonts();
    theme::applyImguiStyle();

    ImGui_ImplOpenGL3_Init("#version 330 core");
    g_imguiReady = true;

    VLOG_INFO("ImGui overlay initialised (GL 3.3 core)");
}

void drawFrame(HDC hdc) {
    RuntimeState& rt = runtime();
    ModuleManager& manager = ModuleManager::instance();

    // Snapshot the game once per frame; modules never call JNI themselves.
    rt.game = Bridge::state();
    rt.tick = rt.game.tick;

    HWND window = WindowFromDC(hdc);

    int width = rt.game.screenW;
    int height = rt.game.screenH;
    if (width <= 1 || height <= 1) {
        RECT client{};
        if (window && GetClientRect(window, &client)) {
            width = client.right - client.left;
            height = client.bottom - client.top;
        }
    }
    if (width <= 1 || height <= 1) return;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));

    const LARGE_INTEGER now = [] {
        LARGE_INTEGER t;
        QueryPerformanceCounter(&t);
        return t;
    }();
    static LARGE_INTEGER frequency = [] {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        return f;
    }();
    static LARGE_INTEGER previous = now;
    const float delta = static_cast<float>(now.QuadPart - previous.QuadPart) /
                        static_cast<float>(frequency.QuadPart);
    previous = now;

    io.DeltaTime = delta > 0.0f ? delta : 1.0f / 60.0f;

    const bool insertPressed = input::menuKeyPressed();
    const bool escapePressed = input::closeKeyPressed();
    if (insertPressed || (escapePressed && manager.menuOpen())) {
        const bool open = !manager.menuOpen();
        manager.setMenuOpen(open);
        input::setCursorReleased(open);
        if (!open) manager.saveConfig();
    }

    input::newFrame(window, manager.menuOpen());

    rt.menuOpen = manager.menuOpen();
    rt.frame.state = &rt.game;
    rt.frame.viewport = ImVec2(static_cast<float>(width), static_cast<float>(height));
    rt.frame.deltaTime = io.DeltaTime;

    manager.tick();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    // Only valid once a frame is open.
    rt.frame.drawList = ImGui::GetBackgroundDrawList();

    // Overlays first so the menu draws on top of them.
    manager.frame(rt.frame);

    if (rt.game.inWorld) {
        ui::drawHud(rt.frame, delta);
    }
    if (manager.menuOpen()) {
        ui::drawMenu();
    }

    ImGui::Render();

    GLint previousViewport[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, previousViewport);
    glViewport(0, 0, width, height);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
}

BOOL WINAPI hookedSwapBuffers(HDC hdc) {
    LUMIN_GUARDED_TRY {
        if (!g_imguiReady) {
            initImGui();
        }
        if (g_imguiReady) {
            drawFrame(hdc);
        }
    }
    LUMIN_GUARDED_CATCH {
        // Never let an overlay fault take the game down: disable and pass through.
        VLOG_ERR("overlay fault, disabling ImGui for this session");
        g_imguiReady = false;
    }

    return g_originalSwap(hdc);
}

} // namespace

bool install() {
    if (g_originalSwap) return true;

    HMODULE opengl = GetModuleHandleA("opengl32.dll");
    if (!opengl) return false;

    void* target = reinterpret_cast<void*>(GetProcAddress(opengl, "wglSwapBuffers"));
    if (!target) return false;

    if (MH_Initialize() != MH_OK && MH_Initialize() != MH_ERROR_ALREADY_INITIALIZED) {
        VLOG_ERR("MinHook init failed");
        return false;
    }

    if (MH_CreateHook(target, reinterpret_cast<LPVOID>(&hookedSwapBuffers),
                      reinterpret_cast<LPVOID*>(&g_originalSwap)) != MH_OK) {
        VLOG_ERR("MH_CreateHook(wglSwapBuffers) failed");
        return false;
    }

    if (MH_EnableHook(target) != MH_OK) {
        VLOG_ERR("MH_EnableHook(wglSwapBuffers) failed");
        return false;
    }

    g_target = target;
    VLOG_INFO("wglSwapBuffers hooked at %p", target);
    return true;
}

void uninstall() {
    if (!g_originalSwap) return;

    MH_DisableHook(MH_ALL_HOOKS);
    if (g_target) MH_RemoveHook(g_target);
    MH_Uninitialize();

    // ImGui is intentionally not torn down: uninstall() only runs from
    // DLL_PROCESS_DETACH, where the GL context may already be gone and any GL
    // call here could take the game down on quit. The process is exiting, so the
    // leak does not matter.
    g_imguiReady = false;
    g_originalSwap = nullptr;
    g_target = nullptr;
}

} // namespace lumin::hook
