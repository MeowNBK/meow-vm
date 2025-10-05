#pragma once

#include "common/pch.h"
#include "core/value.h"
#include "core/type.h"
#include "core/objects/string.h"
#include "memory/gc_visitor.h"

namespace meow::runtime {
    struct BuiltinRegistry {
        std::unordered_map<meow::core::String, std::unordered_map<meow::core::String, meow::core::Value>> methods_;
        std::unordered_map<meow::core::String, std::unordered_map<meow::core::String, meow::core::Value>> getters_;

        inline void trace(meow::memory::GCVisitor& visitor) const noexcept {
            for (const auto& [name, method] : methods_) {
                visitor.visit_object(name);
                for (const auto& [key, value] : method) {
                    visitor.visit_object(key);
                    visitor.visit_value(value);
                }
            }

            for (const auto& [name, getter] : getters_) {
                visitor.visit_object(name);
                for (const auto& [key, value] : getter) {
                    visitor.visit_object(key);
                    visitor.visit_value(value);
                }
            }
        }
    };
}