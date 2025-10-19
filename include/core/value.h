/**
 * @file value.h
 * @author LazyPaws
 * @brief Core definition of Value in TrangMeo
 * @copyright Copyright (c) 2025 LazyPaws
 * @license All rights reserved. Unauthorized copying of this file, in any form or medium, is strictly prohibited
 */

#pragma once

#include "common/pch.h"
#include "core/type.h"
#include "utils/types/variant.h"

namespace meow::core { struct MeowObject; }

namespace meow::core {
    using BaseValue = meow::variant<
        null_t, bool_t, int_t, float_t, object_t
    >;

    class Value;

    template <typename T>

    concept ValueAssignable = std::is_constructible_v<BaseValue, T>;   
    class Value {
    private:
        BaseValue data_;
    public:
        // --- Constructors ---
        Value() : data_(null_t{}) {}
        template <ValueAssignable T>
        Value(T&& value) noexcept(noexcept(data_ = std::forward<T>(value)))
            : data_(std::forward<T>(value)) 
        {}

        // --- Rule of five ---
        Value(const Value& other) : data_(other.data_) {}
        Value(Value&& other) : data_(std::move(other.data_)) {}
        inline Value& operator=(const Value& other) noexcept {
            if (this == &other) return *this;
            data_ = other.data_;
            return *this;
        }
        inline Value& operator=(Value&& other) noexcept {
            if (this == &other) return *this;
            data_ = std::move(other.data_);
            return *this;
        }
        ~Value() noexcept = default;

        // --- Other overload ---
        template <ValueAssignable T>
        inline Value& operator=(T&& value) noexcept(noexcept(data_ = std::forward<T>(value))) {
            data_ = std::forward<T>(value);
            return *this;
        }

        [[nodiscard]] inline bool is_null() const noexcept { return data_.holds<null_t>(); }
        [[nodiscard]] inline bool is_bool() const noexcept { return data_.holds<bool_t>(); }
        [[nodiscard]] inline bool is_int() const noexcept { return data_.holds<int_t>(); }
        [[nodiscard]] inline bool is_float() const noexcept { return data_.holds<float_t>(); }
        [[nodiscard]] inline bool is_array() const noexcept { return data_.holds<array_t>(); }
        [[nodiscard]] inline bool is_string() const noexcept { return data_.holds<string_t>(); }
        [[nodiscard]] inline bool is_hash_table() const noexcept { return data_.holds<hash_table_t>(); }
        [[nodiscard]] inline bool is_upvalue() const noexcept { return data_.holds<upvalue_t>(); }
        [[nodiscard]] inline bool is_proto() const noexcept { return data_.holds<proto_t>(); }
        [[nodiscard]] inline bool is_function() const noexcept { return data_.holds<function_t>(); }
        [[nodiscard]] inline bool is_native_fn() const noexcept { return data_.holds<native_fn_t>(); }
        [[nodiscard]] inline bool is_class() const noexcept { return data_.holds<class_t>(); }
        [[nodiscard]] inline bool is_instance() const noexcept { return data_.holds<instance_t>(); }
        [[nodiscard]] inline bool is_bound_method() const noexcept { return data_.holds<bound_method_t>(); }
        [[nodiscard]] inline bool is_module() const noexcept { return data_.holds<module_t>(); }
        [[nodiscard]] inline bool is_object() const noexcept { return data_.holds<object_t>(); }

        [[nodiscard]] inline bool as_bool() const noexcept { return data_.get<bool_t>(); }
        [[nodiscard]] inline int64_t as_int() const noexcept { return data_.get<int_t>(); }
        [[nodiscard]] inline double as_float() const noexcept { return data_.get<float_t>(); }
        [[nodiscard]] inline array_t as_array() const noexcept { return data_.get<array_t>(); }
        [[nodiscard]] inline string_t as_string() const noexcept { return data_.get<string_t>(); }
        [[nodiscard]] inline hash_table_t as_hash_table() const noexcept { return data_.get<hash_table_t>(); }
        [[nodiscard]] inline upvalue_t as_upvalue() const noexcept { return data_.get<upvalue_t>(); }
        [[nodiscard]] inline proto_t as_proto() const noexcept { return data_.get<proto_t>(); }
        [[nodiscard]] inline function_t as_function() const noexcept { return data_.get<function_t>(); }
        [[nodiscard]] inline native_fn_t as_native_fn() const noexcept { return data_.get<native_fn_t>(); }
        [[nodiscard]] inline class_t as_class() const noexcept { return data_.get<class_t>(); }
        [[nodiscard]] inline instance_t as_instance() const noexcept { return data_.get<instance_t>(); }
        [[nodiscard]] inline bound_method_t as_bound_method() const noexcept { return data_.get<bound_method_t>(); }
        [[nodiscard]] inline module_t as_module() const noexcept { return data_.get<module_t>(); }

        // [[nodiscard]] inline meow::core::MeowObject* as_object() const noexcept {
        //     return data_.visit([](auto&& arg) -> meow::core::MeowObject* {
        //         using T = std::decay_t<decltype(arg)>;
        //         if constexpr (std::is_pointer_v<T> && std::is_base_of_v<meow::core::MeowObject, std::remove_pointer_t<T>>) return arg;
        //         return nullptr;
        //     });
        // }

        [[nodiscard]] inline meow::core::MeowObject* as_object() const noexcept {
            return nullptr;
        }

        inline const bool* as_if_bool() const noexcept { return data_.get_if<bool_t>(); }
        inline const int64_t* as_if_int() const noexcept { return data_.get_if<int_t>(); }
        inline const double* as_if_float() const noexcept { return data_.get_if<float_t>(); }
        // inline meow::core::MeowObject* const* as_if_object() const noexcept {
        //     thread_local static meow::core::MeowObject* object = nullptr; // force to use static variable, no choice left, no multi thread-usage
        //     if (is_object()) {
        //         object = as_object();
        //         return &object;
        //     }
        //     return nullptr;
        // }

        inline bool* as_if_bool() noexcept { return data_.get_if<bool_t>(); }
        inline int64_t* as_if_int() noexcept { return data_.get_if<int_t>(); }
        inline double* as_if_float() noexcept { return data_.get_if<float_t>(); }
        // inline meow::core::MeowObject** as_if_object() noexcept {
        //     thread-local static meow::core::MeowObject* object = nullptr;
        //     if (is_object()) {
        //         object = as_object();
        //         return &object;
        //     }
        //     return nullptr;
        // }

        template <typename Visitor>
        decltype(auto) visit(Visitor&& vis) { return data_.visit(vis); }

        template <typename Visitor>
        decltype(auto) visit(Visitor&& vis) const { return data_.visit(vis); }

        template <typename... Fs>
        decltype(auto) visit(Fs&&... fs) {
            return data_.visit(std::forward<Fs>(fs)...);
        }

        template <typename... Fs>
        decltype(auto) visit(Fs&&... fs) const {
            return data_.visit(std::forward<Fs>(fs)...);
        }
    };
}