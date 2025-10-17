// --- Old codes ---

// #pragma once

// #include "common/pch.h"
// #include "core/definitions.h"
// #include "common/expected.h"
// #include "diagnostics/diagnostic.h"

// class MemoryManager;

// class BytecodeParser {
// public:
//     BytecodeParser() = default;

//     Expected<bool, Diagnostic> parseFile(const std::string& filepath, MemoryManager& mm);

//     std::unordered_map<std::string, proto_t> protos;

// private:
//     MemoryManager* memoryManager = nullptr;
//     std::string current_source_name_;
//     size_t current_line_ = 0;
//     size_t current_col_ = 0;
//     proto_t currentProto = nullptr;

//     Diagnostic make_diagnostic(ParseError code, size_t col = 0, std::vector<std::string> args = {}) const;
//     Diagnostic make_diagnostic(ParseWarning code, size_t col = 0, std::vector<std::string> args = {}) const;
//     Diagnostic make_diagnostic(RuntimeError code, size_t col = 0, std::vector<std::string> args = {}) const;
//     Diagnostic make_diagnostic(SystemError code, size_t col = 0, std::vector<std::string> args = {}) const;

//     Expected<bool, Diagnostic> parseSource(const std::string& source, const std::string& sourceName = "<string>");

//     Expected<bool, Diagnostic> parseLine(const std::string& line);

//     Expected<bool, Diagnostic> parseDirective(const std::vector<std::pair<std::string, size_t>>& tokens);

//     Value parseConstValue(const std::string& token);

//     std::vector<std::pair<std::string, size_t>> tokenize(const std::string& line);

//     Expected<bool, Diagnostic> resolveAllLabels();
//     Expected<bool, Diagnostic> linkProtos();

//     static inline void skipComment(std::string& line);

//     struct LocalProtoData {
//         std::unordered_map<std::string, size_t> labels;
//         std::vector<std::tuple<size_t, size_t, std::string>> pendingJumps;
//     };
//     std::unordered_map<proto_t, LocalProtoData> localProtoData;
// };

// #include "loader/bytecode_parser.h"
// #include "memory/memory_manager.h"
// #include "common/pch.h"
// #include "core/op_codes.h"
// #include "common/endian_helpers.h"

// /**
//  * @brief Removes spaces at the beginning and end of the string
//  * @param[in] str The string to trim
//  * @return The processed string
//  */
// static std::string trim(const std::string& str) {
//     size_t start = str.find_first_not_of(" \t\r\n");
//     if (start == std::string::npos) return "";
//     size_t end = str.find_last_not_of(" \t\r\n");
//     return str.substr(start, end - start + 1);
// }

// static std::string toUpper(const std::string& s) {
//     std::string res = s;
//     std::transform(res.begin(), res.end(), res.begin(), ::toupper);
//     return res;
// }

// inline void BytecodeParser::skipComment(std::string& line) {
//     bool inString = false;
//     for (size_t i = 0; i < line.size(); ++i) {
//         char c = line[i];
//         if (c == '"' ) {
//             bool escaped = (i > 0 && line[i - 1] == '\\');
//             if (!escaped) inString = !inString;
//         }
//         if (!inString && c == '#') {
//             line = line.substr(0, i);
//             break;
//         }
//     }
//     line = trim(line);
// }

// Diagnostic BytecodeParser::make_diagnostic(ParseError code, size_t col, std::vector<std::string> args) const {
//     Diagnostic d(code, current_source_name_.empty() ? std::string("[unknown file]") : current_source_name_, current_line_, col, std::move(args));
//     d.level = Level::Error;
//     return d;
// }
// Diagnostic BytecodeParser::make_diagnostic(ParseWarning code, size_t col, std::vector<std::string> args) const {
//     Diagnostic d(code, current_source_name_.empty() ? std::string("[unknown file]") : current_source_name_, current_line_, col, std::move(args));
//     d.level = Level::Warning;
//     return d;
// }
// Diagnostic BytecodeParser::make_diagnostic(RuntimeError code, size_t col, std::vector<std::string> args) const {
//     Diagnostic d(code, current_source_name_.empty() ? std::string("[unknown file]") : current_source_name_, current_line_, col, std::move(args));
//     d.level = Level::Error;
//     return d;
// }
// Diagnostic BytecodeParser::make_diagnostic(SystemError code, size_t col, std::vector<std::string> args) const {
//     Diagnostic d(code, current_source_name_.empty() ? std::string("[unknown file]") : current_source_name_, current_line_, col, std::move(args));
//     d.level = Level::Error;
//     return d;
// }

