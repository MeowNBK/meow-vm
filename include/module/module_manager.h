#pragma once

#include "common/pch.h"
#include "core/objects/module.h"
#include "core/type.h"

namespace meow::vm {
class MeowEngine;
}
namespace meow::memory {
class MemoryManager;
}

namespace meow::module {
class ModuleManager {
   public:
    explicit ModuleManager(meow::memory::MemoryManager* heap, meow::vm::MeowEngine* engine) noexcept;
    ModuleManager(const ModuleManager&) = delete;
    ModuleManager(ModuleManager&&) = default;
    ModuleManager& operator=(const ModuleManager&) = delete;
    ModuleManager& operator=(ModuleManager&&) = default;
    ModuleManager() = default;
    meow::core::module_t load_module(meow::core::string_t module_path, meow::core::string_t importer_path);

    inline void reset_cache() noexcept {
        module_cache_.clear();
    }

    inline void add_cache(meow::core::string_t name, const meow::core::module_t& mod) {
        module_cache_[name] = mod;
    }

   private:
    std::unordered_map<meow::core::string_t, meow::core::module_t> module_cache_;
    meow::core::string_t entry_path_;

    meow::memory::MemoryManager* heap_;
    meow::vm::MeowEngine* engine_;
};
}  // namespace meow::module