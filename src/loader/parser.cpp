// src/loader/parser.cpp (Minified)
#include "loader/parser.h"
#include <charconv>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include "core/objects/function.h"
#include "core/objects/string.h"
#include "memory/memory_manager.h"
#include "runtime/chunk.h"
namespace meow::loader {
using namespace meow::core;
using namespace meow::runtime;
using namespace meow::memory;
using namespace meow::core::objects;
static const std::unordered_map<std::string_view, OpCode> OPCODE_PARSE_MAP = [] {
    std::unordered_map<std::string_view, OpCode> map;
    map["LOAD_CONST"] = OpCode::LOAD_CONST;
    map["LOAD_NULL"] = OpCode::LOAD_NULL;
    map["LOAD_TRUE"] = OpCode::LOAD_TRUE;
    map["LOAD_FALSE"] = OpCode::LOAD_FALSE;
    map["LOAD_INT"] = OpCode::LOAD_INT;
    map["LOAD_FLOAT"] = OpCode::LOAD_FLOAT;
    map["MOVE"] = OpCode::MOVE;
    map["ADD"] = OpCode::ADD;
    map["SUB"] = OpCode::SUB;
    map["MUL"] = OpCode::MUL;
    map["DIV"] = OpCode::DIV;
    map["MOD"] = OpCode::MOD;
    map["POW"] = OpCode::POW;
    map["EQ"] = OpCode::EQ;
    map["NEQ"] = OpCode::NEQ;
    map["GT"] = OpCode::GT;
    map["GE"] = OpCode::GE;
    map["LT"] = OpCode::LT;
    map["LE"] = OpCode::LE;
    map["NEG"] = OpCode::NEG;
    map["NOT"] = OpCode::NOT;
    map["GET_GLOBAL"] = OpCode::GET_GLOBAL;
    map["SET_GLOBAL"] = OpCode::SET_GLOBAL;
    map["GET_UPVALUE"] = OpCode::GET_UPVALUE;
    map["SET_UPVALUE"] = OpCode::SET_UPVALUE;
    map["CLOSURE"] = OpCode::CLOSURE;
    map["CLOSE_UPVALUES"] = OpCode::CLOSE_UPVALUES;
    map["JUMP"] = OpCode::JUMP;
    map["JUMP_IF_FALSE"] = OpCode::JUMP_IF_FALSE;
    map["JUMP_IF_TRUE"] = OpCode::JUMP_IF_TRUE;
    map["CALL"] = OpCode::CALL;
    map["CALL_VOID"] = OpCode::CALL_VOID;
    map["RETURN"] = OpCode::RETURN;
    map["HALT"] = OpCode::HALT;
    map["NEW_ARRAY"] = OpCode::NEW_ARRAY;
    map["NEW_HASH"] = OpCode::NEW_HASH;
    map["GET_INDEX"] = OpCode::GET_INDEX;
    map["SET_INDEX"] = OpCode::SET_INDEX;
    map["GET_KEYS"] = OpCode::GET_KEYS;
    map["GET_VALUES"] = OpCode::GET_VALUES;
    map["NEW_CLASS"] = OpCode::NEW_CLASS;
    map["NEW_INSTANCE"] = OpCode::NEW_INSTANCE;
    map["GET_PROP"] = OpCode::GET_PROP;
    map["SET_PROP"] = OpCode::SET_PROP;
    map["SET_METHOD"] = OpCode::SET_METHOD;
    map["INHERIT"] = OpCode::INHERIT;
    map["GET_SUPER"] = OpCode::GET_SUPER;
    map["BIT_AND"] = OpCode::BIT_AND;
    map["BIT_OR"] = OpCode::BIT_OR;
    map["BIT_XOR"] = OpCode::BIT_XOR;
    map["BIT_NOT"] = OpCode::BIT_NOT;
    map["LSHIFT"] = OpCode::LSHIFT;
    map["RSHIFT"] = OpCode::RSHIFT;
    map["THROW"] = OpCode::THROW;
    map["SETUP_TRY"] = OpCode::SETUP_TRY;
    map["POP_TRY"] = OpCode::POP_TRY;
    map["IMPORT_MODULE"] = OpCode::IMPORT_MODULE;
    map["EXPORT"] = OpCode::EXPORT;
    map["GET_EXPORT"] = OpCode::GET_EXPORT;
    map["IMPORT_ALL"] = OpCode::IMPORT_ALL;
    return map;
}();
TextParser::TextParser(MemoryManager *heap) noexcept : heap_(heap) {}
[[noreturn]] void TextParser::throw_parse_error(const std::string &message) {
    if (current_token_index_ < tokens_.size()) {
        throw_parse_error(message, current_token());
    } else {
        size_t last_line = tokens_.empty() ? 1 : tokens_.back().line;
        size_t last_col = tokens_.empty() ? 1 : tokens_.back().col + tokens_.back().lexeme.length();
        std::string error_message = std::format("Lỗi phân tích cú pháp [{}:{}:{}]: {}",
                                                current_source_name_, last_line, last_col, message);
        throw std::runtime_error(error_message);
    }
}
[[noreturn]] void TextParser::throw_parse_error(const std::string &message, const Token &token) {
    std::string error_message = std::format("Lỗi phân tích cú pháp [{}:{}:{}]: {}",
                                            current_source_name_, token.line, token.col, message);
    if (!token.lexeme.empty() && token.type != TokenType::END_OF_FILE) {
        error_message += std::format(" (gần '{}')", std::string(token.lexeme));
    }
    throw std::runtime_error(error_message);
}
const Token &TextParser::current_token() const {
    if (current_token_index_ >= tokens_.size()) {
        return tokens_.back();
    }
    return tokens_[current_token_index_];
}
const Token &TextParser::peek_token(size_t offset) const {
    size_t index = current_token_index_ + offset;
    if (index >= tokens_.size()) {
        return tokens_.back();
    }
    return tokens_[index];
}
bool TextParser::is_at_end() const { return current_token().type == TokenType::END_OF_FILE; }
void TextParser::advance() {
    if (current_token().type != TokenType::END_OF_FILE) {
        current_token_index_++;
    }
}
const Token &TextParser::consume_token(TokenType expected, const std::string &error_message) {
    const Token &token = current_token();
    if (token.type != expected) {
        throw_parse_error(error_message, token);
    }
    advance();
    return tokens_[current_token_index_ - 1];
}
bool TextParser::match_token(TokenType type) {
    if (current_token().type == type) {
        advance();
        return true;
    }
    return false;
}
std::string TextParser::unescape_string(const std::string &escaped) {
    std::stringstream ss;
    bool is_escaping = false;
    for (char c : escaped) {
        if (is_escaping) {
            switch (c) {
                case 'n':
                    ss << '\n';
                    break;
                case 't':
                    ss << '\t';
                    break;
                case 'r':
                    ss << '\r';
                    break;
                case '\\':
                    ss << '\\';
                    break;
                case '"':
                    ss << '"';
                    break;
                default:
                    ss << c;
                    break;
            }
            is_escaping = false;
        } else if (c == '\\') {
            is_escaping = true;
        } else {
            ss << c;
        }
    }
    return ss.str();
}
proto_t TextParser::parse_file(const std::string &filepath, MemoryManager &mm) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Không thể mở tệp: " + filepath);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return parse_source(buffer.str(), mm, filepath);
}
proto_t TextParser::parse_source(const std::string &source, MemoryManager &mm,
                                 const std::string &source_name) {
    heap_ = &mm;
    current_source_name_ = source_name;
    current_token_index_ = 0;
    current_proto_data_ = nullptr;
    build_data_map_.clear();
    finalized_protos_.clear();
    Lexer lexer(source);
    tokens_ = lexer.tokenize_all();
    if (!tokens_.empty() && tokens_.back().type == TokenType::UNKNOWN) {
        throw_parse_error("Lỗi Lexer, ký tự không xác định hoặc chuỗi/comment không đóng.",
                          tokens_.back());
    }
    if (tokens_.empty() || tokens_.back().type != TokenType::END_OF_FILE) {
        size_t last_line = tokens_.empty() ? 1 : tokens_.back().line;
        size_t last_col = tokens_.empty() ? 1 : tokens_.back().col + tokens_.back().lexeme.length();
        tokens_.push_back({"", TokenType::END_OF_FILE, last_line, last_col});
    }
    parse();
    for (auto &[name, data] : build_data_map_) {
        resolve_labels_for_build_data(data);
    }
    finalized_protos_.clear();
    std::vector<std::pair<proto_t, ProtoBuildData *>> protos_to_link;
    for (auto &[name, data] : build_data_map_) {
        string_t func_name_obj = heap_->new_string(name);
        std::vector<Value> final_constants = build_final_constant_pool(data);
        Chunk final_chunk(std::move(data.temp_code), std::move(final_constants));
        std::vector<UpvalueDesc> final_descs = std::move(data.upvalue_descs);
        if (final_descs.size() != data.num_upvalues) {
            throw std::runtime_error(
                "Lỗi nội bộ: Số lượng upvalue desc không khớp khai báo cho "
                "hàm " +
                name);
        }
        proto_t new_proto = heap_->new_proto(data.num_registers, data.num_upvalues, func_name_obj,
                                             std::move(final_chunk), std::move(final_descs));
        if (!new_proto) {
            throw std::runtime_error("Lỗi cấp phát bộ nhớ khi tạo proto cho hàm " + name);
        }
        finalized_protos_[name] = new_proto;
        protos_to_link.push_back({new_proto, &data});
    }
    for (auto &pair : protos_to_link) {
        proto_t proto_to_link = pair.first;
        const Chunk &chunk_ref = proto_to_link->get_chunk();
        for (size_t i = 0; i < chunk_ref.get_pool_size(); ++i) {
            Value constant = chunk_ref.get_constant(i);
            if (constant.is_string()) {
                const meow::core::objects::ObjString *str_obj = constant.as_string();
                std::string_view placeholder_name = str_obj->c_str();
                auto target_it = finalized_protos_.find(std::string(placeholder_name));
                if (target_it != finalized_protos_.end()) {
                    Value &const_ref = const_cast<Chunk &>(chunk_ref).get_constant_ref(i);
                    const_ref = Value(target_it->second);
                }
            }
        }
    }
    auto main_it = finalized_protos_.find("main");
    if (main_it == finalized_protos_.end()) {
        throw std::runtime_error("Không tìm thấy hàm chính '@main' trong " + current_source_name_);
    }
    heap_ = nullptr;
    tokens_.clear();
    build_data_map_.clear();
    return main_it->second;
}
const std::unordered_map<std::string, proto_t> &TextParser::get_finalized_protos() const {
    return finalized_protos_;
}
void TextParser::parse() {
    while (!is_at_end()) {
        parse_statement();
    }
    if (current_proto_data_) {
        const Token &last_token = tokens_.empty() ? Token{"", TokenType::UNKNOWN, 0, 0}
                                                  : tokens_[current_token_index_ - 1];
        throw_parse_error("Thiếu chỉ thị '.endfunc' cho hàm '" + current_proto_data_->name +
                              "' bắt đầu tại dòng " +
                              std::to_string(current_proto_data_->func_directive_line),
                          last_token);
    }
}
void TextParser::parse_statement() {
    const Token &token = current_token();
    switch (token.type) {
        case TokenType::DIR_FUNC:
            if (current_proto_data_) {
                throw_parse_error("Không thể định nghĩa hàm lồng nhau.", token);
            }
            parse_func_directive();
            break;
        case TokenType::DIR_ENDFUNC:
            if (!current_proto_data_) {
                throw_parse_error(
                    "Chỉ thị '.endfunc' không mong đợi bên ngoài định nghĩa "
                    "hàm.",
                    token);
            } else {
                throw_parse_error("Lỗi logic nội bộ: Gặp '.endfunc' ở parse_statement.", token);
            }
            break;
        case TokenType::DIR_REGISTERS:
        case TokenType::DIR_UPVALUES:
        case TokenType::DIR_CONST:
        case TokenType::DIR_UPVALUE:
            if (!current_proto_data_)
                throw_parse_error("Chỉ thị phải nằm trong định nghĩa hàm (.func).", token);
            if ((token.type == TokenType::DIR_CONST || token.type == TokenType::DIR_UPVALUE) &&
                (!current_proto_data_->registers_defined ||
                 !current_proto_data_->upvalues_defined)) {
                throw_parse_error(
                    "Chỉ thị '.registers' và '.upvalues' phải được định "
                    "nghĩa trước '" +
                        std::string(token.lexeme) + "'.",
                    token);
            }
            if (token.type == TokenType::DIR_REGISTERS)
                parse_registers_directive();
            else if (token.type == TokenType::DIR_UPVALUES)
                parse_upvalues_directive();
            else if (token.type == TokenType::DIR_CONST)
                parse_const_directive();
            else if (token.type == TokenType::DIR_UPVALUE)
                parse_upvalue_directive();
            break;
        case TokenType::LABEL_DEF:
            if (!current_proto_data_)
                throw_parse_error("Nhãn phải nằm trong định nghĩa hàm (.func).", token);
            if (!current_proto_data_->registers_defined || !current_proto_data_->upvalues_defined) {
                throw_parse_error(
                    "Chỉ thị '.registers' và '.upvalues' phải được định "
                    "nghĩa trước nhãn.",
                    token);
            }
            parse_label_definition();
            break;
        case TokenType::OPCODE:
            if (!current_proto_data_)
                throw_parse_error("Lệnh phải nằm trong định nghĩa hàm (.func).", token);
            if (!current_proto_data_->registers_defined) {
                throw_parse_error(
                    "Chỉ thị '.registers' phải được định nghĩa trước lệnh đầu "
                    "tiên.",
                    token);
            }
            if (!current_proto_data_->upvalues_defined) {
                throw_parse_error(
                    "Chỉ thị '.upvalues' phải được định nghĩa trước lệnh đầu "
                    "tiên.",
                    token);
            }
            parse_instruction();
            break;
        case TokenType::IDENTIFIER:
            throw_parse_error("Token không mong đợi. Có thể thiếu directive hoặc opcode?", token);
            break;
        case TokenType::NUMBER_INT:
        case TokenType::NUMBER_FLOAT:
        case TokenType::STRING:
            throw_parse_error(
                "Giá trị literal không hợp lệ ở đây. Có thể thiếu chỉ "
                "thị '.const'?",
                token);
            break;
        case TokenType::END_OF_FILE:
            break;
        case TokenType::UNKNOWN:
            throw_parse_error("Token không hợp lệ hoặc ký tự không nhận dạng.", token);
            break;
    }
}
void TextParser::parse_func_directive() {
    const Token &func_token = consume_token(TokenType::DIR_FUNC, "Mong đợi '.func'.");
    const Token &name_token = consume_token(TokenType::IDENTIFIER, "Mong đợi tên hàm sau '.func'.");
    std::string func_name(name_token.lexeme);
    if (func_name.empty() ||
        (func_name[0] != '@' && !std::isalpha(func_name[0]) && func_name[0] != '_')) {
        throw_parse_error("Tên hàm không hợp lệ.", name_token);
    }
    std::string map_key = (func_name[0] == '@') ? func_name.substr(1) : func_name;
    if (map_key.empty()) {
        throw_parse_error("Tên hàm không hợp lệ (chỉ có '@').", name_token);
    }
    if (build_data_map_.count(map_key)) {
        throw_parse_error("Hàm '" + func_name + "' đã được định nghĩa.", name_token);
    }
    auto [it, inserted] = build_data_map_.emplace(map_key, ProtoBuildData{});
    current_proto_data_ = &it->second;
    current_proto_data_->name = map_key;
    current_proto_data_->func_directive_line = func_token.line;
    current_proto_data_->func_directive_col = func_token.col;
    parse_registers_directive();
    parse_upvalues_directive();
    while (!is_at_end() && current_token().type != TokenType::DIR_ENDFUNC) {
        parse_statement();
    }
    consume_token(TokenType::DIR_ENDFUNC,
                  "Mong đợi '.endfunc' để kết thúc hàm '" + func_name + "'.");
    current_proto_data_ = nullptr;
}
void TextParser::parse_registers_directive() {
    consume_token(TokenType::DIR_REGISTERS, "Lỗi nội bộ: Mong đợi '.registers'.");
    if (!current_proto_data_)
        throw_parse_error("Lỗi nội bộ: current_proto_data_ là null khi parse .registers.");
    if (current_proto_data_->registers_defined)
        throw_parse_error("Chỉ thị '.registers' đã được định nghĩa cho hàm này.");
    const Token &num_token =
        consume_token(TokenType::NUMBER_INT,
                      "Mong đợi số lượng thanh ghi (số nguyên không âm) sau '.registers'.");
    try {
        uint64_t num_reg;
        auto result = std::from_chars(num_token.lexeme.data(),
                                      num_token.lexeme.data() + num_token.lexeme.size(), num_reg);
        if (result.ec != std::errc() ||
            result.ptr != num_token.lexeme.data() + num_token.lexeme.size() ||
            num_reg > std::numeric_limits<size_t>::max()) {
            throw std::out_of_range("Invalid or out-of-range register count");
        }
        current_proto_data_->num_registers = static_cast<size_t>(num_reg);
        current_proto_data_->registers_defined = true;
    } catch (...) {
        throw_parse_error("Số lượng thanh ghi không hợp lệ.", num_token);
    }
}
void TextParser::parse_upvalues_directive() {
    consume_token(TokenType::DIR_UPVALUES, "Lỗi nội bộ: Mong đợi '.upvalues'.");
    if (!current_proto_data_)
        throw_parse_error("Lỗi nội bộ: current_proto_data_ là null khi parse .upvalues.");
    if (current_proto_data_->upvalues_defined)
        throw_parse_error("Chỉ thị '.upvalues' đã được định nghĩa cho hàm này.");
    const Token &num_token = consume_token(
        TokenType::NUMBER_INT, "Mong đợi số lượng upvalue (số nguyên không âm) sau '.upvalues'.");
    try {
        uint64_t num_up;
        auto result = std::from_chars(num_token.lexeme.data(),
                                      num_token.lexeme.data() + num_token.lexeme.size(), num_up);
        if (result.ec != std::errc() ||
            result.ptr != num_token.lexeme.data() + num_token.lexeme.size() ||
            num_up > std::numeric_limits<size_t>::max()) {
            throw std::out_of_range("Invalid or out-of-range upvalue count");
        }
        current_proto_data_->num_upvalues = static_cast<size_t>(num_up);
        current_proto_data_->upvalue_descs.resize(current_proto_data_->num_upvalues);
        current_proto_data_->upvalues_defined = true;
    } catch (...) {
        throw_parse_error("Số lượng upvalue không hợp lệ.", num_token);
    }
}
void TextParser::parse_const_directive() {
    consume_token(TokenType::DIR_CONST, "Mong đợi '.const'.");
    if (!current_proto_data_)
        throw_parse_error("Lỗi nội bộ: current_proto_data_ là null khi parse .const.");
    Value const_val = parse_const_value_from_tokens();
    current_proto_data_->add_temp_constant(const_val);
}
void TextParser::parse_upvalue_directive() {
    consume_token(TokenType::DIR_UPVALUE, "Mong đợi '.upvalue'.");
    if (!current_proto_data_)
        throw_parse_error("Lỗi nội bộ: current_proto_data_ là null khi parse .upvalue.");
    if (!current_proto_data_->upvalues_defined) {
        throw_parse_error("Chỉ thị '.upvalues' phải được định nghĩa trước '.upvalue'.",
                          tokens_[current_token_index_ - 1]);
    }
    const Token &index_token =
        consume_token(TokenType::NUMBER_INT, "Mong đợi chỉ số upvalue (0-based) sau '.upvalue'.");
    size_t uv_index;
    try {
        uint64_t temp_idx;
        auto result =
            std::from_chars(index_token.lexeme.data(),
                            index_token.lexeme.data() + index_token.lexeme.size(), temp_idx);
        if (result.ec != std::errc() ||
            result.ptr != index_token.lexeme.data() + index_token.lexeme.size() ||
            temp_idx >= current_proto_data_->num_upvalues) {
            throw std::out_of_range("Invalid or out-of-range upvalue index");
        }
        uv_index = static_cast<size_t>(temp_idx);
    } catch (...) {
        throw_parse_error("Chỉ số upvalue không hợp lệ hoặc vượt quá số lượng đã khai báo (" +
                              std::to_string(current_proto_data_->num_upvalues) + ").",
                          index_token);
    }
    const Token &type_token =
        consume_token(TokenType::IDENTIFIER, "Mong đợi loại upvalue ('local' hoặc 'parent').");
    bool is_local;
    if (type_token.lexeme == "local") {
        is_local = true;
    } else if (type_token.lexeme == "parent") {
        is_local = false;
    } else {
        throw_parse_error("Loại upvalue không hợp lệ. Phải là 'local' hoặc 'parent'.", type_token);
    }
    const Token &slot_token = consume_token(TokenType::NUMBER_INT,
                                            "Mong đợi chỉ số slot (thanh ghi nếu 'local', "
                                            "upvalue cha nếu 'parent').");
    size_t slot_index;
    try {
        uint64_t temp_slot;
        auto result =
            std::from_chars(slot_token.lexeme.data(),
                            slot_token.lexeme.data() + slot_token.lexeme.size(), temp_slot);
        if (result.ec != std::errc() ||
            result.ptr != slot_token.lexeme.data() + slot_token.lexeme.size() ||
            temp_slot > std::numeric_limits<size_t>::max()) {
            throw std::out_of_range("Invalid or out-of-range slot index");
        }
        slot_index = static_cast<size_t>(temp_slot);
        if (is_local && slot_index >= current_proto_data_->num_registers) {
            throw_parse_error("Chỉ số slot cho upvalue 'local' (" + std::to_string(slot_index) +
                                  ") phải nhỏ hơn số lượng thanh ghi (" +
                                  std::to_string(current_proto_data_->num_registers) + ").",
                              slot_token);
        }
    } catch (...) {
        throw_parse_error("Chỉ số slot không hợp lệ.", slot_token);
    }
    current_proto_data_->upvalue_descs[uv_index] = UpvalueDesc(is_local, slot_index);
}
void TextParser::parse_label_definition() {
    const Token &label_token =
        consume_token(TokenType::LABEL_DEF, "Lỗi nội bộ: Mong đợi định nghĩa nhãn.");
    if (!current_proto_data_)
        throw_parse_error("Lỗi nội bộ: current_proto_data_ là null khi parse label def.");
    std::string_view label_name = label_token.lexeme;
    if (current_proto_data_->labels.count(label_name)) {
        throw_parse_error(
            "Nhãn '" + std::string(label_name) + "' đã được định nghĩa trong hàm này.",
            label_token);
    }
    current_proto_data_->labels[label_name] = current_proto_data_->temp_code.size();
}
void TextParser::parse_instruction() {
    const Token &opcode_token = consume_token(TokenType::OPCODE, "Lỗi nội bộ: Mong đợi opcode.");
    if (!current_proto_data_)
        throw_parse_error("Lỗi nội bộ: current_proto_data_ là null khi parse instruction.");
    auto it = OPCODE_PARSE_MAP.find(opcode_token.lexeme);
    OpCode opcode = it->second;
    ProtoBuildData &data_ref = *current_proto_data_;
    size_t instruction_opcode_pos = data_ref.temp_code.size();
    data_ref.write_byte(static_cast<uint8_t>(opcode));
    auto parse_u16_arg = [&]() -> uint16_t {
        const Token &token =
            consume_token(TokenType::NUMBER_INT, "Mong đợi đối số là số nguyên 16-bit không dấu.");
        uint64_t value_u64;
        auto result = std::from_chars(token.lexeme.data(),
                                      token.lexeme.data() + token.lexeme.size(), value_u64);
        if (result.ec != std::errc() || result.ptr != token.lexeme.data() + token.lexeme.size() ||
            value_u64 > UINT16_MAX) {
            throw_parse_error("Đối số phải là số nguyên 16-bit không dấu hợp lệ (0-" +
                                  std::to_string(UINT16_MAX) + ").",
                              token);
        }
        return static_cast<uint16_t>(value_u64);
    };
    auto parse_i64_arg = [&]() -> int64_t {
        const Token &token = current_token();
        int64_t value;
        auto result =
            std::from_chars(token.lexeme.data(), token.lexeme.data() + token.lexeme.size(), value);
        if (result.ec == std::errc() && result.ptr == token.lexeme.data() + token.lexeme.size()) {
            advance();
            return value;
        } else if (token.type == TokenType::NUMBER_FLOAT && opcode == OpCode::LOAD_INT) {
            advance();
            try {
                double d_val = std::stod(std::string(token.lexeme));
                if (d_val > static_cast<double>(std::numeric_limits<int64_t>::max()) ||
                    d_val < static_cast<double>(std::numeric_limits<int64_t>::min())) {
                    throw_parse_error(
                        "Giá trị số thực quá lớn/nhỏ để chuyển đổi thành "
                        "số nguyên 64-bit.",
                        token);
                }
                return static_cast<int64_t>(d_val);
            } catch (...) {
                throw_parse_error("Không thể chuyển đổi số thực thành số nguyên 64-bit.", token);
            }
        }
        throw_parse_error("Mong đợi đối số là số nguyên 64-bit.", token);
    };
    auto parse_f64_arg = [&]() -> double {
        const Token &token = current_token();
        if (token.type != TokenType::NUMBER_FLOAT && token.type != TokenType::NUMBER_INT) {
            throw_parse_error("Mong đợi đối số là số thực hoặc số nguyên.", token);
        }
        advance();
        try {
            size_t processed = 0;
            double value = std::stod(std::string(token.lexeme), &processed);
            if (processed != token.lexeme.size()) {
                throw std::invalid_argument("Invalid double format");
            }
            return value;
        } catch (...) {
            throw_parse_error("Đối số số thực không hợp lệ.", token);
        }
    };
    auto parse_address_or_label_arg = [&]() {
        const Token &token = current_token();
        if (token.type == TokenType::NUMBER_INT) {
            uint16_t address = parse_u16_arg();
            data_ref.write_u16(address);
        } else if (token.type == TokenType::IDENTIFIER) {
            advance();
            size_t patch_target_offset = data_ref.temp_code.size();
            data_ref.write_u16(0xDEAD);
            data_ref.pending_jumps.emplace_back(instruction_opcode_pos, patch_target_offset,
                                                token.lexeme);
        } else {
            throw_parse_error("Mong đợi nhãn hoặc địa chỉ cho lệnh nhảy.", token);
        }
    };
    switch (opcode) {
        case OpCode::LOAD_NULL:
        case OpCode::LOAD_TRUE:
        case OpCode::LOAD_FALSE:
        case OpCode::POP_TRY:
            break;
        case OpCode::IMPORT_ALL:
            data_ref.write_u16(parse_u16_arg());
            break;
        case OpCode::LOAD_CONST: {
            uint16_t dst = parse_u16_arg();
            Value const_val = parse_const_value_from_tokens();
            size_t const_idx = data_ref.add_temp_constant(const_val);
            if (const_idx > UINT16_MAX) {
                throw_parse_error("Quá nhiều hằng số, chỉ số vượt quá giới hạn 16-bit.",
                                  tokens_[current_token_index_ - 1]);
            }
            data_ref.write_u16(dst);
            data_ref.write_u16(static_cast<uint16_t>(const_idx));
            break;
        }
        case OpCode::GET_GLOBAL:
        case OpCode::NEW_CLASS:
        case OpCode::GET_SUPER:
        case OpCode::CLOSURE:
        case OpCode::IMPORT_MODULE:
            data_ref.write_u16(parse_u16_arg());
            {
                const Token &name_token = current_token();
                Value name_val;
                if (name_token.type == TokenType::STRING ||
                    name_token.type == TokenType::IDENTIFIER) {
                    name_val = parse_const_value_from_tokens();
                } else {
                    throw_parse_error("Mong đợi tên (chuỗi hoặc @Proto) làm đối số thứ hai.",
                                      name_token);
                }
                size_t name_idx = data_ref.add_temp_constant(name_val);
                if (name_idx > UINT16_MAX) {
                    throw_parse_error(
                        "Quá nhiều hằng số (tên), chỉ số vượt quá giới hạn "
                        "16-bit.",
                        name_token);
                }
                data_ref.write_u16(static_cast<uint16_t>(name_idx));
            }
            break;
        case OpCode::EXPORT:
        case OpCode::SET_GLOBAL: {
            const Token &name_token = current_token();
            Value name_val;
            if (name_token.type == TokenType::STRING) {
                name_val = parse_const_value_from_tokens();
            } else {
                throw_parse_error("Mong đợi tên (chuỗi) làm đối số đầu.", name_token);
            }
            size_t name_idx = data_ref.add_temp_constant(name_val);
            if (name_idx > UINT16_MAX) {
                throw_parse_error("Quá nhiều hằng số (tên), chỉ số vượt quá giới hạn 16-bit.",
                                  name_token);
            }
            data_ref.write_u16(static_cast<uint16_t>(name_idx));
        }
            data_ref.write_u16(parse_u16_arg());
            break;
        case OpCode::MOVE:
        case OpCode::NEG:
        case OpCode::NOT:
        case OpCode::BIT_NOT:
        case OpCode::GET_UPVALUE:
        case OpCode::NEW_INSTANCE:
        case OpCode::GET_KEYS:
        case OpCode::GET_VALUES:
            data_ref.write_u16(parse_u16_arg());
            data_ref.write_u16(parse_u16_arg());
            break;
        case OpCode::SET_UPVALUE:
            data_ref.write_u16(parse_u16_arg());
            data_ref.write_u16(parse_u16_arg());
            break;
        case OpCode::CLOSE_UPVALUES:
            data_ref.write_u16(parse_u16_arg());
            break;
        case OpCode::LOAD_INT:
            data_ref.write_u16(parse_u16_arg());
            data_ref.write_u64(std::bit_cast<uint64_t>(parse_i64_arg()));
            break;
        case OpCode::LOAD_FLOAT:
            data_ref.write_u16(parse_u16_arg());
            data_ref.write_f64(parse_f64_arg());
            break;
        case OpCode::ADD:
        case OpCode::SUB:
        case OpCode::MUL:
        case OpCode::DIV:
        case OpCode::MOD:
        case OpCode::POW:
        case OpCode::EQ:
        case OpCode::NEQ:
        case OpCode::GT:
        case OpCode::GE:
        case OpCode::LT:
        case OpCode::LE:
        case OpCode::BIT_AND:
        case OpCode::BIT_OR:
        case OpCode::BIT_XOR:
        case OpCode::LSHIFT:
        case OpCode::RSHIFT:
        case OpCode::GET_INDEX:
        case OpCode::NEW_ARRAY:
        case OpCode::NEW_HASH:
            data_ref.write_u16(parse_u16_arg());
            data_ref.write_u16(parse_u16_arg());
            data_ref.write_u16(parse_u16_arg());
            break;
        case OpCode::GET_PROP:
        case OpCode::SET_METHOD:
        case OpCode::GET_EXPORT:
            data_ref.write_u16(parse_u16_arg());
            data_ref.write_u16(parse_u16_arg());
            {
                const Token &name_token = current_token();
                Value name_val;
                if (name_token.type == TokenType::STRING) {
                    name_val = parse_const_value_from_tokens();
                } else {
                    throw_parse_error("Mong đợi tên (chuỗi) làm đối số thứ ba.", name_token);
                }
                size_t name_idx = data_ref.add_temp_constant(name_val);
                if (name_idx > UINT16_MAX) {
                    throw_parse_error(
                        "Quá nhiều hằng số (tên), chỉ số vượt quá giới hạn "
                        "16-bit.",
                        name_token);
                }
                data_ref.write_u16(static_cast<uint16_t>(name_idx));
            }
            break;
        case OpCode::SET_INDEX:
            data_ref.write_u16(parse_u16_arg());
            data_ref.write_u16(parse_u16_arg());
            data_ref.write_u16(parse_u16_arg());
            break;
        case OpCode::SET_PROP:
            data_ref.write_u16(parse_u16_arg());
            {
                const Token &name_token = current_token();
                Value name_val;
                if (name_token.type == TokenType::STRING) {
                    name_val = parse_const_value_from_tokens();
                } else {
                    throw_parse_error("Mong đợi tên (chuỗi) làm đối số thứ hai.", name_token);
                }
                size_t name_idx = data_ref.add_temp_constant(name_val);
                if (name_idx > UINT16_MAX) {
                    throw_parse_error(
                        "Quá nhiều hằng số (tên), chỉ số vượt quá giới hạn "
                        "16-bit.",
                        name_token);
                }
                data_ref.write_u16(static_cast<uint16_t>(name_idx));
            }
            data_ref.write_u16(parse_u16_arg());
            break;
        case OpCode::INHERIT:
            data_ref.write_u16(parse_u16_arg());
            data_ref.write_u16(parse_u16_arg());
            break;
        case OpCode::JUMP:
        case OpCode::SETUP_TRY:
            parse_address_or_label_arg();
            break;
        case OpCode::JUMP_IF_FALSE:
        case OpCode::JUMP_IF_TRUE:
            data_ref.write_u16(parse_u16_arg());
            parse_address_or_label_arg();
            break;
        case OpCode::CALL:
            data_ref.write_u16(parse_u16_arg());
            data_ref.write_u16(parse_u16_arg());
            data_ref.write_u16(parse_u16_arg());
            data_ref.write_u16(parse_u16_arg());
            break;
        case OpCode::CALL_VOID:
            data_ref.write_u16(parse_u16_arg());
            data_ref.write_u16(parse_u16_arg());
            data_ref.write_u16(parse_u16_arg());
            break;
        case OpCode::RETURN: {
            const Token &ret_token = current_token();
            if (ret_token.type == TokenType::NUMBER_INT && ret_token.lexeme == "-1") {
                advance();
                data_ref.write_u16(0xFFFF);
            } else if (ret_token.type == TokenType::IDENTIFIER &&
                       (ret_token.lexeme == "FFFF" || ret_token.lexeme == "ffff")) {
                advance();
                data_ref.write_u16(0xFFFF);
            } else if (ret_token.type == TokenType::NUMBER_INT) {
                data_ref.write_u16(parse_u16_arg());
            } else {
                throw_parse_error(
                    "Mong đợi thanh ghi trả về (số nguyên không âm, -1, hoặc "
                    "FFFF).",
                    ret_token);
            }
        } break;
        case OpCode::THROW:
            data_ref.write_u16(parse_u16_arg());
            break;
        case OpCode::HALT:
            break;
        default:
            throw_parse_error("Opcode '" + std::string(opcode_token.lexeme) +
                                  "' chưa được hỗ trợ xử lý đối số trong parser.",
                              opcode_token);
    }
}
Value TextParser::parse_const_value_from_tokens() {
    const Token &token = current_token();
    switch (token.type) {
        case TokenType::STRING: {
            advance();
            if (token.lexeme.length() < 2 || token.lexeme.front() != '"' ||
                token.lexeme.back() != '"') {
                throw_parse_error("Chuỗi literal không hợp lệ (thiếu dấu \"\").", token);
            }
            std::string_view inner = token.lexeme.substr(1, token.lexeme.length() - 2);
            std::string unescaped = TextParser::unescape_string(std::string(inner));
            return Value(heap_->new_string(unescaped));
        }
        case TokenType::NUMBER_INT: {
            advance();
            int64_t value;
            auto result = std::from_chars(token.lexeme.data(),
                                          token.lexeme.data() + token.lexeme.size(), value);
            if (result.ec != std::errc() ||
                result.ptr != token.lexeme.data() + token.lexeme.size()) {
                throw_parse_error("Số nguyên literal không hợp lệ.", token);
            }
            return Value(value);
        }
        case TokenType::NUMBER_FLOAT: {
            advance();
            try {
                size_t processed = 0;
                double value = std::stod(std::string(token.lexeme), &processed);
                if (processed != token.lexeme.size()) {
                    throw std::invalid_argument("Incomplete parse");
                }
                return Value(value);
            } catch (...) {
                throw_parse_error("Số thực literal không hợp lệ.", token);
            }
        }
        case TokenType::IDENTIFIER: {
            advance();
            if (token.lexeme == "true")
                return Value(true);
            if (token.lexeme == "false")
                return Value(false);
            if (token.lexeme == "null")
                return Value(null_t{});
            if (!token.lexeme.empty() && token.lexeme.front() == '@') {
                std::string_view proto_name_view = token.lexeme.substr(1);
                if (proto_name_view.empty()) {
                    throw_parse_error("Tên proto tham chiếu không được rỗng (chỉ có '@').", token);
                }
                std::string ref_string = "::proto_ref::" + std::string(proto_name_view);
                return Value(heap_->new_string(ref_string));
            }
            throw_parse_error("Identifier không hợp lệ cho giá trị hằng số.", token);
        }
        default:
            throw_parse_error("Token không mong đợi cho giá trị hằng số.", token);
    }
}
void TextParser::resolve_labels_for_build_data(ProtoBuildData &data) {
    for (const auto &jump_info : data.pending_jumps) {
        [[maybe_unused]] size_t instruction_opcode_pos = std::get<0>(jump_info);
        size_t patch_target_offset = std::get<1>(jump_info);
        std::string_view label_name = std::get<2>(jump_info);
        auto label_it = data.labels.find(label_name);
        if (label_it == data.labels.end()) {
            throw std::runtime_error("Lỗi liên kết trong hàm '" + data.name +
                                     "': Không tìm thấy nhãn '" + std::string(label_name) + "'.");
        }
        size_t target_address = label_it->second;
        if (target_address > UINT16_MAX) {
            throw std::runtime_error("Lỗi liên kết trong hàm '" + data.name + "': Địa chỉ nhãn '" +
                                     std::string(label_name) + "' (" +
                                     std::to_string(target_address) +
                                     ") vượt quá giới hạn 16-bit.");
        }
        if (!data.patch_u16(patch_target_offset, static_cast<uint16_t>(target_address))) {
            throw std::runtime_error("Lỗi nội bộ khi vá địa chỉ nhảy cho nhãn '" +
                                     std::string(label_name) + "' tại offset " +
                                     std::to_string(patch_target_offset) + " trong hàm '" +
                                     data.name + "'.");
        }
    }
    data.pending_jumps.clear();
}
std::vector<Value> TextParser::build_final_constant_pool(ProtoBuildData &data) {
    const std::string prefix = "::proto_ref::";
    std::vector<Value> final_constants;
    final_constants.reserve(data.temp_constants.size());
    for (const auto &constant : data.temp_constants) {
        if (constant.is_string()) {
            const meow::core::objects::ObjString *str_obj = constant.as_string();
            std::string_view s = str_obj->c_str();
            if (s.size() > prefix.size() && s.substr(0, prefix.size()) == prefix) {
                std::string target_proto_name = std::string(s.substr(prefix.size()));
                final_constants.push_back(Value(heap_->new_string(target_proto_name)));
            } else {
                final_constants.push_back(constant);
            }
        } else {
            final_constants.push_back(constant);
        }
    }
    return final_constants;
}
}  // namespace meow::loader