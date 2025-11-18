#pragma once
// Chứa các handler cho Exception (Try/Catch/Throw)

inline void MeowVM::op_pop_try() {
    if (!context_->exception_handlers_.empty()) {
        context_->exception_handlers_.pop_back();
    }
}