/**
 * @file native.h
 * @author LazyPaws
 * @brief Core definition of NativeFn in TrangMeo
 * @copyright Copyright (c) 2025 LazyPaws
 * @license All rights reserved. Unauthorized copying of this file, in any form or medium, is strictly prohibited
 */

#pragma once

#include "common/pch.h"
#include "core/value.h"
#include "core/meow_object.h"

namespace meow::vm { class MeowEngine; }

namespace meow::core::objects {
    class ObjNativeFunction : public MeowObject {
    private:
        using value_t = meow::core::Value;
        using engine_t = meow::vm::MeowEngine;
    public:
        using arguments = const std::vector<value_t>&;
        using native_fn_simple = std::function<value_t(arguments)>;
        using native_fn_double = std::function<value_t(engine_t*, arguments)>;
    private:
        std::variant<native_fn_simple, native_fn_double> function_;
    public:
        explicit ObjNativeFunction(native_fn_simple f): function_(f) {}
        explicit ObjNativeFunction(native_fn_double f): function_(f) {}

        [[nodiscard]] inline value_t call(arguments args) {
            if (auto p = std::get_if<native_fn_simple>(&function_)) {
                return (*p)(args);
            }
            return value_t();
        }
        [[nodiscard]] inline value_t call(engine_t* engine, arguments args) {
            if (auto p = std::get_if<native_fn_double>(&function_)) {
                return (*p)(engine, args);
            } else if (auto p = std::get_if<native_fn_simple>(&function_)) {
                return (*p)(args);
            }
            return value_t();
        }
    };
}