#pragma once

#include "common/pch.h"

namespace meow::core {
    struct MeowObject;

    namespace objects {
        struct ObjString;
        struct ObjArray;
        struct ObjHash;
        struct ObjClass;
        struct ObjInstance;
        struct ObjBoundMethod;
        struct ObjUpvalue;
        struct ObjFunctionProto;
        struct ObjNativeFunction;
        struct ObjClosure;
        struct ObjModule;
    }
    
    using Array = objects::ObjArray*;
    using String = const objects::ObjString*;
    using Hash = objects::ObjHash*;

    using Instance = objects::ObjInstance*;
    using Class = objects::ObjClass*;
    using BoundMethod = objects::ObjBoundMethod*;
    using Upvalue = objects::ObjUpvalue*;
    using Proto = objects::ObjFunctionProto*;
    using Function = objects::ObjClosure*;
    using NativeFn = objects::ObjNativeFunction*;
    using Module = objects::ObjModule*;

    using Null = std::monostate;
    using Bool = bool;
    using Int = int64_t;
    using Real = double;
    using Object = MeowObject*;

    enum class ValueType : uint8_t {
        Null, Int, Real, Bool, String, Array, Object, Upvalue, Function, Class, Instance, BoundMethod, Proto, NativeFn, TotalValueTypes
    };
}