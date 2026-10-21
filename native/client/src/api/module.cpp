#include "module.hpp"

#include "render/theme.hpp"

#include <algorithm>
#include <cstdio>

namespace lumin {

RuntimeState& runtime() {
    static RuntimeState state;
    return state;
}

const char* categoryName(Category category) {
    switch (category) {
    case Category::Combat: return "Combat";
    case Category::Movement: return "Movement";
    case Category::Render: return "Render";
    case Category::World: return "World";
    case Category::Client: return "Client";
    }
    return "Unknown";
}

Module::Module(std::string name, std::string description, Category category)
    : name_(std::move(name)), description_(std::move(description)), category_(category) {}

void Module::setEnabled(bool value) {
    if (enabled_ == value) return;
    enabled_ = value;
    if (enabled_) {
        onEnable();
    } else {
        onDisable();
    }
}

Setting& Module::addBool(const std::string& name, bool value, const std::string& description) {
    settings_.push_back(std::make_unique<Setting>(Setting::makeBool(name, value, description)));
    return *settings_.back();
}

Setting& Module::addFloat(const std::string& name, float value, float min, float max,
                          const std::string& suffix, const std::string& description) {
    settings_.push_back(
        std::make_unique<Setting>(Setting::makeFloat(name, value, min, max, suffix, description)));
    return *settings_.back();
}

Setting& Module::addInt(const std::string& name, int value, int min, int max,
                        const std::string& description) {
    settings_.push_back(std::make_unique<Setting>(Setting::makeInt(name, value, min, max, description)));
    return *settings_.back();
}

Setting& Module::addEnum(const std::string& name, std::vector<std::string> options, int index,
                         const std::string& description) {
    settings_.push_back(
        std::make_unique<Setting>(Setting::makeEnum(name, std::move(options), index, description)));
    return *settings_.back();
}

bool Module::boolSetting(const std::string& name) const {
    for (const auto& s : settings_) {
        if (s->name == name) return s->boolValue;
    }
    return false;
}

float Module::floatSetting(const std::string& name) const {
    for (const auto& s : settings_) {
        if (s->name == name) return s->floatValue;
    }
    return 0.0f;
}

int Module::intSetting(const std::string& name) const {
    for (const auto& s : settings_) {
        if (s->name == name) return s->intValue;
    }
    return 0;
}

void Module::drawSettings() {
    const Theme& theme = theme::current();

    for (auto& setting : settings_) {
        ImGui::PushID(setting.get());

        switch (setting->type) {
        case Setting::Type::Bool: {
            // Full-width row: label left, switch right.
            ImGui::PushStyleColor(ImGuiCol_Text, theme.textDim);
            ImGui::TextUnformatted(setting->name.c_str());
            ImGui::PopStyleColor();
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 30.0f);
            ImGui::PushStyleColor(ImGuiCol_CheckMark, theme.accent);
            if (ImGui::Checkbox("##v", &setting->boolValue)) {
                // nothing else to do: value is read live by the module
            }
            ImGui::PopStyleColor();
            break;
        }
        case Setting::Type::Float: {
            ImGui::PushStyleColor(ImGuiCol_Text, theme.textDim);
            ImGui::TextUnformatted(setting->name.c_str());
            ImGui::PopStyleColor();
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, theme.panel);
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, theme.accent);
            ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, theme.accentBright);
            char format[16];
            snprintf(format, sizeof(format), "%%.%df%s", setting->floatPrecision,
                     setting->suffix.c_str());
            ImGui::SliderFloat("##v", &setting->floatValue, setting->floatMin, setting->floatMax,
                               format);
            ImGui::PopStyleColor(3);
            break;
        }
        case Setting::Type::Int: {
            ImGui::PushStyleColor(ImGuiCol_Text, theme.textDim);
            ImGui::TextUnformatted(setting->name.c_str());
            ImGui::PopStyleColor();
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, theme.panel);
            ImGui::PushStyleColor(ImGuiCol_SliderGrab, theme.accent);
            ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, theme.accentBright);
            ImGui::SliderInt("##v", &setting->intValue, setting->intMin, setting->intMax);
            ImGui::PopStyleColor(3);
            break;
        }
        case Setting::Type::Enum: {
            ImGui::PushStyleColor(ImGuiCol_Text, theme.textDim);
            ImGui::TextUnformatted(setting->name.c_str());
            ImGui::PopStyleColor();
            ImGui::SetNextItemWidth(-1.0f);
            const char* preview = "";
            if (!setting->options.empty()) {
                const int index = std::clamp(setting->intValue, 0,
                                             static_cast<int>(setting->options.size()) - 1);
                preview = setting->options[static_cast<size_t>(index)].c_str();
            }
            ImGui::PushStyleColor(ImGuiCol_FrameBg, theme.panel);
            if (ImGui::BeginCombo("##v", preview)) {
                for (int i = 0; i < static_cast<int>(setting->options.size()); ++i) {
                    bool selected = (setting->intValue == i);
                    if (ImGui::Selectable(setting->options[i].c_str(), selected)) {
                        setting->intValue = i;
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::PopStyleColor();
            break;
        }
        }

        if (!setting->description.empty() && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", setting->description.c_str());
        }

        ImGui::PopID();
    }

    drawCustomSettings();
}

} // namespace lumin
