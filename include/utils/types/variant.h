#pragma once

// ======================== I. STANDARD C++ HEADERS ========================
#include <cstddef>      // std::size_t, static_cast
#include <cstdint>      // uint8_t, uint64_t, uintptr_t, int64_t
#include <type_traits>  // std::integral_constant, std::decay_t, std::is_pointer, std::is_integral, std::is_floating_point, std::is_same_v, std::conditional_t, std::enable_if_t, ...
#include <utility>      // std::forward, std::move, std::declval, std::swap, std::in_place_type_t, std::in_place_index_t
#include <functional>   // std::invoke, std::invoke_result_t
#include <array>        // std::array
#include <tuple>        // std::tuple, std::tuple_element_t
#include <cstring>      // std::memcpy, std::memcmp, std::memset
#include <algorithm>    // std::min
#include <stdexcept>    // std::bad_variant_access
#include <new>          // Placement new
#include <cmath>        // std::isnan
#include <limits>       // std::numeric_limits
#include <variant>      // std::monostate
#include <bit>          // std::bit_cast (C++20 - RẤT QUAN TRỌNG CHO NaN-BOXING!)

#include "variant/variant_utils.h"
#include "variant/variant_fallback.h"
#include "variant/variant_nanbox.h"

// Detect platform capability for nan-boxing (kept from your original)
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
#  define MEOW_BYTE_ORDER __BYTE_ORDER__
#  define MEOW_ORDER_LITTLE __ORDER_LITTLE_ENDIAN__
#endif

#if (defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)) && (defined(MEOW_BYTE_ORDER) && MEOW_BYTE_ORDER == MEOW_ORDER_LITTLE)
#  define MEOW_CAN_USE_NAN_BOXING 1
#else
#  define MEOW_CAN_USE_NAN_BOXING 0
#endif

// Put public variant in namespace meow
namespace meow {

// Bring a few helpers into scope for nicer code below
using utils::overload; // convenience for non-member visit wrappers

// We will refer to flatten/metadata via utils::detail and to backend helpers via utils::...
// (variant_utils.h defines meow::utils::detail::flattened_unique_t etc.)
// nanbox header defines utils::all_nanboxable_impl
// fallback header defines utils::FallbackVariant

template <typename... Args>
class variant {
private:
    // --- Metadata / choose backend ---
    // flattened_unique_t is in meow::utils::detail
    static constexpr bool can_nanbox_by_platform = MEOW_CAN_USE_NAN_BOXING != 0;
    static constexpr bool small_enough_for_nanbox = (sizeof...(Args) <= 8);

    static constexpr bool should_use_nan_box =
        can_nanbox_by_platform &&
        small_enough_for_nanbox &&
        // all_nanboxable_impl lives in meow::utils (defined by variant_nanbox.h)
        utils::all_nanboxable_impl<utils::detail::flattened_unique_t<Args...>>::value;

    using implementation_t = std::conditional_t<
        should_use_nan_box,
        utils::NaNBoxedVariant<Args...>,
        utils::FallbackVariant<Args...>
    >;

    implementation_t storage_;

public:
    // --- Constructors / assignment ---
    variant() = default;
    variant(const variant&) = default;
    variant(variant&&) = default;

    template <typename T,
              typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, variant>>>
    variant(T&& value) noexcept(noexcept(implementation_t(std::forward<T>(value))))
        : storage_(std::forward<T>(value))
    {}

    variant& operator=(const variant&) = default;
    variant& operator=(variant&&) = default;

    template <typename T,
              typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, variant>>>
    variant& operator=(T&& value) noexcept(noexcept(std::declval<implementation_t&>() = std::forward<T>(value))) {
        storage_ = std::forward<T>(value);
        return *this;
    }

    // --- Queries ---
    [[nodiscard]] std::size_t index() const noexcept { return storage_.index(); }
    [[nodiscard]] bool valueless() const noexcept { return storage_.valueless(); }

    template <typename T>
    [[nodiscard]] bool holds() const noexcept { return storage_.template holds<T>(); }

    template <typename T>
    [[nodiscard]] bool is() const noexcept { return holds<T>(); }

    // --- Accessors ---
    template <typename T>
    [[nodiscard]] decltype(auto) get() { return storage_.template get<T>(); }

    template <typename T>
    [[nodiscard]] decltype(auto) get() const { return storage_.template get<T>(); }

    template <typename T>
    [[nodiscard]] decltype(auto) safe_get() { return storage_.template safe_get<T>(); }

    template <typename T>
    [[nodiscard]] decltype(auto) safe_get() const { return storage_.template safe_get<T>(); }

    template <typename T>
    [[nodiscard]] auto* get_if() noexcept { return storage_.template get_if<T>(); }

    template <typename T>
    [[nodiscard]] const auto* get_if() const noexcept { return storage_.template get_if<T>(); }

    // --- Modifiers ---
    template <typename T, typename... CArgs>
    decltype(auto) emplace(CArgs&&... args) {
        return storage_.template emplace<T>(std::forward<CArgs>(args)...);
    }

    template <std::size_t I, typename... CArgs>
    decltype(auto) emplace_index(CArgs&&... args) {
        return storage_.template emplace_index<I>(std::forward<CArgs>(args)...);
    }

    void swap(variant& other) noexcept { storage_.swap(other.storage_); }

    // --- Visit ---
    template <typename Visitor>
    decltype(auto) visit(Visitor&& vis) { return storage_.visit(std::forward<Visitor>(vis)); }

    template <typename Visitor>
    decltype(auto) visit(Visitor&& vis) const { return storage_.visit(std::forward<Visitor>(vis)); }

    template <typename... Fs>
    decltype(auto) visit(Fs&&... fs) {
        return storage_.visit(overload{ std::forward<Fs>(fs)... });
    }

    // Overload cho const variant
    template <typename... Fs>
    decltype(auto) visit(Fs&&... fs) const {
        return storage_.visit(overload{ std::forward<Fs>(fs)... });
    }

    // --- Comparison ---
    bool operator==(const variant& other) const { return storage_ == other.storage_; }
    bool operator!=(const variant& other) const { return storage_ != other.storage_; }
};

// --- Non-member utilities ---
template <typename... Ts>
void swap(variant<Ts...>& a, variant<Ts...>& b) noexcept {
    a.swap(b);
}

// Non-member visit helpers that construct overload
template <typename... Ts, typename... Fs>
decltype(auto) visit(variant<Ts...>& v, Fs&&... fs) {
    return v.visit(overload{ std::forward<Fs>(fs)... });
}

template <typename... Ts, typename... Fs>
decltype(auto) visit(const variant<Ts...>& v, Fs&&... fs) {
    return v.visit(overload{ std::forward<Fs>(fs)... });
}

template <typename... Ts, typename... Fs>
decltype(auto) visit(variant<Ts...>&& v, Fs&&... fs) {
    return std::move(v).visit(overload{ std::forward<Fs>(fs)... });
}

} // namespace meow
