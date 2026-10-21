#pragma once

#include <string>
#include <vector>

namespace lumin {

// One tunable value on a module. Deliberately a flat struct: the UI, the config
// writer and every module read the same three fields, so adding a module never
// needs new plumbing.
struct Setting {
    enum class Type { Bool, Float, Int, Enum };

    Type type = Type::Bool;
    std::string name;
    std::string description;

    bool boolValue = false;
    bool boolDefault = false;

    float floatValue = 0.0f;
    float floatMin = 0.0f;
    float floatMax = 1.0f;
    float floatDefault = 0.0f;
    int floatPrecision = 2;

    int intValue = 0;
    int intMin = 0;
    int intMax = 100;
    int intDefault = 0;

    std::vector<std::string> options;
    std::string suffix;

    static Setting makeBool(std::string name, bool value, std::string description = {}) {
        Setting s;
        s.type = Type::Bool;
        s.name = std::move(name);
        s.description = std::move(description);
        s.boolValue = s.boolDefault = value;
        return s;
    }

    static Setting makeFloat(std::string name, float value, float min, float max,
                             std::string suffix = "", std::string description = {}) {
        Setting s;
        s.type = Type::Float;
        s.name = std::move(name);
        s.description = std::move(description);
        s.floatValue = s.floatDefault = value;
        s.floatMin = min;
        s.floatMax = max;
        s.suffix = std::move(suffix);
        return s;
    }

    static Setting makeInt(std::string name, int value, int min, int max, std::string description = {}) {
        Setting s;
        s.type = Type::Int;
        s.name = std::move(name);
        s.description = std::move(description);
        s.intValue = s.intDefault = value;
        s.intMin = min;
        s.intMax = max;
        return s;
    }

    static Setting makeEnum(std::string name, std::vector<std::string> options, int index,
                            std::string description = {}) {
        Setting s;
        s.type = Type::Enum;
        s.name = std::move(name);
        s.description = std::move(description);
        s.options = std::move(options);
        s.intValue = s.intDefault = index;
        s.intMin = 0;
        s.intMax = static_cast<int>(s.options.size()) - 1;
        return s;
    }
};

} // namespace lumin
