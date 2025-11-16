#include "core/value.h"
#include "core/objects.h"


namespace meow::core {

[[nodiscard]] inline bool Value::is_array() const noexcept {
    const MeowObject* obj = get_object_ptr();
    return (obj && obj->get_type() == ObjectType::ARRAY);
}
[[nodiscard]] inline bool Value::is_string() const noexcept {
    const MeowObject* obj = get_object_ptr();
    return (obj && obj->get_type() == ObjectType::STRING);
}
[[nodiscard]] inline bool Value::is_hash_table() const noexcept {
    const MeowObject* obj = get_object_ptr();
    return (obj && obj->get_type() == ObjectType::HASH_TABLE);
}
[[nodiscard]] inline bool Value::is_upvalue() const noexcept {
    const MeowObject* obj = get_object_ptr();
    return (obj && obj->get_type() == ObjectType::UPVALUE);
}
[[nodiscard]] inline bool Value::is_proto() const noexcept {
    const MeowObject* obj = get_object_ptr();
    return (obj && obj->get_type() == ObjectType::PROTO);
}
[[nodiscard]] inline bool Value::is_function() const noexcept {
    const MeowObject* obj = get_object_ptr();
    return (obj && obj->get_type() == ObjectType::FUNCTION);
}
[[nodiscard]] inline bool Value::is_native_fn() const noexcept {
    const MeowObject* obj = get_object_ptr();
    return (obj && obj->get_type() == ObjectType::NATIVE_FN);
}
[[nodiscard]] inline bool Value::is_class() const noexcept {
    const MeowObject* obj = get_object_ptr();
    return (obj && obj->get_type() == ObjectType::CLASS);
}
[[nodiscard]] inline bool Value::is_instance() const noexcept {
    const MeowObject* obj = get_object_ptr();
    return (obj && obj->get_type() == ObjectType::INSTANCE);
}
[[nodiscard]] inline bool Value::is_bound_method() const noexcept {
    const MeowObject* obj = get_object_ptr();
    return (obj && obj->get_type() == ObjectType::BOUND_METHOD);
}
[[nodiscard]] inline bool Value::is_module() const noexcept {
    const MeowObject* obj = get_object_ptr();
    return (obj && obj->get_type() == ObjectType::MODULE);
}

// --- Accessors (Định nghĩa) ---
[[nodiscard]] inline array_t Value::as_array() const noexcept {
    return static_cast<array_t>(as_object());
}
[[nodiscard]] inline string_t Value::as_string() const noexcept {
    return static_cast<string_t>(as_object());
}
[[nodiscard]] inline hash_table_t Value::as_hash_table() const noexcept {
    return static_cast<hash_table_t>(as_object());
}
[[nodiscard]] inline upvalue_t Value::as_upvalue() const noexcept {
    return static_cast<upvalue_t>(as_object());
}
[[nodiscard]] inline proto_t Value::as_proto() const noexcept {
    return static_cast<proto_t>(as_object());
}
[[nodiscard]] inline function_t Value::as_function() const noexcept {
    return static_cast<function_t>(as_object());
}
[[nodiscard]] inline native_fn_t Value::as_native_fn() const noexcept {
    return static_cast<native_fn_t>(as_object());
}
[[nodiscard]] inline class_t Value::as_class() const noexcept {
    return static_cast<class_t>(as_object());
}
[[nodiscard]] inline instance_t Value::as_instance() const noexcept {
    return static_cast<instance_t>(as_object());
}
[[nodiscard]] inline bound_method_t Value::as_bound_method() const noexcept {
    return static_cast<bound_method_t>(as_object());
}
[[nodiscard]] inline module_t Value::as_module() const noexcept {
    return static_cast<module_t>(as_object());
}

// --- Safe Getters (Định nghĩa) ---
template <typename T>
[[nodiscard]] inline T Value::as_if() noexcept {
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
[[nodiscard]] inline T Value::as_if() const noexcept {
    using BaseType = std::remove_const_t<std::remove_pointer_t<T>>;
    
    if (auto obj = get_object_ptr()) {
        constexpr ObjectType expected_tag = detail::object_traits<BaseType>::type_tag;

        if (obj->get_type() == expected_tag) {
            return static_cast<T>(obj);
        }
    }
    return nullptr;
}

}  // namespace meow::core // <-- ĐÓNG NAMESPACE SAU KHI ĐỊNH NGHĨA XONG