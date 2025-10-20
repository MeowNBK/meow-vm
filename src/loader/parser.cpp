--- Old codes ---

// #include "loader/parser.h"
// #include "memory/memory_manager.h"
// #include "core/objects/function.h"
// #include "core/objects/string.h"
// #include "runtime/chunk.h"
// #include <stdexcept>
// #include <charconv> // std::from_chars
// #include <limits>   // numeric_limits

// namespace meow::loader {

// using namespace meow::core;
// using namespace meow::runtime;
// using namespace meow::memory;
// using namespace meow::core::objects; // Cho UpvalueDesc

// // --- Bảng Opcode (string_view -> OpCode enum) ---
// // Tạo lại ở đây để parser sử dụng
// static const std::unordered_map<std::string_view, OpCode> OPCODE_PARSE_MAP = []{
//     std::unordered_map<std::string_view, OpCode> map;
//     map["LOAD_CONST"] = OpCode::LOAD_CONST; map["LOAD_NULL"] = OpCode::LOAD_NULL; map["LOAD_TRUE"] = OpCode::LOAD_TRUE;
//     map["LOAD_FALSE"] = OpCode::LOAD_FALSE; map["LOAD_INT"] = OpCode::LOAD_INT; map["LOAD_FLOAT"] = OpCode::LOAD_FLOAT;
//     map["MOVE"] = OpCode::MOVE; map["ADD"] = OpCode::ADD; map["SUB"] = OpCode::SUB; map["MUL"] = OpCode::MUL;
//     map["DIV"] = OpCode::DIV; map["MOD"] = OpCode::MOD; map["POW"] = OpCode::POW; map["EQ"] = OpCode::EQ;
//     map["NEQ"] = OpCode::NEQ; map["GT"] = OpCode::GT; map["GE"] = OpCode::GE; map["LT"] = OpCode::LT;
//     map["LE"] = OpCode::LE; map["NEG"] = OpCode::NEG; map["NOT"] = OpCode::NOT; map["GET_GLOBAL"] = OpCode::GET_GLOBAL;
//     map["SET_GLOBAL"] = OpCode::SET_GLOBAL; map["GET_UPVALUE"] = OpCode::GET_UPVALUE; map["SET_UPVALUE"] = OpCode::SET_UPVALUE;
//     map["CLOSURE"] = OpCode::CLOSURE; map["CLOSE_UPVALUES"] = OpCode::CLOSE_UPVALUES; map["JUMP"] = OpCode::JUMP;
//     map["JUMP_IF_FALSE"] = OpCode::JUMP_IF_FALSE; map["JUMP_IF_TRUE"] = OpCode::JUMP_IF_TRUE; map["CALL"] = OpCode::CALL;
//     map["CALL_VOID"] = OpCode::CALL_VOID; map["RETURN"] = OpCode::RETURN; map["HALT"] = OpCode::HALT;
//     map["NEW_ARRAY"] = OpCode::NEW_ARRAY; map["NEW_HASH"] = OpCode::NEW_HASH; map["GET_INDEX"] = OpCode::GET_INDEX;
//     map["SET_INDEX"] = OpCode::SET_INDEX; map["GET_KEYS"] = OpCode::GET_KEYS; map["GET_VALUES"] = OpCode::GET_VALUES;
//     map["NEW_CLASS"] = OpCode::NEW_CLASS; map["NEW_INSTANCE"] = OpCode::NEW_INSTANCE; map["GET_PROP"] = OpCode::GET_PROP;
//     map["SET_PROP"] = OpCode::SET_PROP; map["SET_METHOD"] = OpCode::SET_METHOD; map["INHERIT"] = OpCode::INHERIT;
//     map["GET_SUPER"] = OpCode::GET_SUPER; map["BIT_AND"] = OpCode::BIT_AND; map["BIT_OR"] = OpCode::BIT_OR;
//     map["BIT_XOR"] = OpCode::BIT_XOR; map["BIT_NOT"] = OpCode::BIT_NOT; map["LSHIFT"] = OpCode::LSHIFT;
//     map["RSHIFT"] = OpCode::RSHIFT; map["THROW"] = OpCode::THROW; map["SETUP_TRY"] = OpCode::SETUP_TRY;
//     map["POP_TRY"] = OpCode::POP_TRY; map["IMPORT_MODULE"] = OpCode::IMPORT_MODULE; map["EXPORT"] = OpCode::EXPORT;
//     map["GET_EXPORT"] = OpCode::GET_EXPORT; map["IMPORT_ALL"] = OpCode::IMPORT_ALL;
//     return map;
// }();


