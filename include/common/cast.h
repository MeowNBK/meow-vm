#pragma once

#include "common/pch.h"
#include "core/definitions.h"
#include "core/objects.h"
#include "core/value.h"

namespace meow::common {

inline int64_t to_int(meow::core::param_t value) noexcept {
    using namespace meow::core;
    using i64_limits = std::numeric_limits<int64_t>;
    return value.visit(
        [](null_t) -> int64_t { return 0; },
        [](int_t i) -> int64_t { return i; },
        [](float_t r) -> int64_t {
            if (std::isinf(r)) return (r > 0) ? i64_limits::max() : i64_limits::min();
            if (std::isnan(r)) return 0;
            return static_cast<int64_t>(r);
        },
        [](bool_t b) -> int64_t { return b ? 1 : 0; },
        [](string_t s) -> int64_t {
            std::string_view sv = s->c_str();

            size_t left = 0;
            while (left < sv.size() && std::isspace(static_cast<unsigned char>(sv[left]))) ++left;
            size_t right = sv.size();
            while (right > left && std::isspace(static_cast<unsigned char>(sv[right - 1]))) --right;
            if (left >= right) return 0;
            sv = sv.substr(left, right - left);

            bool negative = false;
            if (!sv.empty() && sv[0] == '-') {
                negative = true;
                sv.remove_prefix(1);
            } else if (!sv.empty() && sv[0] == '+') {
                sv.remove_prefix(1);
            }

            if (sv.size() >= 2) {
                auto prefix = sv.substr(0, 2);
                if (prefix == "0b" || prefix == "0B") {
                    using ull = unsigned long long;
                    ull acc = 0;
                    const ull limit = static_cast<ull>(i64_limits::max());
                    sv.remove_prefix(2);
                    for (char c : sv) {
                        if (c == '0' || c == '1') {
                            int d = c - '0';
                            if (acc > (limit - d) / 2) {
                                return negative ? i64_limits::min() : i64_limits::max();
                            }
                            acc = (acc << 1) | static_cast<ull>(d);
                        } else {
                            break;
                        }
                    }
                    int64_t result = static_cast<int64_t>(acc);
                    return negative ? -result : result;
                }
            }

            int base = 10;
            std::string token(sv.begin(), sv.end());
            if (token.size() >= 2) {
                std::string p = token.substr(0, 2);
                if (p == "0x" || p == "0X") {
                    base = 16;
                    token = token.substr(2);
                } else if (p == "0o" || p == "0O") {
                    base = 8;
                    token = token.substr(2);
                }
            }

            if (negative) token.insert(token.begin(), '-');

            errno = 0;
            char* endptr = nullptr;
            long long parsed = std::strtoll(token.c_str(), &endptr, base);
            if (endptr == token.c_str()) return 0;
            if (errno == ERANGE) return (parsed > 0) ? i64_limits::max() : i64_limits::min();
            if (parsed > i64_limits::max()) return i64_limits::max();
            if (parsed < i64_limits::min()) return i64_limits::min();
            return static_cast<int64_t>(parsed);
        },
        [](auto&&) -> int64_t { return 0; }
    );
}

inline double to_float(meow::core::param_t value) noexcept {
    using namespace meow::core;
    return value.visit(
        [](null_t) -> double { return 0.0; },
        [](int_t i) -> double { return static_cast<double>(i); },
        [](float_t f) -> double { return f; },
        [](bool_t b) -> double { return b ? 1.0 : 0.0; },
        [](string_t s) -> double {
            std::string str = s->c_str();
            for (auto& c : str) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

            if (str == "nan") return std::numeric_limits<double>::quiet_NaN();
            if (str == "infinity" || str == "+infinity" || str == "inf" || str == "+inf")
                return std::numeric_limits<double>::infinity();
            if (str == "-infinity" || str == "-inf")
                return -std::numeric_limits<double>::infinity();
            const char* cs = str.c_str();
            errno = 0;
            char* endptr = nullptr;
            double val = std::strtod(cs, &endptr);
            if (cs == endptr) return 0.0;
            if (errno == ERANGE) return (val > 0) ? std::numeric_limits<double>::infinity()
                                                 : -std::numeric_limits<double>::infinity();
            return val;
        },
        [](auto&&) -> double { return 0.0; }
    );
}

inline bool to_bool(meow::core::param_t value) noexcept {
    using namespace meow::core;
    return value.visit(
        [](null_t) -> bool { return false; },
        [](int_t i) -> bool { return i != 0; },
        [](float_t f) -> bool { return f != 0.0 && !std::isnan(f); },
        [](bool_t b) -> bool { return b; },
        [](string_t s) -> bool { return !s->empty(); },
        [](array_t a) -> bool { return !a->empty(); },
        [](hash_table_t o) -> bool { return !o->empty(); },
        [](auto&&) -> bool { return true; }
    );
}

}  // namespace meow::common


