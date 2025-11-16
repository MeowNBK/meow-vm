#include "memory/memory_manager.h"
#include "core/objects.h"

using namespace meow::core;
using namespace meow::runtime;

namespace meow::memory {

MemoryManager::MemoryManager(std::unique_ptr<GarbageCollector> gc) : gc_(std::move(gc)), gc_threshold_(1024), object_allocated_(0) {
}

MemoryManager::~MemoryManager() noexcept = default;

string_t MemoryManager::new_string(const std::string& string) {
    auto it = string_pool_.find(string);
    if (it != string_pool_.end()) {
        return it->second;
    }

    string_t new_string_object = new_object<objects::ObjString>(string);
    string_pool_[string] = new_string_object;
    return new_string_object;
}

string_t MemoryManager::new_string(const char* chars, size_t length) {
    return new_string(std::string(chars, length));
}

array_t MemoryManager::new_array(const std::vector<Value>& elements) {
    return new_object<objects::ObjArray>(elements);
}

hash_table_t MemoryManager::new_hash(const std::unordered_map<string_t, Value>& fields) {
    return new_object<objects::ObjHashTable>(fields);
}

upvalue_t MemoryManager::new_upvalue(size_t index) {
    return new_object<objects::ObjUpvalue>(index);
}

proto_t MemoryManager::new_proto(size_t registers, size_t upvalues, string_t name, Chunk&& chunk) {
    return new_object<objects::ObjFunctionProto>(registers, upvalues, name, std::move(chunk));
}

proto_t MemoryManager::new_proto(size_t registers, size_t upvalues, string_t name, Chunk&& chunk, std::vector<objects::UpvalueDesc>&& descs) {
    return new_object<objects::ObjFunctionProto>(registers, upvalues, name, std::move(chunk), std::move(descs));
}

function_t MemoryManager::new_function(proto_t proto) {
    return new_object<objects::ObjClosure>(proto);
}

module_t MemoryManager::new_module(string_t file_name, string_t file_path, proto_t main_proto) {
    return new_object<objects::ObjModule>(file_name, file_path, main_proto);
}

native_fn_t MemoryManager::new_native(objects::ObjNativeFunction::native_fn_simple fn) {
    return new_object<objects::ObjNativeFunction>(fn);
}

native_fn_t MemoryManager::new_native(objects::ObjNativeFunction::native_fn_double fn) {
    return new_object<objects::ObjNativeFunction>(fn);
}

class_t MemoryManager::new_class(string_t name) {
    return new_object<objects::ObjClass>(name);
}

instance_t MemoryManager::new_instance(class_t klass) {
    return new_object<objects::ObjInstance>(klass);
}

bound_method_t MemoryManager::new_bound_method(instance_t instance, function_t function) {
    return new_object<objects::ObjBoundMethod>(instance, function);
}

};  // namespace meow::memory