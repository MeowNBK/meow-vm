#include "loader/lexer.h"
#include <cctype>
#include <string>
#include <unordered_map>
#include "core/op_codes.h"

namespace meow::loader {

// --- Bảng từ khóa/chỉ thị ---
static const std::unordered_map<std::string_view, TokenType> DIRECTIVE_MAP = {
    {".func", TokenType::DIR_FUNC},           {".endfunc", TokenType::DIR_ENDFUNC},
    {".registers", TokenType::DIR_REGISTERS}, {".upvalues", TokenType::DIR_UPVALUES},
    {".upvalue", TokenType::DIR_UPVALUE},     {".const", TokenType::DIR_CONST}};

static const std::unordered_map<std::string_view, TokenType> OPCODE_LEX_MAP = [] {
    std::unordered_map<std::string_view, TokenType> map;
    map["LOAD_CONST"] = TokenType::OPCODE;
    map["LOAD_NULL"] = TokenType::OPCODE;
    map["LOAD_TRUE"] = TokenType::OPCODE;
    map["LOAD_FALSE"] = TokenType::OPCODE;
    map["LOAD_INT"] = TokenType::OPCODE;
    map["LOAD_FLOAT"] = TokenType::OPCODE;
    map["MOVE"] = TokenType::OPCODE;
    map["ADD"] = TokenType::OPCODE;
    map["SUB"] = TokenType::OPCODE;
    map["MUL"] = TokenType::OPCODE;
    map["DIV"] = TokenType::OPCODE;
    map["MOD"] = TokenType::OPCODE;
    map["POW"] = TokenType::OPCODE;
    map["EQ"] = TokenType::OPCODE;
    map["NEQ"] = TokenType::OPCODE;
    map["GT"] = TokenType::OPCODE;
    map["GE"] = TokenType::OPCODE;
    map["LT"] = TokenType::OPCODE;
    map["LE"] = TokenType::OPCODE;
    map["NEG"] = TokenType::OPCODE;
    map["NOT"] = TokenType::OPCODE;
    map["GET_GLOBAL"] = TokenType::OPCODE;
    map["SET_GLOBAL"] = TokenType::OPCODE;
    map["GET_UPVALUE"] = TokenType::OPCODE;
    map["SET_UPVALUE"] = TokenType::OPCODE;
    map["CLOSURE"] = TokenType::OPCODE;
    map["CLOSE_UPVALUES"] = TokenType::OPCODE;
    map["JUMP"] = TokenType::OPCODE;
    map["JUMP_IF_FALSE"] = TokenType::OPCODE;
    map["JUMP_IF_TRUE"] = TokenType::OPCODE;
    map["CALL"] = TokenType::OPCODE;
    map["CALL_VOID"] = TokenType::OPCODE;
    map["RETURN"] = TokenType::OPCODE;
    map["HALT"] = TokenType::OPCODE;
    map["NEW_ARRAY"] = TokenType::OPCODE;
    map["NEW_HASH"] = TokenType::OPCODE;
    map["GET_INDEX"] = TokenType::OPCODE;
    map["SET_INDEX"] = TokenType::OPCODE;
    map["GET_KEYS"] = TokenType::OPCODE;
    map["GET_VALUES"] = TokenType::OPCODE;
    map["NEW_CLASS"] = TokenType::OPCODE;
    map["NEW_INSTANCE"] = TokenType::OPCODE;
    map["GET_PROP"] = TokenType::OPCODE;
    map["SET_PROP"] = TokenType::OPCODE;
    map["SET_METHOD"] = TokenType::OPCODE;
    map["INHERIT"] = TokenType::OPCODE;
    map["GET_SUPER"] = TokenType::OPCODE;
    map["BIT_AND"] = TokenType::OPCODE;
    map["BIT_OR"] = TokenType::OPCODE;
    map["BIT_XOR"] = TokenType::OPCODE;
    map["BIT_NOT"] = TokenType::OPCODE;
    map["LSHIFT"] = TokenType::OPCODE;
    map["RSHIFT"] = TokenType::OPCODE;
    map["THROW"] = TokenType::OPCODE;
    map["SETUP_TRY"] = TokenType::OPCODE;
    map["POP_TRY"] = TokenType::OPCODE;
    map["IMPORT_MODULE"] = TokenType::OPCODE;
    map["EXPORT"] = TokenType::OPCODE;
    map["GET_EXPORT"] = TokenType::OPCODE; /*GET_MODULE_EXPORT is removed?*/
    map["IMPORT_ALL"] = TokenType::OPCODE;
    return map;
}();

static constexpr std::array<std::string_view, static_cast<size_t>(TokenType::TOTAL_TOKENS)>
    TOKEN_MAP = {
        // Directives
        "DIR_FUNC",       // .func
        "DIR_ENDFUNC",    // .endfunc
        "DIR_REGISTERS",  // .registers
        "DIR_UPVALUES",   // .upvalues
        "DIR_UPVALUE",    // .upvalue
        "DIR_CONST",      // .const

        // Symbols
        "LABEL_DEF",   // nhan:
        "IDENTIFIER",  // ten_ham, ten_bien, ten_nhan_nhay_toi,
                       // @ProtoName
        "OPCODE",      // LOAD_CONST, ADD, JUMP etc.

        // Literals
        "NUMBER_INT",    // 123, -45, 0xFF, 0b101
        "NUMBER_FLOAT",  // 3.14, -0.5, 1e6
        "STRING",        // "chuoi ky tu"

        // Other
        "UNKNOWN",  // Lỗi hoặc ký tự không nhận dạng
        "END_OF_FILE"};

// --- Token ---
std::string Token::to_string() const {
    std::string_view type_str = TOKEN_MAP[static_cast<size_t>(type)];
    return std::format("[{}:{}] {} '{}'", line, col, type_str, lexeme);
}

// --- Lexer ---
Lexer::Lexer(std::string_view source)
    : src_(source), pos_(0), line_(1), col_(1), curr_(src_.empty() ? '\0' : src_[0]) {}

unsigned char Lexer::peek() const noexcept {
    size_t next_pos = pos_ + 1;
    return next_pos < src_.size() ? src_[next_pos] : '\0';
}

void Lexer::advance() noexcept {
    if (curr_ == '\n') {
        ++line_;
        col_ = 1;
    } else {
        ++col_;
    }
    ++pos_;
    curr_ = pos_ < src_.size() ? src_[pos_] : '\0';
}

bool Lexer::is_at_end() const noexcept { return pos_ >= src_.size(); }

bool Lexer::is_at_end(size_t index) const noexcept { return index >= src_.size(); }

Token Lexer::make_token(TokenType type) const {
    return Token{src_.substr(token_start_pos_, pos_ - token_start_pos_), type, token_start_line_,
                 token_start_col_};
}

Token Lexer::make_token(TokenType type, size_t length) const {
    return Token{src_.substr(token_start_pos_, length), type, token_start_line_, token_start_col_};
}

Token Lexer::make_error_token(const std::string& message) const {
    return Token{std::string_view(message), TokenType::UNKNOWN, token_start_line_,
                 token_start_col_};
}

void Lexer::skip_whitespace() noexcept {
    while (std::isspace(curr_))
        advance();
}

void Lexer::skip_comments() noexcept {
    advance();
    while (curr_ != '\n' && curr_ != '\0')
        advance();
}

bool Lexer::is_identifier_start(unsigned char c) { return std::isalpha(c) || c == '_' || c == '@'; }

Token Lexer::scan_identifier() {
    bool starts_with_dot = (curr_ == '.');
    if (starts_with_dot) {
        advance();
        if (!is_identifier_start(curr_) || curr_ == '@') {
            pos_ = token_start_pos_ + 1;
            return make_token(TokenType::UNKNOWN);  // Lỗi: '.' không theo sau
                                                    // bởi identifier hợp lệ
        }
    }

    while (std::isalnum(curr_) || curr_ == '_')
        advance();

    std::string_view lexeme = src_.substr(token_start_pos_, pos_ - token_start_pos_);

    if (starts_with_dot) {
        auto dir_it = DIRECTIVE_MAP.find(lexeme);
        if (dir_it != DIRECTIVE_MAP.end()) {
            return make_token(dir_it->second);
        }
        return make_token(TokenType::UNKNOWN);
    } else {
        auto opcode_it = OPCODE_LEX_MAP.find(lexeme);
        if (opcode_it != OPCODE_LEX_MAP.end()) {
            return make_token(TokenType::OPCODE);
        }
        return make_token(TokenType::IDENTIFIER);
    }
}

Token Lexer::scan_number() {
    bool is_float = false;
    bool maybe_negative = false;

    if (curr_ == '-') {
        maybe_negative = true;
        advance();
        // Nếu sau dấu '-' không phải là số -> lỗi hoặc là toán tử (không xử lý
        // ở đây)
        if (!std::isdigit(curr_) && curr_ != '.') {
            pos_ = token_start_pos_;                // Quay lui
            return make_token(TokenType::UNKNOWN);  // Coi như không phải số âm
        }
    }

    // Xử lý tiền tố 0x, 0b, 0o (chỉ cho số nguyên)
    if (!maybe_negative && curr_ == '0' && pos_ + 1 < src_.size()) {
        char next = src_[pos_ + 1];
        if (next == 'x' || next == 'X') {  // Hex
            advance();
            advance();  // Bỏ qua "0x"
            while (std::isxdigit(curr_))
                advance();
            return make_token(TokenType::NUMBER_INT);
        } else if (next == 'b' || next == 'B') {  // Binary
            advance();
            advance();  // Bỏ qua "0b"
            while (curr_ == '0' || curr_ == '1')
                advance();
            return make_token(TokenType::NUMBER_INT);
        } else if (next == 'o' || next == 'O') {  // Octal
            advance();
            advance();  // Bỏ qua "0o"
            while (curr_ >= '0' && curr_ <= '7')
                advance();
            return make_token(TokenType::NUMBER_INT);
        }
        // Nếu chỉ là '0' hoặc '0' theo sau bởi ký tự khác -> số thập phân bắt
        // đầu bằng 0
    }

    // Phần nguyên (thập phân)
    while (std::isdigit(curr_)) {
        advance();
    }

    // Phần thập phân
    if (curr_ == '.' && std::isdigit(peek())) {
        is_float = true;
        advance();  // Bỏ qua '.'
        while (std::isdigit(curr_)) {
            advance();
        }
    }

    // Phần mũ (cho cả int và float?) -> Chỉ cho float
    if (is_float && (curr_ == 'e' || curr_ == 'E')) {
        advance();  // Bỏ qua 'e'/'E'
        if (curr_ == '+' || curr_ == '-') {
            advance();  // Bỏ qua dấu '+' hoặc '-'
        }
        if (!std::isdigit(curr_)) {
            // Lỗi: Thiếu số sau 'e'/'E'
            pos_--;  // Lùi lại trước 'e'/'E' để token number kết thúc đúng
            if (curr_ == '+' || curr_ == '-')
                pos_--;  // Lùi thêm nếu có dấu
                         // Vẫn trả về float, parser sẽ báo lỗi nếu cần
        } else {
            while (std::isdigit(curr_)) {
                advance();
            }
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
    }

    if (curr_ == '.') {
        if (!is_at_end(pos_ + 1) && (std::isalpha(src_[pos_ + 1]) || src_[pos_ + 1] == '_')) {
            return scan_identifier();
        } else {
            advance();
            return make_token(TokenType::UNKNOWN);
        }
    }

    if (is_identifier_start(curr_)) {
        Token token = scan_identifier();
        if (token.type == TokenType::IDENTIFIER && curr_ == ':') {
            advance();
            return make_token(TokenType::LABEL_DEF, token.lexeme.length());
        }
        return token;
    }

    if (std::isdigit(curr_) || (curr_ == '-' && std::isdigit(peek()))) {
        return scan_number();
    } else if (curr_ == '"' || curr_ == '\'') {
        return scan_string(curr_);
    } else if (curr_ == ':') {
        advance();
        return make_token(TokenType::UNKNOWN);
    }

    advance();
    return make_token(TokenType::UNKNOWN);
}

Token Lexer::next_token() { return scan_token(); }

std::vector<Token> Lexer::tokenize_all() {
    std::vector<Token> tokens;
    tokens.reserve(src_.size() / 3);
    Token token;
    do {
        token = next_token();
        tokens.push_back(token);
    } while (token.type != TokenType::END_OF_FILE && token.type != TokenType::UNKNOWN);

    tokens.shrink_to_fit();
    return tokens;
}

}  // namespace meow::loader