// Expected<bool, Diagnostic> BytecodeParser::parseFile(const std::string& filepath, MemoryManager& mm) {
//     this->memoryManager = &mm;
//     std::ifstream ifs(filepath);
//     if (!ifs) {
//         Diagnostic d(SystemError::GetExecutablePathFailed, filepath, std::vector<std::string>{ filepath });
//         d.level = Level::Error;
//         this->memoryManager = nullptr;
//         return Expected<bool, Diagnostic>(d);
//     }
//     std::ostringstream ss;
//     ss << ifs.rdbuf();
//     auto res = parseSource(ss.str(), filepath);
//     this->memoryManager = nullptr;
//     if (!res) return Expected<bool, Diagnostic>(res.error());
//     return Expected<bool, Diagnostic>(true);
// }

// std::vector<std::pair<std::string, size_t>> BytecodeParser::tokenize(const std::string& line) {
//     std::vector<std::pair<std::string, size_t>> out;
//     size_t i = 0;
//     size_t n = line.size();
//     while (i < n) {
//         while (i < n && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r')) ++i;
//         if (i >= n) break;
//         size_t start = i;
//         std::string token;
//         if (line[i] == '"') {
//             token += line[i++];
//             bool escaping = false;
//             while (i < n) {
//                 char c = line[i++];
//                 token += c;
//                 if (escaping) {
//                     escaping = false;
//                 } else if (c == '\\') {
//                     escaping = true;
//                 } else if (c == '"') {
//                     break;
//                 }
//             }
//         } else {
//             while (i < n && !(line[i] == ' ' || line[i] == '\t' || line[i] == '\r')) {
//                 token += line[i++];
//             }
//         }
//         out.emplace_back(token, start + 1);
//     }
//     return out;
// }

// Expected<bool, Diagnostic> BytecodeParser::parseSource(const std::string& source, const std::string& sourceName) {
//     protos.clear();
//     localProtoData.clear();
//     currentProto = nullptr;

//     current_source_name_ = sourceName;
//     current_line_ = 0;
//     current_col_ = 0;

//     std::istringstream iss(source);
//     std::string line;
//     size_t lineno = 0;
//     while (std::getline(iss, line)) {
//         ++lineno;
//         current_line_ = lineno;
//         skipComment(line);
//         if (line.empty()) continue;
//         auto pline = parseLine(line);
//         if (!pline) return pline;
//     }

//     if (currentProto) {
//         Diagnostic d(ParseError::MissingEndFunc, current_source_name_, lineno, 0, {});
//         return Expected<bool, Diagnostic>(d);
//     }

//     auto r1 = resolveAllLabels();
//     if (!r1) return Expected<bool, Diagnostic>(r1.error());

//     auto r2 = linkProtos();
//     if (!r2) return Expected<bool, Diagnostic>(r2.error());

//     return Expected<bool, Diagnostic>(true);
// }

// Expected<bool, Diagnostic> BytecodeParser::parseLine(const std::string& line) {
//     auto tokens = tokenize(line);
//     if (tokens.empty()) return Expected<bool, Diagnostic>(true);

//     const auto& firstTok = tokens[0];
//     std::string t0 = firstTok.first;
//     current_col_ = firstTok.second;

//     if (!t0.empty() && t0[0] == '.') {
//         return parseDirective(tokens);
//     }
    
//     if (!t0.empty() && t0.back() == ':') {
//         if (!currentProto) {
//             return Expected<bool, Diagnostic>(make_diagnostic(ParseError::LabelOutsideFunction, current_col_, {}));
//         }
//         std::string labelName = t0.substr(0, t0.length() - 1);
//         auto& ldata = localProtoData[currentProto];
//         if (ldata.labels.count(labelName)) {
//             return Expected<bool, Diagnostic>(make_diagnostic(ParseError::DuplicateLabel, current_col_, {labelName}));
//         }
//         ldata.labels[labelName] = currentProto->getChunk().getCodeSize();
//         tokens.erase(tokens.begin());
//         if (tokens.empty()) {
//             return Expected<bool, Diagnostic>(true);
//         }
//         t0 = tokens[0].first;
//     }

