#include "theme.hpp"

#include "core/log.hpp"

#include "json.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>

namespace lumin {
namespace {

Theme& active() {
    static Theme theme;
    return theme;
}

bool& externalTheme() {
    static bool loaded = false;
    return loaded;
}

ImVec4 hex(const std::string& value) {
    if (value.empty()) return ImVec4(1, 1, 1, 1);
    unsigned int r = 255, g = 255, b = 255, a = 255;
    std::string s = value;
    if (!s.empty() && s.front() == '#') s.erase(0, 1);
    if (s.empty()) return ImVec4(1, 1, 1, 1);

    if (s.size() == 8) {
        std::sscanf(s.c_str(), "%02x%02x%02x%02x", &r, &g, &b, &a);
    } else if (s.size() == 6) {
        std::sscanf(s.c_str(), "%02x%02x%02x", &r, &g, &b);
    }
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

void readColor(const nlohmann::json& j, const char* key, ImVec4& out) {
    if (!j.contains(key)) return;
    if (j[key].is_string()) {
        out = hex(j[key].get<std::string>());
    } else if (j[key].is_array() && j[key].size() >= 3) {
        auto arr = j[key];
        out = ImVec4(arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>(),
                     arr.size() > 3 ? arr[3].get<float>() : 1.0f);
    }
}

void readFloat(const nlohmann::json& j, const char* key, float& out) {
    if (j.contains(key) && j[key].is_number()) out = j[key].get<float>();
}

} // namespace

ImVec4 Theme::categoryColor(int category) const {
    switch (category) {
    case 0: return categoryCombat;
    case 1: return categoryMovement;
    case 2: return categoryRender;
    case 3: return categoryWorld;
    default: return categoryClient;
    }
}

namespace theme {

const Theme& current() {
    return active();
}

void set(const Theme& theme) {
    active() = theme;
    applyImguiStyle();
}

float scale() {
    return active().uiScale <= 0.1f ? 1.0f : active().uiScale;
}

float scaled(float value) {
    return value * scale();
}

void applyImguiStyle() {
    // The theme is loaded before the overlay has an ImGui context; the style is
    // re-applied by the overlay once the context exists.
    if (ImGui::GetCurrentContext() == nullptr) return;

    const Theme& t = active();
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = t.windowRounding;
    style.ChildRounding = t.windowRounding * 0.7f;
    style.FrameRounding = t.frameRounding;
    style.PopupRounding = t.frameRounding;
    style.TabRounding = t.frameRounding;
    style.ScrollbarRounding = t.frameRounding;

    // 1.92+ scales the dynamic font atlas without rebuilding it, so a UI scale
    // change takes effect immediately.
    style.FontSizeBase = 17.0f;
    style.FontScaleMain = scale();

    style.WindowPadding = ImVec2(t.windowPadding, t.windowPadding);
    style.FramePadding = ImVec2(8.0f, 5.0f);
    style.ItemSpacing = ImVec2(t.itemSpacing, t.itemSpacing);
    style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
    style.WindowBorderSize = t.borderSize;
    style.FrameBorderSize = 0.0f;
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = t.text;
    colors[ImGuiCol_TextDisabled] = t.textDim;
    colors[ImGuiCol_WindowBg] = t.background;
    colors[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_PopupBg] = t.panel;
    colors[ImGuiCol_Border] = t.border;
    colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_FrameBg] = t.panel;
    colors[ImGuiCol_FrameBgHovered] = t.panelHover;
    colors[ImGuiCol_FrameBgActive] = t.panelActive;
    colors[ImGuiCol_TitleBg] = t.background;
    colors[ImGuiCol_TitleBgActive] = t.background;
    colors[ImGuiCol_TitleBgCollapsed] = t.background;
    colors[ImGuiCol_MenuBarBg] = t.sidebar;
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_ScrollbarGrab] = t.panelHover;
    colors[ImGuiCol_ScrollbarGrabHovered] = t.panelActive;
    colors[ImGuiCol_ScrollbarGrabActive] = t.accent;
    colors[ImGuiCol_CheckMark] = t.accent;
    colors[ImGuiCol_SliderGrab] = t.accent;
    colors[ImGuiCol_SliderGrabActive] = t.accentBright;
    colors[ImGuiCol_Button] = t.panel;
    colors[ImGuiCol_ButtonHovered] = t.panelHover;
    colors[ImGuiCol_ButtonActive] = t.panelActive;
    colors[ImGuiCol_Header] = t.accentDim;
    colors[ImGuiCol_HeaderHovered] = t.panelHover;
    colors[ImGuiCol_HeaderActive] = t.panelActive;
    colors[ImGuiCol_Separator] = t.border;
    colors[ImGuiCol_SeparatorHovered] = t.accent;
    colors[ImGuiCol_SeparatorActive] = t.accentBright;
    colors[ImGuiCol_ResizeGrip] = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_Tab] = t.panel;
    colors[ImGuiCol_TabHovered] = t.panelHover;
    colors[ImGuiCol_TabSelected] = t.accentDim;
    colors[ImGuiCol_TextSelectedBg] = t.accentDim;
    colors[ImGuiCol_NavHighlight] = t.accent;
}

