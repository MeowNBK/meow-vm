// --- Old codes ---

// #include "loader/lexer.h"
// #include "core/op_codes.h" // Cần để kiểm tra opcode
// #include <unordered_map>
// #include <cctype>

// namespace meow::loader {

// // --- Bảng từ khóa/chỉ thị ---
// static const std::unordered_map<std::string_view, TokenType> DIRECTIVE_MAP = {
//     {".func", TokenType::DIR_FUNC},
//     {".endfunc", TokenType::DIR_ENDFUNC},
//     {".registers", TokenType::DIR_REGISTERS},
//     {".upvalues", TokenType::DIR_UPVALUES},
//     {".upvalue", TokenType::DIR_UPVALUE},
//     {".const", TokenType::DIR_CONST}
// };

// // --- Bảng Opcode (string_view) ---
// // Tạo map này một lần để tra cứu nhanh
// static const std::unordered_map<std::string_view, TokenType> OPCODE_LEX_MAP = []{
//     std::unordered_map<std::string_view, TokenType> map;
//     // Lấy từ OPCODE_MAP trong text_parser cũ hoặc op_codes.h
//     map["LOAD_CONST"] = TokenType::OPCODE; map["LOAD_NULL"] = TokenType::OPCODE; map["LOAD_TRUE"] = TokenType::OPCODE;
//     map["LOAD_FALSE"] = TokenType::OPCODE; map["LOAD_INT"] = TokenType::OPCODE; map["LOAD_FLOAT"] = TokenType::OPCODE;
//     map["MOVE"] = TokenType::OPCODE; map["ADD"] = TokenType::OPCODE; map["SUB"] = TokenType::OPCODE; map["MUL"] = TokenType::OPCODE;
//     map["DIV"] = TokenType::OPCODE; map["MOD"] = TokenType::OPCODE; map["POW"] = TokenType::OPCODE; map["EQ"] = TokenType::OPCODE;
//     map["NEQ"] = TokenType::OPCODE; map["GT"] = TokenType::OPCODE; map["GE"] = TokenType::OPCODE; map["LT"] = TokenType::OPCODE;
//     map["LE"] = TokenType::OPCODE; map["NEG"] = TokenType::OPCODE; map["NOT"] = TokenType::OPCODE; map["GET_GLOBAL"] = TokenType::OPCODE;
//     map["SET_GLOBAL"] = TokenType::OPCODE; map["GET_UPVALUE"] = TokenType::OPCODE; map["SET_UPVALUE"] = TokenType::OPCODE;
//     map["CLOSURE"] = TokenType::OPCODE; map["CLOSE_UPVALUES"] = TokenType::OPCODE; map["JUMP"] = TokenType::OPCODE;
//     map["JUMP_IF_FALSE"] = TokenType::OPCODE; map["JUMP_IF_TRUE"] = TokenType::OPCODE; map["CALL"] = TokenType::OPCODE;
//     map["CALL_VOID"] = TokenType::OPCODE; map["RETURN"] = TokenType::OPCODE; map["HALT"] = TokenType::OPCODE;
//     map["NEW_ARRAY"] = TokenType::OPCODE; map["NEW_HASH"] = TokenType::OPCODE; map["GET_INDEX"] = TokenType::OPCODE;
//     map["SET_INDEX"] = TokenType::OPCODE; map["GET_KEYS"] = TokenType::OPCODE; map["GET_VALUES"] = TokenType::OPCODE;
//     map["NEW_CLASS"] = TokenType::OPCODE; map["NEW_INSTANCE"] = TokenType::OPCODE; map["GET_PROP"] = TokenType::OPCODE;
//     map["SET_PROP"] = TokenType::OPCODE; map["SET_METHOD"] = TokenType::OPCODE; map["INHERIT"] = TokenType::OPCODE;
//     map["GET_SUPER"] = TokenType::OPCODE; map["BIT_AND"] = TokenType::OPCODE; map["BIT_OR"] = TokenType::OPCODE;
//     map["BIT_XOR"] = TokenType::OPCODE; map["BIT_NOT"] = TokenType::OPCODE; map["LSHIFT"] = TokenType::OPCODE;
//     map["RSHIFT"] = TokenType::OPCODE; map["THROW"] = TokenType::OPCODE; map["SETUP_TRY"] = TokenType::OPCODE;
//     map["POP_TRY"] = TokenType::OPCODE; map["IMPORT_MODULE"] = TokenType::OPCODE; map["EXPORT"] = TokenType::OPCODE;
//     map["GET_EXPORT"] = TokenType::OPCODE; map["IMPORT_ALL"] = TokenType::OPCODE;
//     return map;
// }();


