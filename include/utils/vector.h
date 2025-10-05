/**
 * @file vector.h
 * @author LazyPaws
 * @brief An util for Dynamic Array (Vector) in TrangMeo
 * @copyright Copyright (c) 2025 LazyPaws
 * @license All rights reserved. Unauthorized copying of this file, in any form or medium, is strictly prohibited
 */

#pragma once

namespace meow::utils {
    template <typename T>
    class Vector {
    private:
        using value_type = T;
        using reference_type = value_type&;
        using pointer_type = value_type*;
        using const_reference_type = const value_type&;
        using const_pointer_type = const value_type*;

        // --- Metadata ---
        pointer_type data_;
        size_t size_;
        size_t capacity_;

        // --- Helpers ---
        inline void grow(size_t new_capacity) noexcept {
            pointer_type temp_data = new value_type[new_capacity];
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
            data_ = new value_type[capacity_];
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
        Vector(size_t new_capacity = 10) noexcept: data_(new value_type[new_capacity]()), size_(0), capacity_(new_capacity) {}
        explicit Vector(const Vector& other) noexcept: data_(new value_type[other.capacity_]), size_(other.size_), capacity_(other.capacity_) {
            for (size_t i = 0; i < size_; ++i) {
                data_[i] = other.data_[i];
            }
        }
        explicit Vector(Vector&& other) noexcept { move_from(std::move(other)); }
        inline Vector& operator=(const Vector& other) noexcept { copy_from(other); return *this; }
        inline Vector& operator=(Vector&& other) noexcept { move_from(std::move(other)); return *this; }
        ~Vector() noexcept { delete[] data_;}

        // --- Element access ---
        [[nodiscard]] inline const_reference_type get(size_t index) const noexcept { return data_[index];}
        [[nodiscard]] inline reference_type get(size_t index) noexcept { return data_[index]; }
        [[nodiscard]] inline const_reference_type operator[](size_t index) const noexcept { return data_[index]; }
        [[nodiscard]] inline reference_type operator[](size_t index) noexcept { return data_[index]; }

        // --- Data access ---
        [[nodiscard]] inline const_pointer_type data() const noexcept { return data_; }
        [[nodiscard]] inline pointer_type data() noexcept { return data_; }

        // --- Capacity ---
        [[nodiscard]] inline size_t size() const noexcept { return size_; }
        [[nodiscard]] inline size_t capacity() const noexcept { return capacity_; }

        // --- Modifiers ---
        inline void push(const_reference_type value) noexcept {
            if (size_ >= capacity_) grow(capacity_ * 2);
            data_[size_] = value;
            ++size_;
        }
        inline void pop() noexcept { if (size_ > 0) --size_; }
        inline void resize(size_t new_size, const_reference_type temp_value = value_type()) noexcept {
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
        inline void reserve(size_t new_capacity) noexcept { grow(new_capacity); }
    };
}