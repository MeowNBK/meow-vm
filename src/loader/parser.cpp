#include "loader/parser.h"
#include "memory/memory_manager.h"
#include "core/objects/function.h" // Cần ObjFunctionProto constructor và UpvalueDesc
#include "core/objects/string.h"   // Cần ObjString
#include "runtime/chunk.h"
#include <stdexcept>
#include <charconv> // std::from_chars
#include <limits>   // numeric_limits
#include <sstream>  // Cho unescape_string

namespace meow::loader {

using namespace meow::core;
using namespace meow::runtime;
using namespace meow::memory;
using namespace meow::core::objects; // Cho UpvalueDesc

// --- Bảng Opcode (string_view -> OpCode enum) ---
// Giữ lại map này để parser tra cứu opcode từ token
static const std::unordered_map<std::string_view, OpCode> OPCODE_PARSE_MAP = []{
    std::unordered_map<std::string_view, OpCode> map;
    map["LOAD_CONST"] = OpCode::LOAD_CONST; map["LOAD_NULL"] = OpCode::LOAD_NULL; map["LOAD_TRUE"] = OpCode::LOAD_TRUE;
    map["LOAD_FALSE"] = OpCode::LOAD_FALSE; map["LOAD_INT"] = OpCode::LOAD_INT; map["LOAD_FLOAT"] = OpCode::LOAD_FLOAT;
    map["MOVE"] = OpCode::MOVE; map["ADD"] = OpCode::ADD; map["SUB"] = OpCode::SUB; map["MUL"] = OpCode::MUL;
    map["DIV"] = OpCode::DIV; map["MOD"] = OpCode::MOD; map["POW"] = OpCode::POW; map["EQ"] = OpCode::EQ;
    map["NEQ"] = OpCode::NEQ; map["GT"] = OpCode::GT; map["GE"] = OpCode::GE; map["LT"] = OpCode::LT;
    map["LE"] = OpCode::LE; map["NEG"] = OpCode::NEG; map["NOT"] = OpCode::NOT; map["GET_GLOBAL"] = OpCode::GET_GLOBAL;
    map["SET_GLOBAL"] = OpCode::SET_GLOBAL; map["GET_UPVALUE"] = OpCode::GET_UPVALUE; map["SET_UPVALUE"] = OpCode::SET_UPVALUE;
    map["CLOSURE"] = OpCode::CLOSURE; map["CLOSE_UPVALUES"] = OpCode::CLOSE_UPVALUES; map["JUMP"] = OpCode::JUMP;
    map["JUMP_IF_FALSE"] = OpCode::JUMP_IF_FALSE; map["JUMP_IF_TRUE"] = OpCode::JUMP_IF_TRUE; map["CALL"] = OpCode::CALL;
    map["CALL_VOID"] = OpCode::CALL_VOID; map["RETURN"] = OpCode::RETURN; map["HALT"] = OpCode::HALT;
    map["NEW_ARRAY"] = OpCode::NEW_ARRAY; map["NEW_HASH"] = OpCode::NEW_HASH; map["GET_INDEX"] = OpCode::GET_INDEX;
    map["SET_INDEX"] = OpCode::SET_INDEX; map["GET_KEYS"] = OpCode::GET_KEYS; map["GET_VALUES"] = OpCode::GET_VALUES;
    map["NEW_CLASS"] = OpCode::NEW_CLASS; map["NEW_INSTANCE"] = OpCode::NEW_INSTANCE; map["GET_PROP"] = OpCode::GET_PROP;
    map["SET_PROP"] = OpCode::SET_PROP; map["SET_METHOD"] = OpCode::SET_METHOD; map["INHERIT"] = OpCode::INHERIT;
    map["GET_SUPER"] = OpCode::GET_SUPER; map["BIT_AND"] = OpCode::BIT_AND; map["BIT_OR"] = OpCode::BIT_OR;
    map["BIT_XOR"] = OpCode::BIT_XOR; map["BIT_NOT"] = OpCode::BIT_NOT; map["LSHIFT"] = OpCode::LSHIFT;
    map["RSHIFT"] = OpCode::RSHIFT; map["THROW"] = OpCode::THROW; map["SETUP_TRY"] = OpCode::SETUP_TRY;
    map["POP_TRY"] = OpCode::POP_TRY; map["IMPORT_MODULE"] = OpCode::IMPORT_MODULE; map["EXPORT"] = OpCode::EXPORT;
    map["GET_EXPORT"] = OpCode::GET_EXPORT; /*GET_MODULE_EXPORT removed?*/ map["IMPORT_ALL"] = OpCode::IMPORT_ALL;
    return map;
}();

TextParser::TextParser(MemoryManager* heap) noexcept : heap_(heap) {}

// --- Error Handling ---
[[noreturn]] void TextParser::throw_parse_error(const std::string& message) {
    // Nếu có token hiện tại (không phải EOF), dùng vị trí của nó
    if (current_token_index_ < tokens_.size()) {
         throw_parse_error(message, current_token());
    } else {
         // Nếu ở cuối file hoặc tokens rỗng, dùng vị trí ước lượng cuối cùng
         size_t last_line = tokens_.empty() ? 1 : tokens_.back().line;
         size_t last_col = tokens_.empty() ? 1 : tokens_.back().col + tokens_.back().lexeme.length();
         std::string error_message = std::format("Lỗi phân tích cú pháp [{}:{}:{}]: {}",
                                                current_source_name_, last_line, last_col, message);
         throw std::runtime_error(error_message);
    }
}

[[noreturn]] void TextParser::throw_parse_error(const std::string& message, const Token& token) {
    std::string error_message = std::format("Lỗi phân tích cú pháp [{}:{}:{}]: {}",
                                           current_source_name_, token.line, token.col, message);
    if (!token.lexeme.empty() && token.type != TokenType::END_OF_FILE) {
        // Sử dụng std::string để format, vì string_view không đảm bảo null-terminated
        error_message += std::format(" (gần '{}')", std::string(token.lexeme));
    }
    throw std::runtime_error(error_message);
}

// --- Token Handling ---
const Token& TextParser::current_token() const {
    if (current_token_index_ >= tokens_.size()) {
         // Token cuối cùng luôn là EOF do Lexer thêm vào
         return tokens_.back();
    }
    return tokens_[current_token_index_];
}

const Token& TextParser::peek_token(size_t offset) const {
    size_t index = current_token_index_ + offset;
    if (index >= tokens_.size()) {
         return tokens_.back(); // Trả về EOF
    }
    return tokens_[index];
}

bool TextParser::is_at_end() const {
    // Chỉ thực sự kết thúc khi token hiện tại là EOF
    return current_token().type == TokenType::END_OF_FILE;
}

void TextParser::advance() {
    // Chỉ tăng index nếu chưa phải là EOF
    if (current_token().type != TokenType::END_OF_FILE) {
        current_token_index_++;
    }
}

const Token& TextParser::consume_token(TokenType expected, const std::string& error_message) {
    const Token& token = current_token();
    if (token.type != expected) {
        throw_parse_error(error_message, token);
    }
    advance();
    // Trả về token *trước khi* advance (token đã được consume)
    return tokens_[current_token_index_ - 1];
}

bool TextParser::match_token(TokenType type) {
    if (current_token().type == type) {
        advance();
        return true;
    }
    return false;
}

// --- Unescape String ---
// Basic unescape implementation
std::string TextParser::unescape_string(const std::string& escaped) {
    std::stringstream ss;
    bool is_escaping = false;
    for (char c : escaped) {
        if (is_escaping) {
            switch (c) {
                case 'n': ss << '\n'; break;
                case 't': ss << '\t'; break;
                case 'r': ss << '\r'; break;
                case '\\': ss << '\\'; break;
                case '"': ss << '"'; break;
                // Add more cases like \xNN, \uNNNN if needed
                default: ss << c; break; // Or handle as error
            }
            is_escaping = false;
        } else if (c == '\\') {
            is_escaping = true;
        } else {
            ss << c;
        }
    }
    // Handle error if is_escaping is true at the end?
    return ss.str();
}


// --- Main Parsing Logic ---

proto_t TextParser::parse_file(const std::string& filepath, MemoryManager& mm) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Không thể mở tệp: " + filepath);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close(); // Đóng file sau khi đọc
    return parse_source(buffer.str(), mm, filepath);
}

