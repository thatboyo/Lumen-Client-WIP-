#include "modules/modules.hpp"

#include "api/module_manager.hpp"
#include "render/theme.hpp"

#include <imgui.h>

#include <algorithm>
#include <string>
#include <vector>

namespace lumin::modules {
namespace {

void applyPreset(int index, float scale) {
    const auto& list = theme::presets();
    if (list.empty()) return;

    Theme next = list[static_cast<size_t>(std::max(0, std::min(index, static_cast<int>(list.size()) - 1)))];
    next.uiScale = scale;
    theme::set(next);
}

class Appearance : public Module {
public:
    Appearance()
        : Module("Appearance", "Theme preset, UI scale and config", Category::Client) {
        std::vector<std::string> names;
        for (const Theme& preset : theme::presets()) names.push_back(preset.name);
        if (names.empty()) names.push_back("Lumin");

        addEnum("Theme", names, 0, "Lumin, Rain or Synth; drop a theme.json next to the DLL to override");
        addFloat("UI scale", 1.0f, 0.75f, 1.5f, "x", "Scales fonts, padding and window size");
    }

    void onEnable() override { appliedPreset_ = kAdoptState; }

    void drawCustomSettings() override {
        // The generic settings are drawn first, so the values below are already
        // this frame's. Applying here keeps the theme in step with the widgets
        // without touching ImGui from the tick loop.
        const int preset = intSetting("Theme");
        const float scale = floatSetting("UI scale");

        if (appliedPreset_ == kAdoptState) {
            // Bootstrap already applied whatever the config (or theme.json)
            // asked for, so the first panel draw only records it. Re-applying
            // here would stomp a theme.json palette the user just loaded.
            appliedPreset_ = preset;
            appliedScale_ = scale;
        } else if (preset != appliedPreset_ || std::abs(scale - appliedScale_) > 0.001f) {
            appliedPreset_ = preset;
            appliedScale_ = scale;
            applyPreset(preset, scale);
        }

        const Theme& theme = theme::current();
        ImGui::PushStyleColor(ImGuiCol_Text, theme.textDim);
        ImGui::Text("active: %s", theme.name.c_str());
        ImGui::PopStyleColor();

        if (ImGui::Button("Save config", ImVec2(-1, 0))) {
            ModuleManager::instance().saveConfig();
        }
    }

private:
    static constexpr int kAdoptState = -2;
    int appliedPreset_ = kAdoptState;
    float appliedScale_ = 1.0f;
};

} // namespace

void registerClient() {
    ModuleManager::instance().add<Appearance>();
}

void applySavedTheme() {
    Module* appearance = ModuleManager::instance().find("Appearance");
    if (!appearance) return;
    applyPreset(appearance->intSetting("Theme"), appearance->floatSetting("UI scale"));
}

} // namespace lumin::modules
