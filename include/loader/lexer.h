#pragma once

#include "common/pch.h"  // includes all needed

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
    LABEL_DEF,   // nhan:
    IDENTIFIER,  // ten_ham, ten_bien, ten_nhan_nhay_toi, @ProtoName
    OPCODE,      // LOAD_CONST, ADD, JUMP etc.

    // Literals
    NUMBER_INT,    // 123, -45, 0xFF, 0b101
    NUMBER_FLOAT,  // 3.14, -0.5, 1e6
    STRING,        // "chuoi ky tu"

    // Other
    END_OF_FILE,
    UNKNOWN,  // Lỗi hoặc ký tự không nhận dạng

    TOTAL_TOKENS
};

struct Token {
    std::string_view lexeme;
    TokenType type;
    size_t line = 0;
    size_t col = 0;

    [[nodiscard]] std::string to_string() const;  // Để debug
};

class Lexer {
   public:
    explicit Lexer(std::string_view source);

    [[nodiscard]] std::vector<Token> tokenize();

   private:
    // --- Lexer metadata ---
    std::string_view src_;
    size_t pos_, line_, col_;
    unsigned char curr_;

    std::vector<size_t> line_starts_;

    // --- Token metadata ---
    size_t token_start_pos_ = 0;
    size_t token_start_line_ = 0;
    size_t token_start_col_ = 0;

    [[nodiscard]] unsigned char peek(size_t range = 0) const noexcept;
    [[nodiscard]] unsigned char next() const noexcept { return peek(1); }
    void advance() noexcept;
    void sync() noexcept;
    void retreat(size_t range = 1) noexcept;
    [[nodiscard]] bool is_at_end() const noexcept;
    [[nodiscard]] bool is_at_end(size_t index) const noexcept;

    Token make_token(TokenType type) const;
    Token make_token(TokenType type, size_t length) const;

    void skip_whitespace() noexcept;
    void skip_comments() noexcept;
    Token scan_token();
    Token scan_identifier();
    Token scan_number();
    Token scan_string(unsigned char delimiter = '"');
};

}  // namespace meow::loader