#pragma once

#include "common/pch.h"

namespace meow::core {
    class Value;
    using value_t = Value;
    using param_t = const value_t&;
    using mutable_t = value_t&;
    using arguments_t = const std::vector<value_t>&;
    using return_t = value_t;
}