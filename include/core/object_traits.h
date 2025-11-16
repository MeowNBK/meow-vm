// include/core/object_traits.h

#pragma once

#include "core/meow_object.h"
#include "core/type.h"

namespace meow::core {

namespace detail {

template <typename T>
struct object_traits {
    static_assert(std::is_base_of_v<MeowObject, T>, 
                  "as_if<T> only works on types derived from MeowObject");
};

template <> struct object_traits<objects::ObjArray> {
    static constexpr ObjectType type_tag = ObjectType::ARRAY;
};
template <> struct object_traits<objects::ObjString> {
    static constexpr ObjectType type_tag = ObjectType::STRING;
};
template <> struct object_traits<objects::ObjHashTable> {
    static constexpr ObjectType type_tag = ObjectType::HASH_TABLE;
};
template <> struct object_traits<objects::ObjInstance> {
    static constexpr ObjectType type_tag = ObjectType::INSTANCE;
};
template <> struct object_traits<objects::ObjClass> {
    static constexpr ObjectType type_tag = ObjectType::CLASS;
};
template <> struct object_traits<objects::ObjBoundMethod> {
    static constexpr ObjectType type_tag = ObjectType::BOUND_METHOD;
};
template <> struct object_traits<objects::ObjUpvalue> {
    static constexpr ObjectType type_tag = ObjectType::UPVALUE;
};
template <> struct object_traits<objects::ObjFunctionProto> {
    static constexpr ObjectType type_tag = ObjectType::PROTO;
};
template <> struct object_traits<objects::ObjClosure> {
    static constexpr ObjectType type_tag = ObjectType::FUNCTION;
};
template <> struct object_traits<objects::ObjNativeFunction> {
    static constexpr ObjectType type_tag = ObjectType::NATIVE_FN;
};
template <> struct object_traits<objects::ObjModule> {
    static constexpr ObjectType type_tag = ObjectType::MODULE;
};

} // namespace detail
} // namespace meow::core

// #pragma once

// #include "core/meow_object.h"
// #include "core/objects.h"

// namespace meow::core {

// namespace detail {

// template <typename T>
// struct object_traits {
//     static_assert(std::is_base_of_v<MeowObject, T>, 
//                   "as_if<T> only works on types derived from MeowObject");
// };

// template <> struct object_traits<objects::ObjArray> {
//     static constexpr ObjectType type_tag = ObjectType::ARRAY;
// };
// template <> struct object_traits<objects::ObjString> {
//     static constexpr ObjectType type_tag = ObjectType::STRING;
// };
// template <> struct object_traits<objects::ObjHashTable> {
//     static constexpr ObjectType type_tag = ObjectType::HASH_TABLE;
// };
// template <> struct object_traits<objects::ObjInstance> {
//     static constexpr ObjectType type_tag = ObjectType::INSTANCE;
// };
// template <> struct object_traits<objects::ObjClass> {
//     static constexpr ObjectType type_tag = ObjectType::CLASS;
// };
// template <> struct object_traits<objects::ObjBoundMethod> {
//     static constexpr ObjectType type_tag = ObjectType::BOUND_METHOD;
// };
// template <> struct object_traits<objects::ObjUpvalue> {
//     static constexpr ObjectType type_tag = ObjectType::UPVALUE;
// };
// template <> struct object_traits<objects::ObjFunctionProto> {
//     static constexpr ObjectType type_tag = ObjectType::PROTO;
// };
// template <> struct object_traits<objects::ObjClosure> {
//     static constexpr ObjectType type_tag = ObjectType::FUNCTION;
// };
// template <> struct object_traits<objects::ObjNativeFunction> {
//     static constexpr ObjectType type_tag = ObjectType::NATIVE_FN;
// };
// template <> struct object_traits<objects::ObjModule> {
//     static constexpr ObjectType type_tag = ObjectType::MODULE;
// };

// } // namespace detail
// } // namespace meow::core