//     if (!currentProto) {
//         return Expected<bool, Diagnostic>(make_diagnostic(ParseError::InstructionOutsideFunction, current_col_, {}));
//     }

//     static const std::unordered_map<std::string, OpCode> OPC = {
//         {"LOAD_CONST", OpCode::LOAD_CONST}, {"LOAD_NULL", OpCode::LOAD_NULL}, {"LOAD_TRUE", OpCode::LOAD_TRUE},
//         {"LOAD_FALSE", OpCode::LOAD_FALSE}, {"LOAD_INT", OpCode::LOAD_INT}, {"MOVE", OpCode::MOVE},
//         {"ADD", OpCode::ADD}, {"SUB", OpCode::SUB}, {"MUL", OpCode::MUL}, {"DIV", OpCode::DIV},
//         {"MOD", OpCode::MOD}, {"POW", OpCode::POW}, {"EQ", OpCode::EQ}, {"NEQ", OpCode::NEQ},
//         {"GT", OpCode::GT}, {"GE", OpCode::GE}, {"LT", OpCode::LT}, {"LE", OpCode::LE},
//         {"NEG", OpCode::NEG}, {"NOT", OpCode::NOT}, {"GET_GLOBAL", OpCode::GET_GLOBAL},
//         {"SET_GLOBAL", OpCode::SET_GLOBAL}, {"GET_UPVALUE", OpCode::GET_UPVALUE}, {"SET_UPVALUE", OpCode::SET_UPVALUE},
//         {"CLOSURE", OpCode::CLOSURE}, {"CLOSE_UPVALUES", OpCode::CLOSE_UPVALUES}, {"JUMP", OpCode::JUMP},
//         {"JUMP_IF_FALSE", OpCode::JUMP_IF_FALSE}, {"JUMP_IF_TRUE", OpCode::JUMP_IF_TRUE}, {"CALL", OpCode::CALL}, {"RETURN", OpCode::RETURN},
//         {"HALT", OpCode::HALT}, {"NEW_ARRAY", OpCode::NEW_ARRAY}, {"NEW_HASH", OpCode::NEW_HASH},
//         {"GET_INDEX", OpCode::GET_INDEX}, {"SET_INDEX", OpCode::SET_INDEX}, {"GET_KEYS", OpCode::GET_KEYS}, {"GET_VALUES", OpCode::GET_VALUES}, {"NEW_CLASS", OpCode::NEW_CLASS},
//         {"NEW_INSTANCE", OpCode::NEW_INSTANCE}, {"GET_PROP", OpCode::GET_PROP}, {"SET_PROP", OpCode::SET_PROP},
//         {"SET_METHOD", OpCode::SET_METHOD}, {"INHERIT", OpCode::INHERIT}, {"GET_SUPER", OpCode::GET_SUPER}, {"BIT_AND", OpCode::BIT_AND},
//         {"BIT_OR", OpCode::BIT_OR}, {"BIT_XOR", OpCode::BIT_XOR}, {"BIT_NOT", OpCode::BIT_NOT},
//         {"LSHIFT", OpCode::LSHIFT}, {"RSHIFT", OpCode::RSHIFT}, {"THROW", OpCode::THROW},
//         {"SETUP_TRY", OpCode::SETUP_TRY}, {"POP_TRY", OpCode::POP_TRY}, {"IMPORT_MODULE", OpCode::IMPORT_MODULE},
//         {"EXPORT", OpCode::EXPORT}, {"GET_EXPORT", OpCode::GET_EXPORT}, {"GET_MODULE_EXPORT", OpCode::GET_MODULE_EXPORT}, {"IMPORT_ALL", OpCode::IMPORT_ALL}
//     };

//     std::string upperCmd = toUpper(t0);
//     auto it = OPC.find(upperCmd);
//     if (it == OPC.end()) {
//         return Expected<bool, Diagnostic>(make_diagnostic(ParseError::UnknownOpcode, current_col_, { t0 }));
//     }
//     OpCode op = it->second;
//     Chunk& chunk = currentProto->getChunk();
//     auto &ldata = localProtoData[currentProto];

