#pragma once

#include <array>        // std::array (cho bảng hàm `visit` nhanh)
#include <bit>          // std::bit_cast (C++20) - RẤT QUAN TRỌNG VÌ NÓ DÙNG NAN-BOXING!
#include <cmath>        // std::isnan
#include <cstddef>      // std::size_t, static_cast
#include <cstdint>      // uint64_t, uint8_t, uintptr_t, int64_t
#include <limits>       // std::numeric_limits
#include <stdexcept>    // std::bad_variant_access
#include <type_traits>  // std::integral_constant, std::decay_t, std::is_pointer, std::is_integral, std::is_floating_point
#include <type_traits>  // std::is_same, std::enable_if_t
#include <utility>      // std::forward, std::move
#include <variant>      // std::monostate, std::in_place_type_t, std::in_place_index_t

// Bây giờ include header utils (chứa detail::type_list, flatten, etc.)
#include "variant_utils.h"

namespace meow::utils {

// ---------------------- platform / macros ----------------------
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
#define MEOW_BYTE_ORDER __BYTE_ORDER__
#define MEOW_ORDER_LITTLE __ORDER_LITTLE_ENDIAN__
#endif

#if (defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)) && (defined(MEOW_BYTE_ORDER) && MEOW_BYTE_ORDER == MEOW_ORDER_LITTLE)
#define MEOW_LITTLE_64 1
#else
#define MEOW_LITTLE_64 0
#endif

static_assert(MEOW_LITTLE_64, "variant.h (NaN-boxing ultra) requires 64-bit little-endian platform.");

#if defined(__GNUC__) || defined(__clang__)
#define MEOW_ALWAYS_INLINE inline __attribute__((always_inline))
#define MEOW_PURE __attribute__((pure))
#define MEOW_HOT __attribute__((hot))
#else
#define MEOW_ALWAYS_INLINE inline
#define MEOW_PURE
#define MEOW_HOT
#endif

#if defined(__GNUC__) || defined(__clang__)
#define MEOW_LIKELY(x) (__builtin_expect(!!(x), 1))
#define MEOW_UNLIKELY(x) (__builtin_expect(!!(x), 0))
#define MEOW_ASSUME(x)                     \
    do {                                   \
        if (!(x)) __builtin_unreachable(); \
    } while (0)
#else
#define MEOW_LIKELY(x) (x)
#define MEOW_UNLIKELY(x) (x)
#define MEOW_ASSUME(x) (void)0
#endif

// ---------------------- classifiers ----------------------
template <typename T>
struct is_pointer_like {
    static constexpr bool value = std::is_pointer<std::decay_t<T>>::value && sizeof(std::decay_t<T>) <= 8 && alignof(std::decay_t<T>) <= 8;
};
template <typename T>
struct is_integral_like {
    static constexpr bool value = std::is_integral<std::decay_t<T>>::value && sizeof(std::decay_t<T>) <= 8 && !std::is_same<std::decay_t<T>, bool>::value;
};
template <typename T>
struct is_double_like {
    static constexpr bool value = std::is_floating_point<std::decay_t<T>>::value && sizeof(std::decay_t<T>) == 8;
};
template <typename T>
struct is_monostate_like {
    static constexpr bool value = std::is_same<std::decay_t<T>, std::monostate>::value;
};
template <typename T>
struct is_bool_like {
    static constexpr bool value = std::is_same<std::decay_t<T>, bool>::value;
};

// ---------------------- all_nanboxable_impl dùng
// meow::utils::detail::type_list ----------
template <typename List>
struct all_nanboxable_impl;

template <>
struct all_nanboxable_impl<detail::type_list<>> : std::true_type {};

template <typename H, typename... Ts>
struct all_nanboxable_impl<detail::type_list<H, Ts...>>
    : std::integral_constant<bool, (is_double_like<H>::value || is_integral_like<H>::value || is_pointer_like<H>::value || is_monostate_like<H>::value || is_bool_like<H>::value) &&
                                       all_nanboxable_impl<detail::type_list<Ts...>>::value> {};

// ---------------------- NaN-box constants ----------------------
static constexpr uint64_t MEOW_EXP_MASK = 0x7FF0000000000000ULL;
static constexpr uint64_t MEOW_QNAN_BIT = 0x0008000000000000ULL;
static constexpr uint64_t MEOW_QNAN_PREFIX = (MEOW_EXP_MASK | MEOW_QNAN_BIT);
static constexpr unsigned MEOW_TAG_SHIFT = 48;
static constexpr uint64_t MEOW_TAG_MASK = (0x7ULL << MEOW_TAG_SHIFT);
static constexpr uint64_t MEOW_PAYLOAD_MASK = ((1ULL << MEOW_TAG_SHIFT) - 1ULL);

// ---------------------- Fast primitives ----------------------
MEOW_ALWAYS_INLINE MEOW_PURE MEOW_HOT uint64_t meow_bitcast_double_to_u64(double d) noexcept {
    return std::bit_cast<uint64_t>(d);
}
MEOW_ALWAYS_INLINE MEOW_PURE MEOW_HOT double meow_bitcast_u64_to_double(uint64_t u) noexcept {
    return std::bit_cast<double>(u);
}

MEOW_ALWAYS_INLINE MEOW_PURE bool meow_is_raw_double(uint64_t b) noexcept {
    return ((b & MEOW_EXP_MASK) != MEOW_EXP_MASK);
}

MEOW_ALWAYS_INLINE MEOW_PURE uint8_t meow_tag_for_index(std::size_t idx) noexcept {
    return static_cast<uint8_t>(idx + 1);
}

MEOW_ALWAYS_INLINE MEOW_PURE uint64_t meow_payload_from_ptr(const void* p) noexcept {
    uintptr_t u = reinterpret_cast<uintptr_t>(p);
    return static_cast<uint64_t>(u & MEOW_PAYLOAD_MASK);
}

MEOW_ALWAYS_INLINE MEOW_PURE void* meow_ptr_from_payload(uint64_t payload) noexcept {
    uintptr_t u = static_cast<uintptr_t>(payload);
    return reinterpret_cast<void*>(u);
}

// ---------------------- NaNBoxedVariant (ultra) ----------------------
template <typename... Args>
class NaNBoxedVariant {
    // Dùng các tiện ích từ meow::utils::detail
    using flat_list = detail::flattened_unique_t<Args...>;
    static constexpr std::size_t alternatives_count = detail::type_list_length<flat_list>::value;
    static_assert(alternatives_count > 0, "Variant must have at least one alternative");
    static_assert(alternatives_count <= 8, "NaNBoxedVariant supports up to 8 alternatives");
    static_assert(all_nanboxable_impl<flat_list>::value, "All alternatives must be nanboxable");