proto_t TextParser::parse_source(const std::string& source, MemoryManager& mm, const std::string& source_name) {
    heap_ = &mm;
    current_source_name_ = source_name;
    current_token_index_ = 0;
    current_proto_data_ = nullptr;
    building_protos_.clear();
    finalized_protos_.clear();

    Lexer lexer(source);
    tokens_ = lexer.tokenize_all();

    // Kiểm tra lỗi lexer cuối cùng
    if (!tokens_.empty() && tokens_.back().type == TokenType::UNKNOWN) {
         throw_parse_error("Lỗi Lexer, ký tự không xác định hoặc chuỗi/comment không đóng.", tokens_.back());
     }
    // Đảm bảo token cuối cùng là EOF
     if (tokens_.empty() || tokens_.back().type != TokenType::END_OF_FILE) {
         // Thêm EOF token nếu Lexer chưa thêm (dù nên thêm)
         size_t last_line = tokens_.empty() ? 1 : tokens_.back().line;
         size_t last_col = tokens_.empty() ? 1 : tokens_.back().col + tokens_.back().lexeme.length();
         tokens_.push_back({"", TokenType::END_OF_FILE, last_line, last_col});
     }


    parse(); // Bắt đầu phân tích từ danh sách token

    // Resolve labels cho tất cả các proto đã xây dựng
     for(auto& [name, data] : building_protos_) {
         resolve_labels_for_proto(data);
     }

    // Tạo các đối tượng ObjFunctionProto thực sự từ ProtoParseData
     for (auto& [name, data] : building_protos_) {
         // Tạo string_t cho tên hàm
         string_t func_name_obj = heap_->new_string(name);

         // Tạo proto object, di chuyển chunk vào
         proto_t new_proto = heap_->new_proto(
             data.num_registers,
             data.num_upvalues,
             func_name_obj,
             std::move(data.chunk)
         );

         // Sao chép upvalue descriptions vào proto object
         // Cần đảm bảo ObjFunctionProto có phương thức để làm điều này, ví dụ: set_upvalue_descs
         auto* proto_obj_ptr = const_cast<ObjFunctionProto*>(new_proto);
          if (proto_obj_ptr) { // Kiểm tra null nếu new_proto có thể thất bại
               if(data.upvalue_descs.size() != data.num_upvalues) {
                     // Lỗi logic nội bộ
                     throw std::runtime_error("Lỗi nội bộ: Số lượng upvalue desc không khớp khai báo cho hàm " + name);
               }
               // Giả sử có hàm `set_upvalue_descs(std::vector<UpvalueDesc>)`
               // proto_obj_ptr->set_upvalue_descs(std::move(data.upvalue_descs));
               // Hoặc dùng hàm add từng cái một nếu ObjFunctionProto yêu cầu
                for(size_t i = 0; i < data.upvalue_descs.size(); ++i) {
                     proto_obj_ptr->set_desc(i, data.upvalue_descs[i]);
                }
          } else {
               throw std::runtime_error("Lỗi cấp phát bộ nhớ khi tạo proto cho hàm " + name);
          }


         finalized_protos_[name] = new_proto;
     }

    link_protos(); // Liên kết các proto tham chiếu lẫn nhau

    auto main_it = finalized_protos_.find("@main");
    if (main_it == finalized_protos_.end()) {
        throw std::runtime_error("Không tìm thấy hàm chính '@main' trong " + current_source_name_);
    }

    // Đặt lại trạng thái (không cần thiết nếu parser chỉ dùng 1 lần)
    heap_ = nullptr;
    tokens_.clear();
    building_protos_.clear(); // Xóa dữ liệu tạm

    return main_it->second;
}