//     auto parseIntToken = [&](size_t idx, bool& ok) -> uint16_t {
//         ok = false;
//         if (idx >= tokens.size()) return 0;
//         try {
//             long long val = std::stoll(tokens[idx].first);
//             if (val < 0 || val > UINT16_MAX) return 0;
//             ok = true;
//             return static_cast<uint16_t>(val);
//         } catch (...) {
//             return 0;
//         }
//     };

//     if (op == OpCode::CALL) {
//         if (tokens.size() < 5) { // CALL dst, func, start, argc
//              return Expected<bool, Diagnostic>(make_diagnostic(ParseError::IncorrectArgumentCount, current_col_, { t0, "4" }));
//         }
//         bool retOk;
//         uint16_t retReg = parseIntToken(1, retOk);
        
//         if (retOk && retReg == 0xFFFF) {
//             chunk.writeByte(static_cast<uint8_t>(OpCode::CALL_VOID), current_line_);
//             for (size_t i = 2; i < tokens.size(); ++i) {
//                 bool ok;
//                 uint16_t v = parseIntToken(i, ok);
//                 if (!ok) return Expected<bool, Diagnostic>(make_diagnostic(ParseError::InvalidArgumentType, tokens[i].second, { tokens[i].first, t0 }));
//                 chunk.writeShort(v, current_line_);
//             }
//             return Expected<bool, Diagnostic>(true);
//         }
//     }
    
//     chunk.writeByte(static_cast<uint8_t>(op), current_line_);

//     bool ok;
//     if (op == OpCode::JUMP || op == OpCode::SETUP_TRY) {
//         if (tokens.size() < 2) return Expected<bool, Diagnostic>(make_diagnostic(ParseError::IncorrectArgumentCount, current_col_, { t0, "1" }));
//         uint16_t val = parseIntToken(1, ok);
//         if (ok) {
//             chunk.writeAddress(val, current_line_);
//         } else {
//             ldata.pendingJumps.emplace_back(chunk.getCodeSize(), 0, tokens[1].first);
//             chunk.writeAddress(0, current_line_); // placeholder 2 bytes
//         }
//     } else if (op == OpCode::JUMP_IF_FALSE || op == OpCode::JUMP_IF_TRUE) {
//         if (tokens.size() < 3) return Expected<bool, Diagnostic>(make_diagnostic(ParseError::IncorrectArgumentCount, current_col_, { t0, "2" }));
//         uint16_t v1 = parseIntToken(1, ok);
//         if (!ok) return Expected<bool, Diagnostic>(make_diagnostic(ParseError::InvalidArgumentType, tokens[1].second, { tokens[1].first, t0 }));
//         chunk.writeShort(v1, current_line_);
//         uint16_t v2 = parseIntToken(2, ok);
//         if (ok) {
//             chunk.writeAddress(v2, current_line_);
//         } else {
//             ldata.pendingJumps.emplace_back(chunk.getCodeSize(), 0, tokens[2].first);
//             chunk.writeAddress(0, current_line_);
//         }
//     } else {
//         for (size_t i = 1; i < tokens.size(); ++i) {
//             uint16_t v = parseIntToken(i, ok);
//             if (!ok) return Expected<bool, Diagnostic>(make_diagnostic(ParseError::InvalidArgumentType, tokens[i].second, { tokens[i].first, t0 }));
//             chunk.writeShort(v, current_line_);
//         }
//     }
//     return Expected<bool, Diagnostic>(true);
// }

// Expected<bool, Diagnostic> BytecodeParser::parseDirective(const std::vector<std::pair<std::string, size_t>>& tokens) {
//     if (tokens.empty()) return Expected<bool, Diagnostic>(true);
//     const std::string& cmd = tokens[0].first;
//     current_col_ = tokens[0].second;

