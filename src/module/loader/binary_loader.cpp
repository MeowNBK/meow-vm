#include "module/loader/binary_loader.h"
#include "memory/memory_manager.h"
#include "core/objects/string.h"
#include "core/objects/function.h"
#include "core/value.h"
#include "runtime/chunk.h"

namespace meow::loader {

using namespace meow::core;
using namespace meow::runtime;
using namespace meow::memory;
using namespace meow::core::objects;

constexpr uint32_t MAGIC_NUMBER = 0x4D454F57; // "MEOW"
constexpr uint32_t FORMAT_VERSION = 1;

enum class ConstantTag : uint8_t {
    NULL_T,
    INT_T,
    FLOAT_T,
    STRING_T,
    PROTO_REF_T
};

BinaryLoader::BinaryLoader(MemoryManager* heap, const std::vector<uint8_t>& data)
    : heap_(heap), data_(data), cursor_(0) {}


void BinaryLoader::check_can_read(size_t bytes) {
    if (cursor_ + bytes > data_.size()) {
        throw BinaryLoaderError("Unexpected end of file. File is truncated or corrupt.");
    }
}

uint8_t BinaryLoader::read_u8() {
    check_can_read(1);
    return data_[cursor_++];
}

uint16_t BinaryLoader::read_u16() {
    check_can_read(2);
    uint16_t val = static_cast<uint16_t>(data_[cursor_]) |
                   (static_cast<uint16_t>(data_[cursor_ + 1]) << 8);
    cursor_ += 2;
    return val;
}

uint32_t BinaryLoader::read_u32() {
    check_can_read(4);
    uint32_t val = static_cast<uint32_t>(data_[cursor_]) |
                   (static_cast<uint32_t>(data_[cursor_ + 1]) << 8) |
                   (static_cast<uint32_t>(data_[cursor_ + 2]) << 16) |
                   (static_cast<uint32_t>(data_[cursor_ + 3]) << 24);
    cursor_ += 4;
    return val;
}

uint64_t BinaryLoader::read_u64() {
    check_can_read(8);
    uint64_t val = 0;
    for (int i = 0; i < 8; ++i) {
        val |= (static_cast<uint64_t>(data_[cursor_ + i]) << (i * 8));
    }
    cursor_ += 8;
    return val;
}

double BinaryLoader::read_f64() {
    return std::bit_cast<double>(read_u64());
}

string_t BinaryLoader::read_string() {
    uint32_t length = read_u32();
    check_can_read(length);
    std::string str(reinterpret_cast<const char*>(data_.data() + cursor_), length);
    cursor_ += length;
    return heap_->new_string(str);
}

Value BinaryLoader::read_constant() {
    ConstantTag tag = static_cast<ConstantTag>(read_u8());
    switch (tag) {
        case ConstantTag::NULL_T:   return Value(null_t{});
        case ConstantTag::INT_T:    return Value(static_cast<int64_t>(read_u64()));
        case ConstantTag::FLOAT_T:  return Value(read_f64());
        case ConstantTag::STRING_T: return Value(read_string());
        case ConstantTag::PROTO_REF_T: {
            uint32_t proto_index = read_u32();
            std::string ref_str = "::proto_ref_idx::" + std::to_string(proto_index);
            return Value(heap_->new_string(ref_str)); 
        }
        default:
            throw BinaryLoaderError("Unknown constant tag in binary file.");
    }
}

proto_t BinaryLoader::read_prototype() {
    uint32_t num_registers = read_u32();
    uint32_t num_upvalues = read_u32();
    uint32_t name_idx_in_pool = read_u32();

    uint32_t constant_pool_size = read_u32();
    std::vector<Value> constants;
    constants.reserve(constant_pool_size);
    for (uint32_t i = 0; i < constant_pool_size; ++i) {
        constants.push_back(read_constant());
    }
    
    if (name_idx_in_pool >= constants.size() || !constants[name_idx_in_pool].is_string()) {
        throw BinaryLoaderError("Invalid function prototype name index.");
    }
    string_t name = constants[name_idx_in_pool].as_string();

    uint32_t upvalue_desc_count = read_u32();
    if (upvalue_desc_count != num_upvalues) {
         throw BinaryLoaderError("Upvalue count mismatch.");
    }
    std::vector<UpvalueDesc> upvalue_descs;
    upvalue_descs.reserve(upvalue_desc_count);
    for (uint32_t i = 0; i < upvalue_desc_count; ++i) {
        bool is_local = (read_u8() == 1);
        uint32_t index = read_u32();
        upvalue_descs.emplace_back(is_local, index);
    }

    uint32_t bytecode_size = read_u32();
    check_can_read(bytecode_size);
    std::vector<uint8_t> bytecode(data_.data() + cursor_, data_.data() + cursor_ + bytecode_size);
    cursor_ += bytecode_size;
    
    Chunk chunk(std::move(bytecode), std::move(constants));
    return heap_->new_proto(num_registers, num_upvalues, name, std::move(chunk), std::move(upvalue_descs));
}

void BinaryLoader::check_magic() {
    if (read_u32() != MAGIC_NUMBER) {
        throw BinaryLoaderError("Not a valid Meow bytecode file (magic number mismatch).");
    }
    uint32_t version = read_u32();
    if (version != FORMAT_VERSION) {
        throw BinaryLoaderError(
            std::format("Bytecode version mismatch. File is v{}, VM supports v{}. Please recompile.", 
                        version, FORMAT_VERSION)
        );
    }
}

void BinaryLoader::link_prototypes() {    
    const std::string ref_prefix = "::proto_ref_idx::";
    for (proto_t proto : loaded_protos_) {
        Chunk& chunk = const_cast<Chunk&>(proto->get_chunk()); 
        for (size_t i = 0; i < chunk.get_pool_size(); ++i) {
            Value& constant_ref = chunk.get_constant_ref(i); 
            
            if (constant_ref.is_string()) {
                std::string_view s = constant_ref.as_string()->c_str();
                if (s.rfind(ref_prefix, 0) == 0) {
                    uint32_t proto_index = std::stoul(std::string(s.substr(ref_prefix.size())));
                    if (proto_index >= loaded_protos_.size()) {
                        throw BinaryLoaderError("Invalid prototype reference in constant pool.");
                    }
                    constant_ref = Value(loaded_protos_[proto_index]);
                }
            }
        }
    }
}

proto_t BinaryLoader::load_module() {
    check_magic();
    
    uint32_t main_proto_index = read_u32();
    uint32_t prototype_count = read_u32();
    
    if (prototype_count == 0) {
        throw BinaryLoaderError("No prototypes found in bytecode file.");
    }
    
    loaded_protos_.reserve(prototype_count);
    for (uint32_t i = 0; i < prototype_count; ++i) {
        loaded_protos_.push_back(read_prototype());
    }
    
    if (main_proto_index >= loaded_protos_.size()) {
        throw BinaryLoaderError("Main prototype index is out of bounds.");
    }
    
    link_prototypes();
    
    return loaded_protos_[main_proto_index];
}

} // namespace meow::loader