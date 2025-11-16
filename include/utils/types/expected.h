#pragma once

#include "common/pch.h"

#if __cplusplus >= 202300L
#include <expected>
template <typename T, typename E>
using Expected = std::expected<T, E>;
#else

template <typename T, typename E>
class Expected {
   public:
    Expected(const T& value) : data(value) {
    }
    Expected(T&& value) : data(std::move(value)) {
    }

    Expected(const E& error) : data(error) {
    }
    Expected(E&& error) : data(std::move(error)) {
    }

    [[nodiscard]] inline bool has_value() const noexcept {
        return std::holds_alternative<T>(data);
    }

    explicit operator bool() const noexcept {
        return has_value();
    }

    [[nodiscard]] inline const T& value() const {
        if (!has_value()) {
            throw std::bad_variant_access();
        }
        return std::get<T>(data);
    }

    [[nodiscard]] inline T& value() {
        if (!has_value()) {
            throw std::bad_variant_access();
        }
        return std::get<T>(data);
    }

    [[nodiscard]] inline const E& error() const {
        if (has_value()) {
            throw std::bad_variant_access();
        }
        return std::get<E>(data);
    }

    [[nodiscard]] inline E& error() {
        if (has_value()) {
            throw std::bad_variant_access();
        }
        return std::get<E>(data);
    }

   private:
    std::variant<T, E> data;
};
#endif