// // --- Error Handling ---
// [[noreturn]] void TextParser::throw_parse_error(const std::string& message) {
//     throw_parse_error(message, current_token());
// }

// [[noreturn]] void TextParser::throw_parse_error(const std::string& message, const Token& token) {
//     std::string error_message = std::format("Lỗi phân tích cú pháp [{}:{}:{}]: {}",
//                                            current_source_name_, token.line, token.col, message);
//     if (!token.lexeme.empty() && token.type != TokenType::END_OF_FILE) {
//         error_message += std::format(" (gần '{}')", token.lexeme);
//     }
//     throw std::runtime_error(error_message);
// }

// // --- Token Handling ---
// const Token& TextParser::current_token() const {
//     if (current_token_index_ >= tokens_.size()) {
//          // Nên có token EOF ở cuối, không nên xảy ra
//          static Token eof_token = { "", TokenType::END_OF_FILE, 0, 0 }; // Giá trị mặc định
//           if (!tokens_.empty()) { // Lấy line/col từ token cuối cùng trước EOF
//               eof_token.line = tokens_.back().line;
//               eof_token.col = tokens_.back().col + tokens_.back().lexeme.length();
//           }
//          return eof_token;
//     }
//     return tokens_[current_token_index_];
// }

// const Token& TextParser::peek_token(size_t offset) const {
//     size_t index = current_token_index_ + offset;
//     if (index >= tokens_.size()) {
//         static Token eof_token = { "", TokenType::END_OF_FILE, 0, 0 }; // Giống trên
//          if (!tokens_.empty()) {
//              eof_token.line = tokens_.back().line;
//               eof_token.col = tokens_.back().col + tokens_.back().lexeme.length();
//          }
//         return eof_token;
//     }
//     return tokens_[index];
// }

// bool TextParser::is_at_end() const {
//     return current_token().type == TokenType::END_OF_FILE;
// }

// void TextParser::advance() {
//     if (!is_at_end()) {
//         current_token_index_++;
//     }
// }

// const Token& TextParser::consume_token(TokenType expected, const std::string& error_message) {
//     const Token& token = current_token();
//     if (token.type != expected) {
//         throw_parse_error(error_message, token);
//     }
//     advance();
//     return tokens_[current_token_index_ - 1]; // Trả về token đã consume
// }

// bool TextParser::match_token(TokenType type) {
//     if (current_token().type == type) {
//         advance();
//         return true;
//     }
//     return false;
// }

// // --- Main Parsing Logic ---

// proto_t TextParser::parse_file(const std::string& filepath, MemoryManager& mm) {
//     std::ifstream file(filepath);
//     if (!file.is_open()) {
//         throw std::runtime_error("Không thể mở tệp: " + filepath);
//     }
//     std::stringstream buffer;
//     buffer << file.rdbuf();
//     return parse_source(buffer.str(), mm, filepath);
// }

// proto_t TextParser::parse_source(const std::string& source, MemoryManager& mm, const std::string& source_name) {
//     memory_manager_ = &mm;
//     current_source_name_ = source_name;
//     current_token_index_ = 0;
//     current_proto_data_ = nullptr;
//     building_protos_.clear();
//     finalized_protos_.clear();

//     Lexer lexer(source);
//     tokens_ = lexer.tokenize_all();

//     if (!tokens_.empty() && tokens_.back().type == TokenType::UNKNOWN) {
//          throw_parse_error("Lỗi Lexer, ký tự không xác định hoặc chuỗi không đóng.", tokens_.back());
//      }


//     parse(); // Bắt đầu phân tích từ danh sách token

//     // Tạo các đối tượng ObjFunctionProto thực sự từ ProtoParseData
//      for (auto& [name, data] : building_protos_) {
//          // Di chuyển chunk và descriptions vào proto mới
//          finalized_protos_[name] = memory_manager_->new_proto(
//              data.num_registers,
//              data.num_upvalues,
//              memory_manager_->new_string(name),
//              std::move(data.chunk)
//              // Lưu ý: Constructor ObjFunctionProto đã được sửa để nhận vector descriptions
//              // Nếu không, bạn cần gọi add_upvalue_desc ở đây
//          );
//          // Nếu cần add_upvalue_desc:
//           auto* new_proto_obj = const_cast<ObjFunctionProto*>(finalized_protos_[name]);
//           for(size_t i = 0; i < data.upvalue_descs.size(); ++i) {
//                if (!new_proto_obj->add_upvalue_desc(i, data.upvalue_descs[i])) {
//                      throw std::runtime_error("Lỗi nội bộ: Không thể thêm mô tả upvalue cho proto " + name);
//                }
//           }
//      }


