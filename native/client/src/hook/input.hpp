#pragma once

#include <windows.h>

namespace lumin::input {

// Feeds ImGui from Win32 polling. The game owns the window and its WndProc, so we
// never subclass it; polling keeps the overlay completely passive.
void newFrame(HWND window, bool menuOpen);

// Releases/re-grabs the OS cursor via GLFW when the menu opens/closes.
void setCursorReleased(bool released);

// True once, on the frame the menu key (INSERT) transitions to pressed.
bool menuKeyPressed();

// True once, on the frame ESC transitions to pressed while the menu is open.
bool closeKeyPressed();

} // namespace lumin::input
