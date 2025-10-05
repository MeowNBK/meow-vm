#pragma once

#include "memory/memory_manager.h"

namespace meow::memory {
    class GCDisableGuard {
    private:
        MemoryManager* heap_;
    public:
        explicit GCDisableGuard(MemoryManager* heap) noexcept : heap_(heap) { if (heap_) heap_->disable_gc(); }
        GCDisableGuard(const GCDisableGuard&) = delete;
        GCDisableGuard& operator=(const GCDisableGuard&) = delete;
        GCDisableGuard(GCDisableGuard&&) = delete;
        GCDisableGuard& operator=(GCDisableGuard&&) = delete;
        ~GCDisableGuard() noexcept { if (heap_) heap_->enable_gc(); }
    };
}