//     resolve_labels_for_proto(*building_protos_.find("@main")); // Chỉ resolve cho @main ban đầu? Hay resolve all? -> Resolve all
//      for(auto& [name, data] : building_protos_) {
//          resolve_labels_for_proto(data);
//      }

//     link_protos(); // Liên kết sau khi tất cả proto đã được tạo

//     auto main_it = finalized_protos_.find("@main");
//     if (main_it == finalized_protos_.end()) {
//         throw std::runtime_error("Không tìm thấy hàm chính '@main' trong " + current_source_name_);
//     }

//     // Đặt lại trạng thái
//     memory_manager_ = nullptr;
//     tokens_.clear();

//     return main_it->second;
// }

// std::unordered_map<std::string, proto_t> TextParser::get_protos() const {
//     return finalized_protos_;
// }

// void TextParser::parse() {
//     while (!is_at_end()) {
//         parse_statement();
//     }
//     if (current_proto_data_) {
//         // Lỗi: File kết thúc nhưng vẫn đang trong một .func
//         throw_parse_error("Thiếu chỉ thị '.endfunc' cho hàm '" + current_proto_data_->name + "'.");

//     }
// }

// void TextParser::parse_statement() {
//     const Token& token = current_token();
//     switch (token.type) {
//         case TokenType::DIR_FUNC:
//             parse_func_directive();
//             break;
//         case TokenType::DIR_ENDFUNC: // Chỉ nên gặp trong hàm parse_func_directive
//              throw_parse_error("Chỉ thị '.endfunc' không mong đợi.", token);
//              break; // Không nên đến đây

//          // Các directive khác chỉ hợp lệ bên trong .func
//          case TokenType::DIR_REGISTERS:
//          case TokenType::DIR_UPVALUES:
//          case TokenType::DIR_CONST:
//          case TokenType::DIR_UPVALUE:
//              if (!current_proto_data_) throw_parse_error("Chỉ thị phải nằm trong hàm (.func).", token);
//               if (token.type == TokenType::DIR_REGISTERS) parse_registers_directive();
//               else if (token.type == TokenType::DIR_UPVALUES) parse_upvalues_directive();
//               else if (token.type == TokenType::DIR_CONST) parse_const_directive();
//               else if (token.type == TokenType::DIR_UPVALUE) parse_upvalue_directive();
//              break;

//         case TokenType::LABEL_DEF:
//             if (!current_proto_data_) throw_parse_error("Nhãn phải nằm trong hàm (.func).", token);
//             parse_label_definition();
//             break;

//         case TokenType::OPCODE:
//             if (!current_proto_data_) throw_parse_error("Lệnh phải nằm trong hàm (.func).", token);
//              // Kiểm tra xem .registers và .upvalues đã được định nghĩa chưa
//              if (!current_proto_data_->registers_defined) {
//                  throw_parse_error("Chỉ thị '.registers' phải được định nghĩa trước lệnh đầu tiên.", token);
//              }
//              if (!current_proto_data_->upvalues_defined) {
//                  throw_parse_error("Chỉ thị '.upvalues' phải được định nghĩa trước lệnh đầu tiên.", token);
//              }
//             parse_instruction();
//             break;

//         // Identifier không có ':' theo sau và không phải directive/opcode là lỗi ở cấp độ này
//         case TokenType::IDENTIFIER:
//              throw_parse_error("Token không mong đợi.", token);
//              break;

//         // Các loại token khác cũng không hợp lệ ở đây
//         case TokenType::NUMBER_INT:
//         case TokenType::NUMBER_FLOAT:
//         case TokenType::STRING:
//         case TokenType::END_OF_FILE: // Không nên xảy ra trong vòng lặp chính
//         case TokenType::UNKNOWN:
//             throw_parse_error("Token không hợp lệ hoặc không mong đợi.", token);
//             break;
//     }
// }

// void TextParser::parse_func_directive() {
//     consume_token(TokenType::DIR_FUNC, "Mong đợi '.func'."); // Bỏ qua .func
//     if (current_proto_data_) {
//         throw_parse_error("Không thể định nghĩa hàm lồng nhau.");
//     }

//     const Token& name_token = consume_token(TokenType::IDENTIFIER, "Mong đợi tên hàm sau '.func'.");
//     std::string func_name(name_token.lexeme);

//     if (building_protos_.count(func_name)) {
//         throw_parse_error("Hàm đã được định nghĩa.", name_token);
//     }

