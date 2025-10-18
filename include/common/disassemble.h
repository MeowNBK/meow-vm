// --- Old codes ---

// #pragma once

// #include "common/pch.h"
// #include "core/op_codes.h"
// #include "core/definitions.h"
// #include "runtime/chunk.h"

// static inline std::string opCodeToString(OpCode op) {
//     switch (op) {
//         case OpCode::LOAD_CONST: return "LOAD_CONST";
//         case OpCode::LOAD_NULL: return "LOAD_NULL";
//         case OpCode::LOAD_TRUE: return "LOAD_TRUE";
//         case OpCode::LOAD_FALSE: return "LOAD_FALSE";
//         case OpCode::LOAD_INT: return "LOAD_INT";
//         case OpCode::MOVE: return "MOVE";
//         case OpCode::ADD: return "ADD";
//         case OpCode::SUB: return "SUB";
//         case OpCode::MUL: return "MUL";
//         case OpCode::DIV: return "DIV";
//         case OpCode::MOD: return "MOD";
//         case OpCode::POW: return "POW";
//         case OpCode::EQ: return "EQ";
//         case OpCode::NEQ: return "NEQ";
//         case OpCode::GT: return "GT";
//         case OpCode::GE: return "GE";
//         case OpCode::LT: return "LT";
//         case OpCode::LE: return "LE";
//         case OpCode::NEG: return "NEG";
//         case OpCode::NOT: return "NOT";
//         case OpCode::GET_GLOBAL: return "GET_GLOBAL";
//         case OpCode::SET_GLOBAL: return "SET_GLOBAL";
//         case OpCode::GET_UPVALUE: return "GET_UPVALUE";
//         case OpCode::SET_UPVALUE: return "SET_UPVALUE";
//         case OpCode::CLOSURE: return "CLOSURE";
//         case OpCode::CLOSE_UPVALUES: return "CLOSE_UPVALUES";
//         case OpCode::JUMP: return "JUMP";
//         case OpCode::JUMP_IF_FALSE: return "JUMP_IF_FALSE";
//         case OpCode::JUMP_IF_TRUE: return "JUMP_IF_TRUE";
//         case OpCode::CALL: return "CALL";
//         case OpCode::RETURN: return "RETURN";
//         case OpCode::HALT: return "HALT";
//         case OpCode::NEW_ARRAY: return "NEW_ARRAY";
//         case OpCode::NEW_HASH: return "NEW_HASH";
//         case OpCode::GET_INDEX: return "GET_INDEX";
//         case OpCode::SET_INDEX: return "SET_INDEX";
//         case OpCode::GET_KEYS: return "GET_KEYS";
//         case OpCode::GET_VALUES: return "GET_VALUES";
//         case OpCode::NEW_CLASS: return "NEW_CLASS";
//         case OpCode::NEW_INSTANCE: return "NEW_INSTANCE";
//         case OpCode::GET_PROP: return "GET_PROP";
//         case OpCode::SET_PROP: return "SET_PROP";
//         case OpCode::SET_METHOD: return "SET_METHOD";
//         case OpCode::INHERIT: return "INHERIT";
//         case OpCode::GET_SUPER: return "GET_SUPER";
//         case OpCode::BIT_AND: return "BIT_AND";
//         case OpCode::BIT_OR: return "BIT_OR";
//         case OpCode::BIT_XOR: return "BIT_XOR";
//         case OpCode::BIT_NOT: return "BIT_NOT";
//         case OpCode::LSHIFT: return "LSHIFT";
//         case OpCode::RSHIFT: return "RSHIFT";
//         case OpCode::THROW: return "THROW";
//         case OpCode::SETUP_TRY: return "SETUP_TRY";
//         case OpCode::POP_TRY: return "POP_TRY";
//         case OpCode::IMPORT_MODULE: return "IMPORT_MODULE";
//         case OpCode::EXPORT: return "EXPORT";
//         case OpCode::GET_EXPORT: return "GET_EXPORT";
//         case OpCode::GET_MODULE_EXPORT: return "GET_MODULE_EXPORT";
//         case OpCode::IMPORT_ALL: return "IMPORT_ALL";
//         default: return "UNKNOWN_OP";
//     }
// }

