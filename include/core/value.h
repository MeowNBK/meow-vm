/**
 * @file value.h
 * @author LazyPaws
 * @brief Core definition of Value in TrangMeo
 * @copyright Copyright (c) 2025 LazyPaws
 * @license All rights reserved. Unauthorized copying of this file, in any form or medium, is strictly prohibited
 */

#pragma once

#include <common/pch.h>
#include <core/type.h>
#include <utils/types/variant.h>

namespace meow::core { struct MeowObject; }

namespace meow::core {
    class Value {
    private:
        using BaseValue = meow::variant<
            Null, Bool, Int, Real, Object
        >;
        BaseValue data_;
    public:
        // --- Constructors ---
        Value(): data_(Null{}) {}
        Value(std::monostate): data_(Null{}) {}
        Value(bool b): data_(b) {}
        Value(int64_t i): data_(i) {}
        Value(int i): data_(static_cast<int64_t>(i)) {}
        Value(double r): data_(r) {}

        [[nodiscard]] inline bool is_null() const noexcept { return data_.holds<Null>(); }
        [[nodiscard]] inline bool is_bool() const noexcept { return data_.holds<Bool>(); }
        [[nodiscard]] inline bool is_int() const noexcept { return data_.holds<Int>(); }
        [[nodiscard]] inline bool is_real() const noexcept { return data_.holds<Real>(); }
        [[nodiscard]] inline bool is_array() const noexcept { return data_.holds<Array>(); }
        [[nodiscard]] inline bool is_string() const noexcept { return data_.holds<String>(); }
        [[nodiscard]] inline bool is_hash() const noexcept { return data_.holds<Hash>(); }
        [[nodiscard]] inline bool is_upvalue() const noexcept { return data_.holds<Upvalue>(); }
        [[nodiscard]] inline bool is_proto() const noexcept { return data_.holds<Proto>(); }
        [[nodiscard]] inline bool is_function() const noexcept { return data_.holds<Function>(); }
        [[nodiscard]] inline bool is_native_fn() const noexcept { return data_.holds<NativeFn>(); }
        [[nodiscard]] inline bool is_class() const noexcept { return data_.holds<Class>(); }
        [[nodiscard]] inline bool is_instance() const noexcept { return data_.holds<Instance>(); }
        [[nodiscard]] inline bool is_bound_method() const noexcept { return data_.holds<BoundMethod>(); }
        [[nodiscard]] inline bool is_module() const noexcept { return data_.holds<Module>(); }
        [[nodiscard]] inline bool is_object() const noexcept { return data_.holds<Object>(); }

        [[nodiscard]] inline bool as_bool() const { return data_.get<Bool>(); }
        [[nodiscard]] inline int64_t as_int() const { return data_.get<Int>(); }
        [[nodiscard]] inline double as_real() const { return data_.get<Real>(); }
        [[nodiscard]] inline Array as_array() const { return data_.get<Array>(); }
        [[nodiscard]] inline String as_string() const { return data_.get<String>(); }
        [[nodiscard]] inline Hash as_hash() const { return data_.get<Hash>(); }
        [[nodiscard]] inline Upvalue as_upvalue() const { return data_.get<Upvalue>(); }
        [[nodiscard]] inline Proto as_proto() const { return data_.get<Proto>(); }
        [[nodiscard]] inline Function as_function() const { return data_.get<Function>(); }
        [[nodiscard]] inline NativeFn as_native_fn() const { return data_.get<NativeFn>(); }
        [[nodiscard]] inline Class as_class() const { return data_.get<Class>(); }
        [[nodiscard]] inline Instance as_instance() const { return data_.get<Instance>(); }
        [[nodiscard]] inline BoundMethod as_bound_method() const { return data_.get<BoundMethod>(); }
        [[nodiscard]] inline Module as_module() const { return data_.get<Module>(); }

        [[nodiscard]] inline meow::core::MeowObject* as_object() const {
            return data_.visit([](auto&& arg) -> meow::core::MeowObject* {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_pointer_v<T> && std::is_base_of_v<meow::core::MeowObject, std::remove_pointer_t<T>>) return arg;
                return nullptr;
            });
        }
        [[nodiscard]] inline meow::core::MeowObject* as_object() const {
            return data_.visit([](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_pointer_v<T>) return static_cast<MeowObject*>(arg);
                return nullptr;
            });
        }

        inline const bool* as_if_bool() const noexcept { return data_.get_if<Bool>(); }
        inline const int64_t* as_if_int() const noexcept { return data_.get_if<Int>(); }
        inline const double* as_if_real() const noexcept { return data_.get_if<Real>(); }
        inline meow::core::MeowObject* const* as_if_object() const noexcept {
            static meow::core::MeowObject* object = nullptr;
            if (is_object()) {
                object = as_object();
                return &object;
            }
            return nullptr;
        }

        inline bool* as_if_bool() noexcept { return data_.get_if<Bool>(); }
        inline int64_t* as_if_int() noexcept { return data_.get_if<Int>(); }
        inline double* as_if_real() noexcept { return data_.get_if<Real>(); }
        inline meow::core::MeowObject** as_if_object() noexcept {
            static meow::core::MeowObject* object = nullptr;
            if (is_object()) {
                object = as_object();
                return &object;
            }
            return nullptr;
        }
    };
}