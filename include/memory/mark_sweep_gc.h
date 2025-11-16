#pragma once

#include "common/pch.h"
#include "core/definitions.h"
#include "memory/garbage_collector.h"
#include "memory/gc_visitor.h"

namespace meow::runtime {
struct ExecutionContext;
struct BuiltinRegistry;
}  // namespace meow::runtime

namespace meow::memory {
struct GCMetadata {
    bool is_marked_ = false;
};

class MarkSweepGC : public GarbageCollector, public GCVisitor {
public:
    explicit MarkSweepGC(meow::runtime::ExecutionContext* context, meow::runtime::BuiltinRegistry* builtins) noexcept : context_(context), builtins_(builtins) {
    }
    ~MarkSweepGC() noexcept override;

    // -- Collector ---
    void register_object(const meow::core::MeowObject* object) override;
    size_t collect() noexcept override;

    // --- Visitor ---
    void visit_value(meow::core::param_t value) noexcept override;
    void visit_object(const meow::core::MeowObject* object) noexcept override;
private:
    std::unordered_map<const meow::core::MeowObject*, GCMetadata> metadata_;
    meow::runtime::ExecutionContext* context_ = nullptr;
    meow::runtime::BuiltinRegistry* builtins_ = nullptr;

    void mark(const meow::core::MeowObject* object);

};
}  // namespace meow::memory