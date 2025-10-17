#pragma once

#include <common/pch.h>
#include <core/value.h>

namespace meow::core {
    using value_t = Value;
    using const_value_t = const value_t;
    using param_t = std::conditional_t<sizeof(Value) < sizeof(void*), const_value_t ,const value_t&>;
    using return_t = value_t;
}