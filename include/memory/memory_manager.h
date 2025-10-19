#pragma once

#include "common/pch.h"
#include "memory/garbage_collector.h"
#include "core/objects.h"
#include "core/type.h"

namespace meow::memory {
    class MemoryManager {
    private:
        std::unique_ptr<meow::memory::GarbageCollector> gc_;
        std::unordered_map<std::string, meow::core::string_t> string_pool_;

        size_t gc_threshold_;
        size_t object_allocated_;
        bool gc_enabled_ = true;

        template<typename T, typename... Args>
        [[nodiscard]] T* new_object(Args&&... args) noexcept {
            if (object_allocated_ >= gc_threshold_ && gc_enabled_) {
                collect();
                gc_threshold_ *= 2;
            }
            T* new_object = new T(std::forward<Args>(args)...);
            gc_->register_object(static_cast<meow::core::MeowObject*>(new_object));
            ++object_allocated_;
            return new_object;
        }
    public:
        explicit MemoryManager(std::unique_ptr<meow::memory::GarbageCollector> gc);
        ~MemoryManager() noexcept;
        meow::core::array_t new_array(const std::vector<meow::core::Value>& elements = {});
        meow::core::string_t new_string(const std::string& string);
        meow::core::string_t new_string(const char* chars, size_t length);
        meow::core::hash_table_t new_hash(const std::unordered_map<meow::core::string_t, meow::core::Value>& fields = {});
        meow::core::upvalue_t new_upvalue(size_t index);
        meow::core::proto_t new_proto(size_t registers, size_t upvalues, meow::core::string_t name, meow::runtime::Chunk&& chunk);
        meow::core::function_t new_function(meow::core::proto_t proto);
        meow::core::module_t new_module(meow::core::string_t file_name, meow::core::string_t file_path, meow::core::proto_t main_proto = nullptr);
        meow::core::native_fn_t new_native(meow::core::objects::ObjNativeFunction::native_fn_simple fn);
        meow::core::native_fn_t new_native(meow::core::objects::ObjNativeFunction::native_fn_double fn);
        meow::core::class_t new_class(meow::core::string_t name = nullptr);
        meow::core::instance_t new_instance(meow::core::class_t klass);
        meow::core::bound_method_t new_bound_method(meow::core::instance_t instance, meow::core::function_t function);

        inline void enable_gc() noexcept { gc_enabled_ = true; }
        inline void disable_gc() noexcept { gc_enabled_ = false; }
        inline void collect() noexcept { object_allocated_ = gc_->collect(); }
    };
}