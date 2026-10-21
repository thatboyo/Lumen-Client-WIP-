#include "ui.hpp"

#include "api/module_manager.hpp"
#include "render/logo.hpp"
#include "render/theme.hpp"

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

namespace lumin::ui {
namespace {

Category g_category = Category::Combat;
std::string g_expanded;
char g_search[64] = {0};
float g_smoothedFps = 0.0f;

// The window has no title bar, so dragging is handled by hand: the sidebar
// header reports its drag delta here and the next frame re-applies the position.
ImVec2 g_menuPos{0.0f, 0.0f};
bool g_menuPosKnown = false;
ImVec2& menuPosition() {
    return g_menuPos;
}
bool& menuPositionKnown() {
    return g_menuPosKnown;
}

ImU32 color(const ImVec4& c) {
    return ImGui::ColorConvertFloat4ToU32(c);
}

ImU32 withAlpha(const ImVec4& c, float alpha) {
    ImVec4 copy = c;
    copy.w = alpha;
    return ImGui::ColorConvertFloat4ToU32(copy);
}

bool containsIgnoreCase(const std::string& haystack, const char* needle) {
    if (!needle || !*needle) return true;

    std::string hay = haystack;
    std::string nee = needle;
    std::transform(hay.begin(), hay.end(), hay.begin(), ::tolower);
    std::transform(nee.begin(), nee.end(), nee.begin(), ::tolower);
    return hay.find(nee) != std::string::npos;
}

// Draws text one glyph at a time so it can carry the wide tracking the wordmark
// uses in the design sheet. AddText() has no letter-spacing option.
void drawTrackedText(ImDrawList* draw, const ImVec2& pos, const char* text, ImU32 color,
                     float size, float tracking) {
    ImFont* font = ImGui::GetFont();
    if (!font) return;

    ImVec2 cursor = pos;
    for (const char* c = text; *c; ++c) {
        char glyph[2] = {*c, '\0'};
        draw->AddText(font, size, cursor, color, glyph);
        cursor.x += font->CalcTextSizeA(size, 4096.0f, 0.0f, glyph).x + tracking;
    }
}

// Rounded switch. Returns true on the frame it was clicked.
bool toggleSwitch(const char* id, bool value, ImVec2 size) {
    const Theme& t = theme::current();
    ImGui::PushID(id);

    ImVec2 pos = ImGui::GetCursorScreenPos();
    bool pressed = ImGui::InvisibleButton("##switch", size);
    ImDrawList* draw = ImGui::GetWindowDrawList();

    const float radius = size.y * 0.5f;
    const ImU32 track = value ? color(t.accentDim) : color(t.panelActive);
    const ImU32 border = value ? color(t.accent) : color(t.border);

    draw->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + size.x, pos.y + size.y), track,
                        radius);
    draw->AddRect(ImVec2(pos.x, pos.y), ImVec2(pos.x + size.x, pos.y + size.y), border, radius, 0,
                  theme::scaled(1.0f));

    const float knobRadius = radius - theme::scaled(3.0f);
    const float knobX = value ? (pos.x + size.x - radius) : (pos.x + radius);
    draw->AddCircleFilled(ImVec2(knobX, pos.y + radius), knobRadius,
                          value ? color(t.accentBright) : color(t.textDim), 24);

    ImGui::PopID();
    return pressed;
}

