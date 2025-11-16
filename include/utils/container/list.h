/**
 * @file list.h
 * @author LazyPaws
 * @brief An util for List in TrangMeo
 * @copyright Copyright (c) 2025 LazyPaws
 * @license All rights reserved. Unauthorized copying of this file, in any form
 * or medium, is strictly prohibited
 */

#pragma once

#include "utils/node.h"

namespace meow::utils {
template <typename T>
class List {
   private:
    using value_t = T;
    using const_reference_t = const value_t&;
    using move_t = value_t&&;
    using node_t = utils::Node<value_t>;
    using node_pointer_t = node_t*;
    using const_node_pointer_t = const node_t*;

    // --- Metadata ---
    node_pointer_t head_;
    node_pointer_t tail_;
    size_t size_ = 0;

    // --- Helpers ---
    inline void copy_from(const List& other) noexcept {
        tail_ = head_ = nullptr;
        size_ = 0;
        const_node_pointer_t current = other.head_;
        while (current) {
            push(current->data_);
            current = current->next_;
        }
    }
    inline void move_from(List&& other) noexcept {
        head_ = std::move(other.head_);
        tail_ = std::move(other.tail_);
        size_ = std::move(other.size_);
        other.tail_ = other.head_ = nullptr;
        other.size_ = 0;
    }

   public:
    // --- Constructors & destructor
    List() noexcept : head_(nullptr), tail_(nullptr) {
    }
    explicit List(const List& other) noexcept {
        copy_from(other);
    }
    explicit List(List&& other) noexcept {
        move_from(std::move(other));
    }
    inline List& operator=(const List& other) {
        if (this == &other) return *this;
        copy_from(other);
        return *this;
    }
    inline List& operator=(List&& other) noexcept {
        move_from(std::move(other));
        return *this;
    }
    ~List() noexcept {
        clear();
    }

    // --- Modifiers ---
    inline void push(const_reference_t value) noexcept {
        node_pointer_t new_node = new node_t(value);
        if (head_) {
            tail_->next_ = new_node;
            tail_ = new_node;
        } else {
            tail_ = head_ = new_node;
        }
        ++size_;
    }
    inline void push(move_t value) noexcept {
        node_pointer_t new_node = new node_t(std::move(value));
        if (head_) {
            tail_->next_ = new_node;
            tail_ = new_node;
        } else {
            tail_ = head_ = new_node;
        }
        ++size_;
    }
    inline void push_front(const_reference_t value) noexcept {
        node_pointer_t new_node = new node_t(value);
        if (head_) {
            new_node->next_ = head_;
            head_ = new_node;
        } else {
            tail_ = head_ = new_node;
        }
        ++size_;
    }

    // --- Elements ---
    [[nodiscard]] inline const_node_pointer_t find(const_reference_t value) const noexcept {
        const_node_pointer_t current = head_;
        while (current) {
            if (current->data_ == value) return current;
            current = current->next_;
        }
        return nullptr;
    }
    [[nodiscard]] inline size_t count(const_reference_t value) const noexcept {
        size_t counter = 0;
        const_node_pointer_t current = head_;
        while (current) {
            if (current->data_ == value) ++counter;
            current = current->next_;
        }
        return counter;
    }
    [[nodiscard]] inline bool has(const_reference_t value) const noexcept {
        return find(value);
    }
    [[nodiscard]] inline const_node_pointer_t begin() const noexcept {
        return head_;
    }
    [[nodiscard]] inline const_node_pointer_t end() const noexcept {
        return nullptr;
    }
    [[nodiscard]] inline const_node_pointer_t head() const noexcept {
        return head_;
    }
    [[nodiscard]] inline const_node_pointer_t tail() const noexcept {
        return tail_;
    }

    // --- Capacity ---
    [[nodiscard]] inline size_t size() const noexcept {
        return size_;
    }
    [[nodiscard]] inline bool empty() const noexcept {
        return size_ == 0;
    }

    // --- Destruction ---
    inline void clear() noexcept {
        node_pointer_t current = head_;
        while (current) {
            node_pointer_t temp = current->next_;
            delete current;
            current = temp;
        }
        tail_ = head_ = nullptr;
        size_ = 0;
    }

    // --- Utilities ---
    [[nodiscard]] inline std::string str() const noexcept {
        std::ostringstream os;
        if (!head_) return "";
        node_pointer_t current = head_;

        while (current) {
            os << current->data_ << " ";
            current = current->next_;
        }

        std::string output = os.str();
        if (output.back() == ' ') output.pop_back();

        return output;
    }
};

}  // namespace meow::utils