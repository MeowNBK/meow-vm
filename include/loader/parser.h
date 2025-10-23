#pragma once

#include "common/pch.h"
#include "core/objects/function.h"
#include "core/definitions.h"
#include "core/op_codes.h"
#include "core/type.h"
#include "core/value.h"
#include "runtime/chunk.h"

namespace meow::memory {
class MemoryManager;
}
namespace meow::loader {
class TextParser {
   public:
    explicit TextParser(meow::memory::MemoryManager* heap) noexcept;
    TextParser(const TextParser&) = delete;
    TextParser(TextParser&&) = default;
    TextParser& operator=(const TextParser&) = delete;
    TextParser& operator=(TextParser&&) = delete;
    ~TextParser() noexcept = default;
    meow::core::proto_t parse_file(std::string_view filepath);
    meow::core::proto_t parse_source(std::string_view source,
                                     std::string_view source_name = "<string>");
    [[nodiscard]] const std::unordered_map<std::string, meow::core::proto_t>& get_finalized_protos()
        const;

   private:
    meow::memory::MemoryManager* heap_ = nullptr;
    std::string current_source_name_;
    std::vector<Token> tokens_;
    size_t current_token_index_ = 0;
    struct ProtoBuildData {
        std::string name;
        size_t num_registers = 0;
        size_t num_upvalues = 0;
        std::vector<uint8_t> temp_code;
        std::vector<meow::core::Value> temp_constants;
        std::vector<meow::core::objects::UpvalueDesc> upvalue_descs;
        std::unordered_map<std::string_view, size_t> labels;
        std::vector<std::tuple<size_t, size_t, std::string_view>> pending_jumps;
        bool registers_defined = false;
        bool upvalues_defined = false;
        size_t func_directive_line = 0;
        size_t func_directive_col = 0;
        size_t add_temp_constant(const meow::core::Value& value) {
            size_t new_index = temp_constants.size();
            temp_constants.push_back(value);
            return new_index;
        }
        void write_byte(uint8_t byte) { temp_code.push_back(byte); }
        void write_u16(uint16_t val) {
            temp_code.push_back(static_cast<uint8_t>(val & 0xFF));
            temp_code.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
        }
        void write_u64(uint64_t val) {
            for (int i = 0; i < 8; ++i) {
                temp_code.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
            }
        }
        void write_f64(double val) { write_u64(std::bit_cast<uint64_t>(val)); }
        bool patch_u16(size_t offset, uint16_t value) {
            if (offset + 1 >= temp_code.size()) return false;
            temp_code[offset] = static_cast<uint8_t>(value & 0xFF);
            temp_code[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
            return true;
        }
    };
    ProtoBuildData* current_proto_data_ = nullptr;
    std::unordered_map<std::string, ProtoBuildData> build_data_map_;
    std::unordered_map<std::string, meow::core::proto_t> finalized_protos_;
    void parse();
    void parse_statement();
    void parse_func_directive();
    void parse_registers_directive();
    void parse_upvalues_directive();
    void parse_const_directive();
    void parse_upvalue_directive();
    void parse_label_definition();
    void parse_instruction();
    [[nodiscard]] const Token& current_token() const;
    [[nodiscard]] const Token& peek_token(size_t offset = 1) const;
    [[nodiscard]] bool is_at_end() const;
    void advance();
    const Token& consume_token(TokenType expected, std::string_view error_message);
    bool match_token(TokenType type);
    meow::core::Value parse_const_value_from_tokens();
    [[nodiscard]] static std::string unescape_string(std::string_view escaped);
    void resolve_labels_for_build_data(ProtoBuildData& data);
    std::vector<meow::core::Value> build_final_constant_pool(ProtoBuildData& data);
    void link_final_constant_pool(meow::core::Value& constant);
    [[noreturn]] void throw_parse_error(std::string_view message);
    [[noreturn]] void throw_parse_error(std::string_view message, const Token& token);
};
}  // namespace meow::loader