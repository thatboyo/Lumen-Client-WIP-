#pragma once

#include "setting.hpp"

#include "core/bridge.hpp"

#include <imgui.h>

#include <memory>
#include <string>
#include <vector>

namespace lumin {

enum class Category { Combat, Movement, Render, World, Client };

const char* categoryName(Category category);

// Per-frame data handed to overlay renderers.
struct FrameContext {
    const GameState* state = nullptr;
    ImDrawList* drawList = nullptr;
    ImVec2 viewport{0, 0};
    float deltaTime = 0.0f;
};

// Global, refreshed once per rendered frame. Modules read it instead of calling
// into the bridge on their own, so the cost is one snapshot per frame.
struct RuntimeState {
    GameState game;
    FrameContext frame;
    int tick = 0;
    bool menuOpen = false;
    bool focused = false;
};

RuntimeState& runtime();

class Module {
public:
    Module(std::string name, std::string description, Category category);
    virtual ~Module() = default;

    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;

    const std::string& name() const { return name_; }
    const std::string& description() const { return description_; }
    Category category() const { return category_; }

    bool enabled() const { return enabled_; }
    void setEnabled(bool value);
    void toggle() { setEnabled(!enabled_); }

    // 20 Hz, driven by the game's tick counter.
    virtual void onTick() {}

    // Called when the module is switched on/off. Override to push the first
    // state into the game and to undo it in onDisable.
    virtual void onEnable() {}
    virtual void onDisable() {}

    // Every rendered frame, after the world is drawn.
    virtual void onFrame(const FrameContext&) {}

    // Extra UI inside the settings panel.
    virtual void drawCustomSettings() {}

    // Settings ---------------------------------------------------------
    Setting& addBool(const std::string& name, bool value, const std::string& description = {});
    Setting& addFloat(const std::string& name, float value, float min, float max,
                      const std::string& suffix = "", const std::string& description = {});
    Setting& addInt(const std::string& name, int value, int min, int max,
                    const std::string& description = {});
    Setting& addEnum(const std::string& name, std::vector<std::string> options, int index,
                     const std::string& description = {});

    std::vector<std::unique_ptr<Setting>>& settings() { return settings_; }

    bool boolSetting(const std::string& name) const;
    float floatSetting(const std::string& name) const;
    int intSetting(const std::string& name) const;

    // Renders the generic setting widgets.
    void drawSettings();

private:
    std::string name_;
    std::string description_;
    Category category_;
    bool enabled_ = false;
    std::vector<std::unique_ptr<Setting>> settings_;
};

} // namespace lumin
