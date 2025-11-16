/**
 * @file array.h
 * @author LazyPaws
 * @brief Core definition of Array in TrangMeo
 * @copyright Copyright (c) 2025 LazyPaws
 * @license All rights reserved. Unauthorized copying of this file, in any form
 * or medium, is strictly prohibited
 */

#pragma once

#include "common/pch.h"
#include "core/definitions.h"
#include "core/meow_object.h"
#include "core/value.h"
#include "memory/gc_visitor.h"

namespace meow::core::objects {
class ObjArray : public meow::core::ObjBase<ObjectType::ARRAY> {
   private:
    using container_t = std::vector<meow::core::value_t>;
    using visitor_t = meow::memory::GCVisitor;

    container_t elements_;

   public:
    // --- Constructors & destructor ---
    ObjArray() = default;
    explicit ObjArray(const container_t& elements) : elements_(elements) {
    }
    explicit ObjArray(container_t&& elements) noexcept : elements_(std::move(elements)) {
    }
    explicit ObjArray(std::initializer_list<meow::core::value_t> elements) : elements_(elements) {
    }

    // --- Rule of 5 ---
    ObjArray(const ObjArray&) = delete;
    ObjArray(ObjArray&&) = default;
    ObjArray& operator=(const ObjArray&) = delete;
    ObjArray& operator=(ObjArray&&) = delete;
    ~ObjArray() override = default;

    // --- Iterator types ---
    using iterator = container_t::iterator;
    using const_iterator = container_t::const_iterator;
    using reverse_iterator = container_t::reverse_iterator;
    using const_reverse_iterator = container_t::const_reverse_iterator;

    // --- Element access ---

    /// @brief Unchecked element access. For performance-critical code
    [[nodiscard]] inline meow::core::return_t get(size_t index) const noexcept {
        return elements_[index];
    }
    /// @brief Unchecked element modification. For performance-critical code
    template <typename T>
    inline void set(size_t index, T&& value) noexcept {
        elements_[index] = std::forward<T>(value);
    }
    /// @brief Checked element access. Throws if index is OOB
    [[nodiscard]] inline meow::core::return_t at(size_t index) const {
        return elements_.at(index);
    }
    inline meow::core::return_t operator[](size_t index) const noexcept {
        return elements_[index];
    }
    inline meow::core::mutable_t operator[](size_t index) noexcept {
        return elements_[index];
    }
    [[nodiscard]] inline meow::core::return_t front() const noexcept {
        return elements_.front();
    }
    [[nodiscard]] inline meow::core::mutable_t front() noexcept {
        return elements_.front();
    }
    [[nodiscard]] inline meow::core::return_t back() const noexcept {
        return elements_.back();
    }
    [[nodiscard]] inline meow::core::mutable_t back() noexcept {
        return elements_.back();
    }

    // --- Capacity ---
    [[nodiscard]] inline size_t size() const noexcept {
        return elements_.size();
    }
    [[nodiscard]] inline bool empty() const noexcept {
        return elements_.empty();
    }
    [[nodiscard]] inline size_t capacity() const noexcept {
        return elements_.capacity();
    }

    // --- Modifiers ---
    template <typename T>
    inline void push(T&& value) {
        elements_.emplace_back(std::forward<T>(value));
    }
    inline void pop() noexcept {
        elements_.pop_back();
    }
    template <typename... Args>
    inline void emplace(Args&&... args) {
        elements_.emplace_back(std::forward<Args>(args)...);
    }
    inline void resize(size_t size) {
        elements_.resize(size);
    }
    inline void reserve(size_t capacity) {
        elements_.reserve(capacity);
    }
    inline void shrink() {
        elements_.shrink_to_fit();
    }
    inline void clear() {
        elements_.clear();
    }

    // --- Iterators ---
    inline iterator begin() noexcept {
        return elements_.begin();
    }
    inline iterator end() noexcept {
        return elements_.end();
    }
    inline const_iterator begin() const noexcept {
        return elements_.begin();
    }
    inline const_iterator end() const noexcept {
        return elements_.end();
    }
    inline reverse_iterator rbegin() noexcept {
        return elements_.rbegin();
    }
    inline reverse_iterator rend() noexcept {
        return elements_.rend();
    }
    inline const_reverse_iterator rbegin() const noexcept {
        return elements_.rbegin();
    }
    inline const_reverse_iterator rend() const noexcept {
        return elements_.rend();
    }

    void trace(visitor_t& visitor) const noexcept override;
};
}  // namespace meow::core::objects

// /**
//  * @file array.h
//  * @author LazyPaws
//  * @brief Core definition of Array in TrangMeo
//  * @note No code explained
//  */

// #pragma once

// #include "common/pch.h"
// #include "core/definitions.h"
// #include "core/meow_object.h"
// #include "core/value.h"
// #include "memory/gc_visitor.h"

// #include <vector>
// #include <initializer_list>
// #include <utility>
// #include <cstddef>
// #include <algorithm>
// #if __cplusplus >= 202002L
// #include <span>
// #endif

// namespace meow::core::objects {

// class ObjArray : public meow::core::MeowObject {
// private:
//     using container_t = std::vector<meow::core::value_t>;
//     using visitor_t   = meow::memory::GCVisitor;

//     container_t elements_;

// public:
//     // --- aliases ---
//     using value_type = meow::core::value_t;
//     using size_type  = container_t::size_type;

//     // iterator aliases
//     using iterator               = container_t::iterator;
//     using const_iterator         = container_t::const_iterator;
//     using reverse_iterator       = container_t::reverse_iterator;
//     using const_reverse_iterator = container_t::const_reverse_iterator;