void drawSidebar(const ImVec2& size) {
    const Theme& t = theme::current();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    ImGui::BeginChild("##sidebar", size, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
    ImVec2 origin = ImGui::GetWindowPos();
    draw->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y), color(t.sidebar),
                        t.windowRounding, ImDrawFlags_RoundCornersLeft);

    // Header: ghost mark on the left, stacked LUMIN / CLIENT wordmark beside it.
    const float markHeight = theme::scaled(34);
    const ImVec2 markPos(origin.x + theme::scaled(18), origin.y + theme::scaled(16));
    logo::draw(draw, markPos, markHeight, color(t.text));

    const float textX = markPos.x + logo::widthForHeight(markHeight) + theme::scaled(11);
    drawTrackedText(draw, ImVec2(textX, markPos.y + theme::scaled(1)), "LUMIN", color(t.text),
                    theme::scaled(17), theme::scaled(4.5f));
    drawTrackedText(draw, ImVec2(textX + theme::scaled(1), markPos.y + theme::scaled(23)),
                    "CLIENT", withAlpha(t.textDim, 0.95f), theme::scaled(11),
                    theme::scaled(3.4f));

    ImGui::SetCursorPos(ImVec2(theme::scaled(18), markPos.y - origin.y + markHeight + theme::scaled(4)));
    ImGui::PushStyleColor(ImGuiCol_Text, t.textDim);
    ImGui::TextUnformatted("1.21.11 / fabric");
    ImGui::PopStyleColor();

    // Drag handle over the header: active while the mouse is held down on it.
    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::InvisibleButton("##drag", ImVec2(size.x, theme::scaled(78)));
    if (ImGui::IsItemActive()) {
        const ImVec2 delta = ImGui::GetIO().MouseDelta;
        menuPosition().x += delta.x;
        menuPosition().y += delta.y;
        menuPositionKnown() = true;
    }

    ImGui::SetCursorPos(ImVec2(theme::scaled(12), theme::scaled(80)));

    ModuleManager& manager = ModuleManager::instance();
    const Category categories[] = {Category::Combat, Category::Movement, Category::Render,
                                   Category::World, Category::Client};

    for (Category category : categories) {
        int count = static_cast<int>(manager.byCategory(category).size());
        if (count == 0) continue;

        const bool active = (g_category == category);
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 itemSize(size.x - theme::scaled(24), theme::scaled(36));

        ImGui::PushID(static_cast<int>(category));
        if (ImGui::InvisibleButton("##tab", itemSize)) {
            g_category = category;
        }

        if (active) {
            draw->AddRectFilled(pos, ImVec2(pos.x + itemSize.x, pos.y + itemSize.y),
                                color(t.panel), theme::scaled(8));
            draw->AddRectFilled(pos, ImVec2(pos.x + theme::scaled(3), pos.y + itemSize.y),
                                color(t.accent), theme::scaled(2));
        } else if (ImGui::IsItemHovered()) {
            draw->AddRectFilled(pos, ImVec2(pos.x + itemSize.x, pos.y + itemSize.y),
                                withAlpha(t.panel, 0.5f), theme::scaled(8));
        }

        const ImVec2 label(pos.x + theme::scaled(16), pos.y + theme::scaled(10));
        draw->AddText(label, active ? color(t.text) : color(t.textDim),
                      categoryName(category));

        char badge[8];
        snprintf(badge, sizeof(badge), "%d", count);
        const ImVec2 badgeSize = ImGui::CalcTextSize(badge);
        draw->AddText(ImVec2(pos.x + itemSize.x - badgeSize.x - theme::scaled(12),
                             pos.y + theme::scaled(10)),
                      withAlpha(t.textDim, 0.7f), badge);

        ImGui::PopID();
        ImGui::SetCursorPosX(theme::scaled(12));
    }

    // Footer
    ImGui::SetCursorPos(ImVec2(theme::scaled(18), size.y - theme::scaled(46)));
    ImGui::PushStyleColor(ImGuiCol_Text, t.textDim);
    ImGui::Text("%d active", manager.enabledCount());
    ImGui::SetCursorPos(ImVec2(theme::scaled(18), size.y - theme::scaled(28)));
    ImGui::TextUnformatted("INSERT closes");
    ImGui::PopStyleColor();

    ImGui::EndChild();
}

