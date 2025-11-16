/**
 * @file value.h
 * @author LazyPaws
 * @brief Core definition of Value in TrangMeo
 * @copyright Copyright (c) 2025 LazyPaws
 * @license All rights reserved. Unauthorized copying of this file, in any form
 * or medium, is strictly prohibited
 */

#pragma once

#include "common/pch.h"
#include "core/type.h"
#include "utils/types/variant.h"
#include "core/meow_object.h"
#include "core/object_traits.h"

namespace meow::core {

using BaseValue = meow::variant<null_t, bool_t, int_t, float_t, object_t>;

class Value;

template <typename T>
concept ValueAssignable = std::is_constructible_v<BaseValue, T>;

class Value {
   private:
    BaseValue data_;
    [[nodiscard]] inline meow::core::MeowObject* get_object_ptr() const noexcept {
        if (auto obj_ptr = data_.get_if<object_t>()) {
            return *obj_ptr;
        }
        return nullptr;
    }

   public:
    // --- Constructors ---
    inline Value() noexcept : data_(null_t{}) {}
    
    template <ValueAssignable T>
    inline Value(T&& value) noexcept(noexcept(data_ = std::forward<T>(value)))
        : data_(std::forward<T>(value)) {}

    // --- Rule of five ---
    inline Value(const Value& other) noexcept : data_(other.data_) {}
    inline Value(Value&& other) noexcept : data_(std::move(other.data_)) {}
    
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
    inline ~Value() noexcept = default;

    template <ValueAssignable T>
    inline Value& operator=(T&& value) noexcept(noexcept(data_ = std::forward<T>(value))) {
        data_ = std::forward<T>(value);
        return *this;
    }

    // === Các hàm kiểm tra kiểu (Type Checkers) ===

    // --- Kiểu nguyên thủy ---
    [[nodiscard]] inline bool is_null() const noexcept { return data_.holds<null_t>(); }
    [[nodiscard]] inline bool is_bool() const noexcept { return data_.holds<bool_t>(); }
    [[nodiscard]] inline bool is_int() const noexcept { return data_.holds<int_t>(); }
    [[nodiscard]] inline bool is_float() const noexcept { return data_.holds<float_t>(); }

    // --- Kiểu Object (Chung) ---
    [[nodiscard]] inline bool is_object() const noexcept {
        return get_object_ptr() != nullptr;
    }

    // --- Kiểu Object (Cụ thể, dùng Type Tag) ---
    [[nodiscard]] inline bool is_array() const noexcept {
        const MeowObject* obj = get_object_ptr();
        return (obj && obj->get_type() == ObjectType::ARRAY);
    }
    [[nodiscard]] inline bool is_string() const noexcept {
        const MeowObject* obj = get_object_ptr();
        return (obj && obj->get_type() == ObjectType::STRING);
    }
    [[nodiscard]] inline bool is_hash_table() const noexcept {
        const MeowObject* obj = get_object_ptr();
        return (obj && obj->get_type() == ObjectType::HASH_TABLE);
    }
    [[nodiscard]] inline bool is_upvalue() const noexcept {
        const MeowObject* obj = get_object_ptr();
        return (obj && obj->get_type() == ObjectType::UPVALUE);
    }
    [[nodiscard]] inline bool is_proto() const noexcept {
        const MeowObject* obj = get_object_ptr();
        return (obj && obj->get_type() == ObjectType::PROTO);
    }
    [[nodiscard]] inline bool is_function() const noexcept {
        const MeowObject* obj = get_object_ptr();
        return (obj && obj->get_type() == ObjectType::FUNCTION);
    }
    [[nodiscard]] inline bool is_native_fn() const noexcept {
        const MeowObject* obj = get_object_ptr();
        return (obj && obj->get_type() == ObjectType::NATIVE_FN);
    }
    [[nodiscard]] inline bool is_class() const noexcept {
        const MeowObject* obj = get_object_ptr();
        return (obj && obj->get_type() == ObjectType::CLASS);
    }
    [[nodiscard]] inline bool is_instance() const noexcept {
        const MeowObject* obj = get_object_ptr();
        return (obj && obj->get_type() == ObjectType::INSTANCE);
    }
    [[nodiscard]] inline bool is_bound_method() const noexcept {
        const MeowObject* obj = get_object_ptr();
        return (obj && obj->get_type() == ObjectType::BOUND_METHOD);
    }
    [[nodiscard]] inline bool is_module() const noexcept {
        const MeowObject* obj = get_object_ptr();
        return (obj && obj->get_type() == ObjectType::MODULE);
    }


