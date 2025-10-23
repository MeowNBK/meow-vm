#include "memory/memory_manager.h"
#include "core/objects.h"

namespace meow::memory {

MemoryManager::MemoryManager(std::unique_ptr<GarbageCollector> gc)
    : gc_(std::move(gc)), gc_threshold_(1024), object_allocated_(0) {}

MemoryManager::~MemoryManager() noexcept = default;

meow::core::string_t MemoryManager::new_string(const std::string& string) {
    auto it = string_pool_.find(string);
    if (it != string_pool_.end()) {
        return it->second;
    }

    meow::core::string_t new_string_object = new_object<meow::core::objects::ObjString>(string);
    string_pool_[string] = new_string_object;
    return new_string_object;
}

meow::core::string_t MemoryManager::new_string(const char* chars, size_t length) {
    return new_string(std::string(chars, length));
}

meow::core::array_t MemoryManager::new_array(const std::vector<meow::core::Value>& elements) {
    return new_object<meow::core::objects::ObjArray>(elements);
}

meow::core::hash_table_t MemoryManager::new_hash(
    const std::unordered_map<meow::core::string_t, meow::core::Value>& fields) {
    return new_object<meow::core::objects::ObjHashTable>(fields);
}

meow::core::upvalue_t MemoryManager::new_upvalue(size_t index) {
    return new_object<meow::core::objects::ObjUpvalue>(index);
}

meow::core::proto_t MemoryManager::new_proto(size_t registers, size_t upvalues,
                                             meow::core::string_t name,
                                             meow::runtime::Chunk&& chunk) {
    return new_object<meow::core::objects::ObjFunctionProto>(registers, upvalues, name,
                                                             std::move(chunk));
}

meow::core::proto_t MemoryManager::new_proto(size_t registers, size_t upvalues,
                                             meow::core::string_t name,
                                             meow::runtime::Chunk&& chunk,
                                             std::vector<meow::core::objects::UpvalueDesc>&& descs) {
    return new_object<meow::core::objects::ObjFunctionProto>(registers, upvalues, name,
                                                             std::move(chunk), std::move(descs));
}

meow::core::function_t MemoryManager::new_function(meow::core::proto_t proto) {
    return new_object<meow::core::objects::ObjClosure>(proto);
}

meow::core::module_t MemoryManager::new_module(meow::core::string_t file_name,
                                               meow::core::string_t file_path,
                                               meow::core::proto_t main_proto) {
    return new_object<meow::core::objects::ObjModule>(file_name, file_path, main_proto);
}

meow::core::native_fn_t MemoryManager::new_native(
    meow::core::objects::ObjNativeFunction::native_fn_simple fn) {
    return new_object<meow::core::objects::ObjNativeFunction>(fn);
}

meow::core::native_fn_t MemoryManager::new_native(
    meow::core::objects::ObjNativeFunction::native_fn_double fn) {
    return new_object<meow::core::objects::ObjNativeFunction>(fn);
}

meow::core::class_t MemoryManager::new_class(meow::core::string_t name) {
    return new_object<meow::core::objects::ObjClass>(name);
}

meow::core::instance_t MemoryManager::new_instance(meow::core::class_t klass) {
    return new_object<meow::core::objects::ObjInstance>(klass);
}

meow::core::bound_method_t MemoryManager::new_bound_method(meow::core::instance_t instance,
                                                           meow::core::function_t function) {
    return new_object<meow::core::objects::ObjBoundMethod>(instance, function);
}

};