// // ---------- helper: read var-length arg (matching your writeArg) ----------
// static inline uint16_t readVarArg (const std::vector<uint8_t>& code, size_t &ip, size_t codeSize) noexcept {
//     if (ip >= codeSize) return 0;
//     uint8_t b0 = code[ip++];
//     if ((b0 & 0x80) == 0) {
//         // single byte value (0..127)
//         return static_cast<uint16_t>(b0);
//     } else {
//         // two-byte encoding: low7 in b0, high bits in next byte
//         if (ip >= codeSize) {
//             // truncated: return what we can
//             return static_cast<uint16_t>(b0 & 0x7F);
//         }
//         uint8_t b1 = code[ip++];
//         return static_cast<uint16_t>((b0 & 0x7F) | (static_cast<uint16_t>(b1) << 7));
//     }
// };

// // ---------- keep an i64 reader for immediates (LOAD_INT) ----------
// static inline int64_t read_i64_le (const std::vector<uint8_t>& code, size_t &ip, size_t codeSize) noexcept {
//     if (ip + 7 >= codeSize) {
//         // truncated: read remaining bytes little-endian
//         int64_t out = 0;
//         int shift = 0;
//         while (ip < codeSize && shift < 64) {
//             out |= (static_cast<int64_t>(code[ip++]) << shift);
//             shift += 8;
//         }
//         return out;
//     }
//     uint64_t out = 0;
//     for (int b = 0; b < 8; ++b) {
//         out |= (static_cast<uint64_t>(code[ip++]) << (8 * b));
//     }
//     return static_cast<int64_t>(out);
// };

// // ---------- disassembleChunk using readVarArg ----------
// inline std::string disassembleChunk(const Chunk& chunk) noexcept{
//     std::ostringstream os;
//     const auto &code = chunk.code;
//     size_t codeSize = code.size();

//     auto valueToString = [&](const Value& val) -> std::string {
//         if (val.is<Null>()) return "<null>";
//         if (val.is<Int>()) return std::to_string(val.get<Int>());
//         if (val.is<Real>()) {
//             std::ostringstream t;
//             Real r = val.get<Real>();
//             if (std::isnan(r)) return "NaN";
//             if (std::isinf(r)) return (r > 0) ? "Infinity" : "-Infinity";
//             t << r;
//             return t.str();
//         }
//         if (val.is<Bool>()) return val.get<Bool>() ? "true" : "false";
//         if (val.is<String>()) {
//             ObjString* s = val.get<String>();
//             return s ? std::string("\"") + s->utf8() + "\"" : std::string("\"<null string>\"");
//         }
//         if (val.is<Proto>()) {
//             ObjFunctionProto* p = val.get<Proto>();
//             return p ? std::string("<function proto '") + p->getSourceName() + "'>" : "<function proto null>";
//         }
//         if (val.is<Function>()) return "<closure>";
//         if (val.is<Instance>()) return "<instance>";
//         if (val.is<Class>()) return "<class>";
//         if (val.is<Array>()) return "<array>";
//         if (val.is<Object>()) return "<object>";
//         if (val.is<Upvalue>()) return "<upvalue>";
//         if (val.is<Module>()) return "<module>";
//         if (val.is<BoundMethod>()) return "<bound method>";
//         if (val.is<NativeFn>()) return "<native fn>";
//         return "<unknown value>";
//     };

//     os << "  - Bytecode:\n";
//     for (size_t ip = 0; ip < codeSize; ) {
//         size_t instOffset = ip;
//         uint8_t rawOpcode = code[ip++]; // opcode always 1 byte
//         OpCode op = static_cast<OpCode>(rawOpcode);

//         os << "     " << std::right << std::setw(4) << static_cast<Int>(instOffset) << ": ";
//         std::string opname = opCodeToString(op);
//         os << std::left << std::setw(12) << opname;

//         // Parse args using readVarArg where appropriate.
//         switch (op) {
//             case OpCode::MOVE: {
//                 uint16_t dst = readVarArg(code, ip, codeSize);
//                 uint16_t src = readVarArg(code, ip, codeSize);
//                 os << "  args=[dst=" << dst << ", src=" << src << "]";
//                 break;
//             }
//             case OpCode::LOAD_CONST: {
//                 uint16_t dst = readVarArg(code, ip, codeSize);
//                 uint16_t cidx = readVarArg(code, ip, codeSize);
//                 std::string valStr = (cidx < chunk.constantPool.size()) ? valueToString(chunk.constantPool[cidx]) : "<const OOB>";
//                 os << "  args=[dst=" << dst << ", cidx=" << cidx << " -> " << valStr << "]";
//                 break;
//             }
//             case OpCode::LOAD_INT: {
//                 uint16_t dst = readVarArg(code, ip, codeSize);
//                 int64_t v = read_i64_le(code, ip, codeSize);
//                 os << "  args=[dst=" << dst << ", val=" << v << "]";
//                 break;
//             }
//             case OpCode::LOAD_NULL:
//             case OpCode::LOAD_TRUE:
//             case OpCode::LOAD_FALSE: {
//                 uint16_t dst = readVarArg(code, ip, codeSize);
//                 os << "  args=[dst=" << dst << "]";
//                 break;
//             }