   public:
    using inner_types = flat_list;
    using index_t = uint8_t;
    static constexpr index_t npos = static_cast<index_t>(-1);

    MEOW_ALWAYS_INLINE NaNBoxedVariant() noexcept {
        if constexpr (std::is_same<typename detail::nth_type<0, flat_list>::type, std::monostate>::value) {
            index_ = 0;
            encode_non_double(0, 0ULL);
        } else {
            index_ = npos;
            bits_ = 0ULL;
        }
    }
    ~NaNBoxedVariant() noexcept = default;

    MEOW_ALWAYS_INLINE NaNBoxedVariant(const NaNBoxedVariant& o) noexcept {
        bits_ = o.bits_;
        index_ = o.index_;
    }
    MEOW_ALWAYS_INLINE NaNBoxedVariant(NaNBoxedVariant&& o) noexcept {
        bits_ = o.bits_;
        index_ = o.index_;
        o.index_ = npos;
        o.bits_ = 0ULL;
    }

    template <typename T, typename U = std::decay_t<T>, typename = std::enable_if_t<(detail::type_list_index_of<U, flat_list>::value != detail::invalid_index)>>
    MEOW_ALWAYS_INLINE NaNBoxedVariant(T&& v) noexcept {
        using VT = std::decay_t<T>;
        constexpr std::size_t idx = detail::type_list_index_of<VT, flat_list>::value;
        // assign_from_type_impl<VT>(idx, std::forward<T>(v));
        assign_from_type_impl(idx, std::forward<T>(v));
    }

