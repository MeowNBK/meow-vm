/**
 * @file native.h
 * @author LazyPaws
 * @brief Core definition of NativeFn in TrangMeo
 * @copyright Copyright (c) 2025 LazyPaws
 * @license All rights reserved. Unauthorized copying of this file, in any form
 * or medium, is strictly prohibited
 */

#pragma once

#include "common/pch.h"
#include "core/definitions.h"
#include "core/meow_object.h"
#include "core/value.h"

#include "utils/types/variant.h"

namespace meow::vm {
class MeowEngine;
}

namespace meow::core::objects {
class ObjNativeFunction : public meow::core::ObjBase<ObjectType::NATIVE_FN> {
   private:
    using engine_t = meow::vm::MeowEngine;
    using visitor_t = meow::memory::GCVisitor;

   public:
    using native_fn_simple = std::function<meow::core::return_t(meow::core::arguments_t)>;
    using native_fn_double = std::function<meow::core::return_t(engine_t*, meow::core::arguments_t)>;

   private:
    std::variant<native_fn_simple, native_fn_double> function_;

   public:
    explicit ObjNativeFunction(native_fn_simple f) : function_(f) {
    }
    explicit ObjNativeFunction(native_fn_double f) : function_(f) {
    }

    [[nodiscard]] inline meow::core::return_t call(meow::core::arguments_t args) {
        if (auto p = std::get_if<native_fn_simple>(&function_)) {
            return (*p)(args);
        }
        return meow::core::value_t();
    }
    [[nodiscard]] inline meow::core::return_t call(engine_t* engine, meow::core::arguments_t args) {
        if (auto p = std::get_if<native_fn_double>(&function_)) {
            return (*p)(engine, args);
        } else if (auto p = std::get_if<native_fn_simple>(&function_)) {
            return (*p)(args);
        }
        return meow::core::value_t();
    }

    void trace(visitor_t&) const noexcept override {
    }
};
}  // namespace meow::core::objects