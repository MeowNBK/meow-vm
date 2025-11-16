#pragma once

// ======================== I. STANDARD C++ HEADERS ========================
#include <algorithm>
#include <array>
#include <bit>
#include <cassert>  // assert
#include <cmath>
#include <cstddef>  // std::size_t, static_cast
#include <cstdint>
#include <cstring>
#include <exception>  // std::terminate
#include <functional>
#include <limits>
#include <new>
#include <stdexcept>  // std::bad_variant_access (kept for compatibility header wise)
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "variant/variant_fallback.h"
#include "variant/variant_nanbox.h"
#include "variant/variant_utils.h"

// Detect platform capability for nan-boxing (kept from your original)
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
#define MEOW_BYTE_ORDER __BYTE_ORDER__
#define MEOW_ORDER_LITTLE __ORDER_LITTLE_ENDIAN__
#endif

#if (defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)) && (defined(MEOW_BYTE_ORDER) && MEOW_BYTE_ORDER == MEOW_ORDER_LITTLE)
#define MEOW_CAN_USE_NAN_BOXING 1
#else
#define MEOW_CAN_USE_NAN_BOXING 0
#endif

// Put public variant in namespace meow
namespace meow {

// Bring a few helpers into scope for nicer code below
using utils::overload;  // convenience for non-member visit wrappers

// ---------------- Backend selection based on flattened types ----------------
// Flatten nested meow::variant parameters into a type_list and use that to
// decide backend. This ensures backend receives the actual flattened list.
namespace detail_backend {
template <typename FlattenedList>
struct select_backend_impl;

template <typename... Ts>
struct select_backend_impl<utils::detail::type_list<Ts...>> {
    static constexpr bool can_nanbox = MEOW_CAN_USE_NAN_BOXING != 0;
    static constexpr bool small_enough = (sizeof...(Ts) <= 8);
    static constexpr bool all_nanboxable = utils::all_nanboxable_impl<utils::detail::type_list<Ts...>>::value;
    static constexpr bool use_nanbox = can_nanbox && small_enough && all_nanboxable;
    using type = std::conditional_t<use_nanbox, utils::NaNBoxedVariant<Ts...>, utils::FallbackVariant<Ts...>>;
};
}  // namespace detail_backend

template <typename... Args>
class variant {
   private:
    // Use flattened unique typelist for backend decision and for internal
    // checks
    using flattened_list_t = typename utils::detail::flattened_unique_t<Args...>;
    using implementation_t = typename detail_backend::select_backend_impl<flattened_list_t>::type;

    implementation_t storage_;

    // helper alias: for a meow::variant<Ts...>, variant_inner_list_t yields
    // type_list<Ts...>
    template <typename V>
    using variant_inner_list_t = typename utils::detail::variant_inner_list<std::decay_t<V>>::type;

   public:
    // --- Constructors / assignment ---
    variant() = default;
    variant(const variant&) = default;
    variant(variant&&) = default;

    // Template ctor for non-meow::variant types (unchanged logic; excluded when
    // T is meow::variant)
    template <typename T,
              typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, variant> && !utils::detail::is_meow_variant<std::decay_t<T>>::value && std::is_constructible_v<implementation_t, T>>>
    variant(T&& value) noexcept(noexcept(implementation_t(std::forward<T>(value)))) : storage_(std::forward<T>(value)) {
    }

    variant& operator=(const variant&) = default;
    variant& operator=(variant&&) = default;

    // Template operator= for non-meow::variant types
    template <typename T,
              typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, variant> && !utils::detail::is_meow_variant<std::decay_t<T>>::value && std::is_assignable_v<implementation_t&, T>>>
    variant& operator=(T&& value) noexcept(noexcept(std::declval<implementation_t&>() = std::forward<T>(value))) {
        storage_ = std::forward<T>(value);
        return *this;
    }

    // --- Special handling for nested meow::variant construction/assignment ---
    // These do NOT throw: they assert in debug and call std::terminate() on
    // mismatch (to avoid exception unwinding in hot VM loops)

