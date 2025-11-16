/**
 * @file vector.h
 * @author LazyPaws
 * @brief An util for Dynamic Array (Vector) in TrangMeo
 * @copyright Copyright (c) 2025 LazyPaws
 * @license All rights reserved. Unauthorized copying of this file, in any form
 * or medium, is strictly prohibited
 */

#pragma once

namespace meow::utils {
template <typename T>
class Vector {
   private:
    using value_t = T;
    using reference_t = value_t&;
    using move_t = value_t&&;
    using pointer_t = value_t*;
    using const_reference_t = const value_t&;
    using const_pointer_t = const value_t*;

    // --- Metadata ---
    pointer_t data_;
    size_t size_;
    size_t capacity_;

    // --- Helpers ---
    inline void grow(size_t new_capacity) noexcept {
        pointer_t temp_data = new value_t[new_capacity];
        for (size_t i = 0; i < size_; ++i) {
            temp_data[i] = std::move(data_[i]);
        }
        delete[] data_;
        data_ = temp_data;
        capacity_ = new_capacity;
    }
    inline void copy_from(const Vector& other) noexcept {
        size_ = other.size_;
        capacity_ = other.capacity_;
        delete[] data_;
        data_ = new value_t[capacity_];
        for (size_t i = 0; i < size_; ++i) {
            data_[i] = other.data_[i];
        }
    }
    inline void move_from(Vector&& other) noexcept {
        delete[] data_;
        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;

        other.data_ = nullptr;
        other.size_ = other.capacity_ = 0;
    }

   public:
    // --- Constructors & destructor
    Vector(size_t new_capacity = 10) noexcept : data_(new value_t[new_capacity]()), size_(0), capacity_(new_capacity) {
    }
    explicit Vector(const Vector& other) noexcept : data_(new value_t[other.capacity_]), size_(other.size_), capacity_(other.capacity_) {
        for (size_t i = 0; i < size_; ++i) {
            data_[i] = other.data_[i];
        }
    }
    explicit Vector(Vector&& other) noexcept {
        move_from(std::move(other));
    }
    inline Vector& operator=(const Vector& other) noexcept {
        copy_from(other);
        return *this;
    }
    inline Vector& operator=(Vector&& other) noexcept {
        move_from(std::move(other));
        return *this;
    }
    ~Vector() noexcept {
        delete[] data_;
    }

    // --- Element access ---
    [[nodiscard]] inline const_reference_t get(size_t index) const noexcept {
        return data_[index];
    }
    [[nodiscard]] inline reference_t get(size_t index) noexcept {
        return data_[index];
    }
    [[nodiscard]] inline const_reference_t operator[](size_t index) const noexcept {
        return data_[index];
    }
    [[nodiscard]] inline reference_t operator[](size_t index) noexcept {
        return data_[index];
    }

    // --- Data access ---
    [[nodiscard]] inline const_pointer_t data() const noexcept {
        return data_;
    }
    [[nodiscard]] inline pointer_t data() noexcept {
        return data_;
    }

    // --- Capacity ---
    [[nodiscard]] inline size_t size() const noexcept {
        return size_;
    }
    [[nodiscard]] inline size_t capacity() const noexcept {
        return capacity_;
    }

    // --- Modifiers ---
    inline void push(const_reference_t value) noexcept {
        if (size_ >= capacity_) grow(capacity_ * 2);
        data_[size_] = value;
        ++size_;
    }
    inline void push(move_t value) noexcept {
        if (size >= capacity_) grow(capacity * 2);
        data_[size_] = std::move(value);
        ++size_;
        s
    }
    inline void pop() noexcept {
        if (size_ > 0) --size_;
    }
    inline void resize(size_t new_size, const_reference_t temp_value = value_t()) noexcept {
        if (new_size > size_) {
            reserve(new_size);
            for (size_t i = size_; i < new_size; ++i) {
                data_[i] = temp_value;
            }
            size_ = new_size;
        } else {
            size_ = new_size;
        }
    }
    inline void reserve(size_t new_capacity) noexcept {
        grow(new_capacity);
    }
};
}  // namespace meow::utils