    // === Các hàm lấy giá trị (Accessors) ===
    // (Chỉ gọi sau khi đã is_...() - không an toàn)

    [[nodiscard]] inline bool as_bool() const noexcept { return data_.get<bool_t>(); }
    [[nodiscard]] inline int64_t as_int() const noexcept { return data_.get<int_t>(); }
    [[nodiscard]] inline double as_float() const noexcept { return data_.get<float_t>(); }
    
    // Trả về con trỏ cha MeowObject*
    [[nodiscard]] inline meow::core::MeowObject* as_object() const noexcept {
        return data_.get<object_t>();
    }

    // Ép kiểu (cast) con trỏ cha thành con trỏ cụ thể
    // (Rất nhanh, chỉ là static_cast, nhưng phải đảm bảo kiểu đúng)
    [[nodiscard]] inline array_t as_array() const noexcept {
        return static_cast<array_t>(as_object());
    }
    [[nodiscard]] inline string_t as_string() const noexcept {
        return static_cast<string_t>(as_object());
    }
    [[nodiscard]] inline hash_table_t as_hash_table() const noexcept {
        return static_cast<hash_table_t>(as_object());
    }
    [[nodiscard]] inline upvalue_t as_upvalue() const noexcept {
        return static_cast<upvalue_t>(as_object());
    }
    [[nodiscard]] inline proto_t as_proto() const noexcept {
        return static_cast<proto_t>(as_object());
    }
    [[nodiscard]] inline function_t as_function() const noexcept {
        return static_cast<function_t>(as_object());
    }
    [[nodiscard]] inline native_fn_t as_native_fn() const noexcept {
        return static_cast<native_fn_t>(as_object());
    }
    [[nodiscard]] inline class_t as_class() const noexcept {
        return static_cast<class_t>(as_object());
    }
    [[nodiscard]] inline instance_t as_instance() const noexcept {
        return static_cast<instance_t>(as_object());
    }
    [[nodiscard]] inline bound_method_t as_bound_method() const noexcept {
        return static_cast<bound_method_t>(as_object());
    }
    [[nodiscard]] inline module_t as_module() const noexcept {
        return static_cast<module_t>(as_object());
    }

    // === Các hàm lấy an toàn (Safe Getters) ===
    // (Tương tự get_if của meow::variant)

    [[nodiscard]] inline const bool* as_if_bool() const noexcept { return data_.get_if<bool_t>(); }
    [[nodiscard]] inline const int64_t* as_if_int() const noexcept { return data_.get_if<int_t>(); }
    [[nodiscard]] inline const double* as_if_float() const noexcept { return data_.get_if<float_t>(); }

    [[nodiscard]] inline bool* as_if_bool() noexcept { return data_.get_if<bool_t>(); }
    [[nodiscard]] inline int64_t* as_if_int() noexcept { return data_.get_if<int_t>(); }
    [[nodiscard]] inline double* as_if_float() noexcept { return data_.get_if<float_t>(); }

    template <typename T>
    [[nodiscard]] inline T as_if() noexcept {
        using BaseType = std::remove_pointer_t<T>;
        
        if (auto obj = get_object_ptr()) {
            constexpr ObjectType expected_tag = detail::object_traits<BaseType>::type_tag;

            if (obj->get_type() == expected_tag) {
                return static_cast<T>(obj);
            }
        }
        return nullptr;
    }

    template <typename T>
    [[nodiscard]] inline T as_if() const noexcept {
        using BaseType = std::remove_const_t<std::remove_pointer_t<T>>;
        
        if (auto obj = get_object_ptr()) {
            constexpr ObjectType expected_tag = detail::object_traits<BaseType>::type_tag;

            if (obj->get_type() == expected_tag) {
                return static_cast<T>(obj);
            }
        }
        return nullptr;
    }
    // === Visitor ===
    template <typename Visitor>
    decltype(auto) visit(Visitor&& vis) {
        return data_.visit(vis);
    }

    template <typename Visitor>
    decltype(auto) visit(Visitor&& vis) const {
        return data_.visit(vis);
    }

    template <typename... Fs>
    decltype(auto) visit(Fs&&... fs) {
        return data_.visit(std::forward<Fs>(fs)...);
    }

    template <typename... Fs>
    decltype(auto) visit(Fs&&... fs) const {
        return data_.visit(std::forward<Fs>(fs)...);
    }
};
}  // namespace meow::core