//     // --- Constructors & destructor ---
//     ObjArray() = default;
//     explicit ObjArray(const container_t& elements) : elements_(elements) {}
//     explicit ObjArray(container_t&& elements) noexcept : elements_(std::move(elements)) {}
//     explicit ObjArray(std::initializer_list<value_type> elements) : elements_(elements) {}

//     // --- Rule of 5 ---
//     ObjArray(const ObjArray&) = delete;
//     ObjArray(ObjArray&&) noexcept = default;
//     ObjArray& operator=(const ObjArray&) = delete;
//     ObjArray& operator=(ObjArray&&) noexcept = default;
//     ~ObjArray() override = default;

//     // --- Element access ---

//     /// @brief Unchecked element access. For performance-critical code
//     [[nodiscard]] inline meow::core::return_t get(size_t index) const noexcept {
//         return elements_[index];
//     }

//     /// @brief Unchecked element modification. For performance-critical code
//     template <typename T>
//     inline void set(size_t index, T&& value) noexcept(noexcept(elements_[index] =
//     std::forward<T>(value))) {
//         elements_[index] = std::forward<T>(value);
//     }

//     /// @brief Checked element access. Throws if index is OOB
//     [[nodiscard]] inline meow::core::return_t at(size_t index) const { return
//     elements_.at(index); }

//     inline meow::core::return_t operator[](size_t index) const noexcept { return
//     elements_[index]; } inline meow::core::mutable_t operator[](size_t index) noexcept { return
//     elements_[index]; }

//     [[nodiscard]] inline meow::core::return_t front() const noexcept { return elements_.front();
//     }
//     [[nodiscard]] inline meow::core::mutable_t front() noexcept { return elements_.front(); }
//     [[nodiscard]] inline meow::core::return_t back() const noexcept { return elements_.back(); }
//     [[nodiscard]] inline meow::core::mutable_t back() noexcept { return elements_.back(); }

//     // === raw pointer / span view ===
//     inline value_type* data() noexcept { return elements_.data(); }
//     inline const value_type* data() const noexcept { return elements_.data(); }

// #if __cplusplus >= 202002L
//     inline std::span<value_type> as_span() noexcept { return std::span<value_type>(elements_); }
//     inline std::span<const value_type> as_span() const noexcept { return std::span<const
//     value_type>(elements_); }
// #endif

//     // --- Capacity ---
//     [[nodiscard]] inline size_t size() const noexcept { return elements_.size(); }
//     [[nodiscard]] inline bool empty() const noexcept { return elements_.empty(); }
//     [[nodiscard]] inline size_t capacity() const noexcept { return elements_.capacity(); }

//     // --- Modifiers ---
//     template <typename T>
//     inline void push(T&& value)
//     noexcept(noexcept(std::declval<container_t&>().emplace_back(std::forward<T>(value)))) {
//         elements_.emplace_back(std::forward<T>(value));
//     }

//     inline void pop() noexcept { elements_.pop_back(); }

//     template <typename... Args>
//     inline void emplace(Args&&... args)
//     noexcept(noexcept(std::declval<container_t&>().emplace_back(std::forward<Args>(args)...))) {
//         elements_.emplace_back(std::forward<Args>(args)...);
//     }

//     inline void resize(size_t size) { elements_.resize(size); }
//     inline void reserve(size_t capacity) { elements_.reserve(capacity); }
//     inline void shrink() { elements_.shrink_to_fit(); }
//     inline void clear() noexcept { elements_.clear(); }

//     // --- Iterators ---
//     inline iterator begin() noexcept { return elements_.begin(); }
//     inline iterator end() noexcept { return elements_.end(); }
//     inline const_iterator begin() const noexcept { return elements_.begin(); }
//     inline const_iterator end() const noexcept { return elements_.end(); }

//     inline const_iterator cbegin() const noexcept { return elements_.cbegin(); }
//     inline const_iterator cend() const noexcept { return elements_.cend(); }

//     inline reverse_iterator rbegin() noexcept { return elements_.rbegin(); }
//     inline reverse_iterator rend() noexcept { return elements_.rend(); }
//     inline const_reverse_iterator rbegin() const noexcept { return elements_.rbegin(); }
//     inline const_reverse_iterator rend() const noexcept { return elements_.rend(); }

//     inline const_reverse_iterator crbegin() const noexcept { return elements_.crbegin(); }
//     inline const_reverse_iterator crend() const noexcept { return elements_.crend(); }

//     // --- Utility helpers ---
//     [[nodiscard]] inline bool contains(const value_type& v) const noexcept {
//         return std::find(elements_.cbegin(), elements_.cend(), v) != elements_.cend();
//     }

//     inline ssize_t index_of(const value_type& v) const noexcept {
//         auto it = std::find(elements_.cbegin(), elements_.cend(), v);
//         if (it == elements_.cend()) return -1;
//         return static_cast<ssize_t>(std::distance(elements_.cbegin(), it));
//     }

//     template <typename Pred>
//     inline iterator find_if(Pred&& pred)
//     noexcept(noexcept(std::declval<Pred>()(std::declval<value_type>()))) {
//         return std::find_if(elements_.begin(), elements_.end(), std::forward<Pred>(pred));
//     }

//     template <typename Pred>
//     inline const_iterator find_if(Pred&& pred) const
//     noexcept(noexcept(std::declval<Pred>()(std::declval<value_type>()))) {
//         return std::find_if(elements_.cbegin(), elements_.cend(), std::forward<Pred>(pred));
//     }

//     // --- GC tracing ---
//     void trace(visitor_t& visitor) const noexcept override;
// };

// } // namespace meow::core::objects
