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
#include "core/objects/oop.h" // Thêm
#include "core/objects/module.h" // Thêm

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
using namespace meow::runtime;
using namespace meow::memory;

MeowVM::MeowVM(const std::string& entry_point_directory, const std::string& entry_path, int argc, char* argv[]) {
    args_.entry_point_directory_ = entry_point_directory;
    args_.entry_path_ = entry_path;
    for (int i = 0; i < argc; ++i) {
        args_.command_line_arguments_.emplace_back(argv[i]);
    }

    context_ = std::make_unique<ExecutionContext>();
    builtins_ = std::make_unique<BuiltinRegistry>();

    auto gc = std::make_unique<meow::memory::MarkSweepGC>(context_.get(), builtins_.get());
    
    heap_ = std::make_unique<meow::memory::MemoryManager>(std::move(gc));

    mod_manager_ = std::make_unique<meow::module::ModuleManager>();
    op_dispatcher_ = std::make_unique<OperatorDispatcher>(heap_.get());

    printl("MeowVM initialized successfully!");
}

MeowVM::~MeowVM() {
    printl("MeowVM shutting down.");
}

[[nodiscard]] inline uint8_t operator-(OpCode op_code) noexcept {
    return static_cast<uint8_t>(op_code);
}

void MeowVM::interpret() noexcept {
    prepare();
    try {
        Chunk test_chunk;

        test_chunk.write_byte(-OpCode::LOAD_INT);
        test_chunk.write_u16(0);
        test_chunk.write_u64(123ULL);

        test_chunk.write_byte(-OpCode::LOAD_TRUE);
        test_chunk.write_u16(1); // dst r1
        
        test_chunk.write_byte(-OpCode::HALT);
        
        auto main_proto = heap_->new_proto(2, 0, heap_->new_string("main"), std::move(test_chunk));
        auto main_func = heap_->new_function(main_proto);
        
        // CẬP NHẬT: Tạo module chính
        auto main_module = heap_->new_module(heap_->new_string("main"), heap_->new_string(args_.entry_path_), main_proto);

        context_->registers_.resize(2);
        
        // CẬP NHẬT: Truyền main_module vào CallFrame
        context_->call_stack_.emplace_back(main_func, main_module, 0, static_cast<size_t>(-1), main_func->get_proto()->get_chunk().get_code());

        run();
    } catch (const std::exception& e) {
        printl("An execption was threw: {}", e.what());
    }
}

void MeowVM::prepare() noexcept {
    printl("Preparing for execution...");
}

// --- HÀM TRỢ GIÚP MỚI ---

inline bool is_truthy(param_t value) noexcept {
    if (value.is_null()) return false;
    if (value.is_bool()) return value.as_bool();
    if (value.is_int()) return value.as_int() != 0;
    if (value.is_float()) {
        double r = value.as_float();
        return r != 0.0 && !std::isnan(r);
    }
    if (value.is_string()) return !value.as_string()->empty();
    if (value.is_array()) return !value.as_array()->empty();
    if (value.is_hash_table()) return !value.as_hash_table()->empty();
    return true;
}

upvalue_t capture_upvalue(ExecutionContext* context, MemoryManager* heap, size_t register_index) {
    // Tìm kiếm các upvalue đã mở từ trên xuống dưới (chỉ số stack cao -> thấp)
    for (auto it = context->open_upvalues_.rbegin(); it != context->open_upvalues_.rend(); ++it) {
        upvalue_t uv = *it;
        if (uv->get_index() == register_index) return uv;
        if (uv->get_index() < register_index) break; // Đã đi qua, không cần tìm nữa
    }

    // Không tìm thấy, tạo mới
    upvalue_t new_uv = heap->new_upvalue(register_index);
    
    // Chèn vào danh sách đã sắp xếp (theo chỉ số stack)
    auto it = std::lower_bound(context->open_upvalues_.begin(), context->open_upvalues_.end(), new_uv,
        [](auto a, auto b) { return a->get_index() < b->get_index(); });
    context->open_upvalues_.insert(it, new_uv);
    return new_uv;
}

