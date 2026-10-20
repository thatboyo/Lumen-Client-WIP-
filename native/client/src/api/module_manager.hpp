#pragma once

#include "module.hpp"

#include <memory>
#include <string>
#include <vector>

namespace lumin {

class ModuleManager {
public:
    static ModuleManager& instance();

    template <typename T, typename... Args>
    T* add(Args&&... args) {
        auto module = std::make_unique<T>(std::forward<Args>(args)...);
        T* raw = module.get();
        modules_.push_back(std::move(module));
        return raw;
    }

    // Fires onTick() exactly once per game tick (20 Hz), not per frame.
    void tick();

    // Dispatches onFrame() for enabled modules.
    void frame(const FrameContext& context);

    std::vector<std::unique_ptr<Module>>& all() { return modules_; }

    Module* find(const std::string& name);
    std::vector<Module*> byCategory(Category category);

    int enabledCount() const;

    void setMenuOpen(bool open) { menuOpen_ = open; }
    bool menuOpen() const { return menuOpen_; }

    void saveConfig();
    void loadConfig();

    static std::string configDirectory();

private:
    std::vector<std::unique_ptr<Module>> modules_;
    int lastTick_ = -1;
    bool menuOpen_ = false;
};

} // namespace lumin