    template <typename T, typename... CArgs, typename U = std::decay_t<T>, typename = std::enable_if_t<(detail::type_list_index_of<U, flat_list>::value != detail::invalid_index)>>
    MEOW_ALWAYS_INLINE explicit NaNBoxedVariant(std::in_place_type_t<T>, CArgs&&... args) noexcept {
        using UT = std::decay_t<T>;
        UT tmp(std::forward<CArgs>(args)...);
        constexpr std::size_t idx = detail::type_list_index_of<UT, flat_list>::value;
        assign_from_type_impl<UT>(idx, std::move(tmp));
    }

    template <std::size_t I, typename... CArgs>
    MEOW_ALWAYS_INLINE explicit NaNBoxedVariant(std::in_place_index_t<I>, CArgs&&... args) noexcept {
        static_assert(I < alternatives_count, "in_place_index out of range");
        using U = typename detail::nth_type<I, flat_list>::type;
        U tmp(std::forward<CArgs>(args)...);
        assign_from_type_impl<U>(I, std::move(tmp));
    }

    MEOW_ALWAYS_INLINE NaNBoxedVariant& operator=(const NaNBoxedVariant& o) noexcept {
        bits_ = o.bits_;
        index_ = o.index_;
        return *this;
    }
    MEOW_ALWAYS_INLINE NaNBoxedVariant& operator=(NaNBoxedVariant&& o) noexcept {
        bits_ = o.bits_;
        index_ = o.index_;
        o.index_ = npos;
        o.bits_ = 0ULL;
        return *this;
    }

    template <typename T, typename U = std::decay_t<T>, typename = std::enable_if_t<(detail::type_list_index_of<U, flat_list>::value != detail::invalid_index)>>
    MEOW_ALWAYS_INLINE NaNBoxedVariant& operator=(T&& v) noexcept {
        using VT = std::decay_t<T>;
        constexpr std::size_t idx = detail::type_list_index_of<VT, flat_list>::value;
        // assign_from_type_impl<VT>(idx, std::forward<T>(v));
        assign_from_type_impl(idx, std::forward<T>(v));
        return *this;
    }

    template <typename T, typename... CArgs, typename U = std::decay_t<T>, typename = std::enable_if_t<(detail::type_list_index_of<U, flat_list>::value != detail::invalid_index)>>
    MEOW_ALWAYS_INLINE void emplace(CArgs&&... args) noexcept {
        using UT = std::decay_t<T>;
        UT tmp(std::forward<CArgs>(args)...);
        constexpr std::size_t idx = detail::type_list_index_of<UT, flat_list>::value;
        assign_from_type_impl<UT>(idx, std::move(tmp));
    }

    template <std::size_t I, typename... CArgs>
    MEOW_ALWAYS_INLINE void emplace_index(CArgs&&... args) noexcept {
        static_assert(I < alternatives_count, "emplace_index out of range");
        using U = typename detail::nth_type<I, flat_list>::type;
        U tmp(std::forward<CArgs>(args)...);
        assign_from_type_impl<U>(I, std::move(tmp));
    }

    [[nodiscard]] MEOW_ALWAYS_INLINE std::size_t index() const noexcept {
        return (index_ == npos ? detail::invalid_index : static_cast<std::size_t>(index_));
    }
    [[nodiscard]] MEOW_ALWAYS_INLINE bool valueless() const noexcept {
        return index_ == npos;
    }

