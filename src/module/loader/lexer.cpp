#include "module/loader/lexer.h"
#include "core/op_codes.h"

namespace meow::loader {

static const std::unordered_map<std::string_view, TokenType> DIRECTIVES = {
    {".func", TokenType::DIR_FUNC},
    {".endfunc", TokenType::DIR_ENDFUNC},
    {".registers", TokenType::DIR_REGISTERS},
    {".upvalues", TokenType::DIR_UPVALUES},
    {".upvalue", TokenType::DIR_UPVALUE}, {".const", TokenType::DIR_CONST}
};

static const std::array<std::string_view, static_cast<size_t>(meow::core::OpCode::TOTAL_OPCODES)> OPCODES = [] {
    std::array<std::string_view, static_cast<size_t>(meow::core::OpCode::TOTAL_OPCODES)> array = {
        "LOAD_CONST", "LOAD_NULL",     "LOAD_TRUE",     "LOAD_FALSE", "LOAD_INT",   "LOAD_FLOAT",   "MOVE",        "ADD",       "SUB",
        "MUL",        "DIV",           "MOD",           "POW",        "EQ",         "NEQ",          "GT",          "GE",        "LT",
        "LE",         "NEG",           "NOT",           "GET_GLOBAL", "SET_GLOBAL", "GET_UPVALUE",  "SET_UPVALUE", "CLOSURE",   "CLOSE_UPVALUES",
        "JUMP",       "JUMP_IF_FALSE", "JUMP_IF_TRUE",  "CALL",       "CALL_VOID",  "RETURN",       "HALT",        "NEW_ARRAY", "NEW_HASH",
        "GET_INDEX",  "SET_INDEX",     "GET_KEYS",      "GET_VALUES", "NEW_CLASS",  "NEW_INSTANCE", "GET_PROP",    "SET_PROP",  "SET_METHOD",
        "INHERIT",    "GET_SUPER",     "BIT_AND",       "BIT_OR",     "BIT_XOR",    "BIT_NOT",      "LSHIFT",      "RSHIFT",    "THROW",
        "SETUP_TRY",  "POP_TRY",       "IMPORT_MODULE", "EXPORT",     "GET_EXPORT", "IMPORT_ALL",
    };
    std::sort(array.begin(), array.end());
    return array;
}();

static constexpr std::array<std::string_view, static_cast<size_t>(TokenType::TOTAL_TOKENS)> TOKENS = {
    // Directives
    "DIR_FUNC", "DIR_ENDFUNC", "DIR_REGISTERS", "DIR_UPVALUES", "DIR_UPVALUE", "DIR_CONST",

    // Symbols
    "LABEL_DEF", "IDENTIFIER", "OPCODE",

    // Literals
    "NUMBER_INT", "NUMBER_FLOAT", "STRING",

    // Other
    "END_OF_FILE", "UNKNOWN"};

static constexpr inline bool is_digit(unsigned char c) noexcept {
    return c >= '0' && c <= '9';
}
static constexpr inline bool is_xdigit(unsigned char c) noexcept {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
static constexpr inline bool is_bdigit(unsigned char c) noexcept {
    return (c == '0' || c == '1');
}
static constexpr inline bool is_odigit(unsigned char c) noexcept {
    return (c >= '0' && c <= '7');
}
static constexpr inline bool is_alpha(unsigned char c) noexcept {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
static constexpr inline bool is_alnum(unsigned char c) noexcept {
    return is_alpha(c) || is_digit(c);
}
static constexpr inline bool is_space(unsigned char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

[[nodiscard]] bool is_opcode(std::string_view lexeme) noexcept {
    std::string s(lexeme);
    for (auto& c : s) {
        if (c >= 'a' && c <= 'z') c &= 0xDF;
    }
    return std::binary_search(OPCODES.begin(), OPCODES.end(), s);
}

// --- Token ---
std::string Token::to_string() const {
    std::string_view type_str = TOKENS[static_cast<size_t>(type)];
    return std::format("[{}:{}] {} '{}'", line, col, type_str, lexeme);
}

// --- Lexer ---
Lexer::Lexer(std::string_view source) : src_(source), pos_(0), line_(1), col_(1), curr_(src_.empty() ? '\0' : src_[0]), line_starts_({0}) {
}

unsigned char Lexer::peek_char(size_t range) const noexcept {
    size_t dest_pos = pos_ + range;
    return dest_pos < src_.size() ? src_[dest_pos] : '\0';
}

void Lexer::advance() noexcept {
    if (curr_ == '\n') {
        line_starts_.push_back(pos_ + 1);
        ++line_;
        col_ = 1;
    } else
        ++col_;
    ++pos_;
    curr_ = pos_ < src_.size() ? src_[pos_] : '\0';
}

void Lexer::synchronize() noexcept {
    curr_ = pos_ < src_.size() ? src_[pos_] : '\0';

    auto it = std::upper_bound(line_starts_.begin(), line_starts_.end(), pos_);
    if (it == line_starts_.begin()) {
        line_ = 1;
        col_ = pos_ + 1;
    } else {
        line_ = std::distance(line_starts_.begin(), it);
        col_ = pos_ - line_starts_[line_ - 1] + 1;
    }
}

void Lexer::retreat(size_t range) noexcept {
    if (range == 0) return;

    if (range > pos_) {
        pos_ = 0;
    } else {
        pos_ -= range;
    }

    synchronize();
}

bool Lexer::is_at_end() const noexcept {
    return pos_ >= src_.size();
}

bool Lexer::is_at_end(size_t index) const noexcept {
    return index >= src_.size();
}

Token Lexer::make_token(TokenType type) const {
    return Token{src_.substr(token_start_pos_, pos_ - token_start_pos_), type, token_start_line_, token_start_col_};
}

Token Lexer::make_token(TokenType type, size_t length) const {
    return Token{src_.substr(token_start_pos_, length), type, token_start_line_, token_start_col_};
}

void Lexer::skip_whitespace() noexcept {
    while (is_space(curr_) || curr_ == ',') advance();
}

void Lexer::skip_comments() noexcept {
    advance();
    while (curr_ != '\n' && curr_ != '\0') advance();
}

Token Lexer::scan_identifier() {
    bool is_directive = (curr_ == '.');
    if (is_directive) {
        advance();
        if (!(is_alpha(curr_) || curr_ == '_' || curr_ == '@')) {
            pos_ = token_start_pos_ + 1;
            synchronize();
            return make_token(TokenType::UNKNOWN);
        }
    }

    if (curr_ == '@') {
        advance();
        while (is_alnum(curr_) || curr_ == '_') advance();
    } else {
        while (is_alnum(curr_) || curr_ == '_') advance();
    }

    std::string_view lexeme = src_.substr(token_start_pos_, pos_ - token_start_pos_);

    if (is_directive) {
        auto it = DIRECTIVES.find(lexeme);
        if (it != DIRECTIVES.end()) {
            return make_token(it->second);
        }
        return make_token(TokenType::UNKNOWN);
    } else {
        if (is_opcode(lexeme)) return make_token(TokenType::OPCODE);
        return make_token(TokenType::IDENTIFIER);
    }
}

Token Lexer::scan_number() {
    size_t start_pos = pos_;
    if (curr_ == '+' || curr_ == '-') {
        advance();
        if (!is_digit(curr_)) {
            pos_ = start_pos;
            synchronize();
            return make_token(TokenType::UNKNOWN);
        }
    }

    if (curr_ == '0' && !is_at_end(pos_ + 1)) {
        unsigned char n = next_char();
        unsigned char nl = static_cast<unsigned char>(n | 0x20);
        if (nl == 'x' || nl == 'b' || nl == 'o') {
            advance();
            advance();
            size_t digits = 0;
            if (nl == 'x') {
                while (is_xdigit(curr_)) {
                    advance();
                    ++digits;
                }
                if (digits == 0) {
                    pos_ = start_pos;
                    synchronize();
                    return make_token(TokenType::UNKNOWN);
                }
            } else if (nl == 'b') {
                while (is_bdigit(curr_)) {
                    advance();
                    ++digits;
                }
                if (digits == 0) {
                    pos_ = start_pos;
                    synchronize();
                    return make_token(TokenType::UNKNOWN);
                }
            } else {
                while (is_odigit(curr_)) {
                    advance();
                    ++digits;
                }
                if (digits == 0) {
                    pos_ = start_pos;
                    synchronize();
                    return make_token(TokenType::UNKNOWN);
                }
            }
            return make_token(TokenType::NUMBER_INT);
        }
    }

    bool is_float = false;
    while (is_digit(curr_)) advance();

    if (curr_ == '.' && is_digit(next_char())) {
        is_float = true;
        advance();
        while (is_digit(curr_)) advance();
    }

    if (curr_ == 'e' || curr_ == 'E') {
        size_t exp_start_pos = pos_;
        advance();
        if (curr_ == '+' || curr_ == '-') advance();

        if (!is_digit(curr_)) {
            pos_ = exp_start_pos;
            synchronize();
        } else {
            while (is_digit(curr_)) advance();
            is_float = true;
        }
    }

    return make_token(is_float ? TokenType::NUMBER_FLOAT : TokenType::NUMBER_INT);
}

Token Lexer::scan_string(unsigned char delimiter) {
    advance();

    while (!is_at_end()) {
        if (curr_ == '\\') {
            advance();
            if (!is_at_end()) {
                advance();
            } else {
                return make_token(TokenType::UNKNOWN);
            }
        } else if (curr_ == delimiter) {
            advance();
            return make_token(TokenType::STRING);
        } else if (curr_ == '\n') {
            return make_token(TokenType::UNKNOWN);
        } else {
            advance();
        }
    }

    return make_token(TokenType::UNKNOWN);
}

Token Lexer::scan_token() {
    skip_whitespace();

    token_start_pos_ = pos_;
    token_start_line_ = line_;
    token_start_col_ = col_;

    if (is_at_end()) {
        return make_token(TokenType::END_OF_FILE);
    } else if (curr_ == '.') {
        if (!is_at_end(pos_ + 1) && (is_alpha(next_char()) || next_char() == '_')) {
            return scan_identifier();
        } else {
            advance();
            return make_token(TokenType::UNKNOWN);
        }
    } else if (is_alpha(curr_) || curr_ == '_' || curr_ == '@') {
        Token token = scan_identifier();
        if (token.type == TokenType::IDENTIFIER && curr_ == ':') {
            advance();
            return make_token(TokenType::LABEL_DEF, token.lexeme.length());
        }
        return token;
    } else if (is_digit(curr_) || ((curr_ == '-' || curr_ == '+') && is_digit(next_char()))) {
        return scan_number();
    } else if (curr_ == '"' || curr_ == '\'') {
        return scan_string(curr_);
    } else if (curr_ == ':') {
        advance();
        return make_token(TokenType::UNKNOWN);
    } else if (curr_ == '#') {
        skip_comments();
        return scan_token();
    }

    advance();
    return make_token(TokenType::UNKNOWN);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    tokens.reserve(src_.size() / 2);
    Token token;
    do {
        token = scan_token();
        tokens.push_back(token);
        // } while (token.type != TokenType::END_OF_FILE && token.type != TokenType::UNKNOWN);
    } while (token.type != TokenType::END_OF_FILE);

    tokens.shrink_to_fit();
    return tokens;
}

}  // namespace meow::loader