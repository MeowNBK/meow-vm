#include "vm/meow_vm.h"
#include "common/pch.h"
#include "runtime/execution_context.h"
#include "runtime/builtin_registry.h"
#include "memory/memory_manager.h"
#include "memory/mark_sweep_gc.h"
#include "module/module_manager.h"
#include "runtime/operator_dispatcher.h"
#include "core/op_codes.h"

#include "core/objects/function.h"
#include "core/objects/array.h"
#include "core/objects/hash_table.h"

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

[[nodiscard]] inline uint8_t operator+(OpCode op_code) noexcept {
    return static_cast<uint8_t>(op_code);
}

void MeowVM::interpret() noexcept {
    prepare();
    try {
        meow::runtime::Chunk test_chunk;

        test_chunk.write_byte(+OpCode::LOAD_INT);
        test_chunk.write_u16(0);
        test_chunk.write_u64(123ULL);

        test_chunk.write_byte(+OpCode::LOAD_TRUE);
        test_chunk.write_u16(1); // dst r1
        
        test_chunk.write_byte(+OpCode::HALT);
        
        auto main_proto = heap_->new_proto(2, 0, heap_->new_string("main"), std::move(test_chunk));
        auto main_func = heap_->new_function(main_proto);

        context_->registers_.resize(2);
        context_->call_stack_.emplace_back(main_func, 0, static_cast<size_t>(-1), main_func->get_proto()->get_chunk().get_code());

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

    if (context_->call_stack_.empty()) {
        printl("Execution finished: Call stack is empty.");
        return;
    }

    context_->current_frame_ = &context_->call_stack_.back();
    const uint8_t* ip = context_->current_frame_->ip_;
    context_->current_base_ = context_->current_frame_->start_reg_;

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
    #define READ_ADDRESS() READ_U16()

    #define CURRENT_CHUNK() (context_->current_frame_->function_->get_proto()->get_chunk())
    #define READ_CONSTANT() (CURRENT_CHUNK().get_constant(READ_U16()))

    #define REGISTER(idx) (context_->registers_[context_->current_base_ + (idx)])
    #define CONSTANT(idx) (CURRENT_CHUNK().get_constant(idx))

    while (42) {
        context_->current_frame_->ip_ = ip;

        if (ip >= (CURRENT_CHUNK().get_code() + CURRENT_CHUNK().get_code_size())) {
            printl("End of chunk reached, performing implicit return.");
            
            Value return_value = Value(null_t{}); 
            
            // (Thiếu logic closeUpvalues)

            meow::runtime::CallFrame popped_frame = *context_->current_frame_;
            size_t old_base = popped_frame.start_reg_;
            context_->call_stack_.pop_back();

            if (context_->call_stack_.empty()) {
                printl("Call stack empty. Halting.");
                return;
            }

            context_->current_frame_ = &context_->call_stack_.back();
            ip = context_->current_frame_->ip_;
            context_->current_base_ = context_->current_frame_->start_reg_;
            
            if (popped_frame.ret_reg_ != static_cast<size_t>(-1)) {
                context_->registers_[context_->current_base_ + popped_frame.ret_reg_] = return_value;
            }
            
            context_->registers_.resize(old_base);
            
            continue;
        }
        
        uint8_t instruction = READ_BYTE();
        switch (static_cast<OpCode>(instruction)) {
            
            case OpCode::LOAD_CONST: {
                uint16_t dst = READ_U16();
                Value value = READ_CONSTANT();
                REGISTER(dst) = value;
                // printl("load_const r{}", dst);
                break;
            }
            case OpCode::LOAD_NULL: {
                uint16_t dst = READ_U16();
                REGISTER(dst) = Value(null_t{});
                printl("load_null r{}", dst);
                break;
            }
            case OpCode::LOAD_TRUE: {
                uint16_t dst = READ_U16();
                REGISTER(dst) = Value(true);
                printl("load_true r{}", dst);
                break;
            }
            case OpCode::LOAD_FALSE: {
                uint16_t dst = READ_U16();
                REGISTER(dst) = Value(false);
                printl("load_false r{}", dst);
                break;
            }
            case OpCode::MOVE: {
                uint16_t dst = READ_U16();
                uint16_t src = READ_U16();
                REGISTER(dst) = REGISTER(src);
                break;
            }
            case OpCode::LOAD_INT: {
                uint16_t dst = READ_U16();
                int64_t value = READ_I64();
                REGISTER(dst) = Value(value);
                printl("load_int r{}, {}", dst, value);
                break;
            }
            case OpCode::LOAD_FLOAT: {
                uint16_t dst = READ_U16();
                double value = READ_F64();
                REGISTER(dst) = Value(value);
                printl("load_float r{}, {}", dst, value);
                break;
            }

            #define HANDLE_BINARY_OP(OP) \
            case OpCode::OP: { \
                uint16_t dst = READ_U16(); \
                uint16_t r1 = READ_U16(); \
                uint16_t r2 = READ_U16(); \
                auto& left = REGISTER(r1); \
                auto& right = REGISTER(r2); \
                if (auto func = op_dispatcher_->find(OpCode::OP, left, right)) { \
                    REGISTER(dst) = (*func)(left, right); \
                } else { \
                    throw_vm_error("Unsupported binary operator " #OP); \
                } \
                break; \
            }

            HANDLE_BINARY_OP(ADD)
            HANDLE_BINARY_OP(SUB)
            HANDLE_BINARY_OP(MUL)
            HANDLE_BINARY_OP(DIV)
            HANDLE_BINARY_OP(MOD)
            HANDLE_BINARY_OP(POW)
            HANDLE_BINARY_OP(EQ)
            HANDLE_BINARY_OP(NEQ)
            HANDLE_BINARY_OP(GT)
            HANDLE_BINARY_OP(GE)
            HANDLE_BINARY_OP(LT)
            HANDLE_BINARY_OP(LE)
            HANDLE_BINARY_OP(BIT_AND)
            HANDLE_BINARY_OP(BIT_OR)
            HANDLE_BINARY_OP(BIT_XOR)
            HANDLE_BINARY_OP(LSHIFT)
            HANDLE_BINARY_OP(RSHIFT)

            #define HANDLE_UNARY_OP(OP) \
            case OpCode::OP: { \
                uint16_t dst = READ_U16(); \
                uint16_t src = READ_U16(); \
                auto& val = REGISTER(src); \
                if (auto func = op_dispatcher_->find(OpCode::OP, val)) { \
                    REGISTER(dst) = (*func)(val); \
                } else { \
                    throw_vm_error("Unsupported unary operator " #OP); \
                } \
                break; \
            }

            HANDLE_UNARY_OP(NEG)
            HANDLE_UNARY_OP(NOT)
            HANDLE_UNARY_OP(BIT_NOT)

            // --- GLOBALS (Cần logic Module) ---
            case OpCode::GET_GLOBAL:
            case OpCode::SET_GLOBAL:
                throw_vm_error("GET/SET_GLOBAL opcodes not yet implemented in run()");

            // --- UPVALUES (Cần logic Upvalue) ---
            case OpCode::GET_UPVALUE:
            case OpCode::SET_UPVALUE:
            case OpCode::CLOSURE:
            case OpCode::CLOSE_UPVALUES:
                throw_vm_error("Upvalue opcodes not yet implemented in run()");

            case OpCode::JUMP: {
                uint16_t target = READ_ADDRESS();
                ip = CURRENT_CHUNK().get_code() + target;
                break;
            }
            case OpCode::JUMP_IF_FALSE: {
                uint16_t reg = READ_U16();
                uint16_t target = READ_ADDRESS();
                // (Thiếu _isTruthy helper. Giả sử chuyển đổi bool đơn giản)
                bool is_truthy = REGISTER(reg).is_bool() ? REGISTER(reg).as_bool() : !REGISTER(reg).is_null(); // Simplification
                if (!is_truthy) {
                    ip = CURRENT_CHUNK().get_code() + target;
                }
                break;
            }
            case OpCode::JUMP_IF_TRUE: {
                uint16_t reg = READ_U16();
                uint16_t target = READ_ADDRESS();
                bool is_truthy = REGISTER(reg).is_bool() ? REGISTER(reg).as_bool() : !REGISTER(reg).is_null(); // Simplification
                if (is_truthy) {
                    ip = CURRENT_CHUNK().get_code() + target;
                }
                break;
            }

            // --- CALL / RETURN ---
            case OpCode::CALL: {
                uint16_t dst = READ_U16();
                uint16_t fnReg = READ_U16();
                uint16_t argStart = READ_U16();
                uint16_t argc = READ_U16();
                
                Value& callee = REGISTER(fnReg);
                
                if (!callee.is_function()) {
                    throw_vm_error("CALL: Callee is not a function.");
                }
                
                function_t closure = callee.as_function();
                proto_t proto = closure->get_proto();
                
                size_t new_base = context_->registers_.size();
                size_t ret_reg = (dst == 0xFFFF) ? static_cast<size_t>(-1) : static_cast<size_t>(dst);

                // Đảm bảo đủ không gian cho các thanh ghi mới
                context_->registers_.resize(new_base + proto->get_num_registers());
                
                // Sao chép đối số
                for (size_t i = 0; i < argc; ++i) {
                    if (i < proto->get_num_registers()) {
                        context_->registers_[new_base + i] = REGISTER(argStart + i);
                    }
                }
                
                // Lưu IP hiện tại
                context_->current_frame_->ip_ = ip;
                
                // Đẩy CallFrame mới
                context_->call_stack_.emplace_back(closure, new_base, ret_reg, proto->get_chunk().get_code());
                
                // Cập nhật trạng thái VM
                context_->current_frame_ = &context_->call_stack_.back();
                ip = context_->current_frame_->ip_;
                context_->current_base_ = context_->current_frame_->start_reg_;
                
                break;
            }
            case OpCode::CALL_VOID: {
                uint16_t fnReg = READ_U16();
                uint16_t argStart = READ_U16();
                uint16_t argc = READ_U16();
                
                Value& callee = REGISTER(fnReg);
                if (!callee.is_function()) throw_vm_error("CALL_VOID: Callee is not a function.");
                
                function_t closure = callee.as_function();
                proto_t proto = closure->get_proto();
                size_t new_base = context_->registers_.size();
                size_t ret_reg = static_cast<size_t>(-1); // Không có thanh ghi trả về

                context_->registers_.resize(new_base + proto->get_num_registers());
                for (size_t i = 0; i < argc; ++i) {
                    if (i < proto->get_num_registers()) {
                        context_->registers_[new_base + i] = REGISTER(argStart + i);
                    }
                }
                context_->current_frame_->ip_ = ip;
                context_->call_stack_.emplace_back(closure, new_base, ret_reg, proto->get_chunk().get_code());
                context_->current_frame_ = &context_->call_stack_.back();
                ip = context_->current_frame_->ip_;
                context_->current_base_ = context_->current_frame_->start_reg_;
                break;
            }

            case OpCode::RETURN: {
                uint16_t retRegIdx = READ_U16();
                Value return_value = (retRegIdx == 0xFFFF) ? Value(null_t{}) : REGISTER(retRegIdx);
                
                // (Thiếu logic closeUpvalues)

                meow::runtime::CallFrame popped_frame = *context_->current_frame_;
                size_t old_base = popped_frame.start_reg_;
                context_->call_stack_.pop_back();

                if (context_->call_stack_.empty()) {
                    printl("Call stack empty. Halting.");
                    if (!context_->registers_.empty()) context_->registers_[0] = return_value;
                    return; // Kết thúc thực thi
                }

                context_->current_frame_ = &context_->call_stack_.back();
                ip = context_->current_frame_->ip_;
                context_->current_base_ = context_->current_frame_->start_reg_;
                
                if (popped_frame.ret_reg_ != static_cast<size_t>(-1)) {
                    context_->registers_[context_->current_base_ + popped_frame.ret_reg_] = return_value;
                }
                
                // Khôi phục kích thước ngăn xếp thanh ghi
                context_->registers_.resize(old_base);
                
                break;
            }

            // --- DATA STRUCTURES (adaptado từ) ---
            case OpCode::NEW_ARRAY: {
                uint16_t dst = READ_U16();
                uint16_t start_idx = READ_U16();
                uint16_t count = READ_U16();
                
                auto array = heap_->new_array(); //
                array->reserve(count);
                for (size_t i = 0; i < count; ++i) {
                    array->push(REGISTER(start_idx + i));
                }
                REGISTER(dst) = Value(array);
                break;
            }
            case OpCode::NEW_HASH: {
                uint16_t dst = READ_U16();
                uint16_t start_idx = READ_U16();
                uint16_t count = READ_U16();
                
                auto hash_table = heap_->new_hash(); //
                for (size_t i = 0; i < count; ++i) {
                    Value& key = REGISTER(start_idx + i * 2);
                    Value& val = REGISTER(start_idx + i * 2 + 1);
                    if (!key.is_string()) {
                        throw_vm_error("NEW_HASH: Key is not a string.");
                    }
                    hash_table->set(key.as_string(), val); //
                }
                REGISTER(dst) = Value(hash_table);
                break;
            }
            
            case OpCode::GET_INDEX:
            case OpCode::SET_INDEX:
            case OpCode::GET_KEYS:
            case OpCode::GET_VALUES:
            // --- OOP ---
            case OpCode::NEW_CLASS:
            case OpCode::NEW_INSTANCE:
            case OpCode::GET_PROP:
            case OpCode::SET_PROP:
            case OpCode::SET_METHOD:
            case OpCode::INHERIT:
            case OpCode::GET_SUPER:
            // --- TRY/CATCH ---
            case OpCode::THROW:
            case OpCode::SETUP_TRY:
            case OpCode::POP_TRY:
            // --- MODULES ---
            case OpCode::IMPORT_MODULE:
            case OpCode::EXPORT:
            case OpCode::GET_EXPORT:
            case OpCode::IMPORT_ALL:
                throw_vm_error("Opcode not yet implemented in run()");

            // --- HALT ---
            case OpCode::HALT: {
                printl("halt");
                if (!context_->registers_.empty()) {
                   if (REGISTER(0).is_int()) {
                       printl("Final value in R0: {}", REGISTER(0).as_int());
                   }
                }
                return; // Thoát khỏi hàm run()
            }
            default:
                throw_vm_error("Unknown opcode");
        }
    }
}