#pragma once

namespace meow::core { class Value; struct MeowObject; }

namespace meow::memory {
    struct GCVisitor {
        virtual ~GCVisitor() = default;
        virtual void visit_value(const meow::core::Value& value) noexcept = 0;
        virtual void visit_object(const meow::core::MeowObject* object) noexcept = 0;
    };
}