bool loadJsonString(const std::string& json) {
    try {
        nlohmann::json j = nlohmann::json::parse(json);
        Theme t = active();

        // "preset" lets a theme.json start from one of the built-ins and only
        // override what it cares about.
        if (j.contains("preset") && j["preset"].is_string()) {
            std::string wanted = j["preset"].get<std::string>();
            std::transform(wanted.begin(), wanted.end(), wanted.begin(), ::tolower);
            for (const Theme& candidate : presets()) {
                std::string name = candidate.name;
                std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                if (name == wanted) {
                    t = candidate;
                    break;
                }
            }
        }

        if (j.contains("name") && j["name"].is_string()) t.name = j["name"].get<std::string>();

        readColor(j, "background", t.background);
        readColor(j, "sidebar", t.sidebar);
        readColor(j, "panel", t.panel);
        readColor(j, "panelHover", t.panelHover);
        readColor(j, "panelActive", t.panelActive);
        readColor(j, "accent", t.accent);
        readColor(j, "accentBright", t.accentBright);
        readColor(j, "text", t.text);
        readColor(j, "textDim", t.textDim);
        readColor(j, "border", t.border);
        readColor(j, "success", t.success);
        readColor(j, "danger", t.danger);
        readColor(j, "warning", t.warning);
        readColor(j, "categoryCombat", t.categoryCombat);
        readColor(j, "categoryMovement", t.categoryMovement);
        readColor(j, "categoryRender", t.categoryRender);
        readColor(j, "categoryWorld", t.categoryWorld);
        readColor(j, "categoryClient", t.categoryClient);

        readFloat(j, "windowRounding", t.windowRounding);
        readFloat(j, "frameRounding", t.frameRounding);
        readFloat(j, "windowPadding", t.windowPadding);
        readFloat(j, "itemSpacing", t.itemSpacing);
        readFloat(j, "borderSize", t.borderSize);
        readFloat(j, "uiScale", t.uiScale);

        t.accentDim = ImVec4(t.accent.x, t.accent.y, t.accent.z, 0.20f);

        set(t);
        VLOG_INFO("theme loaded: %s", t.name.c_str());
        return true;
    } catch (const std::exception& e) {
        VLOG_WARN("theme parse failed: %s", e.what());
        return false;
    }
}

bool loadJson(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (!loadJsonString(content)) return false;

    externalTheme() = true;
    return true;
}

bool externalLoaded() {
    return externalTheme();
}