const std::unordered_map<std::string, proto_t>& TextParser::get_finalized_protos() const {
    return finalized_protos_;
}

void TextParser::parse() {
    while (!is_at_end()) {
        parse_statement();
    }
    // Sau vòng lặp, kiểm tra xem có đang ở trong hàm dang dở không
    if (current_proto_data_) {
         const Token& last_token = tokens_.empty() ? Token{"", TokenType::UNKNOWN, 0,0} : tokens_[current_token_index_-1];
         // Báo lỗi tại vị trí của directive .func tương ứng
         throw_parse_error("Thiếu chỉ thị '.endfunc' cho hàm '" + current_proto_data_->name + "' bắt đầu tại dòng "
                            + std::to_string(current_proto_data_->func_directive_line), last_token);
    }
}

void TextParser::parse_statement() {
    const Token& token = current_token();
    switch (token.type) {
        case TokenType::DIR_FUNC:
             if (current_proto_data_) { // Chỉ báo lỗi nếu đang ở trong hàm khác
                  throw_parse_error("Không thể định nghĩa hàm lồng nhau.", token);
             }
            parse_func_directive();
            break;
        case TokenType::DIR_ENDFUNC: // Chỉ hợp lệ khi đang parse nội dung hàm
             if (!current_proto_data_) {
                  throw_parse_error("Chỉ thị '.endfunc' không mong đợi bên ngoài định nghĩa hàm.", token);
             } else {
                  // Logic kết thúc hàm nằm trong parse_func_directive, không nên đến đây
                  throw_parse_error("Lỗi logic nội bộ: Gặp '.endfunc' ở parse_statement.", token);
             }
             break; // Không nên đến đây

         // Các directive khác chỉ hợp lệ bên trong .func
         case TokenType::DIR_REGISTERS:
         case TokenType::DIR_UPVALUES:
         case TokenType::DIR_CONST:
         case TokenType::DIR_UPVALUE:
             if (!current_proto_data_) throw_parse_error("Chỉ thị phải nằm trong định nghĩa hàm (.func).", token);
              // Kiểm tra thứ tự: .registers và .upvalues phải trước .const, .upvalue, opcode, label
              if ((token.type == TokenType::DIR_CONST || token.type == TokenType::DIR_UPVALUE) &&
                  (!current_proto_data_->registers_defined || !current_proto_data_->upvalues_defined)) {
                    throw_parse_error("Chỉ thị '.registers' và '.upvalues' phải được định nghĩa trước '" + std::string(token.lexeme) + "'.", token);
              }

              if (token.type == TokenType::DIR_REGISTERS) parse_registers_directive();
              else if (token.type == TokenType::DIR_UPVALUES) parse_upvalues_directive();
              else if (token.type == TokenType::DIR_CONST) parse_const_directive();
              else if (token.type == TokenType::DIR_UPVALUE) parse_upvalue_directive();
             break;

        case TokenType::LABEL_DEF:
            if (!current_proto_data_) throw_parse_error("Nhãn phải nằm trong định nghĩa hàm (.func).", token);
             if (!current_proto_data_->registers_defined || !current_proto_data_->upvalues_defined) {
                 throw_parse_error("Chỉ thị '.registers' và '.upvalues' phải được định nghĩa trước nhãn.", token);
             }
            parse_label_definition();
            break;

        case TokenType::OPCODE:
            if (!current_proto_data_) throw_parse_error("Lệnh phải nằm trong định nghĩa hàm (.func).", token);
             // Kiểm tra xem .registers và .upvalues đã được định nghĩa chưa
             if (!current_proto_data_->registers_defined) {
                 throw_parse_error("Chỉ thị '.registers' phải được định nghĩa trước lệnh đầu tiên.", token);
             }
             if (!current_proto_data_->upvalues_defined) {
                 throw_parse_error("Chỉ thị '.upvalues' phải được định nghĩa trước lệnh đầu tiên.", token);
             }
            parse_instruction();
            break;

        // Identifier không có ':' theo sau và không phải directive/opcode là lỗi ở cấp độ này
        case TokenType::IDENTIFIER:
             throw_parse_error("Token không mong đợi. Có thể thiếu directive hoặc opcode?", token);
             break;

        // Các loại token khác cũng không hợp lệ ở đây
        case TokenType::NUMBER_INT:
        case TokenType::NUMBER_FLOAT:
        case TokenType::STRING:
             throw_parse_error("Giá trị literal không hợp lệ ở đây. Có thể thiếu chỉ thị '.const'?", token);
             break;
        case TokenType::END_OF_FILE: // Không làm gì cả, vòng lặp parse() sẽ kết thúc
             break;
        case TokenType::UNKNOWN: // Lỗi Lexer đã được bắt trước đó hoặc ở đây
            throw_parse_error("Token không hợp lệ hoặc ký tự không nhận dạng.", token);
            break;
    }
}

