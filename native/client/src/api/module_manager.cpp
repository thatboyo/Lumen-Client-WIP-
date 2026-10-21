#include "module_manager.hpp"

#include "core/log.hpp"

#include "json.hpp"

#include <algorithm>
#include <fstream>
#include <windows.h>

namespace lumin {

ModuleManager& ModuleManager::instance() {
    static ModuleManager manager;
    return manager;
}

void ModuleManager::tick() {
    const int tick = runtime().tick;
    if (tick == lastTick_) return;
    lastTick_ = tick;

    for (auto& module : modules_) {
        if (module->enabled()) {
            module->onTick();
        }
    }
}

void ModuleManager::frame(const FrameContext& context) {
    for (auto& module : modules_) {
        if (module->enabled()) {
            module->onFrame(context);
        }
    }
}

Module* ModuleManager::find(const std::string& name) {
    for (auto& module : modules_) {
        if (module->name() == name) return module.get();
    }
    return nullptr;
}

std::vector<Module*> ModuleManager::byCategory(Category category) {
    std::vector<Module*> out;
    for (auto& module : modules_) {
        if (module->category() == category) out.push_back(module.get());
    }
    return out;
}

int ModuleManager::enabledCount() const {
    int count = 0;
    for (const auto& module : modules_) {
        if (module->enabled()) ++count;
    }
    return count;
}

std::string ModuleManager::configDirectory() {
    char buffer[MAX_PATH] = {0};
    if (GetEnvironmentVariableA("APPDATA", buffer, MAX_PATH) > 0) {
        std::string dir = std::string(buffer) + "\\Lumin";
        CreateDirectoryA(dir.c_str(), nullptr);
        return dir;
    }
    return ".";
}

void ModuleManager::saveConfig() {
    nlohmann::json root;
    root["version"] = 1;

    nlohmann::json modules = nlohmann::json::array();
    for (auto& module : modules_) {
        nlohmann::json entry;
        entry["name"] = module->name();
        entry["enabled"] = module->enabled();

        nlohmann::json settings = nlohmann::json::object();
        for (auto& setting : module->settings()) {
            switch (setting->type) {
            case Setting::Type::Bool: settings[setting->name] = setting->boolValue; break;
            case Setting::Type::Float: settings[setting->name] = setting->floatValue; break;
            case Setting::Type::Int:
            case Setting::Type::Enum: settings[setting->name] = setting->intValue; break;
            }
        }
        entry["settings"] = settings;
        modules.push_back(entry);
    }
    root["modules"] = modules;

    const std::string path = configDirectory() + "\\config.json";
    std::ofstream file(path);
    if (!file.is_open()) {
        VLOG_WARN("could not write %s", path.c_str());
        return;
    }
    file << root.dump(2);
    VLOG_INFO("config saved to %s", path.c_str());
}

void ModuleManager::loadConfig() {
    const std::string path = configDirectory() + "\\config.json";
    std::ifstream file(path);
    if (!file.is_open()) {
        VLOG_INFO("no config at %s (using defaults)", path.c_str());
        return;
    }

    try {
        nlohmann::json root;
        file >> root;

        for (auto& entry : root.value("modules", nlohmann::json::array())) {
            Module* module = find(entry.value("name", std::string{}));
            if (!module) continue;

            // Values first: onEnable() pushes the current settings into the game, so
            // there is no window where a module runs with stale defaults.
            if (entry.contains("settings")) {
                const auto& settings = entry["settings"];
                for (auto& setting : module->settings()) {
                    if (!settings.contains(setting->name)) continue;
                    const auto& value = settings[setting->name];
                    // Clamp: a hand-edited or older config must never index past an
                    // enum's options list.
                    switch (setting->type) {
                    case Setting::Type::Bool:
                        setting->boolValue = value.get<bool>();
                        break;
                    case Setting::Type::Float:
                        setting->floatValue = std::clamp(value.get<float>(), setting->floatMin,
                                                         setting->floatMax);
                        break;
                    case Setting::Type::Int:
                    case Setting::Type::Enum:
                        setting->intValue = std::clamp(value.get<int>(), setting->intMin,
                                                       setting->intMax);
                        break;
                    }
                }
            }

            if (entry.value("enabled", false)) {
                module->setEnabled(true);
            }
        }
        VLOG_INFO("config loaded from %s", path.c_str());
    } catch (const std::exception& e) {
        VLOG_WARN("config load failed: %s", e.what());
    }
}

} // namespace lumin
