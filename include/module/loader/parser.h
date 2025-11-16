#pragma once
#include "common/pch.h"
#include "core/definitions.h"
#include "core/op_codes.h"
#include "core/type.h"
#include "loader/tokens.h"
#include "runtime/chunk.h"
#include "core/objects/function.h"

namespace meow::memory {
class MemoryManager;
}
namespace meow::loader {

enum class ErrCode { OK = 0, UNEXPECTED_TOKEN, INVALID_NUMBER, OUT_OF_RANGE, INVALID_IDENT, MISSING_DIRECTIVE, LABEL_NOT_FOUND, TOO_MANY_CONST, INTERNAL_ERROR };

struct Diagnostic {
    ErrCode code = ErrCode::OK;
    std::string msg;
    std::string src;
    size_t line = 0;
    size_t col = 0;
};

template <typename T>
struct Result {
    bool ok = false;
    T val{};
    Diagnostic diag{};
    static Result<T> Ok(T v) {
        Result<T> r;
        r.ok = true;
        r.val = std::move(v);
        return r;
    }
    static Result<T> Err(Diagnostic d) {
        Result<T> r;
        r.ok = false;
        r.diag = std::move(d);
        return r;
    }
};
template <>
struct Result<void> {
    bool ok = false;
    Diagnostic diag{};
    static Result<void> Ok() {
        Result<void> r;
        r.ok = true;
        return r;
    }
    static Result<void> Err(Diagnostic d) {
        Result<void> r;
        r.ok = false;
        r.diag = std::move(d);
        return r;
    }
};

class TextParser {
   public:
    explicit TextParser(meow::memory::MemoryManager* heap, const std::vector<Token>& t, std::string_view s) noexcept;
    explicit TextParser(meow::memory::MemoryManager* heap, std::vector<Token>&& t, std::string_view s) noexcept;
    TextParser(const TextParser&) = delete;
    TextParser(TextParser&&) = default;
    TextParser& operator=(const TextParser&) = delete;
    TextParser& operator=(TextParser&&) = delete;
    ~TextParser() noexcept = default;

    // new: returns Result<proto_t> with Diagnostic on error
    meow::core::proto_t parse_source();
    [[nodiscard]] const std::unordered_map<std::string, meow::core::proto_t>& get_finalized_protos() const;

   private:
    meow::memory::MemoryManager* heap_{nullptr};
    std::string src_name_;
    std::vector<Token> toks_;
    size_t ti_{0};

    struct PData {
        std::string name;
        size_t nreg = 0;
        size_t nup = 0;
        std::vector<uint8_t> code;
        std::vector<meow::core::value_t> tmp_consts;
        std::vector<meow::core::objects::UpvalueDesc> updesc;
        std::unordered_map<std::string_view, size_t> labels;
        std::vector<std::tuple<size_t, size_t, std::string_view>> pending;
        bool regs_defined = false;
        bool up_defined = false;
        size_t dir_line = 0;
        size_t dir_col = 0;
        size_t add_const(meow::core::param_t v) {
            size_t i = tmp_consts.size();
            tmp_consts.push_back(v);
            return i;
        }
        void wb(uint8_t b) {
            code.push_back(b);
        }
        void wu16(uint16_t x) {
            code.push_back(uint8_t(x & 0xFF));
            code.push_back(uint8_t((x >> 8) & 0xFF));
        }
        void wu64(uint64_t x) {
            for (int i = 0; i < 8; ++i) code.push_back(uint8_t((x >> (i * 8)) & 0xFF));
        }
        void wf64(double d) {
            wu64(std::bit_cast<uint64_t>(d));
        }
        bool patch_u16(size_t ofs, uint16_t v) {
            if (ofs + 1 >= code.size()) return false;
            code[ofs] = uint8_t(v & 0xFF);
            code[ofs + 1] = uint8_t((v >> 8) & 0xFF);
            return true;
        }
    };

    PData* cur_{nullptr};
    std::unordered_map<std::string, PData> map_;
    std::unordered_map<std::string, meow::core::proto_t> final_protos_;

    // parsing API now returns Result<>
    Result<void> parse();
    Result<void> stmt();
    Result<void> dir_func();
    Result<void> dir_registers();
    Result<void> dir_upvalues();
    Result<void> dir_const();
    Result<void> dir_upvalue();
    Result<void> label_def();
    Result<void> instr();
    Result<meow::core::value_t> parse_const_val();

    [[nodiscard]] const Token& cur_tok() const;
    [[nodiscard]] const Token& peek_tok(size_t off = 1) const;
    [[nodiscard]] bool at_end() const;
    void adv();
    const Token& expect_tok(TokenType expected,
                            std::string_view err);  // legacy: throws if internal misuse
    bool match_tok(TokenType t);

    static std::string unescape(std::string_view s);

    // label/linking/building helpers
    Result<void> resolve_labels(PData& d);
    std::vector<meow::core::value_t> build_final_const_pool(PData& d);
    meow::core::proto_t build_proto(const std::string& name, PData& d);

    // error helper
    Diagnostic mkdiag(ErrCode code, std::string msg, const Token* tk = nullptr) const;
};
}  // namespace meow::loader
