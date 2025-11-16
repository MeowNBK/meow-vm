#pragma once

namespace meow::utils {
template <typename T>
struct Node {
    // --- Metadata ---
    T data_;
    Node<T>* next_;

    // --- Constructors ---
    explicit Node(const T& value, Node<T>* next = nullptr) noexcept : data_(value), next_(next) {
    }
    explicit Node(T&& value, Node<T>* next = nullptr) noexcept : data_(std::move(value)), next_(next) {
    }

    [[nodiscard]] inline const T& data() const noexcept {
        return data_;
    }
    [[nodiscard]] inline T& data() noexcept {
        return data_;
    }
    [[nodiscard]] inline const Node<T>* next() const noexcept {
        return next_;
    }
    [[nodiscard]] inline Node<T>* next() noexcept {
        return next_;
    }
};
}  // namespace meow::utils