//             // Binary ops: dst, r1, r2
//             case OpCode::ADD: case OpCode::SUB: case OpCode::MUL: case OpCode::DIV:
//             case OpCode::MOD: case OpCode::POW: case OpCode::EQ: case OpCode::NEQ:
//             case OpCode::GT: case OpCode::GE: case OpCode::LT: case OpCode::LE:
//             case OpCode::BIT_AND: case OpCode::BIT_OR: case OpCode::BIT_XOR:
//             case OpCode::LSHIFT: case OpCode::RSHIFT: {
//                 uint16_t dst = readVarArg(code, ip, codeSize);
//                 uint16_t r1  = readVarArg(code, ip, codeSize);
//                 uint16_t r2  = readVarArg(code, ip, codeSize);
//                 os << "  args=[dst=" << dst << ", r1=" << r1 << ", r2=" << r2 << "]";
//                 break;
//             }

//             // Unary:
//             case OpCode::NEG: case OpCode::NOT: case OpCode::BIT_NOT: {
//                 uint16_t dst = readVarArg(code, ip, codeSize);
//                 uint16_t src = readVarArg(code, ip, codeSize);
//                 os << "  args=[dst=" << dst << ", src=" << src << "]";
//                 break;
//             }

//             case OpCode::GET_GLOBAL: {
//                 uint16_t dst = readVarArg(code, ip, codeSize);
//                 uint16_t cidx = readVarArg(code, ip, codeSize);
//                 std::string name = (cidx < chunk.constantPool.size()) ? valueToString(chunk.constantPool[cidx]) : "<bad-name>";
//                 os << "  args=[dst=" << dst << ", nameIdx=" << cidx << " -> " << name << "]";
//                 break;
//             }
//             case OpCode::SET_GLOBAL: {
//                 uint16_t nameIdx = readVarArg(code, ip, codeSize);
//                 uint16_t src = readVarArg(code, ip, codeSize);
//                 std::string name = (nameIdx < chunk.constantPool.size()) ? valueToString(chunk.constantPool[nameIdx]) : "<bad-name>";
//                 os << "  args=[nameIdx=" << nameIdx << " -> " << name << ", src=" << src << "]";
//                 break;
//             }

//             case OpCode::GET_UPVALUE: {
//                 uint16_t dst = readVarArg(code, ip, codeSize);
//                 uint16_t uv = readVarArg(code, ip, codeSize);
//                 os << "  args=[dst=" << dst << ", uvIndex=" << uv << "]";
//                 break;
//             }
//             case OpCode::SET_UPVALUE: {
//                 uint16_t uv = readVarArg(code, ip, codeSize);
//                 uint16_t src = readVarArg(code, ip, codeSize);
//                 os << "  args=[uvIndex=" << uv << ", src=" << src << "]";
//                 break;
//             }

//             case OpCode::CLOSURE: {
//                 uint16_t dst = readVarArg(code, ip, codeSize);
//                 uint16_t protoIdx = readVarArg(code, ip, codeSize);
//                 os << "  args=[dst=" << dst << ", protoIdx=" << protoIdx;
//                 if (protoIdx < chunk.constantPool.size() && chunk.constantPool[protoIdx].is<Proto>()) {
//                     ObjFunctionProto* inner = chunk.constantPool[protoIdx].get<Proto>();
//                     if (inner) {
//                         size_t nUp = inner->getNumUpvalues();
//                         os << ", upvalues=" << nUp << " {";
//                         for (size_t ui = 0; ui < nUp; ++ui) {
//                             if (ip >= codeSize) { os << "??"; break; }
//                             uint16_t isLocal = readVarArg(code, ip, codeSize);
//                             uint16_t index   = readVarArg(code, ip, codeSize);
//                             os << (ui ? ", " : "") << (isLocal ? "local" : "env") << ":" << index;
//                         }
//                         os << "}";
//                     } else {
//                         os << ", <null proto>";
//                     }
//                 } else {
//                     os << ", <proto not found in const pool>";
//                 }
//                 os << "]";
//                 break;
//             }

