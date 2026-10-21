#pragma once

#include <imgui.h>

namespace lumin::logo {

// The ghost mark, compiled into the DLL by tools/embed_logo.py.
//
// The texture is uploaded on first use, so it has to be requested from the
// render thread while a GL context is current (that is, from inside the swap
// hook). Every call after the first is a cheap lookup.

ImTextureID texture();

bool ready();

// Width the mark occupies at a given height: it keeps the artwork's aspect.
float widthForHeight(float height);

// Draws the mark with the top-left corner at `pos`, tinted by `tint`. No-op
// while the texture is still unavailable.
void draw(ImDrawList* draw, const ImVec2& pos, float height, ImU32 tint);

} // namespace lumin::logo
