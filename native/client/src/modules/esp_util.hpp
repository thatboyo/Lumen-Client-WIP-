#pragma once

#include "api/module.hpp"
#include "core/bridge.hpp"
#include "render/project.hpp"
#include "render/theme.hpp"

#include <imgui.h>

#include <cmath>
#include <vector>

namespace lumin::modules::vis {

struct Rect {
    ImVec2 min{0, 0};
    ImVec2 max{0, 0};

    bool valid() const { return max.x > min.x + 0.5f && max.y > min.y + 0.5f; }
    float width() const { return max.x - min.x; }
    float height() const { return max.y - min.y; }
    ImVec2 center() const { return ImVec2((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f); }
};

// Screen-space bounding rectangle of an entity's 3D box. Corners behind the
// camera are dropped instead of projected, so a box that straddles the camera
// still produces a sane rectangle.
inline bool entityRect(const GameState& state, const EntitySnapshot& entity, const ImVec2& viewport,
                       Rect& out) {
    const project::Basis basis = project::fromCamera(state);

    const double halfWidth = entity.width * 0.5;
    bool any = false;
    float minX = 0, minY = 0, maxX = 0, maxY = 0;

    for (int i = 0; i < 8; ++i) {
        const double x = entity.x + ((i & 1) ? halfWidth : -halfWidth);
        const double y = entity.y + ((i & 2) ? entity.height : 0.0);
        const double z = entity.z + ((i & 4) ? halfWidth : -halfWidth);

        ImVec2 point;
        if (!project::toScreen(state, basis, x, y, z, viewport, point)) continue;

        if (!any) {
            minX = maxX = point.x;
            minY = maxY = point.y;
            any = true;
        } else {
            minX = std::min(minX, point.x);
            maxX = std::max(maxX, point.x);
            minY = std::min(minY, point.y);
            maxY = std::max(maxY, point.y);
        }
    }

    out.min = ImVec2(minX, minY);
    out.max = ImVec2(maxX, maxY);
    return any && out.valid();
}

inline bool entityCenter(const GameState& state, const EntitySnapshot& entity, const ImVec2& viewport,
                         ImVec2& out) {
    const project::Basis basis = project::fromCamera(state);
    return project::toScreen(state, basis, entity.x, entity.y + entity.height * 0.5, entity.z,
                             viewport, out);
}

inline ImVec4 entityColor(const EntitySnapshot& entity) {
    if (entity.isPlayer()) return theme::current().categoryColor(0);
    if (entity.isHostile()) return theme::current().danger;
    return theme::current().categoryColor(3);
}

inline void drawBox(ImDrawList* draw, const Rect& rect, ImU32 color, float thickness) {
    draw->AddRect(rect.min, rect.max, color, 0.0f, 0, thickness);

    // Corner ticks make the box readable at distance without a heavy outline.
    const float tick = std::min(rect.width(), rect.height()) * 0.22f;
    draw->AddLine(rect.min, ImVec2(rect.min.x + tick, rect.min.y), color, thickness + 1.0f);
    draw->AddLine(rect.min, ImVec2(rect.min.x, rect.min.y + tick), color, thickness + 1.0f);
    draw->AddLine(ImVec2(rect.max.x, rect.min.y), ImVec2(rect.max.x - tick, rect.min.y), color,
                  thickness + 1.0f);
    draw->AddLine(ImVec2(rect.max.x, rect.min.y), ImVec2(rect.max.x, rect.min.y + tick), color,
                  thickness + 1.0f);
    draw->AddLine(ImVec2(rect.min.x, rect.max.y), ImVec2(rect.min.x + tick, rect.max.y), color,
                  thickness + 1.0f);
    draw->AddLine(ImVec2(rect.min.x, rect.max.y), ImVec2(rect.min.x, rect.max.y - tick), color,
                  thickness + 1.0f);
    draw->AddLine(rect.max, ImVec2(rect.max.x - tick, rect.max.y), color, thickness + 1.0f);
    draw->AddLine(rect.max, ImVec2(rect.max.x, rect.max.y - tick), color, thickness + 1.0f);
}

inline void drawHealthBar(ImDrawList* draw, const Rect& rect, float health, float maxHealth) {
    if (maxHealth <= 0.0f) return;

    const float fraction = std::max(0.0f, std::min(1.0f, health / maxHealth));
    const float barWidth = std::max(2.0f, rect.width() * 0.09f);
    const ImVec2 top(rect.min.x - barWidth - 4.0f, rect.min.y);
    const ImVec2 bottom(top.x + barWidth, rect.max.y);

    draw->AddRectFilled(top, bottom, IM_COL32(20, 20, 24, 160), 1.0f);
    const float filledTop = bottom.y - rect.height() * fraction;
    const ImU32 fill = fraction > 0.6f   ? IM_COL32(102, 217, 153, 235)
                       : fraction > 0.3f ? IM_COL32(255, 199, 90, 235)
                                         : IM_COL32(249, 103, 115, 235);
    draw->AddRectFilled(ImVec2(top.x, filledTop), bottom, fill, 1.0f);
}

// Shared entity filter so ESP and Tracers always agree on what they show.
inline bool wantEntity(const EntitySnapshot& entity, bool players, bool mobs) {
    if (entity.isPlayer()) return players;
    return mobs;
}

} // namespace lumin::modules::vis
