#pragma once

#include "common/pch.h"
#include "core/type.h"
#include "runtime/chunk.h"
#include "core/objects/function.h"

namespace meow::memory { class MemoryManager; }

namespace meow::loader {

class BinaryLoaderError : public std::runtime_error {
public:
    explicit BinaryLoaderError(const std::string& msg) : std::runtime_error(msg) {}
};

class BinaryLoader {
public:
    BinaryLoader(meow::memory::MemoryManager* heap, const std::vector<uint8_t>& data);
    meow::core::proto_t load_module();
private:
    meow::memory::MemoryManager* heap_;
    const std::vector<uint8_t>& data_;
    size_t cursor_ = 0;

    std::vector<meow::core::proto_t> loaded_protos_;

    void check_can_read(size_t bytes);
    uint8_t  read_u8();
    uint16_t read_u16();
    uint32_t read_u32();
    uint64_t read_u64();
    double   read_f64();
    meow::core::string_t read_string();
    
    meow::core::Value read_constant();
    meow::core::proto_t read_prototype();
    void check_magic();
    void link_prototypes();
};

}