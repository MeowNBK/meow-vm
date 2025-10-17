#include "memory/memory_manager.h"
#include "core/objects.h"

using namespace meow::memory;

MemoryManager::MemoryManager(std::unique_ptr<GarbageCollector> gc) 
    : gc_(std::move(gc)), gc_threshold_(1024), object_allocated_(0) {}

MemoryManager::~MemoryManager() = default;

meow::core::String MemoryManager::new_string(const std::string& string) {
    auto it = string_pool_.find(string);
    if (it != string_pool_.end()) {
        return it->second;
    }

    meow::core::String new_string_object = new_object<meow::core::objects::ObjString>(string);
    string_pool_[string] = new_string_object;
    return new_string_object;
}

meow::core::String MemoryManager::new_string(const char* chars, size_t length) {
    return new_string(std::string(chars, length));
}

meow::core::Array MemoryManager::new_array(const std::vector<meow::core::Value>& elements) {
    return new_object<meow::core::objects::ObjArray>(elements);
}

meow::core::HashTable MemoryManager::new_hash(const std::unordered_map<meow::core::String, meow::core::Value>& fields) {
    return new_object<meow::core::objects::ObjHashTable>(fields);
}

meow::core::Upvalue MemoryManager::new_upvalue(size_t index) {
    return new_object<meow::core::objects::ObjUpvalue>(index);
}

meow::core::Proto MemoryManager::new_proto(size_t registers, size_t upvalues, meow::core::String name, meow::runtime::Chunk&& chunk) {
    return new_object<meow::core::objects::ObjFunctionProto>(registers, upvalues, name, std::move(chunk));
}

meow::core::Function MemoryManager::new_function(meow::core::Proto proto) {
    return new_object<meow::core::objects::ObjClosure>(proto); 
}

meow::core::Module MemoryManager::new_module(meow::core::String file_name, meow::core::String file_path, meow::core::Proto main_proto) {
    return new_object<meow::core::objects::ObjModule>(file_name, file_path, main_proto);
}

meow::core::NativeFn MemoryManager::new_native(meow::core::objects::ObjNativeFunction::native_fn_simple fn) {
    return new_object<meow::core::objects::ObjNativeFunction>(fn);
}

meow::core::NativeFn MemoryManager::new_native(meow::core::objects::ObjNativeFunction::native_fn_double fn) {
    return new_object<meow::core::objects::ObjNativeFunction>(fn);
}

meow::core::Class MemoryManager::new_class(meow::core::String name) {
    return new_object<meow::core::objects::ObjClass>(name);
}

meow::core::Instance MemoryManager::new_instance(meow::core::Class klass) {
    return new_object<meow::core::objects::ObjInstance>(klass);
}

meow::core::BoundMethod MemoryManager::new_bound_method(meow::core::Instance instance, meow::core::Function function) {
    return new_object<meow::core::objects::ObjBoundMethod>(instance, function);
}