    // Construct from another meow::variant (const lvalue)
    template <typename... U>
    variant(const meow::variant<U...>& other) noexcept {
        if (other.valueless()) {
            storage_ = implementation_t{};
            return;
        }
        bool assigned = false;
        other.visit([&](auto const& v) noexcept {
            using VType = std::decay_t<decltype(v)>;
            if constexpr (utils::detail::type_list_contains<VType, flattened_list_t>::value) {
                storage_ = v;
                assigned = true;
            }
        });
        if (!assigned) {
            // Programming error: nested variant contains a type not present in
            // this variant's flattened list.
            assert(false &&
                   "Constructing variant from nested variant with "
                   "incompatible inner type");
            std::terminate();
        }
    }

    // Construct from another meow::variant (rvalue)
    template <typename... U>
    variant(meow::variant<U...>&& other) noexcept {
        if (other.valueless()) {
            storage_ = implementation_t{};
            return;
        }
        bool assigned = false;
        other.visit([&](auto&& v) noexcept {
            using VType = std::decay_t<decltype(v)>;
            if constexpr (utils::detail::type_list_contains<VType, flattened_list_t>::value) {
                storage_ = std::forward<decltype(v)>(v);
                assigned = true;
            }
        });
        if (!assigned) {
            assert(false &&
                   "Move-constructing variant from nested variant with "
                   "incompatible inner type");
            std::terminate();
        }
    }

    // Assign from another meow::variant (const lvalue)
    template <typename... U>
    variant& operator=(const meow::variant<U...>& other) noexcept {
        if (reinterpret_cast<const void*>(&other) == reinterpret_cast<const void*>(this)) return *this;
        if (other.valueless()) {
            storage_ = implementation_t{};
            return *this;
        }
        bool assigned = false;
        other.visit([&](auto const& v) noexcept {
            using VType = std::decay_t<decltype(v)>;
            if constexpr (utils::detail::type_list_contains<VType, flattened_list_t>::value) {
                storage_ = v;
                assigned = true;
            }
        });
        if (!assigned) {
            assert(false &&
                   "Assigning variant from nested variant with incompatible "
                   "inner type");
            std::terminate();
        }
        return *this;
    }

    // Assign from another meow::variant (rvalue)
    template <typename... U>
    variant& operator=(meow::variant<U...>&& other) noexcept {
        if (reinterpret_cast<void*>(&other) == reinterpret_cast<void*>(this)) return *this;
        if (other.valueless()) {
            storage_ = implementation_t{};
            return *this;
        }
        bool assigned = false;
        other.visit([&](auto&& v) noexcept {
            using VType = std::decay_t<decltype(v)>;
            if constexpr (utils::detail::type_list_contains<VType, flattened_list_t>::value) {
                storage_ = std::forward<decltype(v)>(v);
                assigned = true;
            }
        });
        if (!assigned) {
            assert(false &&
                   "Move-assigning variant from nested variant with "
                   "incompatible inner type");
            std::terminate();
        }
        return *this;
    }

    // --- Queries ---
    [[nodiscard]] std::size_t index() const noexcept {
        return storage_.index();
    }
    [[nodiscard]] bool valueless() const noexcept {
        return storage_.valueless();
    }

    template <typename T>
    [[nodiscard]] bool holds() const noexcept {
        return storage_.template holds<T>();
    }

    template <typename T>
    [[nodiscard]] bool is() const noexcept {
        return holds<T>();
    }

    // --- Accessors ---
    // Non-variant get: delegate to backend (keeps noexcept semantics as
    // original)
    template <typename T>
    [[nodiscard]] decltype(auto) get() noexcept {
        return storage_.template get<T>();
    }

    // template <typename T>
    // [[nodiscard]] decltype(auto) get() const noexcept { return
    // storage_.template get<T>(); }

    // If T is a meow::variant<...>, construct that variant from the current
    // held inner. IMPORTANT: this function is noexcept (no exceptions). On
    // mismatch we assert and terminate.
    template <typename T>
    [[nodiscard]] std::decay_t<T> get() noexcept {
        if constexpr (utils::detail::is_meow_variant<std::decay_t<T>>::value) {
            using TargetVariant = std::decay_t<T>;
            // If valueless -> terminate (consistent with not throwing)
            if (valueless()) {
                assert(false && "get<variant> called on valueless variant");
                std::terminate();
            }
            bool constructed = false;
            std::decay_t<T> result{};  // default-initialize; we'll overwrite
                                       // when match found
            visit([&](auto&& v) noexcept {
                using VType = std::decay_t<decltype(v)>;
                using inner_list = typename utils::detail::variant_inner_list<TargetVariant>::type;
                if constexpr (utils::detail::type_list_contains<VType, inner_list>::value) {
                    result = TargetVariant{std::forward<decltype(v)>(v)};
                    constructed = true;
                }
            });
            if (!constructed) {
                assert(false &&
                       "get<target-variant> called but current held type is "
                       "not in target variant");
                std::terminate();
            }
            return result;
        } else {
            return storage_.template get<T>();
        }
    }

