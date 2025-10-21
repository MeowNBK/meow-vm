#pragma once

#include "common/pch.h"
#include "utils/types/variant.h"

namespace meow::core {
struct MeowObject;

namespace objects {
class ObjString;
class ObjArray;
class ObjHashTable;
class ObjClass;
class ObjInstance;
class ObjBoundMethod;
class ObjUpvalue;
class ObjFunctionProto;
class ObjNativeFunction;
class ObjClosure;
class ObjModule;
}  // namespace objects

using array_t = objects::ObjArray *;
using string_t = const objects::ObjString *;
using hash_table_t = objects::ObjHashTable *;

using instance_t = objects::ObjInstance *;
using class_t = objects::ObjClass *;
using bound_method_t = objects::ObjBoundMethod *;
using upvalue_t = objects::ObjUpvalue *;
using proto_t = objects::ObjFunctionProto *;
using function_t = objects::ObjClosure *;
using native_fn_t = objects::ObjNativeFunction *;
using module_t = objects::ObjModule *;

using null_t = std::monostate;
using bool_t = bool;
using int_t = int64_t;
using float_t = double;
using object_t = meow::variant<array_t, string_t, hash_table_t, instance_t, class_t, bound_method_t,
                               upvalue_t, proto_t, function_t, native_fn_t, module_t>;

enum class ValueType : uint8_t {
    Null,
    Int,
    Float,
    Bool,
    String,
    Array,
    Object,
    Upvalue,
    Function,
    Class,
    Instance,
    BoundMethod,
    Proto,
    NativeFn,
    TotalValueTypes
};
}  // namespace meow::core