/**
 * @file oop.h
 * @author LazyPaws
 * @brief Core definition of Class, Instance, BoundMethod in TrangMeo
 * @copyright Copyright (c) 2025 LazyPaws
 * @license All rights reserved. Unauthorized copying of this file, in any form or medium, is strictly prohibited
 */

#pragma once

#include "common/pch.h"
#include "core/value.h"
#include "core/meow_object.h"
#include "memory/gc_visitor.h"
#include "core/type.h"

namespace meow::core::objects {
    class ObjClass : public MeowObject {
    private:
        using value_t = meow::core::Value;
        using const_reference_t = const value_t&;
        using string_t = meow::core::String;
        using class_t = meow::core::Class;
        using method_map = std::unordered_map<string_t, value_t>;

        string_t name_;
        class_t superclass_;
        method_map methods_;
    public:
        explicit ObjClass(string_t name = nullptr) noexcept: name_(name) {}

        // --- Metadata ---
        [[nodiscard]] inline string_t get_name() const noexcept { return name_; }
        [[nodiscard]] inline class_t get_super() const noexcept { return superclass_; }
        inline void set_super(class_t super) noexcept { superclass_ = super; }

        // --- Methods ---
        [[nodiscard]] inline bool has_method(string_t name) const noexcept { return methods_.find(name) != methods_.end(); }
        [[nodiscard]] inline const_reference_t get_method(string_t name) noexcept { return methods_[name]; }
        inline void set_method(string_t name, const_reference_t value) noexcept { methods_[name] = value; }
    };


    class ObjInstance : public MeowObject {
    private:
        using value_t = meow::core::Value;
        using const_reference_t = const value_t&;
        using string_t = meow::core::String;
        using class_t = meow::core::Class;
        using field_map = std::unordered_map<string_t, value_t>;

        class_t klass_;
        field_map fields_;
    public:
        explicit ObjInstance(class_t k = nullptr) noexcept: klass_(k) {}

        // --- Metadata ---
        [[nodiscard]] inline class_t get_class() const noexcept { return klass_; }
        inline void set_class(class_t klass) noexcept { klass_ = klass; }

        // --- Fields ---
        [[nodiscard]] inline const_reference_t get_field(string_t name) noexcept { return fields_[name];}
        [[nodiscard]] inline void set_field(string_t name, const_reference_t value) noexcept { fields_[name] = value; }
        [[nodiscard]] inline bool has_field(string_t name) const { return fields_.find(name) != fields_.end(); }
    };

    class ObjBoundMethod : public MeowObject {
    private:
        using instance_t = meow::core::Instance;
        using function_t = meow::core::Function;

        instance_t instance_;
        function_t function_;
    public:
        explicit ObjBoundMethod(instance_t instance = nullptr, function_t function = nullptr) noexcept: instance_(instance), function_(function) {}

        inline instance_t get_instance() const noexcept { return instance_; }
        inline function_t get_function() const noexcept { return function_; }
    };
}