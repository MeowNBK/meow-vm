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

namespace meow::core { struct MeowObject; }

namespace meow::core {
    class Value {
    private:
        using BaseValue = std::variant<
            Null, Bool, Int, Real,
            String, Array, Hash, Upvalue, Proto, Function,
            NativeFn, Class, Instance, BoundMethod, Module
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

        Value(String s): data_(s) {}
        Value(Array a): data_(a) {}
        Value(Hash h): data_(h) {}
        Value(Upvalue u): data_(u) {}
        Value(Proto p): data_(p) {}
        Value(Function f): data_(f) {}
        Value(NativeFn n): data_(n) {}
        Value(Class c): data_(c) {}
        Value(Instance i): data_(i) {}
        Value(BoundMethod b): data_(b) {}
        Value(Module m): data_(m) {}

        [[nodiscard]] inline bool is_null() const noexcept { return std::holds_alternative<Null>(data_); }
        [[nodiscard]] inline bool is_bool() const noexcept { return std::holds_alternative<Bool>(data_); }
        [[nodiscard]] inline bool is_int() const noexcept { return std::holds_alternative<Int>(data_); }
        [[nodiscard]] inline bool is_real() const noexcept { return std::holds_alternative<Real>(data_); }
        [[nodiscard]] inline bool is_array() const noexcept { return std::holds_alternative<Array>(data_); }
        [[nodiscard]] inline bool is_string() const noexcept { return std::holds_alternative<String>(data_); }
        [[nodiscard]] inline bool is_hash() const noexcept { return std::holds_alternative<Hash>(data_); }
        [[nodiscard]] inline bool is_upvalue() const noexcept { return std::holds_alternative<Upvalue>(data_); }
        [[nodiscard]] inline bool is_proto() const noexcept { return std::holds_alternative<Proto>(data_); }
        [[nodiscard]] inline bool is_function() const noexcept { return std::holds_alternative<Function>(data_); }
        [[nodiscard]] inline bool is_native_fn() const noexcept { return std::holds_alternative<NativeFn>(data_); }
        [[nodiscard]] inline bool is_class() const noexcept { return std::holds_alternative<Class>(data_); }
        [[nodiscard]] inline bool is_instance() const noexcept { return std::holds_alternative<Instance>(data_); }
        [[nodiscard]] inline bool is_bound_method() const noexcept { return std::holds_alternative<BoundMethod>(data_); }
        [[nodiscard]] inline bool is_module() const noexcept { return std::holds_alternative<Module>(data_); }
        [[nodiscard]] inline bool is_object() const noexcept {
            return std::visit([](auto&& arg) -> bool {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (
                    std::is_same_v<T, String> || std::is_same_v<T, Array> ||
                    std::is_same_v<T, Hash> || std::is_same_v<T, Upvalue> ||
                    std::is_same_v<T, Proto> || std::is_same_v<T, Function> ||
                    std::is_same_v<T, NativeFn> || std::is_same_v<T, Class> ||
                    std::is_same_v<T, Instance> || std::is_same_v<T, BoundMethod> ||
                    std::is_same_v<T, Module>
                ) {
                    return true;
                }
                return false;
            }, data_);
        }

        [[nodiscard]] inline bool as_bool() const { return std::get<Bool>(data_); }
        [[nodiscard]] inline int64_t as_int() const { return std::get<Int>(data_); }
        [[nodiscard]] inline double as_real() const { return std::get<Real>(data_); }
        [[nodiscard]] inline Array as_array() const { return std::get<Array>(data_); }
        [[nodiscard]] inline String as_string() const { return std::get<String>(data_); }
        [[nodiscard]] inline Hash as_hash() const { return std::get<Hash>(data_); }
        [[nodiscard]] inline Upvalue as_upvalue() const { return std::get<Upvalue>(data_); }
        [[nodiscard]] inline Proto as_proto() const { return std::get<Proto>(data_); }
        [[nodiscard]] inline Function as_function() const { return std::get<Function>(data_); }
        [[nodiscard]] inline NativeFn as_native_fn() const { return std::get<NativeFn>(data_); }
        [[nodiscard]] inline Class as_class() const { return std::get<Class>(data_); }
        [[nodiscard]] inline Instance as_instance() const { return std::get<Instance>(data_); }
        [[nodiscard]] inline BoundMethod as_bound_method() const { return std::get<BoundMethod>(data_); }
        [[nodiscard]] inline Module as_module() const { return std::get<Module>(data_); }
        [[nodiscard]] inline meow::core::MeowObject* as_object() const {
            meow::core::MeowObject* object = nullptr;
            std::visit([&object](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_pointer_v<T> && std::is_base_of_v<meow::core::MeowObject, std::remove_pointer_t<T>>) {
                    object = arg;
                }
            }, data_);
            return object;
        }
        [[nodiscard]] inline meow::core::MeowObject* as_object() const {
            return std::visit([](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_pointer_v<T>) return static_cast<MeowObject*>(arg);
                return nullptr;
            }, data_);
        }

        inline const bool* as_if_bool() const noexcept { return std::get_if<Bool>(&data_); }
        inline const int64_t* as_if_int() const noexcept { return std::get_if<Int>(&data_); }
        inline const double* as_if_real() const noexcept { return std::get_if<Real>(&data_); }
        inline meow::core::MeowObject* const* as_if_object() const noexcept {
            static meow::core::MeowObject* object = nullptr;
            if (is_object()) {
                object = as_object();
                return &object;
            }
            return nullptr;
        }

        inline bool* as_if_bool() noexcept { return std::get_if<Bool>(&data_); }
        inline int64_t* as_if_int() noexcept { return std::get_if<Int>(&data_); }
        inline double* as_if_real() noexcept { return std::get_if<Real>(&data_); }
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