void TextParser::parse_func_directive() {
    const Token& func_token = consume_token(TokenType::DIR_FUNC, "Mong đợi '.func'."); // Lưu token để lấy vị trí báo lỗi sau
    // Không cần kiểm tra current_proto_data_ nữa vì đã làm ở parse_statement

    const Token& name_token = consume_token(TokenType::IDENTIFIER, "Mong đợi tên hàm sau '.func'.");
    std::string func_name(name_token.lexeme);

    // Kiểm tra tên hàm hợp lệ (ví dụ: không chứa ký tự đặc biệt ngoài '_', không bắt đầu bằng số nếu không phải @)
    if (func_name.empty() || (func_name[0] != '@' && !std::isalpha(func_name[0]) && func_name[0] != '_')) {
        throw_parse_error("Tên hàm không hợp lệ.", name_token);
    }
    // Bỏ ký tự '@' nếu có để dùng làm key map
    std::string map_key = (func_name[0] == '@') ? func_name.substr(1) : func_name;
     if (map_key.empty()) {
          throw_parse_error("Tên hàm không hợp lệ (chỉ có '@').", name_token);
     }


    if (building_protos_.count(map_key)) {
        throw_parse_error("Hàm '" + func_name + "' đã được định nghĩa.", name_token);
    }

    // Khởi tạo ProtoParseData mới và đặt con trỏ current
    // Lưu ý: Dùng map::emplace để tránh copy không cần thiết
    auto [it, inserted] = building_protos_.emplace(map_key, ProtoParseData{});
    current_proto_data_ = &it->second; // Lấy địa chỉ của value trong map
    current_proto_data_->name = map_key; // Lưu tên không có '@'
    current_proto_data_->func_directive_line = func_token.line; // Lưu vị trí để báo lỗi .endfunc thiếu
    current_proto_data_->func_directive_col = func_token.col;


     // Yêu cầu .registers và .upvalues ngay sau .func
     // Phải gọi đúng thứ tự
     parse_registers_directive(); // Sẽ throw nếu không tìm thấy .registers hoặc sai cú pháp
     parse_upvalues_directive();  // Sẽ throw nếu không tìm thấy .upvalues hoặc sai cú pháp


    // Bây giờ xử lý các lệnh, nhãn, hằng số cho đến khi gặp .endfunc hoặc EOF
    while (!is_at_end() && current_token().type != TokenType::DIR_ENDFUNC) {
        parse_statement(); // Phân tích nội dung hàm (const, upvalue, label, instruction)
    }

    // Phải kết thúc bằng .endfunc
    consume_token(TokenType::DIR_ENDFUNC, "Mong đợi '.endfunc' để kết thúc hàm '" + func_name + "'.");
    current_proto_data_ = nullptr; // Kết thúc xử lý hàm hiện tại, quay về global scope
}

void TextParser::parse_registers_directive() {
     // Token .registers đã được kiểm tra bởi caller (parse_func_directive)
     consume_token(TokenType::DIR_REGISTERS, "Lỗi nội bộ: Mong đợi '.registers'.");
     if (!current_proto_data_) throw_parse_error("Lỗi nội bộ: current_proto_data_ là null khi parse .registers.");
     if (current_proto_data_->registers_defined) throw_parse_error("Chỉ thị '.registers' đã được định nghĩa cho hàm này.");

     const Token& num_token = consume_token(TokenType::NUMBER_INT, "Mong đợi số lượng thanh ghi (số nguyên không âm) sau '.registers'.");
     try {
          uint64_t num_reg;
          auto result = std::from_chars(num_token.lexeme.data(), num_token.lexeme.data() + num_token.lexeme.size(), num_reg);
          // Kiểm tra lỗi parse VÀ kiểm tra xem có parse hết chuỗi không VÀ kiểm tra tràn số size_t
          if (result.ec != std::errc() || result.ptr != num_token.lexeme.data() + num_token.lexeme.size() || num_reg > std::numeric_limits<size_t>::max()) {
              throw std::out_of_range("Invalid or out-of-range register count");
          }
         current_proto_data_->num_registers = static_cast<size_t>(num_reg);
         current_proto_data_->registers_defined = true;
     } catch (...) {
         // Bắt lỗi từ from_chars hoặc out_of_range
         throw_parse_error("Số lượng thanh ghi không hợp lệ.", num_token);
     }
 }

 void TextParser::parse_upvalues_directive() {
     // Token .upvalues đã được kiểm tra bởi caller (parse_func_directive)
     consume_token(TokenType::DIR_UPVALUES, "Lỗi nội bộ: Mong đợi '.upvalues'.");
     if (!current_proto_data_) throw_parse_error("Lỗi nội bộ: current_proto_data_ là null khi parse .upvalues.");
     if (current_proto_data_->upvalues_defined) throw_parse_error("Chỉ thị '.upvalues' đã được định nghĩa cho hàm này.");

     const Token& num_token = consume_token(TokenType::NUMBER_INT, "Mong đợi số lượng upvalue (số nguyên không âm) sau '.upvalues'.");
     try {
          uint64_t num_up;
          auto result = std::from_chars(num_token.lexeme.data(), num_token.lexeme.data() + num_token.lexeme.size(), num_up);
          if (result.ec != std::errc() || result.ptr != num_token.lexeme.data() + num_token.lexeme.size() || num_up > std::numeric_limits<size_t>::max()) {
              throw std::out_of_range("Invalid or out-of-range upvalue count");
          }
         current_proto_data_->num_upvalues = static_cast<size_t>(num_up);
          // Chuẩn bị sẵn vector descriptions với kích thước đúng
          current_proto_data_->upvalue_descs.resize(current_proto_data_->num_upvalues);
         current_proto_data_->upvalues_defined = true;
     } catch (...) {
         throw_parse_error("Số lượng upvalue không hợp lệ.", num_token);
     }
 }


void TextParser::parse_const_directive() {
    consume_token(TokenType::DIR_CONST, "Mong đợi '.const'.");
    if (!current_proto_data_) throw_parse_error("Lỗi nội bộ: current_proto_data_ là null khi parse .const.");

    Value const_val = parse_const_value_from_tokens();
    current_proto_data_->chunk.add_constant(const_val);
}

