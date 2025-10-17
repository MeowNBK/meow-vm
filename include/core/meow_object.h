#pragma once

namespace meow::memory { struct GCVisitor; }

namespace meow::core {
    struct MeowObject {
        virtual ~MeowObject() = default;
        virtual void trace(meow::memory::GCVisitor& visitor) const noexcept = 0;
    };
}