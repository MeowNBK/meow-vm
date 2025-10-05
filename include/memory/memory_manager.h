#pragma once

#include "common/pch.h"
#include "memory/garbage_collector.h"
#include "core/objects/native.h"
#include "core/type.h"

namespace meow::memory {
    class MemoryManager {
    private:
        std::unique_ptr<meow::memory::GarbageCollector> gc_;
        std::unordered_map<std::string, meow::core::String> string_pool_;

        size_t gc_threshold_;
        size_t object_allocated_;
        bool gc_enabled_ = true;

        template<typename T, typename... Args>
        [[nodiscard]] T* new_object(Args&&... args) noexcept {
            if (object_allocated_ >= gc_threshold_ && gc_enabled_) {
                collect();
                gc_threshold_ *= 2;
            }
            T* newObj = new T(std::forward<Args>(args)...);
            gc->register_object(static_cast<meow::core::MeowObject*>(newObj));
            ++object_allocated_;
            return newObj;
        }
    public:
        explicit MemoryManager(std::unique_ptr<meow::memory::GarbageCollector> gc);

        meow::core::Array new_array(const std::vector<meow::core::Value>& elements = {});
        meow::core::String new_string(const std::string& string);
        meow::core::String new_string(const char* chars, size_t length);
        meow::core::Hash new_hash(const std::unordered_map<meow::core::String, meow::core::Value>& fields = {});
        meow::core::Upvalue new_upvalue(size_t index);
        meow::core::Proto new_proto(size_t registers, size_t upvalues, meow::core::String name, meow::runtime::Chunk&& chunk);
        meow::core::Function new_function(meow::core::Proto proto);
        meow::core::Module new_module(meow::core::String file_name, meow::core::String file_path, meow::core::Proto main_proto = nullptr);
        meow::core::NativeFn new_native(meow::core::objects::ObjNativeFunction::native_fn_simple fn);
        meow::core::NativeFn new_native(meow::core::objects::ObjNativeFunction::native_fn_double fn);
        meow::core::Class new_class(meow::core::String name = nullptr);
        meow::core::Instance new_instance(meow::core::Class klass);
        meow::core::BoundMethod new_bound_method(meow::core::Instance instance, meow::core::Function function);

        inline void enable_gc() noexcept { gc_enabled_ = true; }
        inline void disable_gc() noexcept { gc_enabled_ = false; }
        inline void collect() noexcept { object_allocated_ = gc_->collect(); }
    };
}