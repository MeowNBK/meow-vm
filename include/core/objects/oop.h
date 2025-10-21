/**
 * @file oop.h
 * @author LazyPaws
 * @brief Core definition of Class, Instance, BoundMethod in TrangMeo
 * @copyright Copyright (c) 2025 LazyPaws
 * @license All rights reserved. Unauthorized copying of this file, in any form
 * or medium, is strictly prohibited
 */

#pragma once

#include "common/pch.h"
#include "core/definitions.h"
#include "core/meow_object.h"
#include "core/type.h"
#include "core/value.h"
#include "memory/gc_visitor.h"

namespace meow::core::objects {
class ObjClass : public meow::core::MeowObject {
   private:
    using string_t = meow::core::string_t;
    using class_t = meow::core::class_t;
    using method_map = std::unordered_map<string_t, meow::core::value_t>;
    using visitor_t = meow::memory::GCVisitor;

    string_t name_;
    class_t superclass_;
    method_map methods_;

   public:
    explicit ObjClass(string_t name = nullptr) noexcept : name_(name) {}

    // --- Metadata ---
    [[nodiscard]] inline string_t get_name() const noexcept { return name_; }
    [[nodiscard]] inline class_t get_super() const noexcept { return superclass_; }
    inline void set_super(class_t super) noexcept { superclass_ = super; }

    // --- Methods ---
    [[nodiscard]] inline bool has_method(string_t name) const noexcept {
        return methods_.find(name) != methods_.end();
    }
    [[nodiscard]] inline meow::core::return_t get_method(string_t name) noexcept {
        return methods_[name];
    }
    inline void set_method(string_t name, meow::core::return_t value) noexcept {
        methods_[name] = value;
    }

    void trace(visitor_t &visitor) const noexcept override;
};

class ObjInstance : public meow::core::MeowObject {
   private:
    using string_t = meow::core::string_t;
    using class_t = meow::core::class_t;
    using field_map = std::unordered_map<string_t, meow::core::value_t>;
    using visitor_t = meow::memory::GCVisitor;

    class_t klass_;
    field_map fields_;

   public:
    explicit ObjInstance(class_t k = nullptr) noexcept : klass_(k) {}

    // --- Metadata ---
    [[nodiscard]] inline class_t get_class() const noexcept { return klass_; }
    inline void set_class(class_t klass) noexcept { klass_ = klass; }

    // --- Fields ---
    [[nodiscard]] inline meow::core::return_t get_field(string_t name) noexcept {
        return fields_[name];
    }
    inline void set_field(string_t name, meow::core::param_t value) noexcept {
        fields_[name] = value;
    }
    [[nodiscard]] inline bool has_field(string_t name) const {
        return fields_.find(name) != fields_.end();
    }

    void trace(visitor_t &visitor) const noexcept override;
};

class ObjBoundMethod : public MeowObject {
   private:
    using instance_t = meow::core::instance_t;
    using function_t = meow::core::function_t;
    using visitor_t = meow::memory::GCVisitor;

    instance_t instance_;
    function_t function_;

   public:
    explicit ObjBoundMethod(instance_t instance = nullptr, function_t function = nullptr) noexcept
        : instance_(instance), function_(function) {}

    inline instance_t get_instance() const noexcept { return instance_; }
    inline function_t get_function() const noexcept { return function_; }

    void trace(visitor_t &visitor) const noexcept override;
};
}  // namespace meow::core::objects