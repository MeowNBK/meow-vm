#pragma once
#include <array>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <vector>
#include "common/pch.h"
#include "core/definitions.h"
#include "core/op_codes.h"
#include "runtime/chunk.h"

using meow::core::OpCode;
using meow::runtime::Chunk;
using meow::runtime::ObjFunctionProto;
using meow::runtime::ObjString;
using meow::runtime::Value;
static constexpr std::array<std::string_view, static_cast<size_t>(meow::core::OpCode::TOTAL_OPCODES)> opcode_strings = {
    "LOAD_CONST", "LOAD_NULL",     "LOAD_TRUE",     "LOAD_FALSE", "LOAD_INT",   "LOAD_FLOAT",   "MOVE",        "ADD",       "SUB",
    "MUL",        "DIV",           "MOD",           "POW",        "EQ",         "NEQ",          "GT",          "GE",        "LT",
    "LE",         "NEG",           "NOT",           "GET_GLOBAL", "SET_GLOBAL", "GET_UPVALUE",  "SET_UPVALUE", "CLOSURE",   "CLOSE_UPVALUES",
    "JUMP",       "JUMP_IF_FALSE", "JUMP_IF_TRUE",  "CALL",       "CALL_VOID",  "RETURN",       "HALT",        "NEW_ARRAY", "NEW_HASH",
    "GET_INDEX",  "SET_INDEX",     "GET_KEYS",      "GET_VALUES", "NEW_CLASS",  "NEW_INSTANCE", "GET_PROP",    "SET_PROP",  "SET_METHOD",
    "INHERIT",    "GET_SUPER",     "BIT_AND",       "BIT_OR",     "BIT_XOR",    "BIT_NOT",      "LSHIFT",      "RSHIFT",    "THROW",
    "SETUP_TRY",  "POP_TRY",       "IMPORT_MODULE", "EXPORT",     "GET_EXPORT", "IMPORT_ALL",
};
inline static std::string_view opcode_to_string(OpCode op) noexcept {
    return opcode_strings[static_cast<size_t>(op)];
}