void TextParser::parse_upvalue_directive() {
     consume_token(TokenType::DIR_UPVALUE, "Mong đợi '.upvalue'.");
     if (!current_proto_data_) throw_parse_error("Lỗi nội bộ: current_proto_data_ là null khi parse .upvalue.");
      if (!current_proto_data_->upvalues_defined) {
           // Mặc dù đã kiểm tra ở parse_statement, kiểm tra lại cho chắc
           throw_parse_error("Chỉ thị '.upvalues' phải được định nghĩa trước '.upvalue'.", tokens_[current_token_index_-1]);
      }


     // Parse index (u16)
     const Token& index_token = consume_token(TokenType::NUMBER_INT, "Mong đợi chỉ số upvalue (0-based) sau '.upvalue'.");
     size_t uv_index;
     try {
          uint64_t temp_idx;
          auto result = std::from_chars(index_token.lexeme.data(), index_token.lexeme.data() + index_token.lexeme.size(), temp_idx);
          if (result.ec != std::errc() || result.ptr != index_token.lexeme.data() + index_token.lexeme.size()
              || temp_idx >= current_proto_data_->num_upvalues) { // Kiểm tra với số lượng đã khai báo
              throw std::out_of_range("Invalid or out-of-range upvalue index");
          }
         uv_index = static_cast<size_t>(temp_idx);
     } catch (...) {
         throw_parse_error("Chỉ số upvalue không hợp lệ hoặc vượt quá số lượng đã khai báo ("
                            + std::to_string(current_proto_data_->num_upvalues) + ").", index_token);
     }

     // Parse type (identifier: local / parent) - đổi tên 'parent_upvalue' thành 'parent' cho ngắn gọn?
     const Token& type_token = consume_token(TokenType::IDENTIFIER, "Mong đợi loại upvalue ('local' hoặc 'parent').");
     bool is_local;
     if (type_token.lexeme == "local") {
         is_local = true;
     } else if (type_token.lexeme == "parent") { // Đổi thành 'parent'
         is_local = false;
     } else {
         throw_parse_error("Loại upvalue không hợp lệ. Phải là 'local' hoặc 'parent'.", type_token);
     }

     // Parse slot index (u16)
     const Token& slot_token = consume_token(TokenType::NUMBER_INT, "Mong đợi chỉ số slot (thanh ghi nếu 'local', upvalue cha nếu 'parent').");
     size_t slot_index;
      try {
           uint64_t temp_slot;
           auto result = std::from_chars(slot_token.lexeme.data(), slot_token.lexeme.data() + slot_token.lexeme.size(), temp_slot);
           // Chỉ kiểm tra parse và tràn số size_t, kiểm tra logic (vd < num_registers) để sau nếu cần
           if (result.ec != std::errc() || result.ptr != slot_token.lexeme.data() + slot_token.lexeme.size() || temp_slot > std::numeric_limits<size_t>::max()) {
               throw std::out_of_range("Invalid or out-of-range slot index");
           }
          slot_index = static_cast<size_t>(temp_slot);
          // Kiểm tra logic cơ bản
           if (is_local && slot_index >= current_proto_data_->num_registers) {
                throw_parse_error("Chỉ số slot cho upvalue 'local' (" + std::to_string(slot_index) + ") phải nhỏ hơn số lượng thanh ghi (" + std::to_string(current_proto_data_->num_registers) + ").", slot_token);
           }
           // Không kiểm tra slot_index cho 'parent' ở đây vì chưa biết số lượng upvalue của hàm cha
      } catch (...) {
          throw_parse_error("Chỉ số slot không hợp lệ.", slot_token);
      }


     // Lưu mô tả vào ProtoParseData tại đúng vị trí index
      // Vector đã được resize ở parse_upvalues_directive
      current_proto_data_->upvalue_descs[uv_index] = UpvalueDesc(is_local, slot_index);
 }


void TextParser::parse_label_definition() {
    const Token& label_token = consume_token(TokenType::LABEL_DEF, "Lỗi nội bộ: Mong đợi định nghĩa nhãn."); // Lexer đã đảm bảo type này
    if (!current_proto_data_) throw_parse_error("Lỗi nội bộ: current_proto_data_ là null khi parse label def.");

    std::string_view label_name = label_token.lexeme; // Lexeme không chứa dấu ':'
    if (current_proto_data_->labels.count(label_name)) {
        throw_parse_error("Nhãn '" + std::string(label_name) + "' đã được định nghĩa trong hàm này.", label_token);
    }
    // Lưu vị trí byte code *hiện tại* làm địa chỉ của nhãn
    current_proto_data_->labels[label_name] = current_proto_data_->chunk.get_code_size();
}

