#pragma once

#include "common/pch.h"
#include "vm/meow_engine.h"

namespace meow::runtime {
    struct ExecutionContext;
    struct BuiltinRegistry;
    class OperatorDispatcher;
}
namespace meow::memory { class MemoryManager; }
namespace meow::module { class ModuleManager; }

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
    explicit MeowVM(const std::string& entry_point_directory, const std::string& entry_path, int argc, char* argv[]);
    MeowVM(const MeowVM&) = delete;
    MeowVM(MeowVM&&) = delete;
    MeowVM& operator=(const MeowVM&) = delete;
    MeowVM& operator=(MeowVM&&) = delete;
    ~MeowVM() noexcept;

    // --- Public API ---
    void interpret() noexcept;
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
    [[noreturn]] inline void throw_vm_error(const std::string& message) {
        throw VMError(message);
    }

    // --- OpCode Handlers (Helpers) ---
    inline void op_load_const(const uint8_t*& ip);
    inline void op_load_null(const uint8_t*& ip);
    inline void op_load_true(const uint8_t*& ip);
    inline void op_load_false(const uint8_t*& ip);
    inline void op_load_int(const uint8_t*& ip);
    inline void op_load_float(const uint8_t*& ip);
    inline void op_move(const uint8_t*& ip);
    inline void op_get_global(const uint8_t*& ip);
    inline void op_set_global(const uint8_t*& ip);
    inline void op_get_upvalue(const uint8_t*& ip);
    inline void op_set_upvalue(const uint8_t*& ip);
    inline void op_closure(const uint8_t*& ip);
    inline void op_close_upvalues(const uint8_t*& ip);
    inline void op_new_array(const uint8_t*& ip);
    inline void op_new_hash(const uint8_t*& ip);
    inline void op_get_index(const uint8_t*& ip);
    inline void op_set_index(const uint8_t*& ip);
    inline void op_get_keys(const uint8_t*& ip);
    inline void op_get_values(const uint8_t*& ip);
    inline void op_new_class(const uint8_t*& ip);
    inline void op_new_instance(const uint8_t*& ip);
    inline void op_get_prop(const uint8_t*& ip);
    inline void op_set_prop(const uint8_t*& ip);
    inline void op_set_method(const uint8_t*& ip);
    inline void op_inherit(const uint8_t*& ip);
    inline void op_get_super(const uint8_t*& ip);
    inline void op_pop_try();  // Không cần 'ip'
    inline void op_export(const uint8_t*& ip);
    inline void op_get_export(const uint8_t*& ip);
    inline void op_import_all(const uint8_t*& ip);
};
}  // namespace meow::vm