    template <typename T>
    [[nodiscard]] MEOW_ALWAYS_INLINE bool holds() const noexcept {
        constexpr std::size_t idx = detail::type_list_index_of<std::decay_t<T>, flat_list>::value;
        if (idx == detail::invalid_index) return false;
        return index_ == static_cast<index_t>(idx);
    }
    template <typename T>
    [[nodiscard]] MEOW_ALWAYS_INLINE bool is() const noexcept {
        return holds<T>();
    }

    template <typename T>
    [[nodiscard]] MEOW_ALWAYS_INLINE std::decay_t<T> get() noexcept {
        return reconstruct_value<std::decay_t<T>>();
    }
    template <typename T>
    [[nodiscard]] MEOW_ALWAYS_INLINE std::decay_t<T> get() const noexcept {
        return reconstruct_value<std::decay_t<T>>();
    }
    template <typename T>
    [[nodiscard]] MEOW_ALWAYS_INLINE std::decay_t<T> safe_get() {
        if (MEOW_UNLIKELY(!holds<T>())) throw std::bad_variant_access();
        return reconstruct_value<std::decay_t<T>>();
    }
    template <typename T>
    [[nodiscard]] MEOW_ALWAYS_INLINE std::decay_t<T> safe_get() const {
        if (MEOW_UNLIKELY(!holds<T>())) throw std::bad_variant_access();
        return reconstruct_value<std::decay_t<T>>();
    }

    template <typename T>
    [[nodiscard]] MEOW_ALWAYS_INLINE std::decay_t<T>* get_if() noexcept {
        if (MEOW_UNLIKELY(!holds<T>())) return nullptr;
        thread_local static std::decay_t<T> temp;
        temp = reconstruct_value<std::decay_t<T>>();
        return &temp;
    }
    template <typename T>
    [[nodiscard]] MEOW_ALWAYS_INLINE const std::decay_t<T>* get_if() const noexcept {
        if (MEOW_UNLIKELY(!holds<T>())) return nullptr;
        thread_local static std::decay_t<T> temp;
        temp = reconstruct_value<std::decay_t<T>>();
        return &temp;
    }

    template <typename Visitor>
    MEOW_ALWAYS_INLINE decltype(auto) visit(Visitor&& vis) {
        if (MEOW_UNLIKELY(index_ == npos)) throw std::bad_variant_access();
        return visit_impl(std::forward<Visitor>(vis));
    }
    template <typename Visitor>
    MEOW_ALWAYS_INLINE decltype(auto) visit(Visitor&& vis) const {
        if (MEOW_UNLIKELY(index_ == npos)) throw std::bad_variant_access();
        return visit_impl_const(std::forward<Visitor>(vis));
    }

    MEOW_ALWAYS_INLINE void swap(NaNBoxedVariant& o) noexcept {
        uint64_t b = bits_;
        uint8_t ix = index_;
        bits_ = o.bits_;
        index_ = o.index_;
        o.bits_ = b;
        o.index_ = ix;
    }

    [[nodiscard]] MEOW_ALWAYS_INLINE uint64_t get_raw_bits() const noexcept {
        return bits_;
    }

    MEOW_ALWAYS_INLINE void set_raw_bits(uint64_t b) noexcept {
        bits_ = b;
        if (meow_is_raw_double(b)) {
            constexpr std::size_t dbl_idx = detail::type_list_index_of<double, flat_list>::value;
            index_ = (dbl_idx != detail::invalid_index) ? static_cast<index_t>(dbl_idx) : npos;
        } else {
            uint8_t tag = static_cast<uint8_t>((b & MEOW_TAG_MASK) >> MEOW_TAG_SHIFT);
            index_ = (tag == 0) ? ((detail::type_list_index_of<double, flat_list>::value != detail::invalid_index) ? static_cast<index_t>(detail::type_list_index_of<double, flat_list>::value) : npos)
                                : static_cast<index_t>((tag - 1) < alternatives_count ? (tag - 1) : 255);
            if (index_ == 255) index_ = npos;
        }
    }

