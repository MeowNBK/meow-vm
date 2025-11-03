#include "runtime/operator_dispatcher.h"
#include "memory/memory_manager.h"

using namespace meow::runtime;
using namespace meow::core;

OperatorDispatcher::OperatorDispatcher(meow::memory::MemoryManager* heap) noexcept : heap_(heap) {
    for (size_t op_code = 0; op_code < NUM_OPCODES; ++op_code) {
        for (size_t type1 = 0; type1 < NUM_VALUE_TYPES; ++type1) {
            unary_dispatch_table_[op_code][type1] = nullptr;

            for (size_t type2 = 0; type2 < NUM_VALUE_TYPES; ++type2) {
                binary_dispatch_table_[op_code][type1][type2] = nullptr;
            }
        }
    }
}