//     // Khởi tạo ProtoParseData mới
//     current_proto_data_ = &building_protos_[func_name];
//     current_proto_data_->name = func_name;

//      // Yêu cầu .registers và .upvalues ngay sau .func
//      parse_registers_directive(); // Sẽ throw nếu không tìm thấy .registers
//      parse_upvalues_directive();  // Sẽ throw nếu không tìm thấy .upvalues


//     // Bây giờ xử lý các lệnh, nhãn, hằng số cho đến khi gặp .endfunc
//     while (!is_at_end() && current_token().type != TokenType::DIR_ENDFUNC) {
//         parse_statement(); // Phân tích nội dung hàm
//     }

//     consume_token(TokenType::DIR_ENDFUNC, "Mong đợi '.endfunc' để kết thúc hàm '" + func_name + "'.");
//     current_proto_data_ = nullptr; // Kết thúc xử lý hàm hiện tại
// }

// void TextParser::parse_registers_directive() {
//      consume_token(TokenType::DIR_REGISTERS, "Mong đợi '.registers' sau tên hàm.");
//      if (!current_proto_data_) throw_parse_error("Lỗi nội bộ: current_proto_data_ là null."); // Không nên xảy ra
//      if (current_proto_data_->registers_defined) throw_parse_error("Chỉ thị '.registers' đã được định nghĩa.");

//      const Token& num_token = consume_token(TokenType::NUMBER_INT, "Mong đợi số lượng thanh ghi sau '.registers'.");
//      try {
//           // Dùng from_chars an toàn hơn
//           uint64_t num_reg; // Dùng kiểu lớn hơn để kiểm tra tràn số
//           auto result = std::from_chars(num_token.lexeme.data(), num_token.lexeme.data() + num_token.lexeme.size(), num_reg);
//           if (result.ec != std::errc() || result.ptr != num_token.lexeme.data() + num_token.lexeme.size() || num_reg > std::numeric_limits<size_t>::max()) {
//               throw std::out_of_range("Invalid register count");
//           }
//          current_proto_data_->num_registers = static_cast<size_t>(num_reg);
//          current_proto_data_->registers_defined = true;
//      } catch (...) {
//          throw_parse_error("Số lượng thanh ghi không hợp lệ.", num_token);
//      }
//  }

//  void TextParser::parse_upvalues_directive() {
//      consume_token(TokenType::DIR_UPVALUES, "Mong đợi '.upvalues' sau '.registers'.");
//      if (!current_proto_data_) throw_parse_error("Lỗi nội bộ: current_proto_data_ là null."); // Không nên xảy ra
//      if (current_proto_data_->upvalues_defined) throw_parse_error("Chỉ thị '.upvalues' đã được định nghĩa.");

//      const Token& num_token = consume_token(TokenType::NUMBER_INT, "Mong đợi số lượng upvalue sau '.upvalues'.");
//      try {
//           uint64_t num_up;
//           auto result = std::from_chars(num_token.lexeme.data(), num_token.lexeme.data() + num_token.lexeme.size(), num_up);
//           if (result.ec != std::errc() || result.ptr != num_token.lexeme.data() + num_token.lexeme.size() || num_up > std::numeric_limits<size_t>::max()) {
//               throw std::out_of_range("Invalid upvalue count");
//           }
//          current_proto_data_->num_upvalues = static_cast<size_t>(num_up);
//           current_proto_data_->upvalue_descs.resize(current_proto_data_->num_upvalues); // Chuẩn bị sẵn vector
//          current_proto_data_->upvalues_defined = true;
//      } catch (...) {
//          throw_parse_error("Số lượng upvalue không hợp lệ.", num_token);
//      }
//  }


// void TextParser::parse_const_directive() {
//     consume_token(TokenType::DIR_CONST, "Mong đợi '.const'.");
//     if (!current_proto_data_) throw_parse_error("Lỗi nội bộ: current_proto_data_ là null.");

//     Value const_val = parse_const_value_from_tokens();
//     current_proto_data_->chunk.add_constant(const_val);
// }

// void TextParser::parse_upvalue_directive() {
//      consume_token(TokenType::DIR_UPVALUE, "Mong đợi '.upvalue'.");
//      if (!current_proto_data_) throw_parse_error("Lỗi nội bộ: current_proto_data_ là null.");