void drawModuleCard(Module* module, float width) {
    const Theme& t = theme::current();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    const bool expanded = (g_expanded == module->name());
    const float headerHeight = theme::scaled(52);
    const float settingsHeight = expanded ? theme::scaled(150) : 0.0f;
    const float cardHeight = headerHeight + settingsHeight;

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 end(pos.x + width, pos.y + cardHeight);

    draw->AddRectFilled(pos, end, color(t.panel), theme::scaled(9));
    draw->AddRect(pos, end, module->enabled() ? withAlpha(t.accent, 0.55f) : color(t.border),
                  theme::scaled(9), 0, theme::scaled(1.0f));

    // Category stripe
    draw->AddRectFilled(pos, ImVec2(pos.x + theme::scaled(3), pos.y + cardHeight),
                        color(t.categoryColor(static_cast<int>(module->category()))),
                        theme::scaled(2), ImDrawFlags_RoundCornersLeft);

    draw->AddText(ImVec2(pos.x + theme::scaled(16), pos.y + theme::scaled(10)), color(t.text),
                  module->name().c_str());
    draw->AddText(ImVec2(pos.x + theme::scaled(16), pos.y + theme::scaled(29)),
                  withAlpha(t.textDim, 0.85f), module->description().c_str());

    ImGui::PushID(module->name().c_str());

    ImGui::SetCursorScreenPos(pos);
    if (ImGui::InvisibleButton("##expand", ImVec2(width - theme::scaled(72), headerHeight))) {
        g_expanded = expanded ? std::string{} : module->name();
    }
    if (ImGui::IsItemHovered()) {
        draw->AddRect(pos, ImVec2(end.x, pos.y + headerHeight), withAlpha(t.accent, 0.25f),
                      theme::scaled(9), 0, theme::scaled(1.0f));
    }

    ImGui::SetCursorScreenPos(
        ImVec2(pos.x + width - theme::scaled(56), pos.y + theme::scaled(15)));
    if (toggleSwitch("##toggle", module->enabled(), ImVec2(theme::scaled(44), theme::scaled(22)))) {
        module->toggle();
        ModuleManager::instance().saveConfig();
    }

    if (expanded) {
        ImGui::SetCursorScreenPos(
            ImVec2(pos.x + theme::scaled(16), pos.y + headerHeight - theme::scaled(4)));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
        ImGui::BeginChild("##settings", ImVec2(width - theme::scaled(32), settingsHeight),
                          ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);
        ImGui::Dummy(ImVec2(0, theme::scaled(4)));
        module->drawSettings();
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    ImGui::PopID();

    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + cardHeight + theme::scaled(8)));
}

} // namespace

