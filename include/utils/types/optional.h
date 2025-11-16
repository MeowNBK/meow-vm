#pragma once

#include "common/pch.h"

namespace meow::utils {
template <typename T>
class Optional {
   private:
    using byte_t = unsigned char;

    bool has_value_;
    // alignas type variable -> căn chỉnh vùng nhớ kiểu type cho biến variable
    // struct alignas(16) Struct {}

    // Ta cần một vùng nhớ cho storage
    // Nhớ lại kiến thức về memory ta thấy vùng nhớ là gồm các byte
    // Nhớ lại kiến thức về con trỏ
    // Nhớ luôn việc vì sao chỉ dereference lại cần kiểu con trỏ cụ thể (ví dụ
    // int*) còn void* thì không Ta sẽ nhận ra ta cần làm việc mà compiler đã
    // làm Nhưng giờ ta sẽ làm thủ công vì cần độ chính xác tuyệt đối Đó là
    // ALIGNMENT

    // alignas(alignment) type variable
    // Thực ra nó không khác gì khai báo biến bình thường cả
    // Ở đây là khai báo mảng như thường "byte_t storage_[sizeof(T)]"
    // Nhưng thêm alignas(T) để địa chỉ của nó chia hết cho số byte của T
    // Tương tự như địa chỉ mà int* giữ chia hết cho sizeof(int)
    alignas(T) byte_t storage_[sizeof(T)];  // Vùng nhớ để chưa object

    [[nodiscard]] inline const T* ptr() const noexcept {
        // Đổi góc nhìn của compiler về storage_
        // Thay góc nhìn của compiler rồi
        // Nhưng CPU yêu cầu object cần nằm ở địa chỉ alignment
        // Ta cần tự căn chỉnh vì với optional, variant cần vùng nhớ thô, thủ
        // công Đồng thời cũng để dùng reinterpret_cast như này
        return reinterpret_cast<T*>(storage_);
    }
    [[nodiscard]] inline T* ptr() noexcept {
        return reinterpret_cast<T*>(storage_);
    }

    template <typename T = int>
    // inline void alloc(const T& var) noexcept {
    //     new (storage_) var;
    // }
    public :
        // --- Constructors ---
        Optional() noexcept
        : has_value_(false) {
    }
    explicit Optional(const T& value) : has_value_(true) {
        // new placement - không tạo vùng nhớ mới
        // Dùng vùng nhớ đã tạo, để object vào đó
        new (storage_) T(value);
    }
    explicit Optional(T&& value) noexcept : has_value_(true) {
        new (storage_) T(std::move(value));
    }
    explicit Optional(const Optional& other) : has_value_(other.has_value_) {
        if (has_value_) {
            new (storage_) T(other.get());
        }
    }
    explicit Optional(Optional&& other) noexcept : has_value_(other.has_value_) {
        other.has_value_ = false;
        if (has_value_) {
            new (storage_) T(std::move(other.get()));
        }
    }
    ~Optional() {
        reset();
    }

    // --- Operator overloads ---
    inline Optional& operator=(const Optional& other) noexcept {
        if (this == &other) return *this;
        reset();
        has_value_ = other.has_value_;
        if (has_value_) {
            new (storage_) T(other.get());
        }
        return *this;
    }
    inline Optional& operator=(Optional&& other) noexcept {
        if (this == &other) return *this;
        reset();
        if (has_value_) {
            new (storage_) T(std::move(other.get()));
        }
        has_value_ = other.has_value_;
        return *this;
    }

    // --- Modifiers ---
    inline void reset() noexcept {
        if (has_value_) {
            ptr()->~T();  // Vùng nhớ thô, không tự gọi destructor được, phải
                          // gọi thủ công
            has_value_ = false;
        }
    }

    template <typename... Args>
    inline void emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args&&...>) {
        reset();
        new (storage_) T(std::forward<Args>(args)...);
        has_value_ = true;
    }

    // --- Observers ---
    explicit operator bool() const noexcept {
        return has_value_;
    }
    [[nodiscard]] inline bool has() const noexcept {
        return has_value_;
    }
    [[nodiscard]] inline const T& get() const noexcept {
        return *ptr();
    }
    [[nodiscard]] inline T& get() noexcept {
        return *ptr();
    }
    [[nodiscard]] inline const T& safe_get() const {
        if (!has_value_) throw std::runtime_error("No value");
        return *ptr();
    }
    [[nodiscard]] inline T& safe_get() {
        if (!has_value_) throw std::runtime_error("No value");
        return *ptr();
    }

    [[nodiscard]] inline const T& operator*() const noexcept {
        return *ptr();
    }
    [[nodiscard]] inline T& operator*() noexcept {
        return *ptr();
    }
};
}  // namespace meow::utils