//      // Parse index (u16)
//      const Token& index_token = consume_token(TokenType::NUMBER_INT, "Mong đợi chỉ số upvalue (0-based).");
//      size_t uv_index;
//      try {
//           uint64_t temp_idx;
//           auto result = std::from_chars(index_token.lexeme.data(), index_token.lexeme.data() + index_token.lexeme.size(), temp_idx);
//           if (result.ec != std::errc() || result.ptr != index_token.lexeme.data() + index_token.lexeme.size() || temp_idx >= current_proto_data_->num_upvalues) {
//               throw std::out_of_range("Invalid upvalue index");
//           }
//          uv_index = static_cast<size_t>(temp_idx);
//      } catch (...) {
//          throw_parse_error("Chỉ số upvalue không hợp lệ hoặc vượt quá số lượng đã khai báo.", index_token);
//      }

//      // Parse type (identifier: local / parent_upvalue)
//      const Token& type_token = consume_token(TokenType::IDENTIFIER, "Mong đợi loại upvalue ('local' hoặc 'parent_upvalue').");
//      bool is_local;
//      if (type_token.lexeme == "local") {
//          is_local = true;
//      } else if (type_token.lexeme == "parent_upvalue") {
//          is_local = false;
//      } else {
//          throw_parse_error("Loại upvalue không hợp lệ.", type_token);
//      }

//      // Parse slot index (u16)
//      const Token& slot_token = consume_token(TokenType::NUMBER_INT, "Mong đợi chỉ số slot (thanh ghi hoặc upvalue cha).");
//      size_t slot_index;
//       try {
//            uint64_t temp_slot;
//            auto result = std::from_chars(slot_token.lexeme.data(), slot_token.lexeme.data() + slot_token.lexeme.size(), temp_slot);
//            if (result.ec != std::errc() || result.ptr != slot_token.lexeme.data() + slot_token.lexeme.size() || temp_slot > std::numeric_limits<size_t>::max()) {
//                throw std::out_of_range("Invalid slot index");
//            }
//           slot_index = static_cast<size_t>(temp_slot);
//            // Thêm kiểm tra logic nếu cần (ví dụ: slot_index < num_registers nếu is_local)
//       } catch (...) {
//           throw_parse_error("Chỉ số slot không hợp lệ.", slot_token);
//       }


//      // Lưu mô tả vào ProtoParseData
//       if (uv_index >= current_proto_data_->upvalue_descs.size()) {
//            // Điều này không nên xảy ra nếu resize ở parse_upvalues_directive
//            throw_parse_error("Lỗi nội bộ: Vector mô tả upvalue có kích thước không đúng.", index_token);
//       }
//       current_proto_data_->upvalue_descs[uv_index] = UpvalueDesc(is_local, slot_index);
//  }


// void TextParser::parse_label_definition() {
//     const Token& label_token = consume_token(TokenType::LABEL_DEF, "Mong đợi định nghĩa nhãn.");
//     if (!current_proto_data_) throw_parse_error("Lỗi nội bộ: current_proto_data_ là null.");

//     std::string_view label_name = label_token.lexeme;
//     if (current_proto_data_->labels.count(label_name)) {
//         throw_parse_error("Nhãn đã được định nghĩa.", label_token);
//     }
//     current_proto_data_->labels[label_name] = current_proto_data_->chunk.get_code_size();
// }

// // --- Phần này gần giống với code cũ, nhưng dùng token và Chunk mới ---
// void TextParser::parse_instruction() {
//     const Token& opcode_token = consume_token(TokenType::OPCODE, "Mong đợi opcode.");
//     if (!current_proto_data_) throw_parse_error("Lỗi nội bộ: current_proto_data_ là null.");

//     auto it = OPCODE_PARSE_MAP.find(opcode_token.lexeme);
//     // Lexer đã đảm bảo đây là OPCODE hợp lệ, không cần kiểm tra lại it == end()

//     OpCode opcode = it->second;
//     Chunk& chunk_ref = current_proto_data_->chunk;
//     size_t instruction_start_pos = chunk_ref.get_code_size(); // Vị trí bắt đầu instruction (cho pending jump)

//     chunk_ref.write_byte(static_cast<uint8_t>(opcode));

