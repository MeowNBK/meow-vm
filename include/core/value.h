#pragma once

#include "common/pch.h"
#include "core/type.h"
#include "utils/types/variant.h"
#include "core/meow_object.h"
#include "core/object_traits.h"

namespace meow::core {

using base_t = meow::variant<null_t, bool_t, int_t, float_t, object_t>;

class Value {
private:
    base_t data_;
    [[nodiscard]] inline meow::core::MeowObject* get_object_ptr() const noexcept {
        if (auto obj_ptr = data_.get_if<object_t>()) {
            return *obj_ptr;
        }
        return nullptr;
    }

public:
    // --- Constructors ---
    inline Value() noexcept : data_(null_t{}) {}
    inline Value(null_t v) noexcept : data_(std::move(v)) {}
    inline Value(bool_t v) noexcept : data_(v) {}
    inline Value(int_t v) noexcept : data_(v) {}
    inline Value(float_t v) noexcept : data_(v) {}
    inline Value(object_t v) noexcept : data_(v) {}

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

    // --- Assignment operators ---
    inline Value& operator=(null_t v) noexcept { data_ = std::move(v); return *this; }
    inline Value& operator=(bool_t v) noexcept { data_ = v; return *this; }
    inline Value& operator=(int_t v) noexcept { data_ = v; return *this; }
    inline Value& operator=(float_t v) noexcept { data_ = v; return *this; }
    inline Value& operator=(object_t v) noexcept { data_ = v; return *this; }

    inline constexpr size_t index() const noexcept {
        return data_.index();
    }

    // === Type Checkers ===

    // --- Primary type ---
    [[nodiscard]] inline bool is_null() const noexcept { return data_.holds<null_t>(); }
    [[nodiscard]] inline bool is_bool() const noexcept { return data_.holds<bool_t>(); }
    [[nodiscard]] inline bool is_int() const noexcept { return data_.holds<int_t>(); }
    [[nodiscard]] inline bool is_float() const noexcept { return data_.holds<float_t>(); }

    // --- Object type (genreric) ---
    [[nodiscard]] inline bool is_object() const noexcept {
        return get_object_ptr() != nullptr;
    }

    // --- Specific object type ---
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

    // === Accessors ===
    [[nodiscard]] inline bool as_bool() const noexcept { return data_.get<bool_t>(); }
    [[nodiscard]] inline int64_t as_int() const noexcept { return data_.get<int_t>(); }
    [[nodiscard]] inline double as_float() const noexcept { return data_.get<float_t>(); }
    
    [[nodiscard]] inline meow::core::MeowObject* as_object() const noexcept {
        return data_.get<object_t>();
    }
    [[nodiscard]] inline array_t as_array() const noexcept {
        return reinterpret_cast<array_t>(as_object());
    }
    [[nodiscard]] inline string_t as_string() const noexcept {
        return reinterpret_cast<string_t>(as_object());
    }
    [[nodiscard]] inline hash_table_t as_hash_table() const noexcept {
        return reinterpret_cast<hash_table_t>(as_object());
    }
    [[nodiscard]] inline upvalue_t as_upvalue() const noexcept {
        return reinterpret_cast<upvalue_t>(as_object());
    }
    [[nodiscard]] inline proto_t as_proto() const noexcept {
        return reinterpret_cast<proto_t>(as_object());
    }
    [[nodiscard]] inline function_t as_function() const noexcept {
        return reinterpret_cast<function_t>(as_object());
    }
    [[nodiscard]] inline native_fn_t as_native_fn() const noexcept {
        return reinterpret_cast<native_fn_t>(as_object());
    }
    [[nodiscard]] inline class_t as_class() const noexcept {
        return reinterpret_cast<class_t>(as_object());
    }
    [[nodiscard]] inline instance_t as_instance() const noexcept {
        return reinterpret_cast<instance_t>(as_object());
    }
    [[nodiscard]] inline bound_method_t as_bound_method() const noexcept {
        return reinterpret_cast<bound_method_t>(as_object());
    }
    [[nodiscard]] inline module_t as_module() const noexcept {
        return reinterpret_cast<module_t>(as_object());
    }

    // === Safe Getters ===
    [[nodiscard]] inline const bool* as_if_bool() const noexcept { return data_.get_if<bool_t>(); }
    [[nodiscard]] inline const int64_t* as_if_int() const noexcept { return data_.get_if<int_t>(); }
    [[nodiscard]] inline const double* as_if_float() const noexcept { return data_.get_if<float_t>(); }

    [[nodiscard]] inline bool* as_if_bool() noexcept { return data_.get_if<bool_t>(); }
    [[nodiscard]] inline int64_t* as_if_int() noexcept { return data_.get_if<int_t>(); }
    [[nodiscard]] inline double* as_if_float() noexcept { return data_.get_if<float_t>(); }

    // Array
    [[nodiscard]] inline array_t as_if_array() noexcept {
        if (auto obj = get_object_ptr()) {
            if (obj->get_type() == ObjectType::ARRAY) {
                return reinterpret_cast<array_t>(obj);
            }
        }
        return nullptr;
    }
    [[nodiscard]] inline array_t as_if_array() const noexcept {
        if (auto obj = get_object_ptr()) {
            if (obj->get_type() == ObjectType::ARRAY) {
                return reinterpret_cast<array_t>(obj);
            }
        }
        return nullptr;
    }

