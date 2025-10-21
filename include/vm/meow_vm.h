#pragma once

#include "common/pch.h"
#include "vm/meow_engine.h"

namespace meow::runtime {
struct ExecutionContext;
struct BuiltinRegistry;
class OperatorDispatcher;
}  // namespace meow::runtime
namespace meow::memory {
class MemoryManager;
}
namespace meow::module {
class ModuleManager;
}

namespace meow::vm {
struct VMError : public std::runtime_error {
    explicit VMError(const std::string& message) : std::runtime_error(message) {}
};

struct VMArgs {
    std::vector<std::string> command_line_arguments_;
    std::string entry_point_directory_;
    std::string entry_path_;
};

class MeowVM : public MeowEngine {
   public:
    // --- Constructors ---
    explicit MeowVM(const std::string& entry_point_directory, const std::string& entry_path,
                    int argc, char* argv[]);
    MeowVM(const MeowVM&) = delete;
    MeowVM& operator=(const MeowVM&) = delete;
    ~MeowVM();

    // --- Public API ---
    void interpret() noexcept;
    // Value interpret() noexcept;
   private:
    // --- Subsystems ---
    std::unique_ptr<meow::runtime::ExecutionContext> context_;
    std::unique_ptr<meow::runtime::BuiltinRegistry> builtins_;
    std::unique_ptr<meow::memory::MemoryManager> heap_;
    std::unique_ptr<meow::module::ModuleManager> mod_manager_;
    std::unique_ptr<meow::runtime::OperatorDispatcher> op_dispatcher_;

    // --- Runtime arguments ---
    VMArgs args_;

    // --- Execution internals ---
    void prepare() noexcept;
    void run();

    // --- Error helpers ---
    [[noreturn]] inline void throw_vm_error(const std::string& message) { throw VMError(message); }
};
}  // namespace meow::vm