// --- Old codes ---

#include "vm/meow_vm.h"
#include "runtime/execution_context.h"
#include "runtime/builtin_registry.h"
#include "memory/memory_manager.h"
#include "memory/mark_sweep_gc.h"
#include "module/module_manager.h"
#include "runtime/operator_dispatcher.h"
#include "core/op_codes.h"

#include <iostream>

using namespace meow::vm;
using namespace meow::core;

// --- Constructor & Destructor ---

MeowVM::MeowVM(const std::string& entry_point_directory, const std::string& entry_path, int argc, char* argv[]) {
    // Lưu các đối số dòng lệnh
    args_.entry_point_directory_ = entry_point_directory;
    args_.entry_path_ = entry_path;
    for (int i = 0; i < argc; ++i) {
        args_.command_line_arguments_.emplace_back(argv[i]);
    }

    // Khởi tạo các thành phần cốt lõi của VM
    // Cần có thứ tự khởi tạo đúng để tránh phụ thuộc vòng tròn
    context_ = std::make_unique<meow::runtime::ExecutionContext>();
    builtins_ = std::make_unique<meow::runtime::BuiltinRegistry>();

    // GC cần context và builtins để trace các đối tượng gốc (root objects)
    auto gc = std::make_unique<meow::memory::MarkSweepGC>(context_.get(), builtins_.get());
    
    // MemoryManager quản lý GC
    heap_ = std::make_unique<meow::memory::MemoryManager>(std::move(gc));

    // Các hệ thống con khác
    mod_manager_ = std::make_unique<meow::module::ModuleManager>();
    op_dispatcher_ = std::make_unique<meow::runtime::OperatorDispatcher>(heap_.get());

    std::cout << "MeowVM initialized successfully!" << std::endl;
}

MeowVM::~MeowVM() {
    std::cout << "MeowVM shutting down." << std::endl;
}


// --- Public API ---

void MeowVM::interpret() noexcept {
    prepare();
    run();
}


// --- Execution Internals ---

void MeowVM::prepare() noexcept {
    // Hiện tại chưa cần chuẩn bị gì đặc biệt
    // Trong tương lai, có thể dùng để load các thư viện built-in, etc.
    std::cout << "Preparing for execution..." << std::endl;
}

void MeowVM::run() {
    std::cout << "Starting MeowVM execution loop..." << std::endl;
    
    // Giả lập một CallFrame và Chunk để kiểm tra
    // Trong thực tế, trình biên dịch sẽ tạo ra Chunk này
    meow::runtime::Chunk test_chunk;
    // Nạp một hằng số integer 123 vào thanh ghi 0
    uint16_t const_idx = test_chunk.add_constant(Value(123));
    test_chunk.write_byte(static_cast<uint8_t>(OpCode::LOAD_INT));
    test_chunk.write_byte(0); // Đích: thanh ghi 0
    test_chunk.write_short(123); // Giá trị integer
    
    // Dừng máy ảo
    test_chunk.write_byte(static_cast<uint8_t>(OpCode::HALT));
    
    // Tạo một đối tượng hàm giả
    auto main_proto = heap_->new_proto(1, 0, heap_->new_string("main"), std::move(test_chunk));
    auto main_func = heap_->new_function(main_proto);

    // Thiết lập call frame đầu tiên
    context_->registers_.resize(1); // Cần 1 thanh ghi
    context_->call_stack_.emplace_back(main_func, 0, 0, main_func->get_proto()->get_chunk().get_code());
    context_->current_frame_ = &context_->call_stack_.back();

    // Con trỏ chỉ lệnh (Instruction Pointer)
    const uint8_t* ip = context_->current_frame_->ip_;

    // Macro để đọc bytecode
    #define READ_BYTE() (*ip++)
    #define READ_SHORT() (ip += 2, (uint16_t)((ip[-2] | (ip[-1] << 8))))
    #define READ_CONSTANT() (context_->current_frame_->function_->get_proto()->get_chunk().get_constant(READ_BYTE()))

    while (true) {
        uint8_t instruction = READ_BYTE();
        switch (static_cast<OpCode>(instruction)) {
            case OpCode::LOAD_INT: {
                uint8_t reg_dst = READ_BYTE();
                uint16_t value = READ_SHORT();
                context_->registers_[reg_dst] = Value(static_cast<int64_t>(value));
                std::cout << "LOAD_INT: R(" << (int)reg_dst << ") = " << value << std::endl;
                break;
            }
            case OpCode::HALT: {
                std::cout << "HALT: Execution finished." << std::endl;
                // In giá trị thanh ghi để xác nhận
                if (!context_->registers_.empty()) {
                   if (context_->registers_[0].is_int()) {
                       std::cout << "Final value in R(0): " << context_->registers_[0].as_int() << std::endl;
                   }
                }
                return;
            }
            default:
                throwVMError("Unknown opcode");
        }
    }
}