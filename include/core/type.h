#pragma once

#include "common/pch.h"
#include "utils/types/variant.h"

namespace meow::core {
    struct MeowObject;

    namespace objects {
        struct ObjString;
        struct ObjArray;
        struct ObjHashTable;
        struct ObjClass;
        struct ObjInstance;
        struct ObjBoundMethod;
        struct ObjUpvalue;
        struct ObjFunctionProto;
        struct ObjNativeFunction;
        struct ObjClosure;
        struct ObjModule;
    }
    
    using array_t = objects::ObjArray*;
    using string_t = const objects::ObjString*;
    using hash_table_t = objects::ObjHashTable*;

    using instance_t = objects::ObjInstance*;
    using class_t = objects::ObjClass*;
    using bound_method_t = objects::ObjBoundMethod*;
    using upvalue_t = objects::ObjUpvalue*;
    using proto_t = objects::ObjFunctionProto*;
    using function_t = objects::ObjClosure*;
    using native_fn_t = objects::ObjNativeFunction*;
    using module_t = objects::ObjModule*;

    using null_t = std::monostate;
    using bool_t = bool;
    using int_t = int64_t;
    using float_t = double;
    using object_t = meow::variant<
        array_t, string_t, hash_table_t,
        instance_t, class_t, bound_method_t,
        upvalue_t, proto_t, function_t,
        native_fn_t, module_t
    >;

    enum class ValueType : uint8_t {
        Null, Int, Float, Bool, String, Array, Object, Upvalue, Function, Class, Instance, BoundMethod, Proto, NativeFn, TotalValueTypes
    };
}