void TextParser::parse_instruction() {
    const Token& opcode_token = consume_token(TokenType::OPCODE, "Lỗi nội bộ: Mong đợi opcode."); // Lexer đảm bảo type này
    if (!current_proto_data_) throw_parse_error("Lỗi nội bộ: current_proto_data_ là null khi parse instruction.");

    auto it = OPCODE_PARSE_MAP.find(opcode_token.lexeme);
    // Lexer đã đảm bảo đây là OPCODE hợp lệ, không cần kiểm tra lại it == end()
    OpCode opcode = it->second;

    Chunk& chunk_ref = current_proto_data_->chunk;
    // Vị trí bắt đầu của opcode (không phải của instruction hoàn chỉnh)
    size_t instruction_opcode_pos = chunk_ref.get_code_size();

    chunk_ref.write_byte(static_cast<uint8_t>(opcode));

    // --- Hàm tiện ích cho parsing đối số từ token ---
    // (Đã chuyển thành lambda để bắt ngữ cảnh this và token hiện tại)
    auto parse_u16_arg = [&]() -> uint16_t {
        const Token& token = consume_token(TokenType::NUMBER_INT, "Mong đợi đối số là số nguyên 16-bit không dấu.");
        uint64_t value_u64; // Đọc vào kiểu lớn hơn để kiểm tra tràn số
        auto result = std::from_chars(token.lexeme.data(), token.lexeme.data() + token.lexeme.size(), value_u64);
        if (result.ec != std::errc() || result.ptr != token.lexeme.data() + token.lexeme.size() || value_u64 > UINT16_MAX) {
            throw_parse_error("Đối số phải là số nguyên 16-bit không dấu hợp lệ (0-" + std::to_string(UINT16_MAX) + ").", token);
        }
        return static_cast<uint16_t>(value_u64);
    };
    auto parse_i64_arg = [&]() -> int64_t {
         const Token& token = current_token();
         int64_t value;
         auto result = std::from_chars(token.lexeme.data(), token.lexeme.data() + token.lexeme.size(), value);
         if (result.ec == std::errc() && result.ptr == token.lexeme.data() + token.lexeme.size()) {
             advance(); // Consume token nếu là int hợp lệ
             return value;
         }
          // Thử parse float nếu opcode cho phép (chỉ LOAD_INT?)
          else if (token.type == TokenType::NUMBER_FLOAT && opcode == OpCode::LOAD_INT) {
              advance();
              try {
                  // Cẩn thận với tràn số khi chuyển từ double lớn sang int64
                   double d_val = std::stod(std::string(token.lexeme)); // stod cần std::string
                   if (d_val > static_cast<double>(std::numeric_limits<int64_t>::max()) ||
                       d_val < static_cast<double>(std::numeric_limits<int64_t>::min())) {
                       throw_parse_error("Giá trị số thực quá lớn/nhỏ để chuyển đổi thành số nguyên 64-bit.", token);
                   }
                  return static_cast<int64_t>(d_val);
              } catch(...) {
                  throw_parse_error("Không thể chuyển đổi số thực thành số nguyên 64-bit.", token);
              }
          }
         throw_parse_error("Mong đợi đối số là số nguyên 64-bit.", token);
     };
    auto parse_f64_arg = [&]() -> double {
        const Token& token = current_token(); // Có thể là INT hoặc FLOAT
         if (token.type != TokenType::NUMBER_FLOAT && token.type != TokenType::NUMBER_INT) {
              throw_parse_error("Mong đợi đối số là số thực hoặc số nguyên.", token);
         }
         advance(); // Consume token
        try {
             size_t processed = 0;
             double value = std::stod(std::string(token.lexeme), &processed);
             if (processed != token.lexeme.size()) { // Kiểm tra xem toàn bộ chuỗi đã được parse chưa
                 throw std::invalid_argument("Invalid double format");
             }
             return value;
         } catch (...) {
             throw_parse_error("Đối số số thực không hợp lệ.", token);
         }
    };
    auto parse_address_or_label_arg = [&]() {
        const Token& token = current_token();
        if (token.type == TokenType::NUMBER_INT) {
            // Là địa chỉ tuyệt đối
            uint16_t address = parse_u16_arg(); // consume và kiểm tra giá trị
            chunk_ref.write_u16(address);
        } else if (token.type == TokenType::IDENTIFIER) {
            // Là tên nhãn
            advance(); // consume identifier (label name)
            size_t patch_target_offset = chunk_ref.get_code_size(); // Vị trí 2 byte cần vá sau này
            chunk_ref.write_u16(0xDEAD); // Placeholder dễ nhận biết khi debug
            current_proto_data_->pending_jumps.emplace_back(instruction_opcode_pos, patch_target_offset, token.lexeme);
        } else {
            throw_parse_error("Mong đợi nhãn (identifier) hoặc địa chỉ tuyệt đối (số nguyên 16-bit) cho lệnh nhảy.", token);
        }
    };

    // --- Xử lý đối số dựa trên opcode ---
     // Lưu ý: Các lệnh ghi vào chunk theo thứ tự đối số trong file asm
     switch (opcode) {
         // --- dst: u16 ---
         case OpCode::LOAD_NULL: case OpCode::LOAD_TRUE: case OpCode::LOAD_FALSE:
         case OpCode::POP_TRY: // Không có đối số? Xem lại định nghĩa opcode -> Có vẻ không có
             // chunk_ref.write_u16(parse_u16_arg()); break; // Nếu POP_TRY có reg đích
              break; // Nếu POP_TRY không có đối số
         case OpCode::IMPORT_ALL: // Chỉ có module reg
             chunk_ref.write_u16(parse_u16_arg()); break;


         // --- dst: u16, constant_index: u16 ---
         case OpCode::LOAD_CONST: case OpCode::GET_GLOBAL: case OpCode::NEW_CLASS:
         case OpCode::GET_SUPER: case OpCode::CLOSURE:
         case OpCode::IMPORT_MODULE:
              chunk_ref.write_u16(parse_u16_arg()); // dst
              chunk_ref.write_u16(parse_u16_arg()); // const_idx
              break;

          // --- name_idx: u16, src_reg: u16 --- (Đảo ngược thứ tự so với comment cũ)
          case OpCode::EXPORT: case OpCode::SET_GLOBAL:
               chunk_ref.write_u16(parse_u16_arg()); // name_idx
               chunk_ref.write_u16(parse_u16_arg()); // src_reg
               break;


         // --- dst: u16, src: u16 ---
         case OpCode::MOVE: case OpCode::NEG: case OpCode::NOT: case OpCode::BIT_NOT:
         case OpCode::GET_UPVALUE:
         case OpCode::NEW_INSTANCE: case OpCode::GET_KEYS: case OpCode::GET_VALUES:
              chunk_ref.write_u16(parse_u16_arg()); // dst
              chunk_ref.write_u16(parse_u16_arg()); // src
              break;

          // --- upvalue_idx: u16, src_reg: u16 --- (Đảo thứ tự)
          case OpCode::SET_UPVALUE:
               chunk_ref.write_u16(parse_u16_arg()); // uv_idx
               chunk_ref.write_u16(parse_u16_arg()); // src_reg
               break;

          // --- last_reg: u16 ---
          case OpCode::CLOSE_UPVALUES:
               chunk_ref.write_u16(parse_u16_arg()); // last_reg
               break;


         // --- dst: u16, value: i64 ---
         case OpCode::LOAD_INT:
              chunk_ref.write_u16(parse_u16_arg()); // dst
              chunk_ref.write_u64(std::bit_cast<uint64_t>(parse_i64_arg())); // value
              break;

         // --- dst: u16, value: f64 ---
         case OpCode::LOAD_FLOAT:
              chunk_ref.write_u16(parse_u16_arg()); // dst
              chunk_ref.write_f64(parse_f64_arg());   // value
              break;

         // --- dst: u16, r1: u16, r2: u16 ---
         case OpCode::ADD: case OpCode::SUB: case OpCode::MUL: case OpCode::DIV:
         case OpCode::MOD: case OpCode::POW: case OpCode::EQ: case OpCode::NEQ:
         case OpCode::GT: case OpCode::GE: case OpCode::LT: case OpCode::LE:
         case OpCode::BIT_AND: case OpCode::BIT_OR: case OpCode::BIT_XOR:
         case OpCode::LSHIFT: case OpCode::RSHIFT:
         case OpCode::GET_INDEX: case OpCode::NEW_ARRAY: case OpCode::NEW_HASH:
         case OpCode::GET_PROP: case OpCode::SET_METHOD: case OpCode::GET_EXPORT:
              chunk_ref.write_u16(parse_u16_arg()); // dst
              chunk_ref.write_u16(parse_u16_arg()); // r1 or equivalent
              chunk_ref.write_u16(parse_u16_arg()); // r2 or equivalent
              break;

         // --- src: u16, key: u16, value: u16 ---
         case OpCode::SET_INDEX: case OpCode::SET_PROP:
              chunk_ref.write_u16(parse_u16_arg()); // src / obj
              chunk_ref.write_u16(parse_u16_arg()); // key / name_idx
              chunk_ref.write_u16(parse_u16_arg()); // value
              break;

          // --- sub_reg: u16, super_reg: u16 ---
          case OpCode::INHERIT:
               chunk_ref.write_u16(parse_u16_arg()); // sub
               chunk_ref.write_u16(parse_u16_arg()); // super
               break;


         // --- target: address/label ---
         case OpCode::JUMP: case OpCode::SETUP_TRY:
              parse_address_or_label_arg(); // target
              break;

         // --- reg: u16, target: address/label ---
         case OpCode::JUMP_IF_FALSE: case OpCode::JUMP_IF_TRUE:
              chunk_ref.write_u16(parse_u16_arg()); // reg
              parse_address_or_label_arg(); // target
              break;

          // --- CALL dst: u16, fn_reg: u16, arg_start: u16, argc: u16 ---
          case OpCode::CALL:
               chunk_ref.write_u16(parse_u16_arg()); // dst (FFFF for void)
               chunk_ref.write_u16(parse_u16_arg()); // fn_reg
               chunk_ref.write_u16(parse_u16_arg()); // arg_start
               chunk_ref.write_u16(parse_u16_arg()); // argc
               break;

          // --- CALL_VOID fn_reg: u16, arg_start: u16, argc: u16 ---
          case OpCode::CALL_VOID:
               chunk_ref.write_u16(parse_u16_arg()); // fn_reg
               chunk_ref.write_u16(parse_u16_arg()); // arg_start
               chunk_ref.write_u16(parse_u16_arg()); // argc
               break;


         // --- ret_reg: u16 (có thể là FFFF hoặc -1) ---
         case OpCode::RETURN: {
             const Token& ret_token = current_token();
             // Chấp nhận số nguyên -1 hoặc hex FFFF/ffff
             if (ret_token.type == TokenType::NUMBER_INT && ret_token.lexeme == "-1") {
                 advance();
                 chunk_ref.write_u16(0xFFFF);
             } else if (ret_token.type == TokenType::IDENTIFIER && (ret_token.lexeme == "FFFF" || ret_token.lexeme == "ffff")) {
                  // Cần Lexer nhận dạng FFFF là IDENTIFIER hoặc NUMBER_INT (nếu chỉ chứa hex)
                  advance();
                  chunk_ref.write_u16(0xFFFF);
             } else if (ret_token.type == TokenType::NUMBER_INT) {
                  // Parse như u16 bình thường
                  chunk_ref.write_u16(parse_u16_arg());
             } else {
                  throw_parse_error("Mong đợi thanh ghi trả về (số nguyên không âm, -1, hoặc FFFF).", ret_token);
             }
             break;
         }

          // --- reg: u16 ---
          case OpCode::THROW:
               chunk_ref.write_u16(parse_u16_arg()); // reg
               break;

         // --- Không có đối số ---
         case OpCode::HALT:
             // Không làm gì thêm
             break;

         default:
             // Nếu có opcode mới mà chưa xử lý ở đây, báo lỗi
             throw_parse_error("Opcode '" + std::string(opcode_token.lexeme) + "' chưa được hỗ trợ xử lý đối số trong parser.", opcode_token);
     }
}

