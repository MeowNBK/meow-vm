/**
 * @file pair.h
 * @author LazyPaws
 * @brief An util for Pair in TrangMeo
 * @copyright Copyright (c) 2025 LazyPaws
 * @license All rights reserved. Unauthorized copying of this file, in any form
 * or medium, is strictly prohibited
 */

#pragma once

template <typename T, typename U>
struct Pair {
    T first_;
    U second_;
    Pair(const T& first, const U& second) : first_(first), second_(second) {
    }
    Pair(T&& first, U&& second) : first_(std::move(first)), second_(std::move(second)) {
    }
};