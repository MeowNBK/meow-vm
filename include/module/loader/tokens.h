#pragma once

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

}  // namespace meow::loader