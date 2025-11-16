#pragma once

#include "common/pch.h"  // includes all needed
#include "loader/tokens.h"

namespace meow::loader {

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

    [[nodiscard]] unsigned char peek_char(size_t range = 0) const noexcept;
    [[nodiscard]] unsigned char next_char() const noexcept {
        return peek_char(1);
    }
    void advance() noexcept;
    void synchronize() noexcept;
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