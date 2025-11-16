#pragma once

#include "common/pch.h"
#include "core/definitions.h"
#include "core/value.h"

namespace meow::loader {
class TextParser;
}

namespace meow::runtime {
class Chunk {
   public:
    Chunk() = default;
    Chunk(std::vector<uint8_t>&& code, std::vector<meow::core::Value>&& constants) noexcept : code_(std::move(code)), constant_pool_(std::move(constants)) {
    }

    // inline void write_byte(uint8_t byte) {
    //     code_.push_back(byte);
    // }

    // inline void write_short(uint16_t value) {
    //     if (value < 128) {
    //         code_.push_back(static_cast<uint8_t>(value));
    //     } else {

    //         // Vis dụ 1 byte có 0b01010101
    //         // 0x80 = 0b100 0b0000
    //         // 0x7F = 0b0111 0b1111

    //         // Vậy, khi dùng value & 0x7F thì ta sẽ lấy 7 bit đầu
    //         // Dùng thêm value | 0x80 tức là bật bit số 8 thành 1
    //         code_.push_back(static_cast<uint8_t>((value & 0x7F) | 0x80));

    //         // Lấy 7 bit rồi thì dịch 7 bit để ăn nốt mấy bit còn lại
    //         // Thực ra không hẳn 7 bit còn lại
    //         // Nhưng mà theo cách lưu byte này thì chỉ lưu được 7 bit dữ liệu
    //         thật code_.push_back(static_cast<uint8_t>(value >> 7));
    //     }
    // }

    // inline void write_address(uint16_t address) {
    //     // Viết theo dạng little-endian
    //     // Tưởng tượng có hai byte 0x1234 (vì mỗi byte có 8 bit)

    //     // Ta có hai byte là 0x12, 0x34
    //     // Nếu là big-endian thì thêm byte y như thứ tự viết hexa
    //     // Vì hexa dựa trên big-endian
    //     // Tức là viết 0x12 - byte cao trước, rồi viết 0x34- byte thấp
    //     // Nếu là little-endian thì viết ngược lại
    //     // Viết 0x34 trước, rồi viết 0x12

    //     code_.push_back(static_cast<uint8_t>(address & 0xFF)); // Lấy trọn 1
    //     byte thấp rồi đẩy vào code
    //     code_.push_back(static_cast<uint8_t>((address >> 8) & 0xFF)); // Dịch
    //     sang phải để lấy nốt 1 byte còn lại
    //     // Ví dụ cụ thể với 0x1234
    //     // Khi dùng address & 0xFF thì tức là ta lấy 1 byte thấp
    //     // Tức là lấy 0x1234 & 0xFF, với 0xFF để lấy trọn 1 byte thấp là 0x34
    //     // Khi dùng address >> 8, thực chất, address chỉ còn 0x0012
    //     // Khi này, ta dùng address & 0xFF sẽ lấy được byte cao là 0x12

    //     // Máy tác giả dùng là little-endian nên dùng vậy thôi
    //     // Máy bạn nếu dùng big-endian có thể đảo ngược quá trình lại
    // }

    // --- Modifiers ---
    inline void write_byte(uint8_t byte) {
        code_.push_back(byte);
    }

    inline void write_u16(uint16_t value) {
        code_.push_back(static_cast<uint8_t>(value & 0xFF));
        code_.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    }

    inline void write_u32(uint32_t value) {
        code_.push_back(static_cast<uint8_t>(value & 0xFF));
        code_.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        code_.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        code_.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    }

    inline void write_u64(uint64_t value) {
        code_.push_back(static_cast<uint8_t>(value & 0xFF));
        code_.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        code_.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        code_.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
        code_.push_back(static_cast<uint8_t>((value >> 32) & 0xFF));
        code_.push_back(static_cast<uint8_t>((value >> 40) & 0xFF));
        code_.push_back(static_cast<uint8_t>((value >> 48) & 0xFF));
        code_.push_back(static_cast<uint8_t>((value >> 56) & 0xFF));
    }

    inline void write_f64(double value) {
        write_u64(std::bit_cast<uint64_t>(value));
    }
    // --- Code buffer ---
    [[nodiscard]] inline const uint8_t* get_code() const noexcept {
        return code_.data();
    }
    [[nodiscard]] inline size_t get_code_size() const noexcept {
        return code_.size();
    }
    [[nodiscard]] inline bool is_code_empty() const noexcept {
        return code_.empty();
    }

    // --- Constant pool ---
    [[nodiscard]] inline size_t get_pool_size() const noexcept {
        return constant_pool_.size();
    }
    [[nodiscard]] inline bool is_pool_empty() const noexcept {
        return constant_pool_.empty();
    }
    [[nodiscard]] inline size_t add_constant(meow::core::param_t value) {
        constant_pool_.push_back(value);
        return constant_pool_.size() - 1;
    }
    [[nodiscard]] inline meow::core::return_t get_constant(size_t index) const noexcept {
        return constant_pool_[index];
    }
    [[nodiscard]] inline meow::core::value_t& get_constant_ref(size_t index) noexcept {
        return constant_pool_[index];
    }
    [[nodiscard]] inline const uint8_t* get_code_buffer_ptr() const noexcept {
        return code_.data();
    }

    inline bool patch_u16(size_t offset, uint16_t value) noexcept {
        if (offset + 1 >= code_.size()) return false;

        code_[offset] = static_cast<uint8_t>(value & 0xFF);
        code_[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);

        return true;
    }

   private:
    std::vector<uint8_t> code_;
    std::vector<meow::core::Value> constant_pool_;
};
}  // namespace meow::runtime