inline void close_upvalues(ExecutionContext* context, size_t last_index) noexcept {
    // Đóng tất cả upvalue có chỉ số stack >= last_index
    while (!context->open_upvalues_.empty() && context->open_upvalues_.back()->get_index() >= last_index) {
        upvalue_t uv = context->open_upvalues_.back();
        uv->close(context->registers_[uv->get_index()]);
        context->open_upvalues_.pop_back();
    }
}


// --- HÀM RUN() ĐÃ HOÀN THIỆN ---

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
      try { // Thêm try-catch bên trong vòng lặp
        context_->current_frame_->ip_ = ip;

        if (ip >= (CURRENT_CHUNK().get_code() + CURRENT_CHUNK().get_code_size())) {
            printl("End of chunk reached, performing implicit return.");
            
            Value return_value = Value(null_t{}); 
            
            CallFrame popped_frame = *context_->current_frame_;
            size_t old_base = popped_frame.start_reg_;

            // CẬP NHẬT: Đóng upvalues khi return
            close_upvalues(context_.get(), popped_frame.start_reg_);

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

            // --- GLOBALS (ĐÃ HOÀN THIỆN) ---
            case OpCode::GET_GLOBAL: {
                uint16_t dst = READ_U16();
                uint16_t name_idx = READ_U16();
                string_t name = CONSTANT(name_idx).as_string();
                module_t module = context_->current_frame_->module_;
                if (module->has_global(name)) {
                    REGISTER(dst) = module->get_global(name);
                } else {
                    REGISTER(dst) = Value(null_t{}); // Trả về null nếu không tìm thấy
                }
                break;
            }
            case OpCode::SET_GLOBAL: {
                uint16_t name_idx = READ_U16();
                uint16_t src = READ_U16();
                string_t name = CONSTANT(name_idx).as_string();
                module_t module = context_->current_frame_->module_;
                module->set_global(name, REGISTER(src));
                break;
            }

            // --- UPVALUES (ĐÃ HOÀN THIỆN) ---
            case OpCode::GET_UPVALUE: {
                uint16_t dst = READ_U16();
                uint16_t uvIdx = READ_U16();
                upvalue_t uv = context_->current_frame_->function_->get_upvalue(uvIdx);
                if (uv->is_closed()) {
                    REGISTER(dst) = uv->get_value();
                } else {
                    REGISTER(dst) = context_->registers_[uv->get_index()];
                }
                break;
            }
            case OpCode::SET_UPVALUE: {
                uint16_t uvIdx = READ_U16();
                uint16_t src = READ_U16();
                upvalue_t uv = context_->current_frame_->function_->get_upvalue(uvIdx);
                if (uv->is_closed()) {
                    uv->close(REGISTER(src)); // 'close' cũng dùng để set giá trị đã đóng
                } else {
                    context_->registers_[uv->get_index()] = REGISTER(src);
                }
                break;
            }
            case OpCode::CLOSURE: {
                uint16_t dst = READ_U16();
                uint16_t protoIdx = READ_U16();
                proto_t proto = CONSTANT(protoIdx).as_proto();
                function_t closure = heap_->new_function(proto);
                
                for (size_t i = 0; i < proto->get_num_upvalues(); ++i) {
                    const auto& desc = proto->get_desc(i);
                    if (desc.is_local_) {
                        // Bắt upvalue từ thanh ghi của frame hiện tại
                        closure->set_upvalue(i, capture_upvalue(context_.get(), heap_.get(), context_->current_base_ + desc.index_));
                    } else {
                        // Chuyển tiếp upvalue từ closure cha
                        closure->set_upvalue(i, context_->current_frame_->function_->get_upvalue(desc.index_));
                    }
                }
                REGISTER(dst) = Value(closure);
                break;
            }
            case OpCode::CLOSE_UPVALUES: {
                uint16_t last_reg = READ_U16();
                close_upvalues(context_.get(), context_->current_base_ + last_reg);
                break;
            }

            case OpCode::JUMP: {
                uint16_t target = READ_ADDRESS();
                ip = CURRENT_CHUNK().get_code() + target;
                break;
            }
            case OpCode::JUMP_IF_FALSE: {
                uint16_t reg = READ_U16();
                uint16_t target = READ_ADDRESS();
                // CẬP NHẬT: Dùng helper is_truthy
                bool is_truthy_val = is_truthy(REGISTER(reg)); 
                if (!is_truthy_val) {
                    ip = CURRENT_CHUNK().get_code() + target;
                }
                break;
            }
            case OpCode::JUMP_IF_TRUE: {
                uint16_t reg = READ_U16();
                uint16_t target = READ_ADDRESS();
                // CẬP NHẬT: Dùng helper is_truthy
                bool is_truthy_val = is_truthy(REGISTER(reg)); 
                if (is_truthy_val) {
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
                     // (TODO: Xử lý native function, class constructor, bound method ở đây)
                    throw_vm_error("CALL: Callee is not a function.");
                }
                
                function_t closure = callee.as_function();
                proto_t proto = closure->get_proto();
                
                size_t new_base = context_->registers_.size();
                size_t ret_reg = (dst == 0xFFFF) ? static_cast<size_t>(-1) : static_cast<size_t>(dst);

                context_->registers_.resize(new_base + proto->get_num_registers());
                
                for (size_t i = 0; i < argc; ++i) {
                    if (i < proto->get_num_registers()) {
                        context_->registers_[new_base + i] = REGISTER(argStart + i);
                    }
                }
                
                context_->current_frame_->ip_ = ip;
                
                // CẬP NHẬT: Truyền module từ frame hiện tại
                module_t current_module = context_->current_frame_->module_;
                context_->call_stack_.emplace_back(closure, current_module, new_base, ret_reg, proto->get_chunk().get_code());
                
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
                size_t ret_reg = static_cast<size_t>(-1); 

                context_->registers_.resize(new_base + proto->get_num_registers());
                for (size_t i = 0; i < argc; ++i) {
                    if (i < proto->get_num_registers()) {
                        context_->registers_[new_base + i] = REGISTER(argStart + i);
                    }
                }
                context_->current_frame_->ip_ = ip;
                
                // CẬP NHẬT: Truyền module từ frame hiện tại
                module_t current_module = context_->current_frame_->module_;
                context_->call_stack_.emplace_back(closure, current_module, new_base, ret_reg, proto->get_chunk().get_code());
                
                context_->current_frame_ = &context_->call_stack_.back();
                ip = context_->current_frame_->ip_;
                context_->current_base_ = context_->current_frame_->start_reg_;
                break;
            }

            case OpCode::RETURN: {
                uint16_t retRegIdx = READ_U16();
                Value return_value = (retRegIdx == 0xFFFF) ? Value(null_t{}) : REGISTER(retRegIdx);
                
                CallFrame popped_frame = *context_->current_frame_;
                size_t old_base = popped_frame.start_reg_;

                // CẬP NHẬT: Đóng upvalues
                close_upvalues(context_.get(), popped_frame.start_reg_);

                context_->call_stack_.pop_back();

                if (context_->call_stack_.empty()) {
                    printl("Call stack empty. Halting.");
                    if (!context_->registers_.empty()) context_->registers_[0] = return_value;
                    return; 
                }

                context_->current_frame_ = &context_->call_stack_.back();
                ip = context_->current_frame_->ip_;
                context_->current_base_ = context_->current_frame_->start_reg_;
                
                if (popped_frame.ret_reg_ != static_cast<size_t>(-1)) {
                    context_->registers_[context_->current_base_ + popped_frame.ret_reg_] = return_value;
                }
                
                context_->registers_.resize(old_base);
                
                break;
            }

            // --- DATA STRUCTURES (ĐÃ HOÀN THIỆN 1 phần) ---
            case OpCode::NEW_ARRAY: {
                uint16_t dst = READ_U16();
                uint16_t start_idx = READ_U16();
                uint16_t count = READ_U16();
                
                auto array = heap_->new_array();
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
                
                auto hash_table = heap_->new_hash();
                for (size_t i = 0; i < count; ++i) {
                    Value& key = REGISTER(start_idx + i * 2);
                    Value& val = REGISTER(start_idx + i * 2 + 1);
                    if (!key.is_string()) {
                        throw_vm_error("NEW_HASH: Key is not a string.");
                    }
                    hash_table->set(key.as_string(), val);
                }
                REGISTER(dst) = Value(hash_table);
                break;
            }
            
            case OpCode::GET_INDEX: {
                uint16_t dst = READ_U16();
                uint16_t srcReg = READ_U16();
                uint16_t keyReg = READ_U16();
                Value& src = REGISTER(srcReg);
                Value& key = REGISTER(keyReg);

                if (src.is_array()) {
                    if (!key.is_int()) throw_vm_error("Array index must be an integer.");
                    int64_t idx = key.as_int();
                    array_t arr = src.as_array();
                    if (idx < 0 || (uint64_t)idx >= arr->size()) {
                        throw_vm_error("Array index out of bounds.");
                    }
                    REGISTER(dst) = arr->get(idx);
                } else if (src.is_hash_table()) {
                    if (!key.is_string()) throw_vm_error("Hash table key must be a string.");
                    hash_table_t hash = src.as_hash_table();
                    if (hash->has(key.as_string())) {
                        REGISTER(dst) = hash->get(key.as_string());
                    } else {
                        REGISTER(dst) = Value(null_t{});
                    }
                } else if (src.is_string()) {
                     if (!key.is_int()) throw_vm_error("String index must be an integer.");
                    int64_t idx = key.as_int();
                    string_t str = src.as_string();
                    if (idx < 0 || (uint64_t)idx >= str->size()) {
                        throw_vm_error("String index out of bounds.");
                    }
                    REGISTER(dst) = Value(heap_->new_string(std::string(1, str->get(idx))));
                } else {
                     // (TODO: Thêm logic getMagicMethod cho __getindex__ và __getprop__)
                    throw_vm_error("Cannot apply index operator to this type.");
                }
                break;
            }
            case OpCode::SET_INDEX: {
                uint16_t srcReg = READ_U16();
                uint16_t keyReg = READ_U16();
                uint16_t valReg = READ_U16();
                Value& src = REGISTER(srcReg);
                Value& key = REGISTER(keyReg);
                Value& val = REGISTER(valReg);

                if (src.is_array()) {
                    if (!key.is_int()) throw_vm_error("Array index must be an integer.");
                    int64_t idx = key.as_int();
                    array_t arr = src.as_array();
                    if (idx < 0) throw_vm_error("Array index cannot be negative.");
                    if ((uint64_t)idx >= arr->size()) {
                        arr->resize(idx + 1); // Tự động mở rộng
                    }
                    arr->set(idx, val);
                } else if (src.is_hash_table()) {
                    if (!key.is_string()) throw_vm_error("Hash table key must be a string.");
                    hash_table_t hash = src.as_hash_table();
                    hash->set(key.as_string(), val);
                } else {
                    // (TODO: Thêm logic getMagicMethod cho __setindex__ và __setprop__)
                    throw_vm_error("Cannot apply index set operator to this type.");
                }
                break;
            }
            case OpCode::GET_KEYS: {
                uint16_t dst = READ_U16();
                uint16_t srcReg = READ_U16();
                Value& src = REGISTER(srcReg);
                auto keys_array = heap_->new_array();
                if (src.is_hash_table()) {
                    hash_table_t hash = src.as_hash_table();
                    for (auto it = hash->begin(); it != hash->end(); ++it) {
                        keys_array->push(Value(it->first));
                    }
                } // (TODO: Thêm logic cho Instance, Array, String)
                REGISTER(dst) = Value(keys_array);
                break;
            }
            case OpCode::GET_VALUES: {
                uint16_t dst = READ_U16();
                uint16_t srcReg = READ_U16();
                Value& src = REGISTER(srcReg);
                auto vals_array = heap_->new_array();
                if (src.is_hash_table()) {
                    hash_table_t hash = src.as_hash_table();
                    for (auto it = hash->begin(); it != hash->end(); ++it) {
                        vals_array->push(it->second);
                    }
                } // (TODO: Thêm logic cho Instance, Array, String)
                REGISTER(dst) = Value(vals_array);
                break;
            }
            
            // --- OOP (ĐÃ HOÀN THIỆN 1 phần) ---
            case OpCode::NEW_CLASS: {
                uint16_t dst = READ_U16();
                uint16_t name_idx = READ_U16();
                string_t name = CONSTANT(name_idx).as_string();
                REGISTER(dst) = Value(heap_->new_class(name));
                break;
            }
            case OpCode::NEW_INSTANCE: {
                uint16_t dst = READ_U16();
                uint16_t classReg = READ_U16();
                Value& classVal = REGISTER(classReg);
                if (!classVal.is_class()) throw_vm_error("NEW_INSTANCE: operand is not a class.");
                REGISTER(dst) = Value(heap_->new_instance(classVal.as_class()));
                break;
            }
            case OpCode::GET_PROP: {
                uint16_t dst = READ_U16();
                uint16_t objReg = READ_U16();
                uint16_t name_idx = READ_U16();
                Value& obj = REGISTER(objReg);
                string_t name = CONSTANT(name_idx).as_string();

                if (obj.is_instance()) {
                    instance_t inst = obj.as_instance();
                    // 1. Tìm trong fields
                    if (inst->has_field(name)) {
                        REGISTER(dst) = inst->get_field(name);
                        break;
                    }
                    // 2. Tìm trong methods (và bind)
                    class_t k = inst->get_class();
                    while (k) {
                        if (k->has_method(name)) {
                            REGISTER(dst) = Value(heap_->new_bound_method(inst, k->get_method(name).as_function()));
                            break;
                        }
                        k = k->get_super();
                    }
                    if (k) break; // Đã tìm thấy method
                }
                // (TODO: Thêm logic cho module và magic methods)
                REGISTER(dst) = Value(null_t{}); // Không tìm thấy
                break;
            }
            case OpCode::SET_PROP: {
                uint16_t objReg = READ_U16();
                uint16_t name_idx = READ_U16();
                uint16_t valReg = READ_U16();
                Value& obj = REGISTER(objReg);
                string_t name = CONSTANT(name_idx).as_string();
                Value& val = REGISTER(valReg);

                if (obj.is_instance()) {
                    obj.as_instance()->set_field(name, val);
                } else {
                    throw_vm_error("SET_PROP: can only set properties on instances.");
                }
                break;
            }
            case OpCode::SET_METHOD: {
                uint16_t classReg = READ_U16();
                uint16_t name_idx = READ_U16();
                uint16_t methodReg = READ_U16();
                Value& classVal = REGISTER(classReg);
                string_t name = CONSTANT(name_idx).as_string();
                Value& methodVal = REGISTER(methodReg);
                if (!classVal.is_class()) throw_vm_error("SET_METHOD: target is not a class.");
                if (!methodVal.is_function()) throw_vm_error("SET_METHOD: value is not a function.");
                classVal.as_class()->set_method(name, methodVal);
                break;
            }
            case OpCode::INHERIT: {
                uint16_t subReg = READ_U16();
                uint16_t superReg = READ_U16();
                Value& subVal = REGISTER(subReg);
                Value& superVal = REGISTER(superReg);
                if (!subVal.is_class() || !superVal.is_class()) throw_vm_error("INHERIT: operands must be classes.");
                class_t sub = subVal.as_class();
                class_t super = superVal.as_class();
                sub->set_super(super);
                // (TODO: Thêm logic sao chép methods từ superclass)
                break;
            }
            case OpCode::GET_SUPER:
                // (Logic này phức tạp, yêu cầu truy cập 'this' và superclass)
                throw_vm_error("Opcode GET_SUPER not yet implemented in run()");
                break;

            // --- TRY/CATCH (ĐÃ HOÀN THIỆN) ---
            case OpCode::THROW: {
                uint16_t reg = READ_U16();
                // (Tạm thời ném string, lý tưởng nên ném chính Value đó)
                throw_vm_error("Explicit throw"); // (Cần helper _toString)
                break;
            }
            case OpCode::SETUP_TRY: {
                uint16_t target = READ_ADDRESS();
                size_t catch_ip = target;
                size_t frame_depth = context_->call_stack_.size() - 1;
                size_t stack_depth = context_->registers_.size(); 
                context_->exception_handlers_.emplace_back(catch_ip, frame_depth, stack_depth);
                break;
            }
            case OpCode::POP_TRY: {
                if (!context_->exception_handlers_.empty()) {
                    context_->exception_handlers_.pop_back();
                }
                break;
            }

            // --- MODULES (ĐÃ HOÀN THIỆN 1 phần) ---
            case OpCode::IMPORT_MODULE: {
                uint16_t dst = READ_U16();
                uint16_t pathIdx = READ_U16();
                string_t path = CONSTANT(pathIdx).as_string();
                string_t importer_path = context_->current_frame_->module_->get_file_path();
                
                // (Logic load module thực tế nằm trong mod_manager_
                // và có thể yêu cầu gọi đệ quy vào VM)
                // module_t mod = mod_manager_->load_module(path, importer_path, heap_.get(), this);
                // REGISTER(dst) = Value(mod);
                // (TODO: Xử lý việc chạy code của module nếu chưa chạy)
                throw_vm_error("Opcode IMPORT_MODULE not yet implemented in run()");
                break;
            }
            case OpCode::EXPORT: {
                uint16_t name_idx = READ_U16();
                uint16_t srcReg = READ_U16();
                string_t name = CONSTANT(name_idx).as_string();
                context_->current_frame_->module_->set_export(name, REGISTER(srcReg));
                break;
            }
            case OpCode::GET_EXPORT: {
                uint16_t dst = READ_U16();
                uint16_t modReg = READ_U16();
                uint16_t name_idx = READ_U16();
                Value& modVal = REGISTER(modReg);
                string_t name = CONSTANT(name_idx).as_string();
                if (!modVal.is_module()) throw_vm_error("GET_EXPORT: operand is not a module.");
                module_t mod = modVal.as_module();
                if (!mod->has_export(name)) throw_vm_error("Module does not export name.");
                REGISTER(dst) = mod->get_export(name);
                break;
            }
            case OpCode::IMPORT_ALL:
                // (Logic này yêu cầu sao chép tất cả export vào globals của module hiện tại)
                throw_vm_error("Opcode IMPORT_ALL not yet implemented in run()");
                break;

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
      } catch (const VMError& e) { // CẬP NHẬT: Khối catch
            printl("An execption was threw: {}", e.what());
            
            if (context_->exception_handlers_.empty()) {
                printl("No exception handler. Halting.");
                // (In stack trace chi tiết ở đây)
                return; // Không có handler, thoát VM
            }
            
            // Có handler, bắt đầu unwind
            ExceptionHandler handler = context_->exception_handlers_.back();
            context_->exception_handlers_.pop_back();

            // 1. Unwind call stack về đúng frame
            while (context_->call_stack_.size() - 1 > handler.frame_depth_) {
                close_upvalues(context_.get(), context_->call_stack_.back().start_reg_);
                context_->call_stack_.pop_back();
            }
            
            // 2. Khôi phục kích thước ngăn xếp thanh ghi
            context_->registers_.resize(handler.stack_depth_);
            
            // 3. Thiết lập lại trạng thái VM để nhảy đến code catch
            context_->current_frame_ = &context_->call_stack_.back();
            ip = CURRENT_CHUNK().get_code() + handler.catch_ip_;
            context_->current_base_ = context_->current_frame_->start_reg_;

            // 4. (Tùy chọn) Đẩy đối tượng exception vào một thanh ghi (ví dụ: R0)
            if (context_->current_base_ < context_->registers_.size()) {
                REGISTER(0) = Value(heap_->new_string(e.what()));
            }
      }
    }
}