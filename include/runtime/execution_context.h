#pragma once

#include "common/pch.h"
#include "core/objects.h"
#include "core/objects/function.h"
#include "core/value.h"
#include "memory/gc_visitor.h"

namespace meow::runtime {
struct CallFrame {
    meow::core::function_t function_;
    meow::core::module_t module_;
    size_t start_reg_;
    size_t ret_reg_;
    const uint8_t* ip_;
    CallFrame(meow::core::function_t function, meow::core::module_t module, size_t start_reg, size_t ret_reg, const uint8_t* ip)
        : function_(function), module_(module), start_reg_(start_reg), ret_reg_(ret_reg), ip_(ip) {
    }
};

struct ExceptionHandler {
    size_t catch_ip_;
    size_t frame_depth_;
    size_t stack_depth_;
    ExceptionHandler(size_t catch_ip = 0, size_t frame_depth = 0, size_t stack_depth = 0) : catch_ip_(catch_ip), frame_depth_(frame_depth), stack_depth_(stack_depth) {
    }
};

struct ExecutionContext {
    std::vector<CallFrame> call_stack_;
    std::vector<meow::core::Value> registers_;
    std::vector<meow::core::upvalue_t> open_upvalues_;
    std::vector<ExceptionHandler> exception_handlers_;

    size_t current_base_ = 0;
    CallFrame* current_frame_ = nullptr;

    inline void reset() noexcept {
        call_stack_.clear();
        registers_.clear();
        open_upvalues_.clear();
        exception_handlers_.clear();
    }

    inline void trace(meow::memory::GCVisitor& visitor) const noexcept {
        for (const auto& reg : registers_) {
            visitor.visit_value(reg);
        }
        for (const auto& upvalue : open_upvalues_) {
            visitor.visit_object(upvalue);
        }
    }
};
}  // namespace meow::runtime