    template <typename T, typename... CArgs>
    MEOW_ALWAYS_INLINE void emplace_or_assign(CArgs&&... args) noexcept {
        using U = std::decay_t<T>;
        constexpr std::size_t idx = detail::type_list_index_of<U, flat_list>::value;
        static_assert(idx != detail::invalid_index, "emplace_or_assign: type not in variant alternatives");
        if (index_ == static_cast<index_t>(idx)) {
            U tmp(std::forward<CArgs>(args)...);
            assign_payload_only<U>(tmp);
        } else {
            U tmp(std::forward<CArgs>(args)...);
            assign_from_type_impl<U>(idx, std::move(tmp));
        }
    }

    MEOW_ALWAYS_INLINE bool operator==(const NaNBoxedVariant& o) const noexcept {
        if (index_ != o.index_) return false;
        if (index_ == npos) return true;
        return bits_ == o.bits_;
    }
    MEOW_ALWAYS_INLINE bool operator!=(const NaNBoxedVariant& o) const noexcept {
        return !(*this == o);
    }

   private:
    uint64_t bits_ = 0ULL;
    index_t index_ = npos;

    MEOW_ALWAYS_INLINE static uint64_t double_to_bits(double d) noexcept {
        return meow_bitcast_double_to_u64(d);
    }
    MEOW_ALWAYS_INLINE static double bits_to_double(uint64_t b) noexcept {
        return meow_bitcast_u64_to_double(b);
    }
    MEOW_ALWAYS_INLINE static uint64_t encode_value_to_payload_impl_ptr(const void* p) noexcept {
        return meow_payload_from_ptr(p);
    }

    template <typename T>
    MEOW_ALWAYS_INLINE static uint64_t encode_value_to_payload(const T& v) noexcept {
        if constexpr (is_pointer_like<T>::value) {
            return encode_value_to_payload_impl_ptr(reinterpret_cast<const void*>(v));
        } else if constexpr (is_integral_like<T>::value) {
            return static_cast<uint64_t>(static_cast<int64_t>(v)) & MEOW_PAYLOAD_MASK;
        } else if constexpr (is_bool_like<T>::value) {
            return static_cast<uint64_t>(v ? 1ULL : 0ULL);
        } else {
            return 0ULL;
        }
    }

    template <typename T>
    MEOW_ALWAYS_INLINE static T decode_payload_to_value(uint64_t payload) noexcept {
        if constexpr (is_pointer_like<T>::value) {
            return reinterpret_cast<T>(meow_ptr_from_payload(payload));
        } else if constexpr (is_integral_like<T>::value) {
            uint64_t mask = (1ULL << 48) - 1ULL;
            uint64_t val = payload & mask;
            if (val & (1ULL << 47)) {
                return static_cast<T>(static_cast<int64_t>(val | (~mask)));
            } else {
                return static_cast<T>(static_cast<int64_t>(val));
            }
        } else if constexpr (is_bool_like<T>::value) {
            return static_cast<T>(payload != 0);
        } else {
            return T{};
        }
    }

    MEOW_ALWAYS_INLINE void encode_non_double(std::size_t idx, uint64_t payload) noexcept {
        uint8_t tag = meow_tag_for_index(idx);
        bits_ = MEOW_QNAN_PREFIX | ((static_cast<uint64_t>(tag) << MEOW_TAG_SHIFT) & MEOW_TAG_MASK) | (payload & MEOW_PAYLOAD_MASK);
    }

    template <typename T>
    MEOW_ALWAYS_INLINE void assign_from_type_impl(std::size_t idx, T&& v) noexcept {
        using U = std::decay_t<T>;
        if constexpr (is_double_like<U>::value) {
            double d = static_cast<double>(v);
            bits_ = (!std::isnan(d)) ? double_to_bits(d) : (MEOW_QNAN_PREFIX | 0ULL);
            index_ = static_cast<index_t>(idx);
        } else {
            uint64_t payload = encode_value_to_payload<U>(v);
            encode_non_double(idx, payload);
            index_ = static_cast<index_t>(idx);
        }
    }

