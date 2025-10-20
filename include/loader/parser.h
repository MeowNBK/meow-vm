#pragma once

#include "common/pch.h"
#include "core/type.h"       // meow::core::proto_t, string_t
#include "core/value.h"      // meow::core::Value
#include "core/op_codes.h"   // meow::core::OpCode
#include "runtime/chunk.h"   // meow::runtime::Chunk
#include "loader/lexer.h"    // meow::loader::Token, TokenType
#include "core/objects/function.h" // meow::core::objects::UpvalueDesc

#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <map>              // Giữ lại map cho labels nếu thứ tự debug quan trọng
#include <tuple>            // Cho pending_jumps

namespace meow::memory { class MemoryManager; } // Forward declaration

namespace meow::loader {

    class TextParser {
    public:
        explicit TextParser(meow::memory::MemoryManager* heap) noexcept;
        TextParser(const TextParser&) = delete;
        TextParser(TextParser&&) = default;
        TextParser& operator=(const TextParser&) = delete;
        TextParser& operator=(TextParser&&) = delete;
        ~TextParser() noexcept = default;

        /**
         * @brief Phân tích tệp văn bản bytecode.
         * @return Proto chính (@main) nếu thành công.
         * @throws std::runtime_error nếu có lỗi đọc file, cú pháp hoặc logic.
         */
        meow::core::proto_t parse_file(const std::string& filepath, meow::memory::MemoryManager& mm);

        /**
         * @brief Phân tích chuỗi văn bản bytecode.
         * @return Proto chính (@main) nếu thành công.
         * @throws std::runtime_error nếu có lỗi cú pháp hoặc logic.
         */
        meow::core::proto_t parse_source(const std::string& source, meow::memory::MemoryManager& mm, const std::string& source_name = "<string>");

        // Lấy tất cả proto đã parse (bao gồm cả @main)
        [[nodiscard]] const std::unordered_map<std::string, meow::core::proto_t>& get_finalized_protos() const;

    private:
        // --- Trạng thái Parser ---
        meow::memory::MemoryManager* heap_ = nullptr;
        std::string current_source_name_;
        std::vector<Token> tokens_;
        size_t current_token_index_ = 0;

        // --- Dữ liệu Proto đang xây dựng ---
        struct ProtoParseData {
            std::string name;             // Tên hàm (không có @)
            size_t num_registers = 0;     // Đọc từ .registers
            size_t num_upvalues = 0;      // Đọc từ .upvalues
            meow::runtime::Chunk chunk;   // Xây dựng chunk tại đây
            std::vector<meow::core::objects::UpvalueDesc> upvalue_descs; // Thu thập mô tả
            // Dùng map để debug dễ hơn, unordered_map nếu hiệu năng quan trọng hơn
            std::map<std::string_view, size_t> labels; // Nhãn -> vị trí bytecode trong chunk này
            // { instruction_opcode_offset, patch_target_offset_in_chunk, label_name }
            std::vector<std::tuple<size_t, size_t, std::string_view>> pending_jumps;
             bool registers_defined = false;
             bool upvalues_defined = false;
             // Thêm thông tin vị trí để báo lỗi tốt hơn
             size_t func_directive_line = 0;
             size_t func_directive_col = 0;

        };

        // Dùng con trỏ để có thể là nullptr khi không ở trong hàm nào
        ProtoParseData* current_proto_data_ = nullptr;
        // Dùng map để chứa dữ liệu tạm thời, string key là tên hàm
        std::map<std::string, ProtoParseData> building_protos_;
        // Kết quả cuối cùng sau khi link
        std::unordered_map<std::string, meow::core::proto_t> finalized_protos_;

        // --- Hàm Parsing chính ---
        void parse(); // Hàm chính điều khiển vòng lặp parsing
        void parse_statement();
        void parse_func_directive();
        // void parse_endfunc_directive(); // Được xử lý trong parse_func_directive
        void parse_registers_directive();
        void parse_upvalues_directive();
        void parse_const_directive();
        void parse_upvalue_directive();
        void parse_label_definition();
        void parse_instruction();

        // --- Hàm tiện ích Parser ---
        [[nodiscard]] const Token& current_token() const;
        [[nodiscard]] const Token& peek_token(size_t offset = 1) const;
        [[nodiscard]] bool is_at_end() const;
        void advance();
        // Trả về token đã consume để dễ lấy thông tin vị trí
        const Token& consume_token(TokenType expected, const std::string& error_message);
        bool match_token(TokenType type); // Kiểm tra và advance nếu khớp

        // Phân tích giá trị hằng số từ 1 hoặc nhiều token
        meow::core::Value parse_const_value_from_tokens();
        // Hàm giải mã escape sequences trong chuỗi
        [[nodiscard]] static std::string unescape_string(const std::string& escaped);

        // Xử lý label và linking sau khi parse xong
        void resolve_labels_for_proto(ProtoParseData& data);
        void link_protos(); // Thay thế các chuỗi "@ProtoName" bằng con trỏ proto thật

        // --- Error Handling ---
        // Ném std::runtime_error với thông tin vị trí
        [[noreturn]] void throw_parse_error(const std::string& message);
        [[noreturn]] void throw_parse_error(const std::string& message, const Token& token);
    };

} // namespace meow::loader