//     if (cmd == ".func") {
//         if (currentProto) {
//             return Expected<bool, Diagnostic>(make_diagnostic(ParseError::NestedFunctionDefinition, current_col_, {}));
//         }
//         if (tokens.size() < 2) {
//             return Expected<bool, Diagnostic>(make_diagnostic(ParseError::MissingArgument, current_col_, {}));
//         }
//         const std::string& name = tokens[1].first;
//         currentProto = memoryManager->newObject<ObjFunctionProto>(0, 0, name);
//         currentProto->sourceName = current_source_name_;
//         protos[name] = currentProto;
//         localProtoData[currentProto] = LocalProtoData{};
//     } else if (cmd == ".endfunc") {
//         if (!currentProto) {
//             return Expected<bool, Diagnostic>(make_diagnostic(ParseError::MissingEndFunc, current_col_, {}));
//         }
//         currentProto = nullptr;
//     } else {
//         if (!currentProto) {
//             return Expected<bool, Diagnostic>(make_diagnostic(ParseError::DirectiveOutsideFunction, current_col_, { cmd }));
//         }

//         if (cmd == ".registers") {
//             if (tokens.size() < 2) {
//                 return Expected<bool, Diagnostic>(make_diagnostic(ParseError::MissingArgument, current_col_, {}));
//             }
//             try {
//                 currentProto->numRegisters = static_cast<size_t>(std::stoull(tokens[1].first));
//             } catch (...) {
//                 return Expected<bool, Diagnostic>(make_diagnostic(ParseError::InvalidArgumentType, tokens[1].second, { tokens[1].first, ".registers" }));
//             }
//         } else if (cmd == ".upvalues") {
//             if (tokens.size() < 2) {
//                 return Expected<bool, Diagnostic>(make_diagnostic(ParseError::MissingArgument, current_col_, {}));
//             }
//             try {
//                 currentProto->numUpvalues = static_cast<size_t>(std::stoull(tokens[1].first));
//             } catch (...) {
//                 return Expected<bool, Diagnostic>(make_diagnostic(ParseError::InvalidArgumentType, tokens[1].second, { tokens[1].first, ".upvalues" }));
//             }
//         } else if (cmd == ".const") {
//             if (tokens.size() < 2) {
//                 std::string allTokens;
//                 for(size_t i = 1; i < tokens.size(); ++i) allTokens += tokens[i].first + " ";
//                 return Expected<bool, Diagnostic>(make_diagnostic(ParseError::InvalidConstant, current_col_, { allTokens }));
//             }
//             std::string rest;
//             for (size_t i = 1; i < tokens.size(); ++i) {
//                 if (i > 1) rest += " ";
//                 rest += tokens[i].first;
//             }
//             try {
//                 currentProto->chunk.constantPool.push_back(parseConstValue(rest));
//             } catch (const Diagnostic& diag) {
//                 return Expected<bool, Diagnostic>(diag);
//             } catch (...) {
//                 return Expected<bool, Diagnostic>(make_diagnostic(ParseError::InvalidConstant, current_col_, { rest }));
//             }
//         } else if (cmd == ".upvalue") {
//             if (tokens.size() < 4) {
//                 return Expected<bool, Diagnostic>(make_diagnostic(ParseError::MissingArgument, current_col_, {}));
//             }
//             size_t uvIndex;
//             std::string uvType;
//             size_t slot;
//             try {
//                 uvIndex = static_cast<size_t>(std::stoll(tokens[1].first));
//                 uvType = tokens[2].first;
//                 slot = static_cast<size_t>(std::stoll(tokens[3].first));
//             } catch (...) {
//                 // return Expected<bool, Diagnostic>(make_diagnostic(ParseError::InvalidArgumentType, current_col_, {}));
//                 return Expected<bool, Diagnostic>(make_diagnostic(ParseError::InvalidArgumentType, current_col_, {"<invalid integer>", ".upvalue"}));
//             }
//             if (uvType != "local" && uvType != "parent_upvalue") {
//                 return Expected<bool, Diagnostic>(make_diagnostic(ParseError::InvalidUpvalueType, tokens[2].second, { uvType }));
//             }
//             bool isLocal = (uvType == "local");
//             if (currentProto->upvalueDescs.size() <= static_cast<size_t>(uvIndex)) {
//                 currentProto->upvalueDescs.resize(static_cast<size_t>(uvIndex) + 1);
//             }
//             currentProto->upvalueDescs[static_cast<size_t>(uvIndex)] = UpvalueDesc(isLocal, static_cast<int>(slot));
//         } else {
//             return Expected<bool, Diagnostic>(make_diagnostic(ParseError::UnknownDirective, current_col_, { cmd }));
//         }
//     }

