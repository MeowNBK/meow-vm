/**
 * @file array.h
 * @author LazyPaws
 * @brief Core definition of Array in TrangMeo
 * @copyright Copyright (c) 2025 LazyPaws
 * @license All rights reserved. Unauthorized copying of this file, in any form or medium, is strictly prohibited
 */

#pragma once

#include "common/pch.h"
#include "core/value.h"
#include "core/meow_object.h"
#include "memory/gc_visitor.h"

namespace meow::core::objects {
    class ObjArray : public meow::core::MeowObject {
    private:
        using value_t = meow::core::Value;
        using reference_t = value_t&;
        using const_reference_t = const value_t&;
        using container_t = std::vector<value_t>;

        container_t elements_;
    public:
        // --- Constructors & destructor ---
        ObjArray() = default;
        explicit ObjArray(const container_t& elements) : elements_(elements) {}
        explicit ObjArray(container_t&& elements) noexcept : elements_(std::move(elements)) {}
        explicit ObjArray(std::initializer_list<value_t> elements) : elements_(elements) {}

        // --- Rule of 5 ---
        ObjArray(const ObjArray&) = delete;
        ObjArray(ObjArray&&) = default;
        ObjArray& operator=(const ObjArray&) = delete;
        ObjArray& operator=(ObjArray&&) = default;
        ~ObjArray() override = default;

        // --- Iterator types ---
        using iterator = container_t::iterator;
        using const_iterator = container_t::const_iterator;
        using reverse_iterator = container_t::reverse_iterator;
        using const_reverse_iterator = container_t::const_reverse_iterator;

        // --- Element access ---
        
        /// @brief Unchecked element access. For performance-critical code
        [[nodiscard]] inline const_reference_t get(size_t index) const noexcept {
            return elements_[index];
        }
        /// @brief Unchecked element modification. For performance-critical code
        template <typename T> inline void set(size_t index, T&& value) noexcept {
            elements_[index] = std::forward<T>(value);
        }
        /// @brief Checked element access. Throws if index is OOB
        [[nodiscard]] inline const_reference_t at(size_t index) const {
            return elements_.at(index);
        }
        inline const_reference_t operator[](size_t index) const noexcept { return elements_[index]; }
        inline reference_t operator[](size_t index) noexcept { return elements_[index]; }
        [[nodiscard]] inline const_reference_t front() const noexcept { return elements_.front(); }
        [[nodiscard]] inline reference_t front() noexcept { return elements_.front(); }
        [[nodiscard]] inline const_reference_t back() const noexcept { return elements_.back(); }
        [[nodiscard]] inline reference_t back() noexcept { return elements_.back(); }

        // --- Capacity ---
        [[nodiscard]] inline size_t size() const noexcept { return elements_.size(); }
        [[nodiscard]] inline bool empty() const noexcept { return elements_.empty(); }
        [[nodiscard]] inline size_t capacity() const noexcept { return elements_.capacity(); }

        // --- Modifiers ---
        template <typename T> inline void push(T&& value) { 
            elements_.emplace_back(std::forward<T>(value));
        }
        inline void pop() noexcept { elements_.pop_back(); }
        template <typename... Args> inline void emplace(Args&&... args) {
            elements_.emplace_back(std::forward<Args>(args)...);
        }
        inline void resize(size_t size) { elements_.resize(size); }
        inline void reserve(size_t capacity) { elements_.reserve(capacity); }
        inline void shrink() { elements_.shrink_to_fit(); }
        inline void clear() { elements_.clear(); }

        // --- Iterators ---
        inline iterator begin() noexcept { return elements_.begin(); }
        inline iterator end() noexcept { return elements_.end(); }
        inline const_iterator begin() const noexcept { return elements_.begin(); }
        inline const_iterator end() const noexcept { return elements_.end(); }
        inline reverse_iterator rbegin() noexcept { return elements_.rbegin(); }
        inline reverse_iterator rend() noexcept { return elements_.rend(); }
        inline const_reverse_iterator rbegin() const noexcept { return elements_.rbegin(); }
        inline const_reverse_iterator rend() const noexcept { return elements_.rend(); }
    };
}