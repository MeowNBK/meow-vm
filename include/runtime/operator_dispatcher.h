#pragma once

#include "common/pch.h"
#include "core/definitions.h"
#include "core/op_codes.h"
#include "core/value.h"
#include "core/meow_object.h"

namespace meow::memory { class MemoryManager; }

namespace meow::runtime {

constexpr size_t NUM_VALUE_TYPES = static_cast<size_t>(core::ValueType::TotalValueTypes);
constexpr size_t NUM_OPCODES = static_cast<size_t>(core::OpCode::TOTAL_OPCODES);
using binary_function_t = meow::core::return_t (*)(meow::core::param_t, meow::core::param_t);
using unary_function_t = meow::core::return_t (*)(meow::core::param_t);

[[nodiscard]] inline constexpr size_t operator+(meow::core::ValueType value_type) noexcept {
    return static_cast<size_t>(value_type);
}
[[nodiscard]] inline constexpr size_t operator+(meow::core::OpCode op_code) noexcept {
    return static_cast<size_t>(op_code);
}

inline meow::core::ValueType get_value_type(meow::core::param_t value) noexcept {
    using namespace meow::core;
    ValueType type = static_cast<ValueType>(value.index());
    if (type == ValueType::Object) {
        return static_cast<ValueType>(value.as_object()->get_type());
    }
    return type;
}

class OperatorDispatcher {
public:
    explicit OperatorDispatcher(memory::MemoryManager* heap) noexcept;

    [[nodiscard]] inline binary_function_t find(core::OpCode op_code, meow::core::param_t left, meow::core::param_t right) const noexcept {
        auto left_type = get_value_type(left);
        auto right_type = get_value_type(right);
        return binary_dispatch_table_[+op_code][+left_type][+right_type];
    }

    [[nodiscard]] inline unary_function_t find(core::OpCode op_code, meow::core::param_t right) const noexcept {
        auto right_type = get_value_type(right);
        return unary_dispatch_table_[+op_code][+right_type];
    }
private:
    memory::MemoryManager* heap_;
    binary_function_t binary_dispatch_table_[NUM_OPCODES][NUM_VALUE_TYPES][NUM_VALUE_TYPES];
    unary_function_t unary_dispatch_table_[NUM_OPCODES][NUM_VALUE_TYPES];
};
}  // namespace meow::runtime