//             case OpCode::CLOSE_UPVALUES: {
//                 uint16_t startSlot = readVarArg(code, ip, codeSize);
//                 os << "  args=[startSlot=" << startSlot << "]";
//                 break;
//             }

//             case OpCode::JUMP: {
//                 uint16_t target = readVarArg(code, ip, codeSize);
//                 os << "  args=[target=" << target << "]";
//                 break;
//             }
//             case OpCode::JUMP_IF_FALSE:
//             case OpCode::JUMP_IF_TRUE: {
//                 uint16_t reg = readVarArg(code, ip, codeSize);
//                 uint16_t target = readVarArg(code, ip, codeSize);
//                 os << "  args=[reg=" << reg << ", target=" << target << "]";
//                 break;
//             }

//             case OpCode::CALL: {
//                 uint16_t dst = readVarArg(code, ip, codeSize);
//                 uint16_t fnReg = readVarArg(code, ip, codeSize);
//                 uint16_t argStart = readVarArg(code, ip, codeSize);
//                 uint16_t argc = readVarArg(code, ip, codeSize);
//                 os << "  args=[dst=" << dst << ", fnReg=" << fnReg
//                    << ", argStart=" << argStart << ", argc=" << argc << "]";
//                 break;
//             }

//             case OpCode::RETURN: {
//                 // As your emitter supports optional arg, we try to read 0 or 1 arg.
//                 // Convention: if writer emitted a register it will be encoded as varArg; we attempt to read it.
//                 // If there are no bytes left, it is empty. If we accidentally read something that is actually next opcode,
//                 // this can mis-sync; hence the emitter should be consistent. (If you prefer explicit flag, we can change.)
//                 if (ip >= codeSize) {
//                     os << "  args=[]";
//                 } else {
//                     // We'll peek: try to decode varArg but *do not* treat 0..(max opcode value) specially.
//                     // This is best-effort: if your emitter sometimes omits the arg, this will consume next opcode incorrectly.
//                     // If that happens, tell me and we add an explicit byte flag in encoding.
//                     size_t saveIp = ip;
//                     uint16_t maybeReg = readVarArg(code, ip, codeSize);
//                     os << "  args=[retReg=" << maybeReg << "]";
//                 }
//                 break;
//             }

//             case OpCode::HALT: {
//                 os << "  args=[]";
//                 break;
//             }

//             case OpCode::NEW_ARRAY:
//             case OpCode::NEW_HASH: {
//                 uint16_t dst = readVarArg(code, ip, codeSize);
//                 uint16_t startIdx = readVarArg(code, ip, codeSize);
//                 uint16_t count = readVarArg(code, ip, codeSize);
//                 os << "  args=[dst=" << dst << ", startIdx=" << startIdx << ", count=" << count << "]";
//                 break;
//             }

//             case OpCode::GET_INDEX: {
//                 uint16_t dst = readVarArg(code, ip, codeSize);
//                 uint16_t srcReg = readVarArg(code, ip, codeSize);
//                 uint16_t keyReg = readVarArg(code, ip, codeSize);
//                 os << "  args=[dst=" << dst << ", src=" << srcReg << ", key=" << keyReg << "]";
//                 break;
//             }

//             case OpCode::SET_INDEX: {
//                 uint16_t srcReg = readVarArg(code, ip, codeSize);
//                 uint16_t keyReg = readVarArg(code, ip, codeSize);
//                 uint16_t valReg = readVarArg(code, ip, codeSize);
//                 os << "  args=[src=" << srcReg << ", key=" << keyReg << ", val=" << valReg << "]";
//                 break;
//             }

//             case OpCode::GET_KEYS:
//             case OpCode::GET_VALUES: {
//                 uint16_t dst = readVarArg(code, ip, codeSize);
//                 uint16_t srcReg = readVarArg(code, ip, codeSize);
//                 os << "  args=[dst=" << dst << ", src=" << srcReg << "]";
//                 break;
//             }

