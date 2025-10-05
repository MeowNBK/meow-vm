#pragma once

#include "common/pch.h"
#include "core/objects/module.h"
#include "core/type.h"

namespace meow::vm { class MeowEngine; }
namespace meow::memory { class MemoryManager; }

namespace meow::module {
    class ModuleManager {
    public:
        meow::core::Module load_module(meow::core::String module_path, meow::core::String importer_path, meow::memory::MemoryManager* heap, meow::vm::MeowEngine* engine);

        inline void reset_module_cache() noexcept {
            module_cache_.clear();
        }

        inline void add_cache(meow::core::String name, const meow::core::Module& mod) {
            module_cache_[name] = mod;
        }
    private:
        std::unordered_map<meow::core::String, meow::core::Module> module_cache_;
        meow::core::String entry_path_;

        meow::vm::MeowEngine* engine_;
        meow::memory::MemoryManager* heap_;
    };
}