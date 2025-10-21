#pragma once

#include <common/pch.h>
#include <core/value.h>

namespace meow::core {
using value_t = Value;
using param_t = std::conditional_t<sizeof(Value) <= sizeof(void *), const value_t, const value_t &>;
using mutable_t = std::conditional_t<sizeof(Value) <= sizeof(void *), value_t, value_t &>;
using arguments_t = const std::vector<value_t> &;
using return_t = value_t;
}  // namespace meow::core