// // --- Token ---
// std::string Token::to_string() const {
//      const char* type_str;
//      switch (type) {
//          case TokenType::DIR_FUNC: type_str = "DIR_FUNC"; break;
//          case TokenType::DIR_ENDFUNC: type_str = "DIR_ENDFUNC"; break;
//          case TokenType::DIR_REGISTERS: type_str = "DIR_REGISTERS"; break;
//          case TokenType::DIR_UPVALUES: type_str = "DIR_UPVALUES"; break;
//          case TokenType::DIR_UPVALUE: type_str = "DIR_UPVALUE"; break;
//          case TokenType::DIR_CONST: type_str = "DIR_CONST"; break;
//          case TokenType::LABEL_DEF: type_str = "LABEL_DEF"; break;
//          case TokenType::IDENTIFIER: type_str = "IDENTIFIER"; break;
//          case TokenType::OPCODE: type_str = "OPCODE"; break;
//          case TokenType::NUMBER_INT: type_str = "NUMBER_INT"; break;
//          case TokenType::NUMBER_FLOAT: type_str = "NUMBER_FLOAT"; break;
//          case TokenType::STRING: type_str = "STRING"; break;
//          case TokenType::END_OF_FILE: type_str = "EOF"; break;
//          default: type_str = "UNKNOWN"; break;
//      }
//      return std::format("[{}:{}] {} '{}'", line, col, type_str, lexeme);
//  }


// // --- Lexer ---
// Lexer::Lexer(std::string_view source) : src_(source) {}

// char Lexer::current_char() const noexcept {
//     return pos_ < src_.size() ? src_[pos_] : '\0';
// }

// char Lexer::peek_char() const noexcept {
//     size_t next_pos = pos_ + 1;
//     return next_pos < src_.size() ? src_[next_pos] : '\0';
// }

// void Lexer::advance() noexcept {
//     if (!is_at_end()) {
//         if (current_char() == '\n') {
//             line_++;
//             line_start_pos_ = pos_ + 1;
//         }
//         pos_++;
//     }
// }

// bool Lexer::is_at_end() const noexcept {
//     return pos_ >= src_.size();
// }

// Token Lexer::make_token(TokenType type) {
//     size_t current_pos = pos_;
//     return Token{
//         src_.substr(token_start_pos_, current_pos - token_start_pos_),
//         type,
//         token_start_line_,
//         token_start_col_
//     };
// }
// Token Lexer::make_token(TokenType type, size_t length) {
//      return Token{
//          src_.substr(token_start_pos_, length),
//          type,
//          token_start_line_,
//          token_start_col_
//      };
//  }

// Token Lexer::make_error_token(const std::string& message) {
//     // Chỉ dùng để debug lexer, không nên trả về cho parser
//     return Token{ std::string_view(message), TokenType::UNKNOWN, token_start_line_, token_start_col_ };
// }


// void Lexer::skip_whitespace_and_comments() {
//     while (!is_at_end()) {
//         char c = current_char();
//         if (std::isspace(c)) {
//             advance();
//         } else if (c == '#') {
//             // Comment đi tới cuối dòng
//             while (!is_at_end() && current_char() != '\n') {
//                 advance();
//             }
//             // Không advance() thêm để ký tự '\n' được xử lý (tăng dòng) ở lần lặp sau
//         } else {
//             break; // Gặp ký tự không phải whitespace/comment
//         }
//     }
// }

// bool Lexer::is_label_char(char c) {
//     return std::isalnum(c) || c == '_';
// }
// bool Lexer::is_identifier_start(char c) {
//      return std::isalpha(c) || c == '_' || c == '@'; // Cho phép '@' bắt đầu identifier (như @main)
// }
// bool Lexer::is_identifier_char(char c) {
//      return std::isalnum(c) || c == '_';
// }

// Token Lexer::scan_identifier_or_keyword() {
//     while (is_identifier_char(current_char())) {
//         advance();
//     }
//     std::string_view lexeme = src_.substr(token_start_pos_, pos_ - token_start_pos_);

//     // Kiểm tra xem có phải là directive không
//     auto dir_it = DIRECTIVE_MAP.find(lexeme);
//     if (dir_it != DIRECTIVE_MAP.end()) {
//         return make_token(dir_it->second);
//     }

//     // Kiểm tra xem có phải là opcode không
//     auto opcode_it = OPCODE_LEX_MAP.find(lexeme);
//      if (opcode_it != OPCODE_LEX_MAP.end()) {
//          return make_token(TokenType::OPCODE);
//      }

