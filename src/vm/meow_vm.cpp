#include "vm/meow_vm.h"
#include "common/pch.h"
#include "runtime/execution_context.h"
#include "runtime/builtin_registry.h"
#include "memory/memory_manager.h"
#include "memory/mark_sweep_gc.h"
#include "module/module_manager.h"
#include "runtime/operator_dispatcher.h"
#include "core/op_codes.h"

template <typename... Args>
inline void print(const std::format_string<Args...> fmt, Args&&... args) {
    std::cout << "[log] " << std::format(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void printl(const std::format_string<Args...> fmt, Args&&... args) {
    std::cout << "[log] " << std::format(fmt, std::forward<Args>(args)...) << '\n';
}

using namespace meow::vm;
using namespace meow::core;

MeowVM::MeowVM(const std::string& entry_point_directory, const std::string& entry_path, int argc, char* argv[]) {
    args_.entry_point_directory_ = entry_point_directory;
    args_.entry_path_ = entry_path;
    for (int i = 0; i < argc; ++i) {
        args_.command_line_arguments_.emplace_back(argv[i]);
    }

    context_ = std::make_unique<meow::runtime::ExecutionContext>();
    builtins_ = std::make_unique<meow::runtime::BuiltinRegistry>();

    auto gc = std::make_unique<meow::memory::MarkSweepGC>(context_.get(), builtins_.get());
    
    heap_ = std::make_unique<meow::memory::MemoryManager>(std::move(gc));

    mod_manager_ = std::make_unique<meow::module::ModuleManager>();
    op_dispatcher_ = std::make_unique<meow::runtime::OperatorDispatcher>(heap_.get());

    printl("MeowVM initialized successfully!");
}

MeowVM::~MeowVM() {
    printl("MeowVM shutting down.");
}

void MeowVM::interpret() noexcept {
    prepare();
    try {
        run();
    } catch (const std::exception& e) {
        printl("An execption was threw: {}", e.what());
    }
}

void MeowVM::prepare() noexcept {
    printl("Preparing for execution...");
}

void MeowVM::run() {
    printl("Starting MeowVM execution loop...");
    meow::runtime::Chunk test_chunk;

    test_chunk.write_byte(static_cast<uint8_t>(OpCode::LOAD_INT));
    test_chunk.write_u16(0);
    test_chunk.write_u64(123ULL);

    test_chunk.write_byte(static_cast<uint8_t>(OpCode::LOAD_TRUE));
    test_chunk.write_u16(1);
    
    test_chunk.write_byte(static_cast<uint8_t>(OpCode::HALT));
    
    auto main_proto = heap_->new_proto(1, 0, heap_->new_string("main"), std::move(test_chunk));
    auto main_func = heap_->new_function(main_proto);

    context_->registers_.resize(2);
    context_->call_stack_.emplace_back(main_func, 0, 0, main_func->get_proto()->get_chunk().get_code());
    context_->current_frame_ = &context_->call_stack_.back();

    const uint8_t* ip = context_->current_frame_->ip_;

    #define READ_BYTE() (*ip++)
    #define READ_U16() (ip += 2, (uint16_t)((ip[-2] | (ip[-1] << 8))))
    #define READ_U64() ( \
        ip += 8, \
        (uint64_t)(ip[-8]) | \
        ((uint64_t)(ip[-7]) << 8) | \
        ((uint64_t)(ip[-6]) << 16) | \
        ((uint64_t)(ip[-5]) << 24) | \
        ((uint64_t)(ip[-4]) << 32) | \
        ((uint64_t)(ip[-3]) << 40) | \
        ((uint64_t)(ip[-2]) << 48) | \
        ((uint64_t)(ip[-1]) << 56) \
    )

    #define READ_I64() (std::bit_cast<int64_t>(READ_U64()))
    #define READ_F64() (std::bit_cast<double>(READ_U64()))
    #define READ_CONSTANT() (context_->current_frame_->function_->get_proto()->get_chunk().get_constant(READ_U16()))

    #define REGISTER(idx) (context_->registers_[idx])
    #define CONSTANT(idx) (context_->current_frame_->function_->get_proto()->get_chunk().get_constant(idx))

    while (42) {
        uint8_t instruction = READ_BYTE();
        switch (static_cast<OpCode>(instruction)) {
            
            // load_const dst, cidx
            case OpCode::LOAD_CONST: {
                uint16_t dst = READ_U16();
                Value value = READ_CONSTANT();

                REGISTER(dst) = value;
                // printl("load_const r{}", dst);
                break;
            }

            // load_null dst
            case OpCode::LOAD_NULL: {
                uint16_t dst = READ_U16();

                REGISTER(dst) = Value();
                printl("load_null r{}", dst);
                break;
            }

            // load_true dst
            case OpCode::LOAD_TRUE: {
                uint16_t dst = READ_U16();

                REGISTER(dst) = Value(true);
                printl("load_true r{}", dst);
                break;
            }

            // load_false dst
            case OpCode::LOAD_FALSE: {
                uint16_t dst = READ_U16();

                REGISTER(dst) = Value(false);
                printl("load_true r{}", dst);
                break;
            }

            case OpCode::MOVE: {
                uint16_t dst = READ_U16();
                uint16_t src = READ_U16();

                REGISTER(dst) = REGISTER(src);
                break;
            }

            // load_int dst, value
            case OpCode::LOAD_INT: {
                uint16_t dst = READ_U16();
                int64_t value = READ_I64();
                                
                REGISTER(dst) = Value(value);
                printl("load_int r{}, {}", dst, value);
                break;
            }

            // load_float dst, value
            case OpCode::LOAD_FLOAT: {
                uint16_t dst = READ_U16();
                double value = READ_F64();

                REGISTER(dst) = Value(value);
                printl("load_float r{}, {}", dst, value);
                break;
            }

            // halt
            case OpCode::HALT: {
                printl("halt");
                if (!context_->registers_.empty()) {
                   if (REGISTER(0).is_int()) {
                       printl("Final value in R0: {}", REGISTER(0).as_int());
                   }
                }
                return;
            }
            default:
                throwVMError("Unknown opcode");
        }
    }
}