//     return Expected<bool, Diagnostic>(true);
// }

// static std::string unescapeString(const std::string& s) {
//     std::string result;
//     result.reserve(s.length());
//     bool escaping = false;
//     for (char c : s) {
//         if (escaping) {
//             switch (c) {
//                 case 'n': result += '\n'; break;
//                 case 't': result += '\t'; break;
//                 case 'r': result += '\r'; break;
//                 case '"': result += '"'; break;
//                 case '\\': result += '\\'; break;
//                 default:
//                     result += '\\';
//                     result += c;
//                     break;
//             }
//             escaping = false;
//         } else if (c == '\\') {
//             escaping = true;
//         } else {
//             result += c;
//         }
//     }
//     return result;
// }

// Value BytecodeParser::parseConstValue(const std::string& token) {
//     std::string s = trim(token);
//     if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
//         std::string inner = s.substr(1, s.size() - 2);
//         return Value(memoryManager->newObject<ObjString>(inner));
//     }
//     if (!s.empty() && s.front() == '@') {
//         return Value(memoryManager->newObject<ObjString>(std::string("::function_proto::") + s.substr(1)));
//     }
//     if (s.find('.') != std::string::npos) {
//         try { return Value(std::stod(s)); } catch (...) {}
//     }
//     try { return Value(std::stoi(s)); } catch (...) {}
//     if (s == "true") return Value(true);
//     if (s == "false") return Value(false);
//     if (s == "null") return Value(Null{});

//     throw make_diagnostic(ParseError::InvalidConstant, current_col_, { s });
// }

// Expected<bool, Diagnostic> BytecodeParser::resolveAllLabels() {
//     for (auto &pair : protos) {
//         auto proto = pair.second;
//         auto itlocal = localProtoData.find(proto);
//         if (itlocal == localProtoData.end()) continue;

//         auto &ldata = itlocal->second;
//         Chunk& chunk = proto->chunk;
//         uint8_t* code = chunk.code.data();

//         for (const auto& jump : ldata.pendingJumps) {
//             size_t patchOffset = std::get<0>(jump);
//             std::string labelName = std::get<2>(jump);
            
//             auto itLabel = ldata.labels.find(labelName);
//             if (itLabel == ldata.labels.end()) {
//                 Diagnostic d(ParseError::LabelNotFound, proto->getSourceName(), 0, 0, { labelName, proto->getSourceName() });
//                 return Expected<bool, Diagnostic>(d);
//             }
            
//             uint16_t jumpTarget = static_cast<uint16_t>(itLabel->second);
            
//             // Patch the bytecode with little-endian uint16_t
//             // writeU16LE(code + patchOffset, jumpTarget);
//             code[patchOffset] = static_cast<uint8_t>(jumpTarget & 0xFF);
//             code[patchOffset + 1] = static_cast<uint8_t>((jumpTarget >> 8) & 0xFF);
//         }
//     }
//     return Expected<bool, Diagnostic>(true);
// }

// Expected<bool, Diagnostic> BytecodeParser::linkProtos() {
//     const std::string prefix = "::function_proto::";
//     for (auto& pair : protos) {
//         auto proto = pair.second;
//         for (size_t i = 0; i < proto->chunk.constantPool.size(); ++i) {
//             if (proto->chunk.constantPool[i].is<String>()) {
//                 auto s = proto->chunk.constantPool[i].get<String>()->utf8();
//                 if (s.rfind(prefix, 0) == 0) {
//                     std::string protoName = s.substr(prefix.length());
//                     auto it = protos.find(protoName);
//                     if (it != protos.end()) {
//                         proto->chunk.constantPool[i] = Value(it->second);
//                     } else {
//                         Diagnostic d(ParseError::LabelNotFound, proto->sourceName, 0, 0, { protoName, proto->getSourceName() });
//                         return Expected<bool, Diagnostic>(d);
//                     }
//                 }
//             }
//         }
//     }
//     return Expected<bool, Diagnostic>(true);
// }