//             case OpCode::IMPORT_MODULE: {
//                 uint16_t dst = readVarArg(code, ip, codeSize);
//                 uint16_t pathIdx = readVarArg(code, ip, codeSize);
//                 os << "  args=[dst=" << dst << ", pathIdx=" << pathIdx << "]";
//                 break;
//             }
//             case OpCode::EXPORT: {
//                 uint16_t nameIdx = readVarArg(code, ip, codeSize);
//                 uint16_t srcReg = readVarArg(code, ip, codeSize);
//                 os << "  args=[nameIdx=" << nameIdx << ", src=" << srcReg << "]";
//                 break;
//             }
//             case OpCode::GET_EXPORT:
//             case OpCode::GET_MODULE_EXPORT: {
//                 uint16_t dst = readVarArg(code, ip, codeSize);
//                 uint16_t moduleReg = readVarArg(code, ip, codeSize);
//                 uint16_t nameIdx = readVarArg(code, ip, codeSize);
//                 os << "  args=[dst=" << dst << ", moduleReg=" << moduleReg << ", nameIdx=" << nameIdx << "]";
//                 break;
//             }
//             case OpCode::IMPORT_ALL: {
//                 uint16_t moduleReg = readVarArg(code, ip, codeSize);
//                 os << "  args=[moduleReg=" << moduleReg << "]";
//                 break;
//             }

//             case OpCode::NEW_CLASS: {
//                 uint16_t dst = readVarArg(code, ip, codeSize);
//                 uint16_t nameIdx = readVarArg(code, ip, codeSize);
//                 os << "  args=[dst=" << dst << ", nameIdx=" << nameIdx << "]";
//                 break;
//             }

//             case OpCode::NEW_INSTANCE: {
//                 uint16_t dst = readVarArg(code, ip, codeSize);
//                 uint16_t classReg = readVarArg(code, ip, codeSize);
//                 os << "  args=[dst=" << dst << ", classReg=" << classReg << "]";
//                 break;
//             }

//             case OpCode::GET_PROP: {
//                 uint16_t dst = readVarArg(code, ip, codeSize);
//                 uint16_t objReg = readVarArg(code, ip, codeSize);
//                 uint16_t nameIdx = readVarArg(code, ip, codeSize);
//                 os << "  args=[dst=" << dst << ", objReg=" << objReg << ", nameIdx=" << nameIdx << "]";
//                 break;
//             }

//             case OpCode::SET_PROP: {
//                 uint16_t objReg = readVarArg(code, ip, codeSize);
//                 uint16_t nameIdx = readVarArg(code, ip, codeSize);
//                 uint16_t valReg = readVarArg(code, ip, codeSize);
//                 os << "  args=[objReg=" << objReg << ", nameIdx=" << nameIdx << ", valReg=" << valReg << "]";
//                 break;
//             }

//             case OpCode::SET_METHOD: {
//                 uint16_t classReg = readVarArg(code, ip, codeSize);
//                 uint16_t nameIdx = readVarArg(code, ip, codeSize);
//                 uint16_t methodReg = readVarArg(code, ip, codeSize);
//                 os << "  args=[classReg=" << classReg << ", nameIdx=" << nameIdx << ", methodReg=" << methodReg << "]";
//                 break;
//             }

//             case OpCode::INHERIT: {
//                 uint16_t subClassReg = readVarArg(code, ip, codeSize);
//                 uint16_t superClassReg = readVarArg(code, ip, codeSize);
//                 os << "  args=[subClassReg=" << subClassReg << ", superClassReg=" << superClassReg << "]";
//                 break;
//             }

//             case OpCode::GET_SUPER: {
//                 uint16_t dst = readVarArg(code, ip, codeSize);
//                 uint16_t nameIdx = readVarArg(code, ip, codeSize);
//                 os << "  args=[dst=" << dst << ", nameIdx=" << nameIdx << "]";
//                 break;
//             }

//             case OpCode::SETUP_TRY: {
//                 uint16_t target = readVarArg(code, ip, codeSize);
//                 os << "  args=[target=" << target << "]";
//                 break;
//             }

//             case OpCode::POP_TRY: {
//                 os << "  args=[]";
//                 break;
//             }

//             case OpCode::THROW: {
//                 uint16_t reg = readVarArg(code, ip, codeSize);
//                 os << "  args=[reg=" << reg << "]";
//                 break;
//             }

//             default: {
//                 os << "  args=[<unparsed>]";
//                 break;
//             }
//         }

//         os << "\n";
//     }

//     return os.str();
// }