void drawMenu() {
    const Theme& t = theme::current();
    ImGuiIO& io = ImGui::GetIO();
    ModuleManager& manager = ModuleManager::instance();

    const float menuWidth = theme::scaled(700);
    const float menuHeight = theme::scaled(470);

    ImGui::SetNextWindowSize(ImVec2(menuWidth, menuHeight), ImGuiCond_FirstUseEver);
    if (g_menuPosKnown) {
        ImGui::SetNextWindowPos(g_menuPos, ImGuiCond_Always);
    } else {
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                                ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, t.background);

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
                                   ImGuiWindowFlags_NoScrollWithMouse;

    if (ImGui::Begin("##lumin", nullptr, flags)) {
        const ImVec2 windowSize = ImGui::GetWindowSize();

        if (!g_menuPosKnown) {
            g_menuPos = ImGui::GetWindowPos();
            g_menuPosKnown = true;
        }
        const float sidebarWidth = theme::scaled(180);

        drawSidebar(ImVec2(sidebarWidth, windowSize.y));

        ImGui::SameLine(0.0f, 0.0f);
        ImGui::BeginChild("##content", ImVec2(windowSize.x - sidebarWidth, windowSize.y),
                          ImGuiChildFlags_None);

        ImGui::Dummy(ImVec2(0, theme::scaled(16)));
        ImGui::Indent(theme::scaled(16));

        // Header row: category title + search
        ImGui::PushStyleColor(ImGuiCol_Text, t.text);
        ImGui::SetWindowFontScale(1.05f);
        ImGui::TextUnformatted(categoryName(g_category));
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();

        ImGui::SameLine(windowSize.x - sidebarWidth - theme::scaled(220));
        ImGui::SetNextItemWidth(theme::scaled(200));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, t.panel);
        ImGui::InputTextWithHint("##search", "search...", g_search, IM_ARRAYSIZE(g_search));
        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(0, theme::scaled(6)));

        ImGui::BeginChild("##modules", ImVec2(windowSize.x - sidebarWidth - theme::scaled(32),
                                              windowSize.y - theme::scaled(96)),
                          ImGuiChildFlags_None);

        const float cardWidth = ImGui::GetContentRegionAvail().x;
        for (Module* module : manager.byCategory(g_category)) {
            if (!containsIgnoreCase(module->name(), g_search)) continue;
            drawModuleCard(module, cardWidth);
        }

        ImGui::EndChild();

        ImGui::Unindent(theme::scaled(16));
        ImGui::EndChild();
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void drawHud(const FrameContext& context, float frameDelta) {
    const Theme& t = theme::current();
    const GameState& game = *context.state;

    if (frameDelta > 0.0001f) {
        const float fps = 1.0f / frameDelta;
        g_smoothedFps = g_smoothedFps <= 0.0f ? fps : (g_smoothedFps * 0.9f + fps * 0.1f);
    }

    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    const float pad = theme::scaled(10);
    const float lineHeight = theme::scaled(16);

    // Watermark plate: mark + wordmark, matching the HUD in the design sheet.
    const ImVec2 markPos(pad, pad);
    const float markHeight = theme::scaled(18);
    const float markWidth = logo::widthForHeight(markHeight);
    const char* title = "LUMIN";
    const ImVec2 titleSize = ImGui::CalcTextSize(title);
    const float plateHeight = std::max(markHeight, titleSize.y) + theme::scaled(10);
    const ImVec2 markEnd(markPos.x + theme::scaled(10) + markWidth + theme::scaled(8) +
                             titleSize.x + theme::scaled(12),
                         markPos.y + plateHeight);

    draw->AddRectFilled(markPos, markEnd, withAlpha(t.background, 0.78f), theme::scaled(6));
    draw->AddRectFilled(markPos, ImVec2(markPos.x + theme::scaled(3), markEnd.y), color(t.accent),
                        theme::scaled(2));
    logo::draw(draw, ImVec2(markPos.x + theme::scaled(10), markPos.y + (plateHeight - markHeight) * 0.5f),
               markHeight, color(t.text));
    draw->AddText(ImVec2(markPos.x + theme::scaled(10) + markWidth + theme::scaled(8),
                         markPos.y + (plateHeight - titleSize.y) * 0.5f),
                  color(t.text), title);

    // Enabled module list, top right
    float listY = pad;
    for (const auto& module : ModuleManager::instance().all()) {
        if (!module->enabled()) continue;
        const char* name = module->name().c_str();
        const ImVec2 size = ImGui::CalcTextSize(name);
        const ImVec2 pos(context.viewport.x - size.x - theme::scaled(16), listY);
        draw->AddText(pos, color(t.categoryColor(static_cast<int>(module->category()))), name);
        listY += lineHeight;
    }

    // Bottom left info
    float infoY = context.viewport.y - pad - lineHeight * 4.0f;
    char line[192];

    snprintf(line, sizeof(line), "xyz  %.1f  %.1f  %.1f", game.px, game.py, game.pz);
    draw->AddText(ImVec2(pad, infoY), withAlpha(t.text, 0.9f), line);
    infoY += lineHeight;

    snprintf(line, sizeof(line), "hp   %.1f / %.1f    food %d", game.health, game.maxHealth,
             game.food);
    draw->AddText(ImVec2(pad, infoY), withAlpha(t.text, 0.9f), line);
    infoY += lineHeight;

    snprintf(line, sizeof(line), "dim  %s", game.dimension.c_str());
    draw->AddText(ImVec2(pad, infoY), withAlpha(t.textDim, 1.0f), line);
    infoY += lineHeight;

    snprintf(line, sizeof(line), "fps  %.0f    tick %d", g_smoothedFps, game.tick);
    draw->AddText(ImVec2(pad, infoY), withAlpha(t.textDim, 1.0f), line);
}

} // namespace lumin::ui