Value TextParser::parse_const_value_from_tokens() {
    // Xử lý giá trị hằng số từ token hiện tại
    const Token& token = current_token();

    switch (token.type) {
        case TokenType::STRING: {
            advance();
            // Bỏ dấu "" ở đầu và cuối lexeme trước khi unescape
            if (token.lexeme.length() < 2 || token.lexeme.front() != '"' || token.lexeme.back() != '"') {
                throw_parse_error("Chuỗi literal không hợp lệ (thiếu dấu \"\").", token);
            }
            std::string_view inner = token.lexeme.substr(1, token.lexeme.length() - 2);
            std::string unescaped = TextParser::unescape_string(std::string(inner));
            // Tạo ObjString thông qua MemoryManager
            return Value(heap_->new_string(unescaped));
        }
        case TokenType::NUMBER_INT: {
            advance();
            int64_t value;
            auto result = std::from_chars(token.lexeme.data(), token.lexeme.data() + token.lexeme.size(), value);
            if (result.ec != std::errc() || result.ptr != token.lexeme.data() + token.lexeme.size()) {
                 throw_parse_error("Số nguyên literal không hợp lệ.", token); // Lỗi Lexer?
            }
            return Value(value);
        }
        case TokenType::NUMBER_FLOAT: {
            advance();
             try {
                 size_t processed = 0;
                 // stod cần std::string, không dùng string_view trực tiếp được
                 double value = std::stod(std::string(token.lexeme), &processed);
                 if (processed != token.lexeme.size()) { // Kiểm tra parse hết chuỗi
                     throw std::invalid_argument("Incomplete parse");
                 }
                 return Value(value);
             } catch (...) {
                 throw_parse_error("Số thực literal không hợp lệ.", token); // Lỗi Lexer?
             }
        }
        case TokenType::IDENTIFIER: {
            advance();
            if (token.lexeme == "true") return Value(true);
            if (token.lexeme == "false") return Value(false);
            if (token.lexeme == "null") return Value(null_t{});
            // Tham chiếu Proto (ví dụ: @main, @myFunc)
            if (!token.lexeme.empty() && token.lexeme.front() == '@') {
                 std::string_view proto_name_view = token.lexeme.substr(1);
                 if (proto_name_view.empty()) {
                      throw_parse_error("Tên proto tham chiếu không được rỗng (chỉ có '@').", token);
                 }
                 // Tạo một chuỗi đặc biệt để đánh dấu đây là tham chiếu proto cần link sau
                 std::string ref_string = "::proto_ref::" + std::string(proto_name_view);
                 return Value(heap_->new_string(ref_string));
            }
            // Nếu không phải bool, null, hay @Proto -> Lỗi
            throw_parse_error("Identifier không hợp lệ cho giá trị hằng số. Chỉ chấp nhận 'true', 'false', 'null', hoặc '@ProtoName'.", token);
        }
        default:
            throw_parse_error("Token không mong đợi cho giá trị hằng số. Mong đợi chuỗi, số, true, false, null, hoặc @ProtoName.", token);
    }
}


