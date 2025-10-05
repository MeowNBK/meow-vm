#pragma once

#include "common/pch.h"
#include "core/value.h"
#include "core/op_code.h"

namespace meow::memory { class MemoryManager; }

namespace meow::runtime {
    constexpr size_t NUM_VALUE_TYPES = static_cast<size_t>(meow::core::ValueType::TotalValueTypes); 
    constexpr size_t NUM_OPCODES = static_cast<size_t>(meow::core::OpCode::TOTAL_OPCODES);

    inline meow::core::ValueType get_value_type(const meow::core::Value& value) noexcept {
        if (value.is_null()) return meow::core::ValueType::Null;
        if (value.is_int()) return meow::core::ValueType::Int;
        if (value.is_real()) return meow::core::ValueType::Real;
        if (value.is_object()) return meow::core::ValueType::Bool;
        return meow::core::ValueType::Null;
    }

    class OperatorDispatcher {
    private:
        using BinaryOpFunction = meow::core::Value(*)(const meow::core::Value&, const meow::core::Value&);
        using UnaryOpFunction = meow::core::Value(*)(const meow::core::Value&);

        meow::memory::MemoryManager* heap_;
        BinaryOpFunction binary_dispatch_table_[NUM_OPCODES][NUM_VALUE_TYPES][NUM_VALUE_TYPES];
        UnaryOpFunction unary_dispatch_table_[NUM_OPCODES][NUM_VALUE_TYPES];
    public:
        explicit OperatorDispatcher(meow::memory::MemoryManager* heap) noexcept;

        [[nodiscard]] inline const BinaryOpFunction* find(meow::core::OpCode op_code, const meow::core::Value& left, const meow::core::Value& right) const noexcept {
            auto left_type = get_value_type(left);
            auto right_type = get_value_type(right);
            const BinaryOpFunction* function = &binary_dispatch_table_[static_cast<size_t>(op_code)][static_cast<size_t>(static_cast<size_t>(left_type))][static_cast<size_t>(right_type)];
            return (*function) ? function : nullptr;
        }

        [[nodiscard]] inline const UnaryOpFunction* find(meow::core::OpCode op_code, const meow::core::Value& right) const noexcept {
            auto right_type = get_value_type(right);
            const UnaryOpFunction* function = &unary_dispatch_table_[static_cast<size_t>(op_code)][static_cast<size_t>(right_type)];
            return (*function) ? function : nullptr;
        }
    };
}