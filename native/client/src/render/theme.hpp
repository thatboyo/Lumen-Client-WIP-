#pragma once

#include <imgui.h>

#include <string>
#include <vector>

namespace lumin {

// ===========================================================================
// EVERY visual choice lives in this struct.
//
// To drop in an existing theme: either edit the defaults in theme.cpp, or ship a
// theme.json next to the DLL and it is loaded at startup (see theme::loadJson).
// No other file has hardcoded colors for chrome (module colors below are the
// per-category accents only).
// ===========================================================================
struct Theme {
    std::string name = "Lumin";

    // Values here mirror the LUMIN column of the design sheet:
    // background #000000 / panel #0B0B0D / border #1A1A1E / accent #F5F7FA.
    ImVec4 background = ImVec4(0.000f, 0.000f, 0.000f, 1.00f); // window base
    ImVec4 sidebar = ImVec4(0.012f, 0.012f, 0.016f, 1.00f);    // left rail
    ImVec4 panel = ImVec4(0.043f, 0.043f, 0.051f, 1.00f);      // cards, inputs
    ImVec4 panelHover = ImVec4(0.071f, 0.071f, 0.086f, 1.00f);
    ImVec4 panelActive = ImVec4(0.090f, 0.090f, 0.110f, 1.00f);

    ImVec4 accent = ImVec4(0.961f, 0.969f, 0.980f, 1.00f);      // primary
    ImVec4 accentBright = ImVec4(1.000f, 1.000f, 1.000f, 1.00f);
    ImVec4 accentDim = ImVec4(0.961f, 0.969f, 0.980f, 0.16f);

    ImVec4 text = ImVec4(0.961f, 0.969f, 0.980f, 1.00f);
    ImVec4 textDim = ImVec4(0.498f, 0.529f, 0.573f, 1.00f);
    ImVec4 border = ImVec4(0.102f, 0.102f, 0.118f, 1.00f);

    ImVec4 success = ImVec4(0.416f, 0.851f, 0.541f, 1.00f);
    ImVec4 danger = ImVec4(0.941f, 0.416f, 0.451f, 1.00f);
    ImVec4 warning = ImVec4(1.000f, 0.780f, 0.353f, 1.00f);

    // Per-category accents. Each preset keeps these inside its own family so
    // module cards, HUD lists and ESP read as one palette rather than confetti.
    ImVec4 categoryCombat = ImVec4(0.941f, 0.949f, 0.961f, 1.00f);
    ImVec4 categoryMovement = ImVec4(0.843f, 0.871f, 0.902f, 1.00f);
    ImVec4 categoryRender = ImVec4(0.961f, 0.969f, 0.980f, 1.00f);
    ImVec4 categoryWorld = ImVec4(0.682f, 0.718f, 0.761f, 1.00f);
    ImVec4 categoryClient = ImVec4(0.498f, 0.529f, 0.573f, 1.00f);

    float windowRounding = 10.0f;
    float frameRounding = 6.0f;
    float windowPadding = 12.0f;
    float itemSpacing = 8.0f;
    float borderSize = 1.0f;
    float uiScale = 1.0f;

    // Per-category accent for module cards and ESP coloring.
    ImVec4 categoryColor(int category) const;
};

namespace theme {

const Theme& current();
void set(const Theme& theme);
void applyImguiStyle();

// Loads named fields from a JSON object. Unknown keys are ignored, so a theme
// from another project can be pasted in with minimal cleanup.
bool loadJson(const std::string& path);
bool loadJsonString(const std::string& json);

// True once a theme.json next to the DLL has replaced the palette. The saved
// preset in config.json is skipped in that case.
bool externalLoaded();

// Built-in presets (also written to theme.json on first run as a template).
const std::vector<Theme>& presets();

// Dimensions derived from the theme, used by the menu layout.
float scale();
float scaled(float value);

} // namespace theme
} // namespace lumin
