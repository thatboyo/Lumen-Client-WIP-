#include "input.hpp"

#include "core/log.hpp"

#include <imgui.h>

#include <atomic>
#include <cstring>
#include <thread>

namespace lumin::input {
namespace {

using GlfwGetCurrentContextFn = void* (*)();
using GlfwSetInputModeFn = void (*)(void*, int, int);

constexpr int GLFW_CURSOR = 0x00033001;
constexpr int GLFW_CURSOR_NORMAL = 0x00034001;
constexpr int GLFW_CURSOR_DISABLED = 0x00034003;

struct Glfw {
    GlfwGetCurrentContextFn getContext = nullptr;
    GlfwSetInputModeFn setInputMode = nullptr;
    bool resolved = false;
};

Glfw& glfw() {
    static Glfw api;
    if (!api.resolved) {
        api.resolved = true;
        HMODULE module = GetModuleHandleA("glfw.dll");
        if (!module) module = GetModuleHandleA("glfw3.dll");
        if (module) {
            api.getContext = reinterpret_cast<GlfwGetCurrentContextFn>(
                GetProcAddress(module, "glfwGetCurrentContext"));
            api.setInputMode = reinterpret_cast<GlfwSetInputModeFn>(
                GetProcAddress(module, "glfwSetInputMode"));
        }
        if (!api.setInputMode) {
            VLOG_WARN("glfw.dll not found; cursor handoff disabled");
        }
    }
    return api;
}

// ---------------------------------------------------------------------
// Wheel: the game owns the window procedure, so the overlay cannot receive
// WM_MOUSEWHEEL. A low-level hook on its own message-pumping thread sees every
// wheel event and passes it straight through.
// ---------------------------------------------------------------------
std::atomic<int> g_wheelDelta{0};
std::atomic<bool> g_wheelThreadStarted{false};

LRESULT CALLBACK mouseProc(int code, WPARAM wparam, LPARAM lparam) {
    if (code == HC_ACTION && wparam == WM_MOUSEWHEEL) {
        const auto* info = reinterpret_cast<MSLLHOOKSTRUCT*>(lparam);
        g_wheelDelta.fetch_add(GET_WHEEL_DELTA_WPARAM(info->mouseData),
                               std::memory_order_relaxed);
    }
    return CallNextHookEx(nullptr, code, wparam, lparam);
}

void wheelThread() {
    HHOOK hook = SetWindowsHookExW(WH_MOUSE_LL, mouseProc, GetModuleHandleW(nullptr), 0);
    if (!hook) {
        VLOG_WARN("low-level mouse hook failed (%lu); mouse wheel scrolling is disabled",
                  GetLastError());
        return;
    }

    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    UnhookWindowsHookEx(hook);
}

void ensureWheelThread() {
    if (g_wheelThreadStarted.exchange(true)) return;
    std::thread(wheelThread).detach();
}

struct KeyMap {
    int vk;
    ImGuiKey key;
};

const KeyMap kKeyMap[] = {
    {VK_TAB, ImGuiKey_Tab},
    {VK_LEFT, ImGuiKey_LeftArrow},
    {VK_RIGHT, ImGuiKey_RightArrow},
    {VK_UP, ImGuiKey_UpArrow},
    {VK_DOWN, ImGuiKey_DownArrow},
    {VK_PRIOR, ImGuiKey_PageUp},
    {VK_NEXT, ImGuiKey_PageDown},
    {VK_HOME, ImGuiKey_Home},
    {VK_END, ImGuiKey_End},
    {VK_INSERT, ImGuiKey_Insert},
    {VK_DELETE, ImGuiKey_Delete},
    {VK_BACK, ImGuiKey_Backspace},
    {VK_SPACE, ImGuiKey_Space},
    {VK_RETURN, ImGuiKey_Enter},
    {VK_ESCAPE, ImGuiKey_Escape},
    {VK_LSHIFT, ImGuiKey_LeftShift},
    {VK_RSHIFT, ImGuiKey_RightShift},
    {VK_LCONTROL, ImGuiKey_LeftCtrl},
    {VK_RCONTROL, ImGuiKey_RightCtrl},
    {VK_LMENU, ImGuiKey_LeftAlt},
    {VK_RMENU, ImGuiKey_RightAlt},
    {VK_LWIN, ImGuiKey_LeftSuper},
    {VK_OEM_1, ImGuiKey_Semicolon},
    {VK_OEM_PLUS, ImGuiKey_Equal},
    {VK_OEM_COMMA, ImGuiKey_Comma},
    {VK_OEM_MINUS, ImGuiKey_Minus},
    {VK_OEM_PERIOD, ImGuiKey_Period},
    {VK_OEM_2, ImGuiKey_Slash},
    {VK_OEM_3, ImGuiKey_GraveAccent},
    {VK_OEM_4, ImGuiKey_LeftBracket},
    {VK_OEM_5, ImGuiKey_Backslash},
    {VK_OEM_6, ImGuiKey_RightBracket},
    {VK_OEM_7, ImGuiKey_Apostrophe},
    {VK_F1, ImGuiKey_F1},
    {VK_F2, ImGuiKey_F2},
    {VK_F3, ImGuiKey_F3},
    {VK_F4, ImGuiKey_F4},
    {VK_F5, ImGuiKey_F5},
    {VK_F6, ImGuiKey_F6},
    {VK_F7, ImGuiKey_F7},
    {VK_F8, ImGuiKey_F8},
    {VK_F9, ImGuiKey_F9},
    {VK_F10, ImGuiKey_F10},
    {VK_F11, ImGuiKey_F11},
    {VK_F12, ImGuiKey_F12},
};

bool down(int vk) {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

void feedKeyboard() {
    ImGuiIO& io = ImGui::GetIO();

    for (const auto& mapping : kKeyMap) {
        io.AddKeyEvent(mapping.key, down(mapping.vk));
    }

    for (int c = 'A'; c <= 'Z'; ++c) {
        io.AddKeyEvent(static_cast<ImGuiKey>(ImGuiKey_A + (c - 'A')), down(c));
    }
    for (int c = '0'; c <= '9'; ++c) {
        io.AddKeyEvent(static_cast<ImGuiKey>(ImGuiKey_0 + (c - '0')), down(c));
    }

    io.AddKeyEvent(ImGuiMod_Ctrl, down(VK_CONTROL));
    io.AddKeyEvent(ImGuiMod_Shift, down(VK_SHIFT));
    io.AddKeyEvent(ImGuiMod_Alt, down(VK_MENU));
    io.AddKeyEvent(ImGuiMod_Super, down(VK_LWIN) || down(VK_RWIN));

    // Text entry: translate newly pressed keys to characters. There is no
    // WndProc to borrow, so ToUnicode does the work.
    static bool previous[256] = {false};
    BYTE keyboardState[256];
    GetKeyboardState(keyboardState);

    for (int vk = 8; vk < 256; ++vk) {
        bool now = down(vk);
        bool was = previous[vk];
        previous[vk] = now;

        if (!now || was) continue;
        if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU || vk == VK_LWIN || vk == VK_RWIN) {
            continue;
        }

        wchar_t buffer[8] = {0};
        int count = ToUnicode(static_cast<UINT>(vk), 0, keyboardState, buffer, 8, 0);
        for (int i = 0; i < count && i < 8; ++i) {
            if (buffer[i] >= 32) {
                io.AddInputCharacter(static_cast<unsigned int>(buffer[i]));
            }
        }
    }
}

void feedMouse(HWND window, bool menuOpen) {
    ImGuiIO& io = ImGui::GetIO();

    // ImGui works in framebuffer pixels while Win32 reports client pixels, so on
    // a display with a GUI or DPI scale the two need mapping.
    float scaleX = 1.0f;
    float scaleY = 1.0f;

    RECT client{};
    if (window && GetClientRect(window, &client)) {
        const float clientWidth = static_cast<float>(client.right - client.left);
        const float clientHeight = static_cast<float>(client.bottom - client.top);
        if (clientWidth > 1.0f && io.DisplaySize.x > 1.0f) {
            scaleX = io.DisplaySize.x / clientWidth;
            scaleY = io.DisplaySize.y / clientHeight;
        }
    }

    if (menuOpen) {
        POINT point;
        if (GetCursorPos(&point) && window) {
            ScreenToClient(window, &point);
            io.AddMousePosEvent(static_cast<float>(point.x) * scaleX,
                                static_cast<float>(point.y) * scaleY);
        }
    }

    io.AddMouseButtonEvent(0, down(VK_LBUTTON));
    io.AddMouseButtonEvent(1, down(VK_RBUTTON));
    io.AddMouseButtonEvent(2, down(VK_MBUTTON));
    io.AddMouseButtonEvent(3, down(VK_XBUTTON1));
    io.AddMouseButtonEvent(4, down(VK_XBUTTON2));

    const int wheel = g_wheelDelta.exchange(0, std::memory_order_relaxed);
    if (menuOpen && wheel != 0) {
        io.AddMouseWheelEvent(0.0f, static_cast<float>(wheel) / static_cast<float>(WHEEL_DELTA));
    }
}

} // namespace

void setCursorReleased(bool released) {
    Glfw& api = glfw();
    if (!api.setInputMode || !api.getContext) return;

    void* context = api.getContext();
    if (!context) return;

    static bool current = false;
    if (current == released) return;
    current = released;

    api.setInputMode(context, GLFW_CURSOR, released ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

    if (released) {
        while (ShowCursor(TRUE) < 0) {
        }
        ClipCursor(nullptr);
    } else {
        while (ShowCursor(FALSE) >= 0) {
        }
    }
}

bool menuKeyPressed() {
    static bool previous = false;
    bool now = down(VK_INSERT);
    bool pressed = now && !previous;
    previous = now;
    return pressed;
}

bool closeKeyPressed() {
    static bool previous = false;
    bool now = down(VK_ESCAPE);
    bool pressed = now && !previous;
    previous = now;
    return pressed;
}

void newFrame(HWND window, bool menuOpen) {
    ensureWheelThread();
    feedKeyboard();
    feedMouse(window, menuOpen);
}

} // namespace lumin::input