    template <typename T>
    [[nodiscard]] std::decay_t<T> get() const noexcept {
        if constexpr (utils::detail::is_meow_variant<std::decay_t<T>>::value) {
            using TargetVariant = std::decay_t<T>;
            if (valueless()) {
                assert(false && "get<variant> const called on valueless variant");
                std::terminate();
            }
            bool constructed = false;
            std::decay_t<T> result{};
            visit([&](auto const& v) noexcept {
                using VType = std::decay_t<decltype(v)>;
                using inner_list = typename utils::detail::variant_inner_list<TargetVariant>::type;
                if constexpr (utils::detail::type_list_contains<VType, inner_list>::value) {
                    result = TargetVariant{v};
                    constructed = true;
                }
            });
            if (!constructed) {
                assert(false &&
                       "get<target-variant> const called but current held "
                       "type is not in target variant");
                std::terminate();
            }
            return result;
        } else {
            return storage_.template get<T>();
        }
    }

    // safe_get delegates to backend
    template <typename T>
    [[nodiscard]] decltype(auto) safe_get() {
        return storage_.template safe_get<T>();
    }

    template <typename T>
    [[nodiscard]] decltype(auto) safe_get() const {
        return storage_.template safe_get<T>();
    }

    template <typename T>
    [[nodiscard]] auto* get_if() noexcept {
        return storage_.template get_if<T>();
    }

    template <typename T>
    [[nodiscard]] const auto* get_if() const noexcept {
        return storage_.template get_if<T>();
    }

    // --- Modifiers ---
    template <typename T, typename... CArgs>
    decltype(auto) emplace(CArgs&&... args) {
        return storage_.template emplace<T>(std::forward<CArgs>(args)...);
    }

    template <std::size_t I, typename... CArgs>
    decltype(auto) emplace_index(CArgs&&... args) {
        return storage_.template emplace_index<I>(std::forward<CArgs>(args)...);
    }

    void swap(variant& other) noexcept {
        storage_.swap(other.storage_);
    }

    // --- Visit ---
    template <typename Visitor>
    decltype(auto) visit(Visitor&& vis) {
        return storage_.visit(std::forward<Visitor>(vis));
    }

    template <typename Visitor>
    decltype(auto) visit(Visitor&& vis) const {
        return storage_.visit(std::forward<Visitor>(vis));
    }

    template <typename... Fs>
    decltype(auto) visit(Fs&&... fs) {
        return storage_.visit(overload{std::forward<Fs>(fs)...});
    }

    template <typename... Fs>
    decltype(auto) visit(Fs&&... fs) const {
        return storage_.visit(overload{std::forward<Fs>(fs)...});
    }

    // --- Comparison ---
    bool operator==(const variant& other) const {
        return storage_ == other.storage_;
    }
    bool operator!=(const variant& other) const {
        return storage_ != other.storage_;
    }
};

// --- Non-member utilities ---
template <typename... Ts>
void swap(variant<Ts...>& a, variant<Ts...>& b) noexcept {
    a.swap(b);
}

// Non-member visit helpers that construct overload
template <typename... Ts, typename... Fs>
decltype(auto) visit(variant<Ts...>& v, Fs&&... fs) {
    return v.visit(overload{std::forward<Fs>(fs)...});
}

template <typename... Ts, typename... Fs>
decltype(auto) visit(const variant<Ts...>& v, Fs&&... fs) {
    return v.visit(overload{std::forward<Fs>(fs)...});
}

template <typename... Ts, typename... Fs>
decltype(auto) visit(variant<Ts...>&& v, Fs&&... fs) {
    return std::move(v).visit(overload{std::forward<Fs>(fs)...});
}

}  // namespace meow