// --- Linking and Resolving ---

void TextParser::resolve_labels_for_proto(ProtoParseData& data) {
    // uint8_t* code_buffer = data.chunk.get_mutable_code(); // Cần hàm này nếu muốn ghi trực tiếp byte

    for (const auto& jump_info : data.pending_jumps) {
        [[maybe_unused]] size_t instruction_opcode_pos = std::get<0>(jump_info);
        size_t patch_target_offset = std::get<1>(jump_info); // Offset của 2 byte địa chỉ cần vá
        std::string_view label_name = std::get<2>(jump_info);

        // Tìm địa chỉ của nhãn trong map
        auto label_it = data.labels.find(label_name);
        if (label_it == data.labels.end()) {
             // Tạo token giả để báo lỗi đúng vị trí jump instruction (cần lưu line/col của jump)
             // Tạm thời báo lỗi chung chung
             throw std::runtime_error("Lỗi liên kết trong hàm '" + data.name + "': Không tìm thấy nhãn '" + std::string(label_name) + "'.");
             // Có thể cải thiện bằng cách lưu line/col của token label khi thêm vào pending_jumps
        }

        size_t target_address = label_it->second; // Địa chỉ byte code của nhãn
        if (target_address > UINT16_MAX) {
             throw std::runtime_error("Lỗi liên kết trong hàm '" + data.name + "': Địa chỉ nhãn '" + std::string(label_name) + "' ("
                                      + std::to_string(target_address) + ") vượt quá giới hạn 16-bit.");
        }

        // Vá địa chỉ vào chunk bằng hàm patch_u16
         if (!data.chunk.patch_u16(patch_target_offset, static_cast<uint16_t>(target_address))) {
             throw std::runtime_error("Lỗi nội bộ khi vá địa chỉ nhảy cho nhãn '" + std::string(label_name) + "' tại offset "
                                      + std::to_string(patch_target_offset) + " trong hàm '" + data.name + "'.");
         }
    }
    data.pending_jumps.clear(); // Xóa danh sách chờ sau khi vá xong
}


void TextParser::link_protos() {
    const std::string prefix = "::proto_ref::";
    for (auto& [func_name, proto] : finalized_protos_) {
        // Cần truy cập và sửa đổi constant pool của proto đã tạo
        auto* proto_obj_ptr = const_cast<ObjFunctionProto*>(proto);
        if (!proto_obj_ptr) continue; // Bỏ qua nếu proto null

        Chunk& chunk_ref = const_cast<Chunk&>(proto_obj_ptr->get_chunk());

        // Lấy vector constant pool để sửa đổi
        // Cần hàm `get_constants_for_linking()` hoặc tương tự trả về vector<Value>&
        // Hoặc lặp qua index và dùng hàm `set_constant(index, value)` nếu có
        // Giả sử có cách lấy vector<Value>&:
        // std::vector<Value>& constant_pool = chunk_ref.get_mutable_constant_pool_ref();

        for (size_t i = 0; i < chunk_ref.get_pool_size(); ++i) {
heap_->new_proto            auto constant = chunk_ref.get_constant_ref(i);
            if (constant.is_string()) {
                const meow::core::objects::ObjString* str_obj = constant.as_string();
                std::string_view s = str_obj->c_str(); // Lấy C-string từ ObjString

                if (s.size() > prefix.size() && s.substr(0, prefix.size()) == prefix) {
                    // Đây là một tham chiếu proto
                    std::string target_proto_name = std::string(s.substr(prefix.size()));

                    // Tìm proto tương ứng trong danh sách đã hoàn thành
                    auto target_proto_it = finalized_protos_.find(target_proto_name);
                    if (target_proto_it == finalized_protos_.end()) {
                         // Lỗi: proto được tham chiếu không tồn tại
                         throw std::runtime_error("Lỗi liên kết trong hàm '" + func_name
                                                  + "': Không tìm thấy proto '@" + target_proto_name
                                                  + "' được tham chiếu trong hằng số.");
                    }
                    // Thay thế hằng số chuỗi tham chiếu bằng con trỏ proto thực sự
                    constant = Value(target_proto_it->second);
                    // HOẶC nếu dùng index: chunk_ref.set_constant(i, Value(target_proto_it->second));
                }
            }
        }
    }
}


} // namespace meow::loader