//     // --- Hàm tiện ích cho parsing đối số từ token ---
//     auto parse_u16_arg = [&]() -> uint16_t {
//         const Token& token = consume_token(TokenType::NUMBER_INT, "Mong đợi số nguyên 16-bit.");
//         uint16_t value;
//         auto result = std::from_chars(token.lexeme.data(), token.lexeme.data() + token.lexeme.size(), value);
//         if (result.ec != std::errc() || result.ptr != token.lexeme.data() + token.lexeme.size()) {
//             throw_parse_error("Đối số phải là số nguyên 16-bit không dấu.", token);
//         }
//         return value;
//     };
//     auto parse_i64_arg = [&]() -> int64_t {
//          const Token& token = current_token(); // Không consume ngay, có thể là int hoặc float
//          int64_t value;
//          auto result = std::from_chars(token.lexeme.data(), token.lexeme.data() + token.lexeme.size(), value);
//          if (result.ec == std::errc() && result.ptr == token.lexeme.data() + token.lexeme.size()) {
//              advance(); // Consume token nếu là int
//              return value;
//          } else if (token.type == TokenType::NUMBER_FLOAT && opcode == OpCode::LOAD_INT) {
//              // Cho phép float cho LOAD_INT
//              advance();
//              try {
//                  return static_cast<int64_t>(std::stod(std::string(token.lexeme)));
//              } catch(...) {
//                  throw_parse_error("Không thể chuyển đổi số thực thành số nguyên 64-bit.", token);
//              }
//          }
//          throw_parse_error("Mong đợi số nguyên 64-bit.", token);
//      };
//     auto parse_f64_arg = [&]() -> double {
//         const Token& token = consume_token(TokenType::NUMBER_FLOAT, "Mong đợi số thực 64-bit.");
//         try {
//              size_t processed = 0;
//              double value = std::stod(std::string(token.lexeme), &processed); // stod cần std::string
//              if (processed != token.lexeme.size()) {
//                  throw std::invalid_argument("Invalid double format");
//              }
//              return value;
//          } catch (...) {
//              throw_parse_error("Đối số phải là số thực (double).", token);
//          }
//     };
//     auto parse_address_or_label_arg = [&]() {
//         const Token& token = current_token();
//         if (token.type == TokenType::NUMBER_INT) {
//             uint16_t address = parse_u16_arg(); // consume luôn
//             chunk_ref.write_u16(address);
//         } else if (token.type == TokenType::IDENTIFIER) {
//             advance(); // consume identifier (label name)
//             size_t patch_pos = chunk_ref.get_code_size(); // Vị trí cần vá
//             chunk_ref.write_u16(0); // Placeholder
//             current_proto_data_->pending_jumps.emplace_back(instruction_start_pos, patch_pos, token.lexeme);
//         } else {
//             throw_parse_error("Mong đợi nhãn (identifier) hoặc địa chỉ (số nguyên 16-bit).", token);
//         }
//     };

//     // --- Xử lý đối số dựa trên opcode ---
//      switch (opcode) {
//          // --- dst: u16 ---
//          case OpCode::LOAD_NULL: case OpCode::LOAD_TRUE: case OpCode::LOAD_FALSE:
//          case OpCode::POP_TRY: case OpCode::IMPORT_ALL:
//               chunk_ref.write_u16(parse_u16_arg()); break;

//          // --- dst: u16, constant_index: u16 ---
//          case OpCode::LOAD_CONST: case OpCode::GET_GLOBAL: case OpCode::NEW_CLASS:
//          case OpCode::GET_SUPER: case OpCode::CLOSURE: case OpCode::EXPORT:
//          case OpCode::IMPORT_MODULE:
//               chunk_ref.write_u16(parse_u16_arg()); chunk_ref.write_u16(parse_u16_arg()); break;

//          // --- dst: u16, src: u16 ---
//          case OpCode::MOVE: case OpCode::NEG: case OpCode::NOT: case OpCode::BIT_NOT:
//          case OpCode::GET_UPVALUE: case OpCode::SET_UPVALUE: case OpCode::CLOSE_UPVALUES:
//          case OpCode::NEW_INSTANCE: case OpCode::GET_KEYS: case OpCode::GET_VALUES:
//          case OpCode::INHERIT: case OpCode::THROW:
//               chunk_ref.write_u16(parse_u16_arg()); chunk_ref.write_u16(parse_u16_arg()); break;

//          // --- dst: u16, value: i64 ---
//          case OpCode::LOAD_INT:
//               chunk_ref.write_u16(parse_u16_arg());
//               chunk_ref.write_u64(std::bit_cast<uint64_t>(parse_i64_arg()));
//               break;

//          // --- dst: u16, value: f64 ---
//          case OpCode::LOAD_FLOAT:
//               chunk_ref.write_u16(parse_u16_arg()); chunk_ref.write_f64(parse_f64_arg()); break;