// inline std::string toString(Value value) noexcept {
//     return value.visit(
//         [](Null) -> std::string { return "null"; },
//         [](Int val) -> std::string { return std::to_string(val); },
//         [](Real val) -> std::string {
//             if (std::isnan(val)) return "NaN";
//             if (std::isinf(val)) return (val > 0) ? "Infinity" : "-Infinity";

//             if (val == 0.0 && std::signbit(val)) return "-0";
//             std::ostringstream os;
//             os << std::fixed << std::setprecision(15) << val;

//             // Removes extra '0' character
//             std::string str = os.str();
//             auto pos = str.find('.');

//             // This is impossible for floating-point string value
//             // How it can be? Maybe C++ casted failed
//             if (pos == std::string::npos) return str;
//             size_t end = str.size();

//             // Search from the end of string makes a bit faster
//             while (end > pos + 1 && str[end - 1] == '0') --end;
//             if (end == pos + 1) --pos;
//             return str.substr(0, end);
//         },
//         [](Bool val) -> std::string { return val ? "true" : "false"; },
//         [](String str) -> std::string { return str->utf8(); },
//         [](Array arr) -> std::string {
//             std::string out = "[";
//             for (size_t i = 0; i < arr->size(); ++i) {
//                 if (i > 0) out += ", ";
//                 out += toString(arr->getElement(i));
//             }
//             out += "]";
//             return out;
//         },
//         [](Object obj) -> std::string {
//             std::string out = "{";
//             bool first = true;
//             for (auto it = obj->begin(); it != obj->end(); ++it) {
//                 if (!first) out += ", ";
//                 out += it->first + ": " + toString((it->second));
//                 first = false;
//             }
//             out += "}";
//             return out;
//         },
//         [](Class klass) {
//             return "<class '" + klass->getName() + "'>";
//         },
//         [](Instance inst) {
//             return "<" + inst->getClass()->getName() + " object>";
//         },
//         [](BoundMethod) {
//             return "<bound method>";
//         },
//         [](Module mod) {
//             return "<module '" + mod->getName() + "'>";
//         },
//         [](NativeFn) {
//             return "<native fn>";
//         },
//         [](Function func) {
//             return "<fn '" + func->getProto()->getSourceName() + "'>";
//         },
//         [](Proto proto) -> std::string {
//             if (!proto) return "<null proto>";
//             std::ostringstream os;

//             os << "<function proto '" << proto->getSourceName() << "'>\n";

//             // code size (chunk-based)
//             os << "  - code size: " << proto->getChunk().getCodeSize() <<
//             "\n";

//             // disassemble bằng helper chung
//             if (!proto->getChunk().isCodeEmpty()) {
//                 os << disassembleChunk(proto->getChunk());
//             } else {
//                 os << "  - (Bytecode rỗng)\n";
//             }
//             auto valueToString = [&](Value val) -> std::string {
//                 if (val.is<Null>()) return "<null>";
//                 if (val.is<Int>()) return std::to_string(val.get<Int>());
//                 if (val.is<Real>()) {
//                     std::ostringstream t;
//                     Real r = val.get<Real>();
//                     if (std::isnan(r)) return "NaN";
//                     if (std::isinf(r)) return (r > 0) ? "Infinity" :
//                     "-Infinity"; t << r; return t.str();
//                 }
//                 if (val.is<Bool>()) return val.get<Bool>() ? "true" :
//                 "false"; if (val.is<String>()) {
//                     return "\"" + val.get<String>()->utf8() + "\"";
//                 }
//                 if (val.is<Proto>()) {
//                     ObjFunctionProto* p = val.get<Proto>();
//                     return p ? ("<function proto '" + p->getSourceName() +
//                     "'>") : "<function proto>";
//                 }
//                 if (val.is<Function>()) return "<closure>";
//                 if (val.is<Instance>()) return "<instance>";
//                 if (val.is<Class>()) return "<class>";
//                 if (val.is<Array>()) return "<array>";
//                 if (val.is<Object>()) return "<object>";
//                 if (val.is<Upvalue>()) return "<upvalue>";
//                 if (val.is<Module>()) return "<module>";
//                 if (val.is<BoundMethod>()) return "<bound method>";
//                 if (val.is<NativeFn>()) return "<native fn>";
//                 return "<unknown value>";
//             };

//             // constant pool preview (up to 10)
//             if (!proto->getChunk().isConstantPoolEmpty()) {
//                 os << "\n  - Constant pool (preview up to 10):\n";
//                 size_t maxShow =
//                 std::min<size_t>(proto->getChunk().getConstantPoolSize(),
//                 10); for (size_t ci = 0; ci < maxShow; ++ci) {
//                     os << "     [" << ci << "]: " <<
//                     valueToString(proto->getChunk().getConstant(ci)) << "\n";
//                 }
//             }

//             return os.str();
//         },
//     [](Upvalue) {
//         return "<upvalue>";
//     },
//     [](auto&&) {
//         return "<unknown value>";
//     }
//     );
// }