static inline uint16_t read_var_arg(const std::vector<uint8_t>& code, size_t& ip, size_t code_size) noexcept {
    if (ip >= code_size) return 0;
    uint8_t b0 = code[ip++];
    if ((b0 & 0x80) == 0) return static_cast<uint16_t>(b0);
    if (ip >= code_size) return static_cast<uint16_t>(b0 & 0x7F);
    uint8_t b1 = code[ip++];
    return static_cast<uint16_t>((b0 & 0x7F) | (static_cast<uint16_t>(b1) << 7));
}
static inline int64_t read_i64_le(const std::vector<uint8_t>& code, size_t& ip, size_t code_size) noexcept {
    if (ip + 7 >= code_size) {
        int64_t out = 0;
        int shift = 0;
        while (ip < code_size && shift < 64) {
            out |= (static_cast<int64_t>(code[ip++]) << shift);
            shift += 8;
        }
        return out;
    }
    uint64_t out = 0;
    for (int b = 0; b < 8; ++b) out |= (static_cast<uint64_t>(code[ip++]) << (8 * b));
    return static_cast<int64_t>(out);
}
inline std::string disassemble_chunk(const Chunk& chunk) noexcept {
    std::ostringstream os;
    const auto& code = chunk.get_code();
    size_t code_size = chunk.get_code_size();
    auto value_to_string = [&](const Value& val) -> std::string {
        if (val.is<meow::runtime::Null>()) return "<null>";
        if (val.is<meow::runtime::Int>()) return std::to_string(val.get<meow::runtime::Int>());
        if (val.is<meow::runtime::Real>()) {
            std::ostringstream t;
            double r = val.get<meow::runtime::Real>();
            if (std::isnan(r)) return "NaN";
            if (std::isinf(r)) return (r > 0) ? "Infinity" : "-Infinity";
            t << r;
            return t.str();
        }
        if (val.is<meow::runtime::Bool>()) return val.get<meow::runtime::Bool>() ? "true" : "false";
        if (val.is<meow::runtime::String>()) {
            ObjString* s = val.get<meow::runtime::String>();
            return s ? std::string("\"") + s->utf8() + "\"" : std::string("\"<null string>\"");
        }
        if (val.is<meow::runtime::Proto>()) {
            ObjFunctionProto* p = val.get<meow::runtime::Proto>();
            return p ? std::string("<function proto '") + p->getSourceName() + "'>" : "<function proto null>";
        }
        if (val.is<meow::runtime::Function>()) return "<closure>";
        if (val.is<meow::runtime::Instance>()) return "<instance>";
        if (val.is<meow::runtime::Class>()) return "<class>";
        if (val.is<meow::runtime::Array>()) return "<array>";
        if (val.is<meow::runtime::Object>()) return "<object>";
        if (val.is<meow::runtime::Upvalue>()) return "<upvalue>";
        if (val.is<meow::runtime::Module>()) return "<module>";
        if (val.is<meow::runtime::BoundMethod>()) return "<bound method>";
        if (val.is<meow::runtime::NativeFn>()) return "<native fn>";
        return "<unknown value>";
    };
    os << "  - Bytecode:\n";
    for (size_t ip = 0; ip < code_size;) {
        size_t instOffset = ip;
        uint8_t rawOpcode = code[ip++];
        OpCode op = static_cast<OpCode>(rawOpcode);
        os << "     " << std::right << std::setw(4) << static_cast<int>(instOffset) << ": ";
        std::string opname = std::string(opcode_to_string(op));
        os << std::left << std::setw(12) << opname;
        switch (op) {
            case OpCode::MOVE: {
                uint16_t dst = read_var_arg(code, ip, code_size);
                uint16_t src = read_var_arg(code, ip, code_size);
                os << "  args=[dst=" << dst << ", src=" << src << "]";
                break;
            }
            case OpCode::LOAD_CONST: {
                uint16_t dst = read_var_arg(code, ip, code_size);
                uint16_t cidx = read_var_arg(code, ip, code_size);
                std::string valStr = (cidx < chunk.constantPool.size()) ? value_to_string(chunk.constantPool[cidx]) : "<const OOB>";
                os << "  args=[dst=" << dst << ", cidx=" << cidx << " -> " << valStr << "]";
                break;
            }
            case OpCode::LOAD_INT: {
                uint16_t dst = read_var_arg(code, ip, code_size);
                int64_t v = read_i64_le(code, ip, code_size);
                os << "  args=[dst=" << dst << ", val=" << v << "]";
                break;
            }
            case OpCode::LOAD_NULL:
            case OpCode::LOAD_TRUE:
            case OpCode::LOAD_FALSE: {
                uint16_t dst = read_var_arg(code, ip, code_size);
                os << "  args=[dst=" << dst << "]";
                break;
            }
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
            case OpCode::RSHIFT: {
                uint16_t dst = read_var_arg(code, ip, code_size);
                uint16_t r1 = read_var_arg(code, ip, code_size);
                uint16_t r2 = read_var_arg(code, ip, code_size);
                os << "  args=[dst=" << dst << ", r1=" << r1 << ", r2=" << r2 << "]";
                break;
            }
            case OpCode::NEG:
            case OpCode::NOT:
            case OpCode::BIT_NOT: {
                uint16_t dst = read_var_arg(code, ip, code_size);
                uint16_t src = read_var_arg(code, ip, code_size);
                os << "  args=[dst=" << dst << ", src=" << src << "]";
                break;
            }
            case OpCode::GET_GLOBAL: {
                uint16_t dst = read_var_arg(code, ip, code_size);
                uint16_t cidx = read_var_arg(code, ip, code_size);
                std::string name = (cidx < chunk.constantPool.size()) ? value_to_string(chunk.constantPool[cidx]) : "<bad-name>";
                os << "  args=[dst=" << dst << ", nameIdx=" << cidx << " -> " << name << "]";
                break;
            }
            case OpCode::SET_GLOBAL: {
                uint16_t nameIdx = read_var_arg(code, ip, code_size);
                uint16_t src = read_var_arg(code, ip, code_size);
                std::string name = (nameIdx < chunk.constantPool.size()) ? value_to_string(chunk.constantPool[nameIdx]) : "<bad-name>";
                os << "  args=[nameIdx=" << nameIdx << " -> " << name << ", src=" << src << "]";
                break;
            }
            case OpCode::GET_UPVALUE: {
                uint16_t dst = read_var_arg(code, ip, code_size);
                uint16_t uv = read_var_arg(code, ip, code_size);
                os << "  args=[dst=" << dst << ", uvIndex=" << uv << "]";
                break;
            }
            case OpCode::SET_UPVALUE: {
                uint16_t uv = read_var_arg(code, ip, code_size);
                uint16_t src = read_var_arg(code, ip, code_size);
                os << "  args=[uvIndex=" << uv << ", src=" << src << "]";
                break;
            }
            case OpCode::CLOSURE: {
                uint16_t dst = read_var_arg(code, ip, code_size);
                uint16_t protoIdx = read_var_arg(code, ip, code_size);
                os << "  args=[dst=" << dst << ", protoIdx=" << protoIdx;
                if (protoIdx < chunk.constantPool.size() && chunk.constantPool[protoIdx].is<meow::runtime::Proto>()) {
                    ObjFunctionProto* inner = chunk.constantPool[protoIdx].get<meow::runtime::Proto>();
                    if (inner) {
                        size_t nUp = inner->getNumUpvalues();
                        os << ", upvalues=" << nUp << " {";
                        for (size_t ui = 0; ui < nUp; ++ui) {
                            if (ip >= code_size) {
                                os << "??";
                                break;
                            }
                            uint16_t isLocal = read_var_arg(code, ip, code_size);
                            uint16_t index = read_var_arg(code, ip, code_size);
                            os << (ui ? ", " : "") << (isLocal ? "local" : "env") << ":" << index;
                        }
                        os << "}";
                    } else
                        os << ", <null proto>";
                } else
                    os << ", <proto not found in const pool>";
                os << "]";
                break;
            }
            case OpCode::CLOSE_UPVALUES: {
                uint16_t startSlot = read_var_arg(code, ip, code_size);
                os << "  args=[startSlot=" << startSlot << "]";
                break;
            }
            case OpCode::JUMP: {
                uint16_t target = read_var_arg(code, ip, code_size);
                os << "  args=[target=" << target << "]";
                break;
            }
            case OpCode::JUMP_IF_FALSE:
            case OpCode::JUMP_IF_TRUE: {
                uint16_t reg = read_var_arg(code, ip, code_size);
                uint16_t target = read_var_arg(code, ip, code_size);
                os << "  args=[reg=" << reg << ", target=" << target << "]";
                break;
            }
            case OpCode::CALL: {
                uint16_t dst = read_var_arg(code, ip, code_size);
                uint16_t fnReg = read_var_arg(code, ip, code_size);
                uint16_t argStart = read_var_arg(code, ip, code_size);
                uint16_t argc = read_var_arg(code, ip, code_size);
                os << "  args=[dst=" << dst << ", fnReg=" << fnReg << ", argStart=" << argStart << ", argc=" << argc << "]";
                break;
            }
            case OpCode::RETURN: {
                if (ip >= code_size)
                    os << "  args=[]";
                else {
                    size_t saveIp = ip;
                    uint16_t maybeReg = read_var_arg(code, ip, code_size);
                    os << "  args=[retReg=" << maybeReg << "]";
                }
                break;
            }
            case OpCode::HALT: {
                os << "  args=[]";
                break;
            }
            case OpCode::NEW_ARRAY:
            case OpCode::NEW_HASH: {
                uint16_t dst = read_var_arg(code, ip, code_size);
                uint16_t startIdx = read_var_arg(code, ip, code_size);
                uint16_t count = read_var_arg(code, ip, code_size);
                os << "  args=[dst=" << dst << ", startIdx=" << startIdx << ", count=" << count << "]";
                break;
            }
            case OpCode::GET_INDEX: {
                uint16_t dst = read_var_arg(code, ip, code_size);
                uint16_t srcReg = read_var_arg(code, ip, code_size);
                uint16_t keyReg = read_var_arg(code, ip, code_size);
                os << "  args=[dst=" << dst << ", src=" << srcReg << ", key=" << keyReg << "]";
                break;
            }
            case OpCode::SET_INDEX: {
                uint16_t srcReg = read_var_arg(code, ip, code_size);
                uint16_t keyReg = read_var_arg(code, ip, code_size);
                uint16_t valReg = read_var_arg(code, ip, code_size);
                os << "  args=[src=" << srcReg << ", key=" << keyReg << ", val=" << valReg << "]";
                break;
            }
            case OpCode::GET_KEYS:
            case OpCode::GET_VALUES: {
                uint16_t dst = read_var_arg(code, ip, code_size);
                uint16_t srcReg = read_var_arg(code, ip, code_size);
                os << "  args=[dst=" << dst << ", src=" << srcReg << "]";
                break;
            }
            case OpCode::IMPORT_MODULE: {
                uint16_t dst = read_var_arg(code, ip, code_size);
                uint16_t pathIdx = read_var_arg(code, ip, code_size);
                os << "  args=[dst=" << dst << ", pathIdx=" << pathIdx << "]";
                break;
            }
            case OpCode::EXPORT: {
                uint16_t nameIdx = read_var_arg(code, ip, code_size);
                uint16_t srcReg = read_var_arg(code, ip, code_size);
                os << "  args=[nameIdx=" << nameIdx << ", src=" << srcReg << "]";
                break;
            }
            case OpCode::GET_EXPORT:
            case OpCode::GET_MODULE_EXPORT: {
                uint16_t dst = read_var_arg(code, ip, code_size);
                uint16_t moduleReg = read_var_arg(code, ip, code_size);
                uint16_t nameIdx = read_var_arg(code, ip, code_size);
                os << "  args=[dst=" << dst << ", moduleReg=" << moduleReg << ", nameIdx=" << nameIdx << "]";
                break;
            }
            case OpCode::IMPORT_ALL: {
                uint16_t moduleReg = read_var_arg(code, ip, code_size);
                os << "  args=[moduleReg=" << moduleReg << "]";
                break;
            }
            case OpCode::NEW_CLASS: {
                uint16_t dst = read_var_arg(code, ip, code_size);
                uint16_t nameIdx = read_var_arg(code, ip, code_size);
                os << "  args=[dst=" << dst << ", nameIdx=" << nameIdx << "]";
                break;
            }
            case OpCode::NEW_INSTANCE: {
                uint16_t dst = read_var_arg(code, ip, code_size);
                uint16_t classReg = read_var_arg(code, ip, code_size);
                os << "  args=[dst=" << dst << ", classReg=" << classReg << "]";
                break;
            }
            case OpCode::GET_PROP: {
                uint16_t dst = read_var_arg(code, ip, code_size);
                uint16_t objReg = read_var_arg(code, ip, code_size);
                uint16_t nameIdx = read_var_arg(code, ip, code_size);
                os << "  args=[dst=" << dst << ", objReg=" << objReg << ", nameIdx=" << nameIdx << "]";
                break;
            }
            case OpCode::SET_PROP: {
                uint16_t objReg = read_var_arg(code, ip, code_size);
                uint16_t nameIdx = read_var_arg(code, ip, code_size);
                uint16_t valReg = read_var_arg(code, ip, code_size);
                os << "  args=[objReg=" << objReg << ", nameIdx=" << nameIdx << ", valReg=" << valReg << "]";
                break;
            }
            case OpCode::SET_METHOD: {
                uint16_t classReg = read_var_arg(code, ip, code_size);
                uint16_t nameIdx = read_var_arg(code, ip, code_size);
                uint16_t methodReg = read_var_arg(code, ip, code_size);
                os << "  args=[classReg=" << classReg << ", nameIdx=" << nameIdx << ", methodReg=" << methodReg << "]";
                break;
            }
            case OpCode::INHERIT: {
                uint16_t subClassReg = read_var_arg(code, ip, code_size);
                uint16_t superClassReg = read_var_arg(code, ip, code_size);
                os << "  args=[subClassReg=" << subClassReg << ", superClassReg=" << superClassReg << "]";
                break;
            }
            case OpCode::GET_SUPER: {
                uint16_t dst = read_var_arg(code, ip, code_size);
                uint16_t nameIdx = read_var_arg(code, ip, code_size);
                os << "  args=[dst=" << dst << ", nameIdx=" << nameIdx << "]";
                break;
            }
            case OpCode::SETUP_TRY: {
                uint16_t target = read_var_arg(code, ip, code_size);
                os << "  args=[target=" << target << "]";
                break;
            }
            case OpCode::POP_TRY: {
                os << "  args=[]";
                break;
            }
            case OpCode::THROW: {
                uint16_t reg = read_var_arg(code, ip, code_size);
                os << "  args=[reg=" << reg << "]";
                break;
            }
            default: {
                os << "  args=[<unparsed>]";
                break;
            }
        }
        os << "\n";
    }
    return os.str();
}