const std::vector<Theme>& presets() {
    static std::vector<Theme> list = [] {
        std::vector<Theme> out;

        // LUMIN: clean / minimal / focused. Background #000000, panel #0B0B0D,
        // border #1A1A1E, accent #F5F7FA. The default above already is this one.
        out.push_back(Theme{});

        // RAIN: deep / stormy / atmospheric. Background #020708, panel #050E15,
        // border #0B2433, accent #4CB8D0 (bright #7ADCF2 for active states).
        Theme rain{};
        rain.name = "Rain";
        rain.background = ImVec4(0.008f, 0.027f, 0.031f, 1.0f);
        rain.sidebar = ImVec4(0.012f, 0.043f, 0.059f, 1.0f);
        rain.panel = ImVec4(0.020f, 0.055f, 0.082f, 1.0f);
        rain.panelHover = ImVec4(0.031f, 0.086f, 0.122f, 1.0f);
        rain.panelActive = ImVec4(0.043f, 0.118f, 0.165f, 1.0f);
        rain.accent = ImVec4(0.298f, 0.722f, 0.816f, 1.0f);
        rain.accentBright = ImVec4(0.478f, 0.863f, 0.949f, 1.0f);
        rain.accentDim = ImVec4(0.298f, 0.722f, 0.816f, 0.18f);
        rain.text = ImVec4(0.898f, 0.980f, 1.000f, 1.0f);
        rain.textDim = ImVec4(0.447f, 0.549f, 0.588f, 1.0f);
        rain.border = ImVec4(0.043f, 0.141f, 0.200f, 1.0f);
        rain.categoryCombat = ImVec4(0.435f, 0.827f, 0.910f, 1.0f);
        rain.categoryMovement = ImVec4(0.298f, 0.722f, 0.816f, 1.0f);
        rain.categoryRender = ImVec4(0.478f, 0.863f, 0.949f, 1.0f);
        rain.categoryWorld = ImVec4(0.180f, 0.498f, 0.588f, 1.0f);
        rain.categoryClient = ImVec4(0.447f, 0.549f, 0.588f, 1.0f);
        out.push_back(rain);

        // SYNTH: neon / dark / electric. Background #04030A, panel #0B0715,
        // border #1A1033, accent #FF4FA3 (bright #FF6BD6, purple #A46BFF).
        Theme synth{};
        synth.name = "Synth";
        synth.background = ImVec4(0.016f, 0.012f, 0.039f, 1.0f);
        synth.sidebar = ImVec4(0.024f, 0.016f, 0.055f, 1.0f);
        synth.panel = ImVec4(0.043f, 0.027f, 0.082f, 1.0f);
        synth.panelHover = ImVec4(0.071f, 0.047f, 0.133f, 1.0f);
        synth.panelActive = ImVec4(0.118f, 0.075f, 0.220f, 1.0f);
        synth.accent = ImVec4(1.000f, 0.310f, 0.639f, 1.0f);
        synth.accentBright = ImVec4(1.000f, 0.420f, 0.839f, 1.0f);
        synth.accentDim = ImVec4(1.000f, 0.310f, 0.639f, 0.20f);
        synth.text = ImVec4(1.000f, 0.910f, 0.984f, 1.0f);
        synth.textDim = ImVec4(0.565f, 0.514f, 0.616f, 1.0f);
        synth.border = ImVec4(0.102f, 0.063f, 0.200f, 1.0f);
        synth.categoryCombat = ImVec4(1.000f, 0.310f, 0.639f, 1.0f);
        synth.categoryMovement = ImVec4(0.643f, 0.420f, 1.000f, 1.0f);
        synth.categoryRender = ImVec4(1.000f, 0.420f, 0.839f, 1.0f);
        synth.categoryWorld = ImVec4(0.482f, 0.357f, 1.000f, 1.0f);
        synth.categoryClient = ImVec4(0.565f, 0.514f, 0.616f, 1.0f);
        out.push_back(synth);

        return out;
    }();
    return list;
}

} // namespace theme
} // namespace lumin
