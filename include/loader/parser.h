// // --- Old codes ---

// #pragma once

// #include "common/pch.h"
// #include "core/type.h"
// #include "core/value.h"
// #include "core/op_codes.h"
// #include "runtime/chunk.h"
// #include "loader/lexer.h" // Sử dụng Lexer mới
// #include "core/objects/function.h"
// #include <vector>
// #include <map> // Dùng map thay unordered_map cho ProtoParseData để giữ thứ tự (nếu cần debug)

// namespace meow::memory { class MemoryManager; }

// namespace meow::loader {

//     class TextParser {
//     public:
//         TextParser() = default;

//         /**
//          * @brief Phân tích tệp văn bản bytecode.
//          * @return Proto chính (@main) nếu thành công.
//          * @throws std::runtime_error nếu có lỗi cú pháp hoặc logic.
//          */
//         meow::core::proto_t parse_file(const std::string& filepath, meow::memory::MemoryManager& mm);

//         /**
//          * @brief Phân tích chuỗi văn bản bytecode.
//          * @return Proto chính (@main) nếu thành công.
//          * @throws std::runtime_error nếu có lỗi cú pháp hoặc logic.
//          */
//         meow::core::proto_t parse_source(const std::string& source, meow::memory::MemoryManager& mm, const std::string& source_name = "<string>");

//         [[nodiscard]] std::unordered_map<std::string, meow::core::proto_t> get_protos() const;

//     private:
//         // --- Trạng thái Parser ---
//         meow::memory::MemoryManager* memory_manager_ = nullptr;
//         std::string current_source_name_;
//         std::vector<Token> tokens_;
//         size_t current_token_index_ = 0;

//         // --- Dữ liệu Proto ---
//         struct ProtoParseData {
//             std::string name;
//             size_t num_registers = 0; // Đọc từ .registers
//             size_t num_upvalues = 0;  // Đọc từ .upvalues
//             meow::runtime::Chunk chunk;              // Xây dựng chunk tại đây
//             std::vector<meow::core::objects::UpvalueDesc> upvalue_descs; // Thu thập mô tả
//             std::map<std::string_view, size_t> labels; // Nhãn -> vị trí bytecode trong chunk này
//             std::vector<std::tuple<size_t, size_t, std::string_view>> pending_jumps; // { instruction_offset, patch_offset_in_chunk, label_name }
//              bool registers_defined = false;
//              bool upvalues_defined = false;
//         };
//         ProtoParseData* current_proto_data_ = nullptr; // Con trỏ đến dữ liệu proto đang xử lý
//         std::unordered_map<std::string, ProtoParseData> building_protos_; // Tên proto -> Dữ liệu đang xây dựng
//         std::unordered_map<std::string, meow::core::proto_t> finalized_protos_; // Tên proto -> Proto đã hoàn thành

//         // --- Hàm Parsing chính ---
//         void parse(); // Hàm chính điều khiển vòng lặp parsing
//         void parse_statement();
//         void parse_func_directive();
//         void parse_endfunc_directive();
//         void parse_registers_directive();
//         void parse_upvalues_directive();
//         void parse_const_directive();
//         void parse_upvalue_directive();
//         void parse_label_definition();
//         void parse_instruction();

//         // --- Hàm tiện ích Parser ---
//         [[nodiscard]] const Token& current_token() const;
//         [[nodiscard]] const Token& peek_token(size_t offset = 1) const;
//         [[nodiscard]] bool is_at_end() const;
//         void advance();
//         const Token& consume_token(TokenType expected, const std::string& error_message);
//         bool match_token(TokenType type); // Kiểm tra và advance nếu khớp

//         meow::core::Value parse_const_value_from_tokens(); // Xử lý hằng số nhiều token
//         void resolve_labels_for_proto(ProtoParseData& data);
//         void link_protos();

//         // --- Error Handling ---
//         [[noreturn]] void throw_parse_error(const std::string& message);
//         [[noreturn]] void throw_parse_error(const std::string& message, const Token& token);
//     };

// } // namespace meow::loader