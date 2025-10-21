#pragma once

#include "core/definitions.h"

namespace meow::core {
class Value;
struct MeowObject;
}  // namespace meow::core

namespace meow::memory {
struct GCVisitor {
    virtual ~GCVisitor() = default;
    virtual void visit_value(meow::core::param_t value) noexcept = 0;
    virtual void visit_object(const meow::core::MeowObject* object) noexcept = 0;
};
}  // namespace meow::memory