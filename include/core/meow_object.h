#pragma once

namespace meow::memory {
struct GCVisitor;
}

namespace meow::core {

enum class ObjectType : uint8_t {
    ARRAY = 5,
    STRING,
    HASH_TABLE,
    INSTANCE,
    CLASS,
    BOUND_METHOD,
    UPVALUE,
    PROTO,
    FUNCTION,
    NATIVE_FN,
    MODULE
};

struct MeowObject {
    const ObjectType type;

    explicit MeowObject(ObjectType type_tag) noexcept : type(type_tag) {}
    
    virtual ~MeowObject() = default;
    virtual void trace(meow::memory::GCVisitor& visitor) const noexcept = 0;

    [[nodiscard]] inline ObjectType get_type() const noexcept { return type; }
};

template <ObjectType type_tag>
struct ObjBase : public MeowObject {
    ObjBase() noexcept : MeowObject(type_tag) {}
};

}  // namespace meow::core