//          // --- dst: u16, r1: u16, r2: u16 ---
//          case OpCode::ADD: case OpCode::SUB: case OpCode::MUL: case OpCode::DIV:
//          case OpCode::MOD: case OpCode::POW: case OpCode::EQ: case OpCode::NEQ:
//          case OpCode::GT: case OpCode::GE: case OpCode::LT: case OpCode::LE:
//          case OpCode::BIT_AND: case OpCode::BIT_OR: case OpCode::BIT_XOR:
//          case OpCode::LSHIFT: case OpCode::RSHIFT:
//          case OpCode::GET_INDEX: case OpCode::NEW_ARRAY: case OpCode::NEW_HASH:
//          case OpCode::GET_PROP: case OpCode::SET_METHOD: case OpCode::GET_EXPORT:
//               chunk_ref.write_u16(parse_u16_arg()); chunk_ref.write_u16(parse_u16_arg()); chunk_ref.write_u16(parse_u16_arg()); break;

//          // --- src: u16, key: u16, value: u16 ---
//          case OpCode::SET_INDEX: case OpCode::SET_PROP:
//               chunk_ref.write_u16(parse_u16_arg()); chunk_ref.write_u16(parse_u16_arg()); chunk_ref.write_u16(parse_u16_arg()); break;

//          // --- target: address/label ---
//          case OpCode::JUMP: case OpCode::SETUP_TRY:
//               parse_address_or_label_arg(); break;

//          // --- reg: u16, target: address/label ---
//          case OpCode::JUMP_IF_FALSE: case OpCode::JUMP_IF_TRUE:
//               chunk_ref.write_u16(parse_u16_arg()); parse_address_or_label_arg(); break;

//           // --- CALL dst: u16, fn_reg: u16, arg_start: u16, argc: u16 ---
//           case OpCode::CALL:
//                chunk_ref.write_u16(parse_u16_arg()); chunk_ref.write_u16(parse_u16_arg());
//                chunk_ref.write_u16(parse_u16_arg()); chunk_ref.write_u16(parse_u16_arg());
//                break;

//           // --- CALL_VOID fn_reg: u16, arg_start: u16, argc: u16 ---
//           case OpCode::CALL_VOID:
//                chunk_ref.write_u16(parse_u16_arg()); chunk_ref.write_u16(parse_u16_arg()); chunk_ref.write_u16(parse_u16_arg());
//                break;


//          // --- ret_reg: u16 (có thể là FFFF) ---
//          case OpCode::RETURN: {
//              const Token& ret_token = current_token();
//              if (ret_token.type == TokenType::IDENTIFIER && (ret_token.lexeme == "FFFF" || ret_token.lexeme == "ffff")) {
//                  advance();
//                  chunk_ref.write_u16(0xFFFF);
//              } else if (ret_token.type == TokenType::NUMBER_INT && ret_token.lexeme == "-1") {
//                   advance();
//                   chunk_ref.write_u16(0xFFFF);
//              } else {
//                  chunk_ref.write_u16(parse_u16_arg());
//              }
//              break;
//          }
//          // --- Không có đối số ---
//          case OpCode::HALT:
//              // Không làm gì thêm
//              break;

//          default:
//              throw_parse_error("Opcode chưa được hỗ trợ xử lý đối số.", opcode_token);
//      }

// }

// Value TextParser::parse_const_value_from_tokens() {
//     // Hàm này xử lý trường hợp hằng số có thể gồm nhiều token (ví dụ: số âm, hoặc tên proto có khoảng trắng?)
//     // Hiện tại, giả định hằng số chỉ là MỘT token (NUMBER_INT, NUMBER_FLOAT, STRING, IDENTIFIER cho bool/null/@proto)
//     const Token& token = current_token();

//     if (token.type == TokenType::STRING) {
//         advance();
//         // Bỏ dấu "" ở đầu và cuối lexeme
//         if (token.lexeme.length() < 2) throw_parse_error("Chuỗi hằng số không hợp lệ.", token);
//         std::string_view inner = token.lexeme.substr(1, token.lexeme.length() - 2);
//          // Cần tạo std::string vì unescape có thể thay đổi độ dài
//          std::string unescaped = TextParser::unescape_string(std::string(inner));
//         return Value(memory_manager_->new_string(unescaped));
//     } else if (token.type == TokenType::NUMBER_INT) {
//         advance();
//         int64_t value;
//         auto result = std::from_chars(token.lexeme.data(), token.lexeme.data() + token.lexeme.size(), value);
//         if (result.ec != std::errc() || result.ptr != token.lexeme.data() + token.lexeme.size()) {
//              throw_parse_error("Số nguyên hằng số không hợp lệ.", token); // Không nên xảy ra nếu lexer đúng
//         }
//         return Value(value);
//     } else if (token.type == TokenType::NUMBER_FLOAT) {
//         advance();
//          try {
//              size_t processed = 0;
//              double value = std::stod(std::string(token.lexeme), &processed);
//              if (processed != token.lexeme.size()) throw std::invalid_argument("Invalid double");
//              return Value(value);
//          } catch (...) {
//              throw_parse_error("Số thực hằng số không hợp lệ.", token); // Không nên xảy ra nếu lexer đúng
//          }
//     } else if (token.type == TokenType::IDENTIFIER) {
//         advance();
//         if (token.lexeme == "true") return Value(true);
//         if (token.lexeme == "false") return Value(false);
//         if (token.lexeme == "null") return Value(null_t{});
//         // Tham chiếu Proto
//         if (!token.lexeme.empty() && token.lexeme.front() == '@') {
//              std::string proto_name(token.lexeme.substr(1));
//              return Value(memory_manager_->new_string("::function_proto::" + proto_name));
//         }
//         throw_parse_error("Identifier không hợp lệ cho hằng số (chỉ chấp nhận true, false, null, @ProtoName).", token);
//     } else {
//         throw_parse_error("Token không mong đợi cho giá trị hằng số.", token);
//     }
// }