//     // Nếu không, đó là identifier (có thể là tên hàm, tên nhãn nhảy tới)
//     return make_token(TokenType::IDENTIFIER);
// }

// Token Lexer::scan_number() {
//     bool is_float = false;
//     // (Có thể thêm xử lý 0x, 0b, 0o ở đây nếu cần)
//     while (std::isdigit(current_char())) {
//         advance();
//     }
//     // Xử lý phần thập phân
//     if (current_char() == '.' && std::isdigit(peek_char())) {
//         is_float = true;
//         advance(); // Bỏ qua '.'
//         while (std::isdigit(current_char())) {
//             advance();
//         }
//     }
//     // (Có thể thêm xử lý phần mũ 'e'/'E' ở đây nếu cần)

//     return make_token(is_float ? TokenType::NUMBER_FLOAT : TokenType::NUMBER_INT);
// }

// Token Lexer::scan_string() {
//     advance(); // Bỏ qua dấu " mở đầu
//     bool escaped = false;
//     while (!is_at_end()) {
//         char c = current_char();
//         if (escaped) {
//             escaped = false;
//         } else if (c == '\\') {
//             escaped = true;
//         } else if (c == '"') {
//             break; // Kết thúc chuỗi
//         } else if (c == '\n') {
//              // Lỗi: Chuỗi không đóng trên cùng một dòng (hoặc cần hỗ trợ chuỗi đa dòng)
//              // Tạm thời coi là lỗi và kết thúc token tại đây
//              return make_token(TokenType::UNKNOWN); // Hoặc ném exception
//         }
//         advance();
//     }

//     if (is_at_end()) {
//         // Lỗi: Chuỗi không được đóng
//         return make_token(TokenType::UNKNOWN); // Hoặc ném exception
//     }

//     advance(); // Bỏ qua dấu " kết thúc
//     // Lexeme bao gồm cả dấu ""
//     return make_token(TokenType::STRING);
// }


// Token Lexer::scan_token() {
//     skip_whitespace_and_comments();

//     token_start_pos_ = pos_;
//     token_start_line_ = line_;
//     token_start_col_ = pos_ - line_start_pos_ + 1;

//     if (is_at_end()) {
//         return make_token(TokenType::END_OF_FILE);
//     }

//     char c = current_char();

//     // Directive
//     if (c == '.') {
//         advance();
//         if (is_identifier_start(current_char())) {
//             return scan_identifier_or_keyword(); // Lexer sẽ tự check map directive
//         } else {
//              pos_ = token_start_pos_ + 1; // Quay lại sau dấu '.'
//              return make_token(TokenType::UNKNOWN); // Lỗi: '.' không theo sau bởi identifier
//         }
//     }

//     // Identifier / Opcode
//     if (is_identifier_start(c)) {
//         Token token = scan_identifier_or_keyword();
//         // Kiểm tra xem có phải định nghĩa label không (vd: myLabel:)
//         if (current_char() == ':' && (token.type == TokenType::IDENTIFIER || token.type == TokenType::OPCODE /* Cho phép opcode làm nhãn? */ )) {
//              advance(); // Bỏ qua dấu ':'
//              // Trả về token LABEL_DEF, lexeme không bao gồm dấu ':'
//              return make_token(TokenType::LABEL_DEF, token.lexeme.length());
//         }
//         return token; // Trả về IDENTIFIER hoặc OPCODE đã xác định
//     }


//     // Number
//     if (std::isdigit(c) || (c == '-' && std::isdigit(peek_char()))) {
//         return scan_number();
//     }

//     // String
//     if (c == '"') {
//         return scan_string();
//     }

//     // Unknown character
//     advance();
//     return make_token(TokenType::UNKNOWN);
// }

// Token Lexer::next_token() {
//     return scan_token();
// }

// std::vector<Token> Lexer::tokenize_all() {
//     std::vector<Token> tokens;
//     Token token;
//     do {
//         token = next_token();
//         tokens.push_back(token);
//     } while (token.type != TokenType::END_OF_FILE && token.type != TokenType::UNKNOWN);

//     // Nếu token cuối là UNKNOWN, có thể báo lỗi hoặc để parser xử lý
//     // if (!tokens.empty() && tokens.back().type == TokenType::UNKNOWN) {
//     //     throw std::runtime_error("Lỗi lexer tại dòng " + std::to_string(tokens.back().line) + " cột " + std::to_string(tokens.back().col));
//     // }

//     return tokens;
// }


// } // namespace meow::loader