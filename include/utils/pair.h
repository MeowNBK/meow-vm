#pragma once

template <typename T, typename U>
struct Pair {
    T first_;
    U second_;
    Pair(const T& first, const U& second): first_(first), second_(second) {}
    Pair(T&& first, U&& second): first_(std::move(first)), second_(std::move(second)) {}
};