// // --- Linking and Resolving ---

// void TextParser::resolve_labels_for_proto(ProtoParseData& data) {
//     uint8_t* code_buffer = data.chunk.get_mutable_code(); // Cần hàm này trong Chunk

//     for (const auto& jump_info : data.pending_jumps) {
//         [[maybe_unused]] size_t instruction_pos = std::get<0>(jump_info); // Vị trí opcode
//         size_t patch_offset = std::get<1>(jump_info);         // Vị trí byte đầu tiên của u16 cần vá
//         std::string_view label_name = std::get<2>(jump_info);

//         auto label_it = data.labels.find(label_name);
//         if (label_it == data.labels.end()) {
//              // Dùng line/col của jump instruction nếu có thể, tạm thời dùng dòng 0
//              Token jump_token = { label_name, TokenType::IDENTIFIER, 0, 0 }; // Token giả
//              throw_parse_error("Không tìm thấy nhãn '" + std::string(label_name) + "' trong hàm '" + data.name + "'.", jump_token);
//         }

//         size_t target_address = label_it->second;
//         if (target_address > UINT16_MAX) {
//              Token jump_token = { label_name, TokenType::IDENTIFIER, 0, 0 }; // Token giả
//              throw_parse_error("Địa chỉ nhãn '" + std::string(label_name) + "' vượt quá giới hạn 16-bit.", jump_token);
//         }

//         // Ghi đè 2 byte tại vị trí patch_pos (little-endian)
//          if (!data.chunk.patch_u16(patch_offset, static_cast<uint16_t>(target_address))) {
//              Token jump_token = { label_name, TokenType::IDENTIFIER, 0, 0 }; // Token giả
//              throw_parse_error("Lỗi nội bộ: Không thể vá địa chỉ nhảy cho nhãn '" + std::string(label_name) + "'. Offset không hợp lệ?", jump_token);
//          }
//     }
//     data.pending_jumps.clear(); // Xóa sau khi vá xong
// }


// void TextParser::link_protos() {
//     const std::string prefix = "::function_proto::";
//     for (auto& [name, proto] : finalized_protos_) {
//         // Cần truy cập vào constant pool của proto đã tạo
//         auto* proto_obj = const_cast<ObjFunctionProto*>(proto);
//         Chunk& chunk_ref = const_cast<Chunk&>(proto_obj->get_chunk());
//         std::vector<Value>& constant_pool = chunk_ref.get_mutable_constant_pool(); // Cần hàm này trong Chunk

//         for (Value& constant : constant_pool) {
//             if (constant.is_string()) {
//                 const objects::ObjString* str_obj = constant.as_string();
//                 std::string_view s = str_obj->c_str();

//                 if (s.size() > prefix.size() && s.substr(0, prefix.size()) == prefix) {
//                     std::string proto_name = std::string(s.substr(prefix.size()));
//                     auto target_proto_it = finalized_protos_.find(proto_name);
//                     if (target_proto_it == finalized_protos_.end()) {
//                          // Lỗi: proto được tham chiếu không tồn tại
//                          Token ref_token = { s, TokenType::STRING, 0, 0 }; // Token giả
//                          throw_parse_error("Không tìm thấy proto '" + proto_name + "' được tham chiếu trong hằng số của hàm '" + name + "'.", ref_token);
//                     }
//                     // Thay thế hằng số chuỗi bằng con trỏ proto thực sự
//                     constant = Value(target_proto_it->second);
//                 }
//             }
//         }
//     }
// }


// } // namespace meow::loader