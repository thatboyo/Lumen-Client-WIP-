#include "modules/modules.hpp"

#include "api/module_manager.hpp"
#include "core/bridge.hpp"
#include "modules/esp_util.hpp"
#include "render/theme.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>

namespace lumin::modules {
namespace {

class ESP : public Module {
public:
    ESP() : Module("ESP", "Boxes, names and health for nearby entities", Category::Render) {
        addBool("Players", true, "Draw other players");
        addBool("Mobs", true, "Draw living mobs");
        addBool("Boxes", true, "Draw the bounding box");
        addBool("Names", true, "Draw the entity name");
        addBool("Health", true, "Draw a health bar");
        addFloat("Thickness", 1.5f, 1.0f, 4.0f, " px", "Outline thickness");
    }

    void onFrame(const FrameContext& context) override {
        const GameState& game = *context.state;
        if (!game.inWorld) return;

        ImDrawList* draw = context.drawList ? context.drawList : ImGui::GetBackgroundDrawList();

        const bool players = boolSetting("Players");
        const bool mobs = boolSetting("Mobs");
        const bool boxes = boolSetting("Boxes");
        const bool names = boolSetting("Names");
        const bool health = boolSetting("Health");
        const float thickness = floatSetting("Thickness");

        for (const EntitySnapshot& entity : Bridge::entities()) {
            if (!vis::wantEntity(entity, players, mobs)) continue;

            vis::Rect rect;
            if (!vis::entityRect(game, entity, context.viewport, rect)) continue;

            const ImVec4 base = vis::entityColor(entity);
            const ImU32 color = ImGui::ColorConvertFloat4ToU32(base);

            if (boxes) {
                vis::drawBox(draw, rect, color, thickness);
            }
            if (health) {
                vis::drawHealthBar(draw, rect, entity.health, entity.maxHealth);
            }
            if (names && !entity.name.empty()) {
                const ImVec2 size = ImGui::CalcTextSize(entity.name.c_str());
                const ImVec2 position(rect.center().x - size.x * 0.5f, rect.min.y - size.y - 2.0f);
                draw->AddText(ImVec2(position.x + 1.0f, position.y + 1.0f), IM_COL32(0, 0, 0, 180),
                              entity.name.c_str());
                draw->AddText(position, color, entity.name.c_str());
            }
        }
    }
};

class Tracers : public Module {
public:
    Tracers() : Module("Tracers", "Lines from the crosshair to each entity", Category::Render) {
        addBool("Players", true, "Trace players");
        addBool("Mobs", false, "Trace mobs");
        addFloat("Thickness", 1.0f, 1.0f, 3.0f, " px", "Line thickness");
        addBool("From bottom", true, "Start at the bottom edge instead of the crosshair");
    }

    void onFrame(const FrameContext& context) override {
        const GameState& game = *context.state;
        if (!game.inWorld) return;

        ImDrawList* draw = context.drawList ? context.drawList : ImGui::GetBackgroundDrawList();
        const ImVec2 origin(context.viewport.x * 0.5f,
                            boolSetting("From bottom") ? context.viewport.y : context.viewport.y * 0.5f);

        const bool players = boolSetting("Players");
        const bool mobs = boolSetting("Mobs");
        const float thickness = floatSetting("Thickness");

        for (const EntitySnapshot& entity : Bridge::entities()) {
            if (!vis::wantEntity(entity, players, mobs)) continue;

            ImVec2 target;
            if (!vis::entityCenter(game, entity, context.viewport, target)) continue;

            draw->AddLine(origin, target, ImGui::ColorConvertFloat4ToU32(vis::entityColor(entity)),
                          thickness);
        }
    }
};

class Fullbright : public Module {
public:
    Fullbright() : Module("Fullbright", "Raises the brightness option past its slider", Category::Render) {
        addFloat("Brightness", 5.0f, 1.0f, 15.0f, "x", "Gamma override");
    }

    void onEnable() override {
        previous_ = static_cast<float>(runtime().game.gamma);
        Bridge::setGamma(floatSetting("Brightness"));
    }

    void onTick() override {
        if (runtime().game.inWorld) Bridge::setGamma(floatSetting("Brightness"));
    }

    void onDisable() override { Bridge::setGamma(previous_ > 0.0f ? previous_ : 1.0f); }

private:
    float previous_ = 1.0f;
};

class Crosshair : public Module {
public:
    Crosshair() : Module("Crosshair", "Replaces the vanilla crosshair with a custom one",
                         Category::Render) {
        addFloat("Size", 6.0f, 2.0f, 24.0f, " px", "Arm length");
        addFloat("Gap", 3.0f, 0.0f, 16.0f, " px", "Space in the middle");
        addFloat("Thickness", 1.5f, 1.0f, 4.0f, " px", "Line thickness");
        addBool("Dot", false, "Draw a dot in the centre");
    }

    void onFrame(const FrameContext& context) override {
        if (!context.state->inWorld) return;

        ImDrawList* draw = context.drawList ? context.drawList : ImGui::GetBackgroundDrawList();
        const Theme& theme = theme::current();
        const ImU32 color = ImGui::ColorConvertFloat4ToU32(theme.accent);

        const float cx = std::floor(context.viewport.x * 0.5f);
        const float cy = std::floor(context.viewport.y * 0.5f);
        const float size = floatSetting("Size");
        const float gap = floatSetting("Gap");
        const float thickness = floatSetting("Thickness");

        draw->AddLine(ImVec2(cx - gap - size, cy), ImVec2(cx - gap, cy), color, thickness);
        draw->AddLine(ImVec2(cx + gap, cy), ImVec2(cx + gap + size, cy), color, thickness);
        draw->AddLine(ImVec2(cx, cy - gap - size), ImVec2(cx, cy - gap), color, thickness);
        draw->AddLine(ImVec2(cx, cy + gap), ImVec2(cx, cy + gap + size), color, thickness);

        if (boolSetting("Dot")) {
            draw->AddCircleFilled(ImVec2(cx, cy), std::max(1.0f, thickness), color, 8);
        }
    }
};

} // namespace

void registerRender() {
    ModuleManager& manager = ModuleManager::instance();
    manager.add<ESP>();
    manager.add<Tracers>();
    manager.add<Fullbright>();
    manager.add<Crosshair>();
}

} // namespace lumin::modules
