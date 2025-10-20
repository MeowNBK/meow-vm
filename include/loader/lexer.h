#pragma once

#include "common/pch.h"
#include <string_view>
#include <vector>
#include <string> // Thêm để dùng trong Token::to_string

namespace meow::loader {

    enum class TokenType {
        // Directives
        DIR_FUNC,       // .func
        DIR_ENDFUNC,    // .endfunc
        DIR_REGISTERS,  // .registers
        DIR_UPVALUES,   // .upvalues
        DIR_UPVALUE,    // .upvalue
        DIR_CONST,      // .const

        // Symbols
        LABEL_DEF,      // nhan:
        IDENTIFIER,     // ten_ham, ten_bien, ten_nhan_nhay_toi, @ProtoName
        OPCODE,         // LOAD_CONST, ADD, JUMP etc.

        // Literals
        NUMBER_INT,     // 123, -45, 0xFF, 0b101
        NUMBER_FLOAT,   // 3.14, -0.5, 1e6
        STRING,         // "chuoi ky tu"

        // Other
        END_OF_FILE,
        UNKNOWN,        // Lỗi hoặc ký tự không nhận dạng

        TOTAL_TOKENS
    };

    struct Token {
        std::string_view lexeme;
        TokenType type;
        size_t line = 0;
        size_t col = 0;

        [[nodiscard]] std::string to_string() const; // Để debug
    };

    class Lexer {
    public:
        explicit Lexer(std::string_view source);

        [[nodiscard]] std::vector<Token> tokenize_all();
        [[nodiscard]] Token next_token();

    private:
        std::string_view src_;
        size_t pos_, line_, col_;
        size_t line_start_pos_ = 0; // Vị trí bắt đầu của dòng hiện tại

        unsigned char curr_;

        size_t token_start_pos_ = 0;
        size_t token_start_line_ = 0;
        size_t token_start_col_ = 0;

        [[nodiscard]] unsigned char peek() const noexcept;
        void advance() noexcept;
        [[nodiscard]] bool is_at_end() const noexcept;
        [[nodiscard]] bool is_at_end(size_t idx) const noexcept;

        Token make_token(TokenType type) const;
        Token make_token(TokenType type, size_t length) const;
        Token make_error_token(const std::string& message) const;

        void skip_whitespace_and_comments();
        void skip_whitespace() noexcept;
        void skip_comments() noexcept;
        Token scan_token();
        Token scan_identifier(); // Gộp chung
        Token scan_number();
        Token scan_string(unsigned char delimiter = '"');

        [[nodiscard]] static bool is_identifier_start(unsigned char c);
    };

} // namespace meow::loader