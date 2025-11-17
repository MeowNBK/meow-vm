#include "module/loader/parser.h"
#include "core/objects/function.h"
#include "core/objects/string.h"
#include "memory/memory_manager.h"
#include "module/loader/lexer.h"
#include "runtime/chunk.h"

namespace meow::loader {
using namespace meow::core;
using namespace meow::runtime;
using namespace meow::memory;
using namespace meow::core::objects;

static const std::unordered_map<std::string_view, OpCode> OP_MAP = {{"LOAD_CONST", OpCode::LOAD_CONST},
                                                                    {"LOAD_NULL", OpCode::LOAD_NULL},
                                                                    {"LOAD_TRUE", OpCode::LOAD_TRUE},
                                                                    {"LOAD_FALSE", OpCode::LOAD_FALSE},
                                                                    {"LOAD_INT", OpCode::LOAD_INT},
                                                                    {"LOAD_FLOAT", OpCode::LOAD_FLOAT},
                                                                    {"MOVE", OpCode::MOVE},
                                                                    {"ADD", OpCode::ADD},
                                                                    {"SUB", OpCode::SUB},
                                                                    {"MUL", OpCode::MUL},
                                                                    {"DIV", OpCode::DIV},
                                                                    {"MOD", OpCode::MOD},
                                                                    {"POW", OpCode::POW},
                                                                    {"EQ", OpCode::EQ},
                                                                    {"NEQ", OpCode::NEQ},
                                                                    {"GT", OpCode::GT},
                                                                    {"GE", OpCode::GE},
                                                                    {"LT", OpCode::LT},
                                                                    {"LE", OpCode::LE},
                                                                    {"NEG", OpCode::NEG},
                                                                    {"NOT", OpCode::NOT},
                                                                    {"GET_GLOBAL", OpCode::GET_GLOBAL},
                                                                    {"SET_GLOBAL", OpCode::SET_GLOBAL},
                                                                    {"GET_UPVALUE", OpCode::GET_UPVALUE},
                                                                    {"SET_UPVALUE", OpCode::SET_UPVALUE},
                                                                    {"CLOSURE", OpCode::CLOSURE},
                                                                    {"CLOSE_UPVALUES", OpCode::CLOSE_UPVALUES},
                                                                    {"JUMP", OpCode::JUMP},
                                                                    {"JUMP_IF_FALSE", OpCode::JUMP_IF_FALSE},
                                                                    {"JUMP_IF_TRUE", OpCode::JUMP_IF_TRUE},
                                                                    {"CALL", OpCode::CALL},
                                                                    {"CALL_VOID", OpCode::CALL_VOID},
                                                                    {"RETURN", OpCode::RETURN},
                                                                    {"HALT", OpCode::HALT},
                                                                    {"NEW_ARRAY", OpCode::NEW_ARRAY},
                                                                    {"NEW_HASH", OpCode::NEW_HASH},
                                                                    {"GET_INDEX", OpCode::GET_INDEX},
                                                                    {"SET_INDEX", OpCode::SET_INDEX},
                                                                    {"GET_KEYS", OpCode::GET_KEYS},
                                                                    {"GET_VALUES", OpCode::GET_VALUES},
                                                                    {"NEW_CLASS", OpCode::NEW_CLASS},
                                                                    {"NEW_INSTANCE", OpCode::NEW_INSTANCE},
                                                                    {"GET_PROP", OpCode::GET_PROP},
                                                                    {"SET_PROP", OpCode::SET_PROP},
                                                                    {"SET_METHOD", OpCode::SET_METHOD},
                                                                    {"INHERIT", OpCode::INHERIT},
                                                                    {"GET_SUPER", OpCode::GET_SUPER},
                                                                    {"BIT_AND", OpCode::BIT_AND},
                                                                    {"BIT_OR", OpCode::BIT_OR},
                                                                    {"BIT_XOR", OpCode::BIT_XOR},
                                                                    {"BIT_NOT", OpCode::BIT_NOT},
                                                                    {"LSHIFT", OpCode::LSHIFT},
                                                                    {"RSHIFT", OpCode::RSHIFT},
                                                                    {"THROW", OpCode::THROW},
                                                                    {"SETUP_TRY", OpCode::SETUP_TRY},
                                                                    {"POP_TRY", OpCode::POP_TRY},
                                                                    {"IMPORT_MODULE", OpCode::IMPORT_MODULE},
                                                                    {"EXPORT", OpCode::EXPORT},
                                                                    {"GET_EXPORT", OpCode::GET_EXPORT},
                                                                    {"IMPORT_ALL", OpCode::IMPORT_ALL}};

TextParser::TextParser(meow::memory::MemoryManager* h, const std::vector<Token>& t, std::string_view s) noexcept : heap_(h), src_name_(s), toks_(t), ti_(0) {
}
TextParser::TextParser(meow::memory::MemoryManager* h, std::vector<Token>&& t, std::string_view s) noexcept : heap_(h), src_name_(s), toks_(std::move(t)), ti_(0) {
}

const Token& TextParser::cur_tok() const {
    if (ti_ >= toks_.size()) return toks_.back();
    return toks_[ti_];
}
const Token& TextParser::peek_tok(size_t off) const {
    size_t i = ti_ + off;
    return i >= toks_.size() ? toks_.back() : toks_[i];
}
bool TextParser::at_end() const {
    return cur_tok().type == TokenType::END_OF_FILE;
}
void TextParser::adv() {
    if (cur_tok().type != TokenType::END_OF_FILE) ++ti_;
}
const Token& TextParser::expect_tok(TokenType exp, std::string_view err) {
    const Token& tk = cur_tok();
    if (tk.type != exp) throw std::logic_error(std::string(err));
    adv();
    return toks_[ti_ - 1];
}
bool TextParser::match_tok(TokenType t) {
    if (cur_tok().type == t) {
        adv();
        return true;
    }
    return false;
}

Diagnostic TextParser::mkdiag(ErrCode c, std::string msg, const Token* tk) const {
    Diagnostic d;
    d.code = c;
    d.msg = std::move(msg);
    d.src = src_name_;
    if (tk) {
        d.line = tk->line;
        d.col = tk->col;
    } else {
        d.line = 0;
        d.col = 0;
    }
    return d;
}

std::string TextParser::unescape(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    bool esc = false;
    for (char ch : s) {
        if (esc) {
            switch (ch) {
                case 'n':
                    out += '\n';
                    break;
                case 't':
                    out += '\t';
                    break;
                case 'r':
                    out += '\r';
                    break;
                case '\\':
                    out += '\\';
                    break;
                case '"':
                    out += '"';
                    break;
                default:
                    out += ch;
                    break;
            }
            esc = false;
        } else if (ch == '\\')
            esc = true;
        else
            out += ch;
    }
    return out;
}

/* ---------- top-level parse_source returns Result<proto_t> ---------- */
proto_t TextParser::parse_source() {
    if (toks_.empty()) toks_.push_back({"", TokenType::END_OF_FILE, 1, 1});
    auto r = parse();
    // if(!r.ok) return Result<meow::core::proto_t>::Err(r.diag);
    if (!r.ok) throw std::runtime_error("???");
    for (auto& p : map_) {
        auto rr = resolve_labels(p.second);
        // if(!rr.ok) return Result<meow::core::proto_t>::Err(rr.diag);
        throw std::runtime_error("???");
    }
    // build final protos using build_proto
    for (auto& kv : map_) {
        try {
            auto pr = build_proto(kv.first, kv.second);
            final_protos_[kv.first] = pr;
        } catch (const std::exception& ex) {
            // return
            // Result<meow::core::proto_t>::Err(mkdiag(ErrCode::INTERNAL_ERROR,std::string("build_proto
            // error: ")+ex.what(),nullptr));

            throw std::runtime_error("???");
        }
    }
    // link proto refs inside constant pools
    for (auto& fp : final_protos_) {
        const Chunk& ck = fp.second->get_chunk();
        for (size_t i = 0; i < ck.get_pool_size(); ++i) {
            Value c = ck.get_constant(i);
            if (c.is_string()) {
                auto s = c.as_string()->c_str();
                auto it = final_protos_.find(std::string(s));
                if (it != final_protos_.end()) {
                    Value& ref = const_cast<Chunk&>(ck).get_constant_ref(i);
                    ref = Value(it->second);
                }
            }
        }
    }
    auto it = final_protos_.find("main");
    if (it == final_protos_.end()) {
        // return Result<meow::core::proto_t>::Err(mkdiag(ErrCode::MISSING_DIRECTIVE,"Không tìm thấy
        // hàm chính '@main'.",nullptr));
        throw std::runtime_error("???");
    }
    meow::core::proto_t mainp = it->second;
    heap_ = nullptr;
    toks_.clear();
    map_.clear();
    // return Result<meow::core::proto_t>::Ok(mainp);
    return mainp;
    // throw std::runtime_error("???");
}

const std::unordered_map<std::string, meow::core::proto_t>& TextParser::get_finalized_protos() const {
    return final_protos_;
}

/* ---------- parse() and stmt() return Result<void> ---------- */
Result<void> TextParser::parse() {
    while (!at_end()) {
        auto r = stmt();
        if (!r.ok) return r;
    }
    if (cur_) {
        const Token& lt = toks_[ti_ - 1];
        return Result<void>::Err(mkdiag(ErrCode::MISSING_DIRECTIVE, "Thiếu chỉ thị '.endfunc' cho hàm '" + cur_->name + "' bắt đầu tại dòng " + std::to_string(cur_->dir_line), &lt));
    }
    return Result<void>::Ok();
}

Result<void> TextParser::stmt() {
    const Token& tk = cur_tok();
    switch (tk.type) {
        case TokenType::DIR_FUNC:
            if (cur_) return Result<void>::Err(mkdiag(ErrCode::UNEXPECTED_TOKEN, "Không thể định nghĩa hàm lồng nhau.", &tk));
            return dir_func();
        case TokenType::DIR_ENDFUNC:
            if (!cur_) return Result<void>::Err(mkdiag(ErrCode::UNEXPECTED_TOKEN, "Chỉ thị '.endfunc' không mong đợi bên ngoài định nghĩa hàm.", &tk));
            return Result<void>::Err(mkdiag(ErrCode::INTERNAL_ERROR, "Gặp '.endfunc' tại stmt (logic bug).", &tk));
        case TokenType::DIR_REGISTERS:
        case TokenType::DIR_UPVALUES:
        case TokenType::DIR_CONST:
        case TokenType::DIR_UPVALUE:
            if (!cur_) return Result<void>::Err(mkdiag(ErrCode::UNEXPECTED_TOKEN, "Chỉ thị phải nằm trong định nghĩa hàm (.func).", &tk));
            if ((tk.type == TokenType::DIR_CONST || tk.type == TokenType::DIR_UPVALUE) && (!cur_->regs_defined || !cur_->up_defined))
                return Result<void>::Err(mkdiag(ErrCode::MISSING_DIRECTIVE, "Chỉ thị '.registers' và '.upvalues' phải được định nghĩa trước '" + std::string(tk.lexeme) + "'.", &tk));
            if (tk.type == TokenType::DIR_REGISTERS) return dir_registers();
            if (tk.type == TokenType::DIR_UPVALUES) return dir_upvalues();
            if (tk.type == TokenType::DIR_CONST) return dir_const();
            return dir_upvalue();
        case TokenType::LABEL_DEF:
            if (!cur_) return Result<void>::Err(mkdiag(ErrCode::UNEXPECTED_TOKEN, "Nhãn phải nằm trong định nghĩa hàm (.func).", &tk));
            if (!cur_->regs_defined || !cur_->up_defined) return Result<void>::Err(mkdiag(ErrCode::MISSING_DIRECTIVE, "Chỉ thị '.registers' và '.upvalues' phải được định nghĩa trước nhãn.", &tk));
            return label_def();
        case TokenType::OPCODE:
            if (!cur_) return Result<void>::Err(mkdiag(ErrCode::UNEXPECTED_TOKEN, "Lệnh phải nằm trong định nghĩa hàm (.func).", &tk));
            if (!cur_->regs_defined) return Result<void>::Err(mkdiag(ErrCode::MISSING_DIRECTIVE, "Chỉ thị '.registers' phải được định nghĩa trước lệnh đầu tiên.", &tk));
            if (!cur_->up_defined) return Result<void>::Err(mkdiag(ErrCode::MISSING_DIRECTIVE, "Chỉ thị '.upvalues' phải được định nghĩa trước lệnh đầu tiên.", &tk));
            return instr();
        case TokenType::IDENTIFIER:
            return Result<void>::Err(mkdiag(ErrCode::UNEXPECTED_TOKEN, "Token không mong đợi. Có thể thiếu directive hoặc opcode?", &tk));
        case TokenType::NUMBER_INT:
        case TokenType::NUMBER_FLOAT:
        case TokenType::STRING:
            return Result<void>::Err(mkdiag(ErrCode::UNEXPECTED_TOKEN, "Giá trị literal không hợp lệ ở đây. Có thể thiếu chỉ thị '.const'?", &tk));
        case TokenType::END_OF_FILE:
            return Result<void>::Ok();
        case TokenType::UNKNOWN:
            return Result<void>::Err(mkdiag(ErrCode::INVALID_IDENT, "Token không hợp lệ hoặc ký tự không nhận dạng.", &tk));
        default:
            break;
    }
    return Result<void>::Ok();
}

/* ---------- directives ---------- */
Result<void> TextParser::dir_func() {
    const Token& f = expect_tok(TokenType::DIR_FUNC, "internal");
    const Token& n = expect_tok(TokenType::IDENTIFIER, "internal");
    std::string fname(n.lexeme);
    if (fname.empty() || (fname[0] != '@' && !std::isalpha(fname[0]) && fname[0] != '_')) return Result<void>::Err(mkdiag(ErrCode::INVALID_IDENT, "Tên hàm không hợp lệ.", &n));
    std::string key = (fname[0] == '@') ? fname.substr(1) : fname;
    if (key.empty()) return Result<void>::Err(mkdiag(ErrCode::INVALID_IDENT, "Tên hàm không hợp lệ (chỉ có '@').", &n));
    if (map_.count(key)) return Result<void>::Err(mkdiag(ErrCode::INVALID_IDENT, "Hàm '" + fname + "' đã được định nghĩa.", &n));
    auto [it, ins] = map_.emplace(key, PData{});
    cur_ = &it->second;
    cur_->name = key;
    cur_->dir_line = f.line;
    cur_->dir_col = f.col;
    // require registers & upvalues right away
    auto r = dir_registers();
    if (!r.ok) return r;
    r = dir_upvalues();
    if (!r.ok) return r;
    while (!at_end() && cur_ && cur_->name.size() && cur_tok().type != TokenType::DIR_ENDFUNC) {
        auto s = stmt();
        if (!s.ok) return s;
    }
    // consume .endfunc
    if (cur_tok().type != TokenType::DIR_ENDFUNC) return Result<void>::Err(mkdiag(ErrCode::MISSING_DIRECTIVE, "Mong đợi '.endfunc' để kết thúc hàm '" + fname + "'.", &cur_tok()));
    expect_tok(TokenType::DIR_ENDFUNC, "internal");
    cur_ = nullptr;
    return Result<void>::Ok();
}

Result<void> TextParser::dir_registers() {
    expect_tok(TokenType::DIR_REGISTERS, "internal");
    if (!cur_) return Result<void>::Err(mkdiag(ErrCode::INTERNAL_ERROR, "current proto null in .registers", nullptr));
    if (cur_->regs_defined) return Result<void>::Err(mkdiag(ErrCode::INVALID_IDENT, "Chỉ thị '.registers' đã được định nghĩa cho hàm này.", &cur_tok()));
    const Token& n = expect_tok(TokenType::NUMBER_INT, "internal");
    uint64_t v;
    auto r = std::from_chars(n.lexeme.data(), n.lexeme.data() + n.lexeme.size(), v);
    if (r.ec != std::errc() || r.ptr != n.lexeme.data() + n.lexeme.size() || v > std::numeric_limits<size_t>::max())
        return Result<void>::Err(mkdiag(ErrCode::INVALID_NUMBER, "Số lượng thanh ghi không hợp lệ.", &n));
    cur_->nreg = static_cast<size_t>(v);
    cur_->regs_defined = true;
    return Result<void>::Ok();
}

Result<void> TextParser::dir_upvalues() {
    expect_tok(TokenType::DIR_UPVALUES, "internal");
    if (!cur_) return Result<void>::Err(mkdiag(ErrCode::INTERNAL_ERROR, "current proto null in .upvalues", nullptr));
    if (cur_->up_defined) return Result<void>::Err(mkdiag(ErrCode::INVALID_IDENT, "Chỉ thị '.upvalues' đã được định nghĩa cho hàm này.", &cur_tok()));
    const Token& n = expect_tok(TokenType::NUMBER_INT, "internal");
    uint64_t v;
    auto r = std::from_chars(n.lexeme.data(), n.lexeme.data() + n.lexeme.size(), v);
    if (r.ec != std::errc() || r.ptr != n.lexeme.data() + n.lexeme.size() || v > std::numeric_limits<size_t>::max())
        return Result<void>::Err(mkdiag(ErrCode::INVALID_NUMBER, "Số lượng upvalue không hợp lệ.", &n));
    cur_->nup = static_cast<size_t>(v);
    cur_->updesc.resize(cur_->nup);
    cur_->up_defined = true;
    return Result<void>::Ok();
}

Result<void> TextParser::dir_const() {
    expect_tok(TokenType::DIR_CONST, "internal");
    if (!cur_) return Result<void>::Err(mkdiag(ErrCode::INTERNAL_ERROR, "current proto null in .const", nullptr));
    auto rv = parse_const_val();
    if (!rv.ok) return Result<void>::Err(rv.diag);
    cur_->add_const(rv.val);
    return Result<void>::Ok();
}

Result<void> TextParser::dir_upvalue() {
    expect_tok(TokenType::DIR_UPVALUE, "internal");
    if (!cur_) return Result<void>::Err(mkdiag(ErrCode::INTERNAL_ERROR, "current proto null in .upvalue", nullptr));
    if (!cur_->up_defined) return Result<void>::Err(mkdiag(ErrCode::MISSING_DIRECTIVE, "Chỉ thị '.upvalues' phải được định nghĩa trước '.upvalue'.", &toks_[ti_ - 1]));
    const Token& idxT = expect_tok(TokenType::NUMBER_INT, "internal");
    uint64_t idx;
    auto r = std::from_chars(idxT.lexeme.data(), idxT.lexeme.data() + idxT.lexeme.size(), idx);
    if (r.ec != std::errc() || r.ptr != idxT.lexeme.data() + idxT.lexeme.size() || idx >= cur_->nup)
        return Result<void>::Err(mkdiag(ErrCode::OUT_OF_RANGE, "Chỉ số upvalue không hợp lệ hoặc vượt quá số lượng đã khai báo.", &idxT));
    const Token& typT = expect_tok(TokenType::IDENTIFIER, "internal");
    bool is_local;
    if (typT.lexeme == "local")
        is_local = true;
    else if (typT.lexeme == "parent")
        is_local = false;
    else
        return Result<void>::Err(mkdiag(ErrCode::INVALID_IDENT, "Loại upvalue không hợp lệ. Phải là 'local' hoặc 'parent'.", &typT));
    const Token& slotT = expect_tok(TokenType::NUMBER_INT, "internal");
    uint64_t si;
    auto r2 = std::from_chars(slotT.lexeme.data(), slotT.lexeme.data() + slotT.lexeme.size(), si);
    if (r2.ec != std::errc() || r2.ptr != slotT.lexeme.data() + slotT.lexeme.size()) return Result<void>::Err(mkdiag(ErrCode::INVALID_NUMBER, "Chỉ số slot không hợp lệ.", &slotT));
    size_t slot = static_cast<size_t>(si);
    if (is_local && slot >= cur_->nreg) return Result<void>::Err(mkdiag(ErrCode::OUT_OF_RANGE, "Chỉ số slot cho upvalue 'local' vượt quá số thanh ghi.", &slotT));
    cur_->updesc[static_cast<size_t>(idx)] = UpvalueDesc(is_local, slot);
    return Result<void>::Ok();
}

Result<void> TextParser::label_def() {
    const Token& L = expect_tok(TokenType::LABEL_DEF, "internal");
    if (!cur_) return Result<void>::Err(mkdiag(ErrCode::INTERNAL_ERROR, "current proto null in label", nullptr));
    std::string_view nm = L.lexeme;
    if (cur_->labels.count(nm)) return Result<void>::Err(mkdiag(ErrCode::INVALID_IDENT, "Nhãn đã được định nghĩa trong hàm này.", &L));
    cur_->labels[nm] = cur_->code.size();
    return Result<void>::Ok();
}

/* ---------- instruction parsing (Result-based) ---------- */
Result<void> TextParser::instr() {
    const Token& opT = expect_tok(TokenType::OPCODE, "internal");
    if (!cur_) return Result<void>::Err(mkdiag(ErrCode::INTERNAL_ERROR, "current proto null in instr", nullptr));
    auto it = OP_MAP.find(opT.lexeme);
    if (it == OP_MAP.end()) return Result<void>::Err(mkdiag(ErrCode::INVALID_IDENT, "Opcode không hợp lệ.", &opT));
    OpCode op = it->second;
    PData& d = *cur_;
    size_t pos = d.code.size();
    d.wb(static_cast<uint8_t>(op));

    auto rd_u16 = [&](const Token*& outTk) -> Result<uint16_t> {
        const Token& tk = expect_tok(TokenType::NUMBER_INT, "internal");
        outTk = &tk;
        uint64_t v;
        auto r = std::from_chars(tk.lexeme.data(), tk.lexeme.data() + tk.lexeme.size(), v);
        if (r.ec != std::errc() || r.ptr != tk.lexeme.data() + tk.lexeme.size() || v > UINT16_MAX)
            return Result<uint16_t>::Err(mkdiag(ErrCode::OUT_OF_RANGE, "Đối số phải là số nguyên 16-bit không dấu hợp lệ.", &tk));
        return Result<uint16_t>::Ok(static_cast<uint16_t>(v));
    };
    auto rd_i64 = [&]() -> Result<int64_t> {
        const Token& tk = cur_tok();
        int64_t v;
        auto r = std::from_chars(tk.lexeme.data(), tk.lexeme.data() + tk.lexeme.size(), v);
        if (r.ec == std::errc() && r.ptr == tk.lexeme.data() + tk.lexeme.size()) {
            adv();
            return Result<int64_t>::Ok(v);
        }
        if (tk.type == TokenType::NUMBER_FLOAT && op == OpCode::LOAD_INT) {
            adv();
            try {
                double dv = std::stod(std::string(tk.lexeme));
                if (dv > static_cast<double>(std::numeric_limits<int64_t>::max()) || dv < static_cast<double>(std::numeric_limits<int64_t>::min()))
                    return Result<int64_t>::Err(mkdiag(ErrCode::OUT_OF_RANGE, "Giá trị số thực quá lớn/nhỏ để chuyển đổi thành số nguyên 64-bit.", &tk));
                return Result<int64_t>::Ok(static_cast<int64_t>(dv));
            } catch (...) {
                return Result<int64_t>::Err(mkdiag(ErrCode::INVALID_NUMBER, "Không thể chuyển đổi số thực thành số nguyên 64-bit.", &tk));
            }
        }
        return Result<int64_t>::Err(mkdiag(ErrCode::INVALID_NUMBER, "Mong đợi đối số là số nguyên 64-bit.", &tk));
    };
    auto rd_f64 = [&]() -> Result<double> {
        const Token& tk = cur_tok();
        if (tk.type != TokenType::NUMBER_FLOAT && tk.type != TokenType::NUMBER_INT) return Result<double>::Err(mkdiag(ErrCode::INVALID_NUMBER, "Mong đợi đối số là số thực hoặc số nguyên.", &tk));
        adv();
        try {
            size_t p = 0;
            double v = std::stod(std::string(tk.lexeme), &p);
            if (p != tk.lexeme.size()) return Result<double>::Err(mkdiag(ErrCode::INVALID_NUMBER, "Đối số số thực không hợp lệ.", &tk));
            return Result<double>::Ok(v);
        } catch (...) {
            return Result<double>::Err(mkdiag(ErrCode::INVALID_NUMBER, "Đối số số thực không hợp lệ.", &tk));
        }
    };
    auto rd_addr_or_lbl = [&]() -> Result<void> {
        const Token& tk = cur_tok();
        if (tk.type == TokenType::NUMBER_INT) {
            const Token* tmp = nullptr;
            auto r = rd_u16(tmp);
            if (!r.ok) return Result<void>::Err(r.diag);
            d.wu16(r.val);
            return Result<void>::Ok();
        } else if (tk.type == TokenType::IDENTIFIER) {
            adv();
            size_t patch_at = d.code.size();
            d.wu16(0xDEAD);
            d.pending.emplace_back(pos, patch_at, tk.lexeme);
            return Result<void>::Ok();
        } else
            return Result<void>::Err(mkdiag(ErrCode::INVALID_NUMBER, "Mong đợi nhãn hoặc địa chỉ cho lệnh nhảy.", &tk));
    };

    // big switch like before, but we use Results to propagate errors
    switch (op) {
        case OpCode::LOAD_NULL:
        case OpCode::LOAD_TRUE:
        case OpCode::LOAD_FALSE:
        case OpCode::POP_TRY:
            return Result<void>::Ok();
        case OpCode::IMPORT_ALL: {
            const Token* t = nullptr;
            auto r = rd_u16(t);
            if (!r.ok) return Result<void>::Err(r.diag);
            d.wu16(r.val);
            return Result<void>::Ok();
        }
        case OpCode::LOAD_CONST: {
            const Token* t = nullptr;
            auto r = rd_u16(t);
            if (!r.ok) return Result<void>::Err(r.diag);
            auto rc = parse_const_val();
            if (!rc.ok) return Result<void>::Err(rc.diag);
            size_t idx = d.add_const(rc.val);
            if (idx > UINT16_MAX) return Result<void>::Err(mkdiag(ErrCode::TOO_MANY_CONST, "Quá nhiều hằng số, chỉ số vượt quá giới hạn 16-bit.", &toks_[ti_ - 1]));
            d.wu16(r.val);
            d.wu16(static_cast<uint16_t>(idx));
            return Result<void>::Ok();
        }
        case OpCode::GET_GLOBAL:
        case OpCode::NEW_CLASS:
        case OpCode::GET_SUPER:
        case OpCode::CLOSURE:
        case OpCode::IMPORT_MODULE: {
            const Token* t = nullptr;
            auto r = rd_u16(t);
            if (!r.ok) return Result<void>::Err(r.diag);
            const Token& nt = cur_tok();
            if (!(nt.type == TokenType::STRING || nt.type == TokenType::IDENTIFIER))
                return Result<void>::Err(mkdiag(ErrCode::UNEXPECTED_TOKEN, "Mong đợi tên (chuỗi hoặc @Proto) làm đối số thứ hai.", &nt));
            auto rv = parse_const_val();
            if (!rv.ok) return Result<void>::Err(rv.diag);
            size_t ii = d.add_const(rv.val);
            if (ii > UINT16_MAX) return Result<void>::Err(mkdiag(ErrCode::TOO_MANY_CONST, "Quá nhiều hằng số (tên), chỉ số vượt quá giới hạn 16-bit.", &nt));
            d.wu16(r.val);
            d.wu16(static_cast<uint16_t>(ii));
            return Result<void>::Ok();
        }
        case OpCode::EXPORT:
        case OpCode::SET_GLOBAL: {
            const Token& nt = cur_tok();
            if (nt.type != TokenType::STRING) return Result<void>::Err(mkdiag(ErrCode::UNEXPECTED_TOKEN, "Mong đợi tên (chuỗi) làm đối số đầu.", &nt));
            auto rv = parse_const_val();
            if (!rv.ok) return Result<void>::Err(rv.diag);
            size_t ii = d.add_const(rv.val);
            if (ii > UINT16_MAX) return Result<void>::Err(mkdiag(ErrCode::TOO_MANY_CONST, "Quá nhiều hằng số (tên), chỉ số vượt quá giới hạn 16-bit.", &nt));
            const Token* tmp = nullptr;
            auto r = rd_u16(tmp);
            if (!r.ok) return Result<void>::Err(r.diag);
            d.wu16(static_cast<uint16_t>(ii));
            d.wu16(r.val);
            return Result<void>::Ok();
        }
        case OpCode::MOVE:
        case OpCode::NEG:
        case OpCode::NOT:
        case OpCode::BIT_NOT:
        case OpCode::GET_UPVALUE:
        case OpCode::NEW_INSTANCE:
        case OpCode::GET_KEYS:
        case OpCode::GET_VALUES: {
            const Token* t = nullptr;
            auto a = rd_u16(t);
            if (!a.ok) return Result<void>::Err(a.diag);
            const Token* t2 = nullptr;
            auto b = rd_u16(t2);
            if (!b.ok) return Result<void>::Err(b.diag);
            d.wu16(a.val);
            d.wu16(b.val);
            return Result<void>::Ok();
        }
        case OpCode::SET_UPVALUE: {
            const Token* t = nullptr;
            auto a = rd_u16(t);
            if (!a.ok) return Result<void>::Err(a.diag);
            const Token* t2 = nullptr;
            auto b = rd_u16(t2);
            if (!b.ok) return Result<void>::Err(b.diag);
            d.wu16(a.val);
            d.wu16(b.val);
            return Result<void>::Ok();
        }
        case OpCode::CLOSE_UPVALUES: {
            const Token* t = nullptr;
            auto a = rd_u16(t);
            if (!a.ok) return Result<void>::Err(a.diag);
            d.wu16(a.val);
            return Result<void>::Ok();
        }
        case OpCode::LOAD_INT: {
            const Token* t = nullptr;
            auto dst = rd_u16(t);
            if (!dst.ok) return Result<void>::Err(dst.diag);
            auto vi = rd_i64();
            if (!vi.ok) return Result<void>::Err(vi.diag);
            d.wu16(dst.val);
            d.wu64(std::bit_cast<uint64_t>(vi.val));
            return Result<void>::Ok();
        }
        case OpCode::LOAD_FLOAT: {
            const Token* t = nullptr;
            auto dst = rd_u16(t);
            if (!dst.ok) return Result<void>::Err(dst.diag);
            auto fv = rd_f64();
            if (!fv.ok) return Result<void>::Err(fv.diag);
            d.wu16(dst.val);
            d.wf64(fv.val);
            return Result<void>::Ok();
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
        case OpCode::RSHIFT:
        case OpCode::GET_INDEX:
        case OpCode::NEW_ARRAY:
        case OpCode::NEW_HASH: {
            const Token* t = nullptr;
            auto a = rd_u16(t);
            if (!a.ok) return Result<void>::Err(a.diag);
            const Token* t2 = nullptr;
            auto b = rd_u16(t2);
            if (!b.ok) return Result<void>::Err(b.diag);
            const Token* t3 = nullptr;
            auto c = rd_u16(t3);
            if (!c.ok) return Result<void>::Err(c.diag);
            d.wu16(a.val);
            d.wu16(b.val);
            d.wu16(c.val);
            return Result<void>::Ok();
        }
        case OpCode::GET_PROP:
        case OpCode::SET_METHOD:
        case OpCode::GET_EXPORT: {
            const Token* t = nullptr;
            auto a = rd_u16(t);
            if (!a.ok) return Result<void>::Err(a.diag);
            const Token* t2 = nullptr;
            auto b = rd_u16(t2);
            if (!b.ok) return Result<void>::Err(b.diag);
            const Token& nt = cur_tok();
            if (nt.type != TokenType::STRING) return Result<void>::Err(mkdiag(ErrCode::UNEXPECTED_TOKEN, "Mong đợi tên (chuỗi) làm đối số thứ ba.", &nt));
            auto rv = parse_const_val();
            if (!rv.ok) return Result<void>::Err(rv.diag);
            size_t ii = d.add_const(rv.val);
            if (ii > UINT16_MAX) return Result<void>::Err(mkdiag(ErrCode::TOO_MANY_CONST, "Quá nhiều hằng số (tên).", &nt));
            d.wu16(a.val);
            d.wu16(b.val);
            d.wu16(static_cast<uint16_t>(ii));
            return Result<void>::Ok();
        }
        case OpCode::SET_INDEX: {
            const Token* t = nullptr;
            auto a = rd_u16(t);
            if (!a.ok) return Result<void>::Err(a.diag);
            const Token* t2 = nullptr;
            auto b = rd_u16(t2);
            if (!b.ok) return Result<void>::Err(b.diag);
            const Token* t3 = nullptr;
            auto c = rd_u16(t3);
            if (!c.ok) return Result<void>::Err(c.diag);
            d.wu16(a.val);
            d.wu16(b.val);
            d.wu16(c.val);
            return Result<void>::Ok();
        }
        case OpCode::SET_PROP: {
            const Token* t = nullptr;
            auto a = rd_u16(t);
            if (!a.ok) return Result<void>::Err(a.diag);
            const Token& nt = cur_tok();
            if (nt.type != TokenType::STRING) return Result<void>::Err(mkdiag(ErrCode::UNEXPECTED_TOKEN, "Mong đợi tên (chuỗi) làm đối số thứ hai.", &nt));
            auto rv = parse_const_val();
            if (!rv.ok) return Result<void>::Err(rv.diag);
            size_t ii = d.add_const(rv.val);
            if (ii > UINT16_MAX) return Result<void>::Err(mkdiag(ErrCode::TOO_MANY_CONST, "Quá nhiều hằng số (tên).", &nt));
            const Token* t2 = nullptr;
            auto b = rd_u16(t2);
            if (!b.ok) return Result<void>::Err(b.diag);
            d.wu16(a.val);
            d.wu16(static_cast<uint16_t>(ii));
            d.wu16(b.val);
            return Result<void>::Ok();
        }
        case OpCode::INHERIT: {
            const Token* ta = nullptr;
            auto a = rd_u16(ta);
            if (!a.ok) return Result<void>::Err(a.diag);
            const Token* tb = nullptr;
            auto b = rd_u16(tb);
            if (!b.ok) return Result<void>::Err(b.diag);
            d.wu16(a.val);
            d.wu16(b.val);
            return Result<void>::Ok();
        }
        case OpCode::JUMP:
        case OpCode::SETUP_TRY: {
            auto r = rd_addr_or_lbl();
            if (!r.ok) return r;
            return Result<void>::Ok();
        }
        case OpCode::JUMP_IF_FALSE:
        case OpCode::JUMP_IF_TRUE: {
            const Token* tmp = nullptr;
            auto a = rd_u16(tmp);
            if (!a.ok) return Result<void>::Err(a.diag);
            d.wu16(a.val);
            auto rr = rd_addr_or_lbl();
            if (!rr.ok) return rr;
            return Result<void>::Ok();
        }
        case OpCode::CALL: {
            const Token* ta = nullptr;
            auto a = rd_u16(ta);
            if (!a.ok) return Result<void>::Err(a.diag);
            const Token* tb = nullptr;
            auto b = rd_u16(tb);
            if (!b.ok) return Result<void>::Err(b.diag);
            const Token* tc = nullptr;
            auto c = rd_u16(tc);
            if (!c.ok) return Result<void>::Err(c.diag);
            const Token* td = nullptr;
            auto dpar = rd_u16(td);
            if (!dpar.ok) return Result<void>::Err(dpar.diag);
            d.wu16(a.val);
            d.wu16(b.val);
            d.wu16(c.val);
            d.wu16(dpar.val);
            return Result<void>::Ok();
        }
        case OpCode::CALL_VOID: {
            const Token* ta = nullptr;
            auto a = rd_u16(ta);
            if (!a.ok) return Result<void>::Err(a.diag);
            const Token* tb = nullptr;
            auto b = rd_u16(tb);
            if (!b.ok) return Result<void>::Err(b.diag);
            const Token* tc = nullptr;
            auto c = rd_u16(tc);
            if (!c.ok) return Result<void>::Err(c.diag);
            d.wu16(a.val);
            d.wu16(b.val);
            d.wu16(c.val);
            return Result<void>::Ok();
        }
        case OpCode::RETURN: {
            const Token& rt = cur_tok();
            if (rt.type == TokenType::NUMBER_INT && rt.lexeme == "-1") {
                adv();
                d.wu16(0xFFFF);
                return Result<void>::Ok();
            }
            if (rt.type == TokenType::IDENTIFIER && (rt.lexeme == "FFFF" || rt.lexeme == "ffff")) {
                adv();
                d.wu16(0xFFFF);
                return Result<void>::Ok();
            }
            if (rt.type == TokenType::NUMBER_INT) {
                const Token* tmp = nullptr;
                auto a = rd_u16(tmp);
                if (!a.ok) return Result<void>::Err(a.diag);
                d.wu16(a.val);
                return Result<void>::Ok();
            }
            return Result<void>::Err(mkdiag(ErrCode::UNEXPECTED_TOKEN, "Mong đợi thanh ghi trả về (số nguyên không âm, -1, hoặc FFFF).", &rt));
        }
        case OpCode::THROW: {
            const Token* tmp = nullptr;
            auto a = rd_u16(tmp);
            if (!a.ok) return Result<void>::Err(a.diag);
            d.wu16(a.val);
            return Result<void>::Ok();
        }
        case OpCode::HALT:
            return Result<void>::Ok();
        default:
            return Result<void>::Err(mkdiag(ErrCode::INTERNAL_ERROR, "Opcode chưa được hỗ trợ xử lý đối số trong parser.", &opT));
    }
}

/* ---------- constants + pool building ---------- */
Result<meow::core::value_t> TextParser::parse_const_val() {
    const Token& tk = cur_tok();
    if (tk.type == TokenType::STRING) {
        adv();
        if (tk.lexeme.size() < 2 || tk.lexeme.front() != '"' || tk.lexeme.back() != '"')
            return Result<meow::core::value_t>::Err(mkdiag(ErrCode::INVALID_IDENT, "Chuỗi literal không hợp lệ (thiếu dấu \").", &tk));
        auto inner = tk.lexeme.substr(1, tk.lexeme.size() - 2);
        return Result<meow::core::value_t>::Ok(Value(heap_->new_string(unescape(inner))));
    }
    if (tk.type == TokenType::NUMBER_INT) {
        adv();
        int64_t v;
        auto r = std::from_chars(tk.lexeme.data(), tk.lexeme.data() + tk.lexeme.size(), v);
        if (r.ec != std::errc() || r.ptr != tk.lexeme.data() + tk.lexeme.size()) return Result<meow::core::value_t>::Err(mkdiag(ErrCode::INVALID_NUMBER, "Số nguyên literal không hợp lệ.", &tk));
        return Result<meow::core::value_t>::Ok(Value(v));
    }
    if (tk.type == TokenType::NUMBER_FLOAT) {
        adv();
        try {
            size_t p = 0;
            double d = std::stod(std::string(tk.lexeme), &p);
            if (p != tk.lexeme.size()) return Result<meow::core::value_t>::Err(mkdiag(ErrCode::INVALID_NUMBER, "Số thực literal không hợp lệ.", &tk));
            return Result<meow::core::value_t>::Ok(Value(d));
        } catch (...) {
            return Result<meow::core::value_t>::Err(mkdiag(ErrCode::INVALID_NUMBER, "Số thực literal không hợp lệ.", &tk));
        }
    }
    if (tk.type == TokenType::IDENTIFIER) {
        adv();
        if (tk.lexeme == "true") return Result<meow::core::value_t>::Ok(Value(true));
        if (tk.lexeme == "false") return Result<meow::core::value_t>::Ok(Value(false));
        if (tk.lexeme == "null") return Result<meow::core::value_t>::Ok(Value(null_t{}));
        if (!tk.lexeme.empty() && tk.lexeme.front() == '@') {
            auto nm = tk.lexeme.substr(1);
            if (nm.empty()) return Result<meow::core::value_t>::Err(mkdiag(ErrCode::INVALID_IDENT, "Tên proto tham chiếu không được rỗng (chỉ có '@').", &tk));
            return Result<meow::core::value_t>::Ok(Value(heap_->new_string(std::string("::proto_ref::") + std::string(nm))));
        }
        return Result<meow::core::value_t>::Err(mkdiag(ErrCode::INVALID_IDENT, "Identifier không hợp lệ cho giá trị hằng số.", &tk));
    }
    return Result<meow::core::value_t>::Err(mkdiag(ErrCode::UNEXPECTED_TOKEN, "Token không mong đợi cho giá trị hằng số.", &tk));
}

Result<void> TextParser::resolve_labels(PData& d) {
    for (const auto& pj : d.pending) {
        size_t patch = std::get<1>(pj);
        std::string_view lbl = std::get<2>(pj);
        auto it = d.labels.find(lbl);
        if (it == d.labels.end()) return Result<void>::Err(mkdiag(ErrCode::LABEL_NOT_FOUND, "Không tìm thấy nhãn '" + std::string(lbl) + "' trong hàm '" + d.name + "'.", nullptr));
        size_t addr = it->second;
        if (addr > UINT16_MAX) return Result<void>::Err(mkdiag(ErrCode::OUT_OF_RANGE, "Địa chỉ nhãn vượt quá giới hạn 16-bit.", nullptr));
        if (!d.patch_u16(patch, static_cast<uint16_t>(addr))) return Result<void>::Err(mkdiag(ErrCode::INTERNAL_ERROR, "Không thể patch địa chỉ nhảy.", nullptr));
    }
    d.pending.clear();
    return Result<void>::Ok();
}

std::vector<meow::core::value_t> TextParser::build_final_const_pool(PData& d) {
    const std::string pref = "::proto_ref::";
    std::vector<meow::core::value_t> out;
    out.reserve(d.tmp_consts.size());
    for (auto& c : d.tmp_consts) {
        if (c.is_string()) {
            auto s = c.as_string()->c_str();
            std::string_view sv = s;
            if (sv.size() > pref.size() && sv.substr(0, pref.size()) == pref) {
                out.push_back(Value(heap_->new_string(std::string(sv.substr(pref.size())))));
            } else
                out.push_back(c);
        } else
            out.push_back(c);
    }
    return out;
}

/* build_proto: single responsibility */
meow::core::proto_t TextParser::build_proto(const std::string& name, PData& d) {
    string_t nm = heap_->new_string(name);
    auto final_consts = build_final_const_pool(d);
    Chunk ck(std::move(d.code), std::move(final_consts));
    auto udesc = std::move(d.updesc);
    if (udesc.size() != d.nup) throw std::runtime_error("Lỗi nội bộ: upvalue desc mismatch for " + name);
    proto_t p = heap_->new_proto(d.nreg, d.nup, nm, std::move(ck), std::move(udesc));
    if (!p) throw std::runtime_error("Lỗi cấp phát proto cho hàm " + name);
    return p;
}

}  // namespace meow::loader