    template <typename T>
    MEOW_ALWAYS_INLINE void assign_payload_only(const T& v) noexcept {
        using U = std::decay_t<T>;
        if constexpr (is_double_like<U>::value) {
            double d = static_cast<double>(v);
            bits_ = (!std::isnan(d)) ? double_to_bits(d) : (MEOW_QNAN_PREFIX | 0ULL);
        } else {
            uint64_t payload = encode_value_to_payload<U>(v);
            uint8_t tag = meow_tag_for_index(static_cast<std::size_t>(index_));
            bits_ = MEOW_QNAN_PREFIX | ((static_cast<uint64_t>(tag) << MEOW_TAG_SHIFT) & MEOW_TAG_MASK) | (payload & MEOW_PAYLOAD_MASK);
        }
    }

    MEOW_ALWAYS_INLINE uint64_t get_payload() const noexcept {
        return bits_ & MEOW_PAYLOAD_MASK;
    }
    MEOW_ALWAYS_INLINE uint8_t get_tag() const noexcept {
        return static_cast<uint8_t>((bits_ & MEOW_TAG_MASK) >> MEOW_TAG_SHIFT);
    }

    // template <typename T>
    // MEOW_ALWAYS_INLINE T reconstruct_value() const noexcept {
    //     using U = std::decay_t<T>;
    //     if (meow_is_raw_double(bits_)) {
    //         return static_cast<T>(bits_to_double(bits_));
    //     } else {
    //         if (get_tag() == 0) {
    //             return (is_double_like<U>::value) ? std::numeric_limits<double>::quiet_NaN() : U{};
    //         }
    //         return decode_payload_to_value<U>(get_payload());
    //     }
    // }

    template <typename T>
    MEOW_ALWAYS_INLINE T reconstruct_value() const noexcept {
        using U = std::decay_t<T>;
        if (meow_is_raw_double(bits_)) {
            if constexpr (is_double_like<U>::value) {
                return static_cast<T>(bits_to_double(bits_));
            } else {
                return T{};
            }
        } else {
            if (get_tag() == 0) {
                if constexpr (is_double_like<U>::value) {
                    return std::numeric_limits<double>::quiet_NaN();
                } else {
                    return U{};
                }
            }
            if constexpr (!is_double_like<U>::value) {
                return decode_payload_to_value<U>(get_payload());
            } else {
                return T{};
            }
        }
    }


    // visit impl
    template <typename Visitor, std::size_t... Is>
    MEOW_ALWAYS_INLINE decltype(auto) visit_impl_helper(Visitor&& vis, std::index_sequence<Is...>) const {
        using R = decltype(std::declval<Visitor>()(reconstruct_value<typename detail::nth_type<0, flat_list>::type>()));
        using fn_t = R (*)(const NaNBoxedVariant*, Visitor&&);

        static constexpr std::array<fn_t, alternatives_count> fns = {{+[](const NaNBoxedVariant* v, Visitor&& vis2) -> R {
            using T = typename detail::nth_type<Is, flat_list>::type;
            return std::forward<Visitor>(vis2)(v->template reconstruct_value<T>());
        }...}};
        return fns[index_](this, std::forward<Visitor>(vis));
    }

    template <typename Visitor>
    MEOW_ALWAYS_INLINE decltype(auto) visit_impl(Visitor&& vis) {
        return visit_impl_helper(std::forward<Visitor>(vis), std::make_index_sequence<alternatives_count>{});
    }

    template <typename Visitor>
    MEOW_ALWAYS_INLINE decltype(auto) visit_impl_const(Visitor&& vis) const {
        return visit_impl_helper(std::forward<Visitor>(vis), std::make_index_sequence<alternatives_count>{});
    }
};

// non-member swap (fast)
template <typename... Ts>
MEOW_ALWAYS_INLINE void swap(NaNBoxedVariant<Ts...>& a, NaNBoxedVariant<Ts...>& b) noexcept {
    a.swap(b);
}

}  // namespace meow::utils
