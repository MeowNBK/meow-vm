#pragma once

#include "common/pch.h"

namespace meow::utils::string {
    struct StringHash {
        using is_transparent = void;
        
        size_t operator()(const char* txt) const { return std::hash<std::string_view>{}(txt); }
        size_t operator()(std::string_view txt) const { return std::hash<std::string_view>{}(txt); }
        size_t operator()(const std::string& txt) const { return std::hash<std::string>{}(txt); }
    };
}