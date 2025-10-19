#pragma once

#include "common/pch.h"
#include "core/value.h"
#include "core/definitions.h"
#include "core/op_codes.h"

namespace meow::memory { class MemoryManager; }

namespace meow::runtime {
    constexpr size_t NUM_VALUE_TYPES = static_cast<size_t>(core::ValueType::TotalValueTypes); 
    constexpr size_t NUM_OPCODES = static_cast<size_t>(core::OpCode::TOTAL_OPCODES);

    inline core::ValueType get_value_type(meow::core::param_t value) noexcept {
        if (value.is_null()) return core::ValueType::Null;
        if (value.is_int()) return core::ValueType::Int;
        if (value.is_float()) return core::ValueType::Float;
        if (value.is_object()) return core::ValueType::Bool;
        return core::ValueType::Null;
    }

    [[nodiscard]] inline constexpr size_t operator+(core::ValueType value_type) noexcept {
        return static_cast<size_t>(value_type);
    }
    [[nodiscard]] inline constexpr size_t operator+(core::OpCode op_code) noexcept {
        return static_cast<size_t>(op_code);
    }

    class OperatorDispatcher {
    private:
        using BinaryOpFunction = core::return_t(*)(meow::core::param_t, meow::core::param_t);
        using UnaryOpFunction = core::return_t(*)(meow::core::param_t);

        memory::MemoryManager* heap_;
        BinaryOpFunction binary_dispatch_table_[NUM_OPCODES][NUM_VALUE_TYPES][NUM_VALUE_TYPES];
        UnaryOpFunction unary_dispatch_table_[NUM_OPCODES][NUM_VALUE_TYPES];
    public:
        explicit OperatorDispatcher(memory::MemoryManager* heap) noexcept;

        [[nodiscard]] inline const BinaryOpFunction* find(core::OpCode op_code, meow::core::param_t left, meow::core::param_t right) const noexcept {
            auto left_type = get_value_type(left);
            auto right_type = get_value_type(right);
            const BinaryOpFunction* function = &binary_dispatch_table_[+op_code][+left_type][+right_type];
            return (*function) ? function : nullptr;
        }

        [[nodiscard]] inline const UnaryOpFunction* find(core::OpCode op_code, meow::core::param_t right) const noexcept {
            auto right_type = get_value_type(right);
            const UnaryOpFunction* function = &unary_dispatch_table_[+op_code][+right_type];
            return (*function) ? function : nullptr;
        }
    };
}