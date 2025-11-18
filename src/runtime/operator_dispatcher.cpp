#include "runtime/operator_dispatcher.h"
#include "memory/memory_manager.h"

using namespace meow::runtime;
using namespace meow::core;

#define BINARY(opcode, type1, type2) \
    binary_dispatch_table_[+OpCode::opcode][+ValueType::type1][+ValueType::type2] = [](param_t lhs, param_t rhs) -> return_t
#define UNARY(opcode, type) unary_dispatch_table_[+OpCode::opcode][+ValueType::type] = [](param_t rhs) -> return_t

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
    BINARY(ADD, String, String) {
        // the enclosing-function 'this' cannot be referenced in a lambda body unless it is in the capture list
        // but dispatch table need function pointer
        // return Value(heap_->new_string(std::string(lhs.as_string()->c_str()) + rhs.as_string()->c_str()));
        // TODO: Change this later
        auto str = new meow::core::objects::ObjString(std::string(lhs.as_string()->c_str()) + rhs.as_string()->c_str());
        return Value(str);
    };
}