#include "runtime/operator_dispatcher.h"
#include "memory/memory_manager.h"

using namespace meow::runtime;
using namespace meow::core;

#define BINARY(opcode, type1, type2) \
    binary_dispatch_table_[static_cast<size_t>(OpCode::opcode)][static_cast<size_t>(ValueType::type1)][static_cast<size_t>(ValueType::type2)] = [](param_t lhs, param_t rhs) -> return_t
#define UNARY(opcode, type) unary_dispatch_table_[static_cast<size_t>(OpCode::opcode)][static_cast<size_t>(ValueType::type)] = [](param_t rhs) -> return_t

OperatorDispatcher::OperatorDispatcher(meow::memory::MemoryManager* heap) noexcept : heap_(heap) {
    for (size_t op_code = 0; op_code < NUM_OPCODES; ++op_code) {
        for (size_t type1 = 0; type1 < NUM_VALUE_TYPES; ++type1) {
            unary_dispatch_table_[op_code][type1] = nullptr;

            for (size_t type2 = 0; type2 < NUM_VALUE_TYPES; ++type2) {
                binary_dispatch_table_[op_code][type1][type2] = nullptr;
            }
        }
    }

    using enum OpCode;
    using enum ValueType;

    binary_dispatch_table_[+ADD][+Int][+Int] = [](param_t lhs, param_t rhs) -> return_t { return Value(lhs.as_int() + rhs.as_int()); };

    BINARY(ADD, Float, Float) {
        return Value(lhs.as_float() + rhs.as_float());
    };

    // too lazy to implement ~300 lambdas like old vm
}