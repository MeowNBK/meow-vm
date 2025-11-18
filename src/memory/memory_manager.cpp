#include "memory/memory_manager.h"
#include "core/objects.h"

using namespace meow::core;
using namespace meow::runtime;

namespace meow::memory {

MemoryManager::MemoryManager(std::unique_ptr<GarbageCollector> gc) noexcept : gc_(std::move(gc)), gc_threshold_(1024), object_allocated_(0) {
}

MemoryManager::~MemoryManager() noexcept = default;

// string_t MemoryManager::new_string(const std::string& string) noexcept {
//     auto it = string_pool_.find(string);
//     if (it != string_pool_.end()) {
//         return it->second;
//     }

//     string_t new_string_object = new_object<objects::ObjString>(string);
//     string_pool_[string] = new_string_object;
//     return new_string_object;
// }

string_t MemoryManager::new_string(std::string_view str_view) noexcept {
    auto it = string_pool_.find(str_view);
    if (it != string_pool_.end()) {
        return it->second;
    }
    
    std::string s(str_view);
    string_t new_obj = new_object<objects::ObjString>(s);
    string_pool_.emplace(std::move(s), new_obj);
    return new_obj;
}

string_t MemoryManager::new_string(const char* chars, size_t length) noexcept {
    return new_string(std::string(chars, length));
}

array_t MemoryManager::new_array(const std::vector<Value>& elements) noexcept {
    return new_object<objects::ObjArray>(elements);
}

hash_table_t MemoryManager::new_hash(const std::unordered_map<string_t, Value>& fields) noexcept {
    return new_object<objects::ObjHashTable>(fields);
}

upvalue_t MemoryManager::new_upvalue(size_t index) noexcept {
    return new_object<objects::ObjUpvalue>(index);
}

proto_t MemoryManager::new_proto(size_t registers, size_t upvalues, string_t name, Chunk&& chunk) noexcept {
    return new_object<objects::ObjFunctionProto>(registers, upvalues, name, std::move(chunk));
}

proto_t MemoryManager::new_proto(size_t registers, size_t upvalues, string_t name, Chunk&& chunk, std::vector<objects::UpvalueDesc>&& descs) noexcept {
    return new_object<objects::ObjFunctionProto>(registers, upvalues, name, std::move(chunk), std::move(descs));
}

function_t MemoryManager::new_function(proto_t proto) noexcept {
    return new_object<objects::ObjClosure>(proto);
}

module_t MemoryManager::new_module(string_t file_name, string_t file_path, proto_t main_proto) noexcept {
    return new_object<objects::ObjModule>(file_name, file_path, main_proto);
}

native_fn_t MemoryManager::new_native(objects::ObjNativeFunction::native_fn_simple fn) noexcept {
    return new_object<objects::ObjNativeFunction>(fn);
}

native_fn_t MemoryManager::new_native(objects::ObjNativeFunction::native_fn_double fn) noexcept {
    return new_object<objects::ObjNativeFunction>(fn);
}

class_t MemoryManager::new_class(string_t name) noexcept {
    return new_object<objects::ObjClass>(name);
}

instance_t MemoryManager::new_instance(class_t klass) noexcept {
    return new_object<objects::ObjInstance>(klass);
}

bound_method_t MemoryManager::new_bound_method(instance_t instance, function_t function) noexcept {
    return new_object<objects::ObjBoundMethod>(instance, function);
}

};  // namespace meow::memory