    // String
    [[nodiscard]] inline string_t as_if_string() noexcept {
        if (auto obj = get_object_ptr()) {
            if (obj->get_type() == ObjectType::STRING) {
                return reinterpret_cast<string_t>(obj);
            }
        }
        return nullptr;
    }
    [[nodiscard]] inline string_t as_if_string() const noexcept {
        if (auto obj = get_object_ptr()) {
            if (obj->get_type() == ObjectType::STRING) {
                return reinterpret_cast<string_t>(obj);
            }
        }
        return nullptr;
    }

    // Hash table
    [[nodiscard]] inline hash_table_t as_if_hash_table() noexcept {
        if (auto obj = get_object_ptr()) {
            if (obj->get_type() == ObjectType::HASH_TABLE) {
                return reinterpret_cast<hash_table_t>(obj);
            }
        }
        return nullptr;
    }
    [[nodiscard]] inline hash_table_t as_if_hash_table() const noexcept {
        if (auto obj = get_object_ptr()) {
            if (obj->get_type() == ObjectType::HASH_TABLE) {
                return reinterpret_cast<hash_table_t>(obj);
            }
        }
        return nullptr;
    }

    // Upvalue
    [[nodiscard]] inline upvalue_t as_if_upvalue() noexcept {
        if (auto obj = get_object_ptr()) {
            if (obj->get_type() == ObjectType::UPVALUE) {
                return reinterpret_cast<upvalue_t>(obj);
            }
        }
        return nullptr;
    }
    [[nodiscard]] inline upvalue_t as_if_upvalue() const noexcept {
        if (auto obj = get_object_ptr()) {
            if (obj->get_type() == ObjectType::UPVALUE) {
                return reinterpret_cast<upvalue_t>(obj);
            }
        }
        return nullptr;
    }

    // Proto
    [[nodiscard]] inline proto_t as_if_proto() noexcept {
        if (auto obj = get_object_ptr()) {
            if (obj->get_type() == ObjectType::PROTO) {
                return reinterpret_cast<proto_t>(obj);
            }
        }
        return nullptr;
    }
    [[nodiscard]] inline proto_t as_if_proto() const noexcept {
        if (auto obj = get_object_ptr()) {
            if (obj->get_type() == ObjectType::PROTO) {
                return reinterpret_cast<proto_t>(obj);
            }
        }
        return nullptr;
    }

    // Function
    [[nodiscard]] inline function_t as_if_function() noexcept {
        if (auto obj = get_object_ptr()) {
            if (obj->get_type() == ObjectType::FUNCTION) {
                return reinterpret_cast<function_t>(obj);
            }
        }
        return nullptr;
    }
    [[nodiscard]] inline function_t as_if_function() const noexcept {
        if (auto obj = get_object_ptr()) {
            if (obj->get_type() == ObjectType::FUNCTION) {
                return reinterpret_cast<function_t>(obj);
            }
        }
        return nullptr;
    }

    // Native function
    [[nodiscard]] inline native_fn_t as_if_native_fn() noexcept {
        if (auto obj = get_object_ptr()) {
            if (obj->get_type() == ObjectType::NATIVE_FN) {
                return reinterpret_cast<native_fn_t>(obj);
            }
        }
        return nullptr;
    }
    [[nodiscard]] inline native_fn_t as_if_native_fn() const noexcept {
        if (auto obj = get_object_ptr()) {
            if (obj->get_type() == ObjectType::NATIVE_FN) {
                return reinterpret_cast<native_fn_t>(obj);
            }
        }
        return nullptr;
    }

    // Class
    [[nodiscard]] inline class_t as_if_class() noexcept {
        if (auto obj = get_object_ptr()) {
            if (obj->get_type() == ObjectType::CLASS) {
                return reinterpret_cast<class_t>(obj);
            }
        }
        return nullptr;
    }
    [[nodiscard]] inline class_t as_if_class() const noexcept {
        if (auto obj = get_object_ptr()) {
            if (obj->get_type() == ObjectType::CLASS) {
                return reinterpret_cast<class_t>(obj);
            }
        }
        return nullptr;
    }

    // Instance
    [[nodiscard]] inline instance_t as_if_instance() noexcept {
        if (auto obj = get_object_ptr()) {
            if (obj->get_type() == ObjectType::INSTANCE) {
                return reinterpret_cast<instance_t>(obj);
            }
        }
        return nullptr;
    }
    [[nodiscard]] inline instance_t as_if_instance() const noexcept {
        if (auto obj = get_object_ptr()) {
            if (obj->get_type() == ObjectType::INSTANCE) {
                return reinterpret_cast<instance_t>(obj);
            }
        }
        return nullptr;
    }

    // Bound method
    [[nodiscard]] inline bound_method_t as_if_bound_method() noexcept {
        if (auto obj = get_object_ptr()) {
            if (obj->get_type() == ObjectType::BOUND_METHOD) {
                return reinterpret_cast<bound_method_t>(obj);
            }
        }
        return nullptr;
    }
    [[nodiscard]] inline bound_method_t as_if_bound_method() const noexcept {
        if (auto obj = get_object_ptr()) {
            if (obj->get_type() == ObjectType::BOUND_METHOD) {
                return reinterpret_cast<bound_method_t>(obj);
            }
        }
        return nullptr;
    }

    // Module
    [[nodiscard]] inline module_t as_if_module() noexcept {
        if (auto obj = get_object_ptr()) {
            if (obj->get_type() == ObjectType::MODULE) {
                return reinterpret_cast<module_t>(obj);
            }
        }
        return nullptr;
    }
    [[nodiscard]] inline module_t as_if_module() const noexcept {
        if (auto obj = get_object_ptr()) {
            if (obj->get_type() == ObjectType::MODULE) {
                return reinterpret_cast<module_t>(obj);
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
