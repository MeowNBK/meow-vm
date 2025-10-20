// --- Old codes ---

// #pragma once

// #include "common/pch.h"
// #include <string_view>
// #include <vector>

// namespace meow::loader {

//     enum class TokenType {
//         // Directives
//         DIR_FUNC,       // .func
//         DIR_ENDFUNC,    // .endfunc
//         DIR_REGISTERS,  // .registers
//         DIR_UPVALUES,   // .upvalues
//         DIR_UPVALUE,    // .upvalue
//         DIR_CONST,      // .const

//         // Symbols
//         LABEL_DEF,      // nhan:
//         IDENTIFIER,     // ten_ham, ten_bien, ten_nhan_nhay_toi
//         OPCODE,         // LOAD_CONST, ADD, JUMP etc.

//         // Literals
//         NUMBER_INT,     // 123, -45, 0xFF, 0b101
//         NUMBER_FLOAT,   // 3.14, -0.5, 1e6
//         STRING,         // "chuoi ky tu"

//         // Other
//         END_OF_FILE,
//         UNKNOWN
//     };

//     struct Token {
//         std::string_view lexeme;
//         TokenType type;
//         size_t line = 0;
//         size_t col = 0; // Cột bắt đầu của token

//         [[nodiscard]] std::string to_string() const; // Để debug
//     };

//     class Lexer {
//     public:
//         explicit Lexer(std::string_view source);

//         [[nodiscard]] std::vector<Token> tokenize_all();
//         [[nodiscard]] Token next_token();

//     private:
//         std::string_view src_;
//         size_t pos_ = 0;
//         size_t line_ = 1;
//         size_t line_start_pos_ = 0; // Vị trí bắt đầu của dòng hiện tại

//         size_t token_start_pos_ = 0;
//         size_t token_start_line_ = 0;
//         size_t token_start_col_ = 0;

//         [[nodiscard]] char current_char() const noexcept;
//         [[nodiscard]] char peek_char() const noexcept;
//         void advance() noexcept;
//         bool is_at_end() const noexcept;

//         Token make_token(TokenType type);
//         Token make_token(TokenType type, size_t length);
//         Token make_error_token(const std::string& message); // Không dùng trực tiếp, chỉ để debug lexer

//         void skip_whitespace_and_comments();
//         Token scan_token();
//         Token scan_identifier_or_keyword();
//         Token scan_number();
//         Token scan_string();
//         Token scan_directive_or_opcode(); // Xử lý cả '.' và tên lệnh/opcode
//         Token scan_label_def(); // Xử lý nhan:

//         [[nodiscard]] static bool is_label_char(char c);
//         [[nodiscard]] static bool is_identifier_start(char c);
//         [[nodiscard]] static bool is_identifier_char(char c);
//         [[nodiscard]] TokenType check_keyword(std::string_view lexeme);
//     };

// } // namespace meow::loader