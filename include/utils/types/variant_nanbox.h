/**
 * @file variant.h
 * @brief NaN-boxing Variant — ultra-optimized edition (64-bit little-endian).
 * @author LazyPaws & Mèo mất não (2025)
 *
 * FULL SPEED MODE:
 *  - Requires C++20 (std::bit_cast) or GCC/Clang builtins.
 *  - Assumes 64-bit little-endian. All nanboxability checks remain.
 *  - Uses uintptr_t conversions for pointer packing/unpacking (fast).
 *  - Optional tiny asm fast-paths for x86_64 (guarded).
 *
 * WARNING: Extremely performance-oriented. Thread-local scratch is used for get()/get_if().
 *          Reentrancy on same thread will clobber returned references. You asked for this.
 */

#pragma once

#include "common/pch.h"

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>
#include <limits>
#include <stdexcept>
#include <array>
#include <bit>         // std::bit_cast (C++20)
#include <cassert>

namespace meow::utils {

// ---------------------- platform / macros ----------------------
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
#  define MEOW_BYTE_ORDER __BYTE_ORDER__
#  define MEOW_ORDER_LITTLE __ORDER_LITTLE_ENDIAN__
#endif

#if (defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)) && (defined(MEOW_BYTE_ORDER) && MEOW_BYTE_ORDER == MEOW_ORDER_LITTLE)
#  define MEOW_LITTLE_64 1
#else
#  define MEOW_LITTLE_64 0
#endif

static_assert(MEOW_LITTLE_64, "variant.h (NaN-boxing ultra) requires 64-bit little-endian platform.");

#if defined(__GNUC__) || defined(__clang__)
#  define MEOW_ALWAYS_INLINE inline __attribute__((always_inline))
#  define MEOW_PURE __attribute__((pure))
#  define MEOW_HOT __attribute__((hot))
#else
#  define MEOW_ALWAYS_INLINE inline
#  define MEOW_PURE
#  define MEOW_HOT
#endif

#if defined(__GNUC__) || defined(__clang__)
#  define MEOW_LIKELY(x)   (__builtin_expect(!!(x), 1))
#  define MEOW_UNLIKELY(x) (__builtin_expect(!!(x), 0))
#  define MEOW_ASSUME(x)   do { if(!(x)) __builtin_unreachable(); } while(0)
#else
#  define MEOW_LIKELY(x) (x)
#  define MEOW_UNLIKELY(x) (x)
#  define MEOW_ASSUME(x) (void)0
#endif

// ---------------------- tiny meta helpers ----------------------
template <typename... Ts> struct type_list {};

template <typename List> struct tl_length;
template <typename... Ts>
struct tl_length<type_list<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)> {};

template <std::size_t I, typename List> struct nth_type;
template <std::size_t I, typename Head, typename... Tail>
struct nth_type<I, type_list<Head, Tail...>> : nth_type<I - 1, type_list<Tail...>> {};
template <typename Head, typename... Tail>
struct nth_type<0, type_list<Head, Tail...>> { using type = Head; };

template <typename T, typename List> struct tl_index_of;
template <typename T> struct tl_index_of<T, type_list<>> { static constexpr std::size_t value = static_cast<std::size_t>(-1); };
template <typename T, typename Head, typename... Tail>
struct tl_index_of<T, type_list<Head, Tail...>> {
private:
    static constexpr std::size_t next = tl_index_of<T, type_list<Tail...>>::value;
public:
    static constexpr std::size_t value = std::is_same<T, Head>::value ? 0
        : (next == static_cast<std::size_t>(-1) ? static_cast<std::size_t>(-1) : 1 + next);
};

template <typename... Args>
using flattened_unique_t = type_list<Args...>; // caller ensures flattening/uniqueness

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

template <typename List> struct all_nanboxable_impl;
template <> struct all_nanboxable_impl<type_list<>> : std::true_type {};
template <typename H, typename... Ts>
struct all_nanboxable_impl<type_list<H, Ts...>> : std::integral_constant<bool,
    (is_double_like<H>::value || is_integral_like<H>::value || is_pointer_like<H>::value || is_monostate_like<H>::value || is_bool_like<H>::value)
    && all_nanboxable_impl<type_list<Ts...>>::value> {};

// ---------------------- NaN-box constants ----------------------
static constexpr uint64_t MEOW_EXP_MASK = 0x7FF0000000000000ULL;
static constexpr uint64_t MEOW_QNAN_BIT = 0x0008000000000000ULL;
static constexpr uint64_t MEOW_QNAN_PREFIX = (MEOW_EXP_MASK | MEOW_QNAN_BIT);
static constexpr unsigned MEOW_TAG_SHIFT = 48;
static constexpr uint64_t MEOW_TAG_MASK = (0x7ULL << MEOW_TAG_SHIFT);
static constexpr uint64_t MEOW_PAYLOAD_MASK = ((1ULL << MEOW_TAG_SHIFT) - 1ULL);

// ---------------------- Fast primitives ----------------------
MEOW_ALWAYS_INLINE MEOW_PURE MEOW_HOT
uint64_t meow_bitcast_double_to_u64(double d) noexcept {
    // std::bit_cast should compile to a register move
    return std::bit_cast<uint64_t>(d);
}
MEOW_ALWAYS_INLINE MEOW_PURE MEOW_HOT
double meow_bitcast_u64_to_double(uint64_t u) noexcept {
    return std::bit_cast<double>(u);
}

MEOW_ALWAYS_INLINE MEOW_PURE
bool meow_is_raw_double(uint64_t b) noexcept {
    return ((b & MEOW_EXP_MASK) != MEOW_EXP_MASK);
}

MEOW_ALWAYS_INLINE MEOW_PURE
uint8_t meow_tag_for_index(std::size_t idx) noexcept {
    return static_cast<uint8_t>(idx + 1);
}

// pointer packing/unpacking using uintptr_t (fast & standard)
MEOW_ALWAYS_INLINE MEOW_PURE
uint64_t meow_payload_from_ptr(const void* p) noexcept {
#if defined(__x86_64__) && defined(MEOW_USE_ASM_FAST_PTR)
    // optional asm path (very small): mov pointer -> rax ; and mask
    uint64_t v;
    asm volatile("movq %1, %0" : "=r"(v) : "r"(p) : );
    return v & MEOW_PAYLOAD_MASK;
#else
    uintptr_t u = reinterpret_cast<uintptr_t>(p);
    return static_cast<uint64_t>(u & MEOW_PAYLOAD_MASK);
#endif
}

MEOW_ALWAYS_INLINE MEOW_PURE
void* meow_ptr_from_payload(uint64_t payload) noexcept {
#if defined(__x86_64__) && defined(MEOW_USE_ASM_FAST_PTR)
    uint64_t v = payload;
    void* p;
    asm volatile("movq %1, %0" : "=r"(p) : "r"(v) : );
    return p;
#else
    uintptr_t u = static_cast<uintptr_t>(payload);
    return reinterpret_cast<void*>(u);
#endif
}

// ---------------------- NaNBoxedVariant (ultra) ----------------------
template <typename... Args>
class NaNBoxedVariant {
    using flat_list = flattened_unique_t<Args...>;
    static constexpr std::size_t alternatives_count = tl_length<flat_list>::value;
    static_assert(alternatives_count > 0, "Variant must have at least one alternative");
    static_assert(alternatives_count <= 8, "NaNBoxedVariant supports up to 8 alternatives");
    static_assert(all_nanboxable_impl<flat_list>::value, "All alternatives must be nanboxable");

public:
    using inner_types = flat_list;
    using index_t = uint8_t;
    static constexpr index_t npos = static_cast<index_t>(-1);

    // ctor/dtor
    MEOW_ALWAYS_INLINE NaNBoxedVariant() noexcept {
        if constexpr (std::is_same<typename nth_type<0, flat_list>::type, std::monostate>::value) {
            index_ = 0;
            encode_non_double(0, 0ULL);
        } else {
            index_ = npos;
            bits_ = 0ULL;
        }
    }
    ~NaNBoxedVariant() noexcept = default;

    MEOW_ALWAYS_INLINE NaNBoxedVariant(const NaNBoxedVariant& o) noexcept { bits_ = o.bits_; index_ = o.index_; }
    MEOW_ALWAYS_INLINE NaNBoxedVariant(NaNBoxedVariant&& o) noexcept { bits_ = o.bits_; index_ = o.index_; o.index_ = npos; o.bits_ = 0ULL; }

    // construct-from-value (fast)
    template <typename T, typename U = std::decay_t<T>,
              typename = std::enable_if_t<(tl_index_of<U, flat_list>::value != static_cast<std::size_t>(-1))>>
    MEOW_ALWAYS_INLINE NaNBoxedVariant(T&& v) noexcept {
        using VT = std::decay_t<T>;
        constexpr std::size_t idx = tl_index_of<VT, flat_list>::value;
        assign_from_type_impl<VT>(idx, std::forward<T>(v));
    }

    // in_place
    template <typename T, typename... CArgs, typename U = std::decay_t<T>,
              typename = std::enable_if_t<(tl_index_of<U, flat_list>::value != static_cast<std::size_t>(-1))>>
    MEOW_ALWAYS_INLINE explicit NaNBoxedVariant(std::in_place_type_t<T>, CArgs&&... args) noexcept {
        using U = std::decay_t<T>;
        U tmp(std::forward<CArgs>(args)...);
        constexpr std::size_t idx = tl_index_of<U, flat_list>::value;
        assign_from_type_impl<U>(idx, std::move(tmp));
    }

    template <std::size_t I, typename... CArgs>
    MEOW_ALWAYS_INLINE explicit NaNBoxedVariant(std::in_place_index_t<I>, CArgs&&... args) noexcept {
        static_assert(I < alternatives_count, "in_place_index out of range");
        using U = typename nth_type<I, flat_list>::type;
        U tmp(std::forward<CArgs>(args)...);
        assign_from_type_impl<U>(I, std::move(tmp));
    }

    // assign
    MEOW_ALWAYS_INLINE NaNBoxedVariant& operator=(const NaNBoxedVariant& o) noexcept { bits_ = o.bits_; index_ = o.index_; return *this; }
    MEOW_ALWAYS_INLINE NaNBoxedVariant& operator=(NaNBoxedVariant&& o) noexcept { bits_ = o.bits_; index_ = o.index_; o.index_ = npos; o.bits_ = 0ULL; return *this; }

    template <typename T, typename U = std::decay_t<T>,
              typename = std::enable_if_t<(tl_index_of<U, flat_list>::value != static_cast<std::size_t>(-1))>>
    MEOW_ALWAYS_INLINE NaNBoxedVariant& operator=(T&& v) noexcept {
        using VT = std::decay_t<T>;
        constexpr std::size_t idx = tl_index_of<VT, flat_list>::value;
        assign_from_type_impl<VT>(idx, std::forward<T>(v));
        return *this;
    }

    // emplace/emplace_index
    template <typename T, typename... CArgs, typename U = std::decay_t<T>,
              typename = std::enable_if_t<(tl_index_of<U, flat_list>::value != static_cast<std::size_t>(-1))>>
    MEOW_ALWAYS_INLINE void emplace(CArgs&&... args) noexcept {
        using U = std::decay_t<T>;
        U tmp(std::forward<CArgs>(args)...);
        constexpr std::size_t idx = tl_index_of<U, flat_list>::value;
        assign_from_type_impl<U>(idx, std::move(tmp));
    }

    template <std::size_t I, typename... CArgs>
    MEOW_ALWAYS_INLINE void emplace_index(CArgs&&... args) noexcept {
        static_assert(I < alternatives_count, "emplace_index out of range");
        using U = typename nth_type<I, flat_list>::type;
        U tmp(std::forward<CArgs>(args)...);
        assign_from_type_impl<U>(I, std::move(tmp));
    }

    // queries
    MEOW_ALWAYS_INLINE [[nodiscard]] std::size_t index() const noexcept { return (index_ == npos ? static_cast<std::size_t>(-1) : static_cast<std::size_t>(index_)); }
    MEOW_ALWAYS_INLINE [[nodiscard]] bool valueless() const noexcept { return index_ == npos; }

    // holds/is
    template <typename T>
    MEOW_ALWAYS_INLINE [[nodiscard]] bool holds() const noexcept {
        constexpr std::size_t idx = tl_index_of<std::decay_t<T>, flat_list>::value;
        if (idx == static_cast<std::size_t>(-1)) return false;
        return index_ == static_cast<index_t>(idx);
    }
    template <typename T>
    MEOW_ALWAYS_INLINE [[nodiscard]] bool is() const noexcept { return holds<T>(); }

    // ---------------------- ultra-fast thread_local scratch (you asked) ----------------------
    // 8-byte aligned buffer; returned references are valid until next Variant op on same thread.
    static inline void* tls_scratch_ptr() noexcept {
        thread_local alignas(8) unsigned char tls_storage[8];
        return static_cast<void*>(tls_storage);
    }
    template <typename T>
    static MEOW_ALWAYS_INLINE T* tls_write_and_get(T&& v) noexcept {
        void* p = tls_scratch_ptr();
        // zero 8 bytes (fast) then copy
        std::memset(p, 0, 8);
        std::memcpy(p, &v, sizeof(T));
        return reinterpret_cast<T*>(p);
    }

    // ---------------------- ACCESSORS (UNSAFE but fastest) ----------------------

    // unchecked get (reference into tls)
    template <typename T>
    MEOW_ALWAYS_INLINE [[nodiscard]] T& get() noexcept {
        using U = std::decay_t<T>;
        U tmp = reconstruct_value<U>();
        return *tls_write_and_get<U>(std::move(tmp));
    }
    template <typename T>
    MEOW_ALWAYS_INLINE [[nodiscard]] const T& get() const noexcept {
        using U = std::decay_t<T>;
        U tmp = reconstruct_value<U>();
        return *tls_write_and_get<U>(std::move(tmp));
    }

    // safe_get (checks)
    template <typename T>
    MEOW_ALWAYS_INLINE [[nodiscard]] T& safe_get() {
        if (MEOW_UNLIKELY(!holds<T>())) throw std::bad_variant_access();
        T tmp = reconstruct_value<std::decay_t<T>>();
        return *tls_write_and_get<T>(std::move(tmp));
    }
    template <typename T>
    MEOW_ALWAYS_INLINE [[nodiscard]] const T& safe_get() const {
        if (MEOW_UNLIKELY(!holds<T>())) throw std::bad_variant_access();
        T tmp = reconstruct_value<std::decay_t<T>>();
        return *tls_write_and_get<T>(std::move(tmp));
    }

    // get_if returns pointer into tls or nullptr
    template <typename T>
    MEOW_ALWAYS_INLINE [[nodiscard]] T* get_if() noexcept {
        if (MEOW_UNLIKELY(!holds<T>())) return nullptr;
        T tmp = reconstruct_value<std::decay_t<T>>();
        return tls_write_and_get<T>(std::move(tmp));
    }
    template <typename T>
    MEOW_ALWAYS_INLINE [[nodiscard]] const T* get_if() const noexcept {
        if (MEOW_UNLIKELY(!holds<T>())) return nullptr;
        T tmp = reconstruct_value<std::decay_t<T>>();
        return tls_write_and_get<T>(std::move(tmp));
    }

    // visit (O(1) jump table)
    template <typename Visitor>
    MEOW_ALWAYS_INLINE decltype(auto) visit(Visitor&& vis) {
        if (MEOW_UNLIKELY(index_ == npos)) throw std::bad_variant_access();
        return visit_impl(std::forward<Visitor>(vis), std::make_index_sequence<alternatives_count>{});
    }
    template <typename Visitor>
    MEOW_ALWAYS_INLINE decltype(auto) visit(Visitor&& vis) const {
        if (MEOW_UNLIKELY(index_ == npos)) throw std::bad_variant_access();
        return visit_impl_const(std::forward<Visitor>(vis), std::make_index_sequence<alternatives_count>{});
    }

    MEOW_ALWAYS_INLINE void swap(NaNBoxedVariant& o) noexcept { uint64_t b = bits_; uint8_t ix = index_; bits_ = o.bits_; index_ = o.index_; o.bits_ = b; o.index_ = ix; }

    // ---------------------- ultra low-level super-fast helpers ----------------------
    MEOW_ALWAYS_INLINE [[nodiscard]] uint64_t get_raw_bits() const noexcept { return bits_; }

    MEOW_ALWAYS_INLINE void set_raw_bits(uint64_t b) noexcept {
        bits_ = b;
        // best-effort index decode (branchless-ish)
        if (meow_is_raw_double(b)) {
            constexpr std::size_t dbl_idx = tl_index_of<double, flat_list>::value;
            index_ = (dbl_idx != static_cast<std::size_t>(-1)) ? static_cast<index_t>(dbl_idx) : npos;
        } else {
            uint8_t tag = static_cast<uint8_t>((b & MEOW_TAG_MASK) >> MEOW_TAG_SHIFT);
            index_ = (tag == 0) ? ( (tl_index_of<double, flat_list>::value != static_cast<std::size_t>(-1)) ? static_cast<index_t>(tl_index_of<double, flat_list>::value) : npos )
                                : static_cast<index_t>((tag - 1) < alternatives_count ? (tag - 1) : 255);
            if (index_ == 255) index_ = npos;
        }
    }

    MEOW_ALWAYS_INLINE [[nodiscard]] uint64_t peek_payload() const noexcept { return bits_ & MEOW_PAYLOAD_MASK; }
    MEOW_ALWAYS_INLINE [[nodiscard]] uint8_t raw_tag() const noexcept { return static_cast<uint8_t>((bits_ & MEOW_TAG_MASK) >> MEOW_TAG_SHIFT); }

    MEOW_ALWAYS_INLINE [[nodiscard]] int64_t as_int48() const noexcept {
        uint64_t payload = peek_payload();
        const uint64_t mask = (1ULL << 48) - 1ULL;
        uint64_t val = payload & mask;
        if (val & (1ULL << 47)) {
            uint64_t full = val | (~mask);
            return static_cast<int64_t>(full);
        } else {
            return static_cast<int64_t>(val);
        }
    }

    template <typename PtrT = void*>
    MEOW_ALWAYS_INLINE [[nodiscard]] PtrT as_ptr() const noexcept {
        void* p = meow_ptr_from_payload(peek_payload());
        return reinterpret_cast<PtrT>(p);
    }

    template <typename T, typename... CArgs>
    MEOW_ALWAYS_INLINE void emplace_or_assign(CArgs&&... args) noexcept {
        using U = std::decay_t<T>;
        constexpr std::size_t idx = tl_index_of<U, flat_list>::value;
        static_assert(idx != static_cast<std::size_t>(-1), "emplace_or_assign: type not in variant alternatives");
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
    MEOW_ALWAYS_INLINE bool operator!=(const NaNBoxedVariant& o) const noexcept { return !(*this == o); }

    static constexpr std::size_t alternatives() noexcept { return alternatives_count; }
    static constexpr std::size_t storage_size_bytes() noexcept { return sizeof(bits_); }

private:
    uint64_t bits_ = 0ULL;
    index_t index_ = npos;

    // fast bit casts (std::bit_cast -> register moves)
    MEOW_ALWAYS_INLINE static uint64_t double_to_bits(double d) noexcept { return meow_bitcast_double_to_u64(d); }
    MEOW_ALWAYS_INLINE static double bits_to_double(uint64_t b) noexcept { return meow_bitcast_u64_to_double(b); }

    MEOW_ALWAYS_INLINE static uint64_t encode_value_to_payload_impl_ptr(const void* p) noexcept { return meow_payload_from_ptr(p); }

    template <typename T>
    MEOW_ALWAYS_INLINE static uint64_t encode_value_to_payload(const T& v) noexcept {
        if constexpr (is_pointer_like<T>::value) {
            return encode_value_to_payload_impl_ptr(reinterpret_cast<const void*>(v));
        } else if constexpr (is_integral_like<T>::value) {
            int64_t vi = static_cast<int64_t>(v);
            uint64_t u = static_cast<uint64_t>(vi) & MEOW_PAYLOAD_MASK;
            return u;
        } else if constexpr (is_bool_like<T>::value) {
            return static_cast<uint64_t>(v ? 1ULL : 0ULL);
        } else {
            return 0ULL;
        }
    }

    template <typename T>
    MEOW_ALWAYS_INLINE static T decode_payload_to_value(uint64_t payload) noexcept {
        if constexpr (is_pointer_like<T>::value) {
            void* p = meow_ptr_from_payload(payload);
            T pp = reinterpret_cast<T>(p);
            return pp;
        } else if constexpr (is_integral_like<T>::value) {
            uint64_t mask = (1ULL << 48) - 1ULL;
            uint64_t val = payload & mask;
            if (val & (1ULL << 47)) {
                uint64_t sign_ext = (~mask);
                uint64_t full = val | sign_ext;
                return static_cast<T>(static_cast<int64_t>(full));
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
            if (!std::isnan(d)) {
                bits_ = double_to_bits(d);
                index_ = static_cast<index_t>(idx);
            } else {
                bits_ = MEOW_QNAN_PREFIX | 0ULL;
                index_ = static_cast<index_t>(idx);
            }
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

    MEOW_ALWAYS_INLINE uint64_t get_payload() const noexcept { return bits_ & MEOW_PAYLOAD_MASK; }
    MEOW_ALWAYS_INLINE uint8_t get_tag() const noexcept { return static_cast<uint8_t>((bits_ & MEOW_TAG_MASK) >> MEOW_TAG_SHIFT); }

    template <typename T>
    MEOW_ALWAYS_INLINE T reconstruct_value() const noexcept {
        using U = std::decay_t<T>;
        uint64_t b = bits_;
        if (meow_is_raw_double(b)) {
            double d = bits_to_double(b);
            return static_cast<T>(d);
        } else {
            uint8_t tag = get_tag();
            if (tag == 0) {
                if constexpr (is_double_like<U>::value) {
                    return std::numeric_limits<double>::quiet_NaN();
                } else {
                    return U{};
                }
            } else {
                uint64_t payload = get_payload();
                return decode_payload_to_value<U>(payload);
            }
        }
    }

    // visit impl (fast array of fptrs)
    template <typename Visitor, std::size_t... Is>
    MEOW_ALWAYS_INLINE decltype(auto) visit_impl(Visitor&& vis, std::index_sequence<Is...>) {
        using R = decltype(std::declval<Visitor>()(reconstruct_value<typename nth_type<0, flat_list>::type>()));
        using fn_t = R(*)(const NaNBoxedVariant*, Visitor&&);
        std::array<fn_t, alternatives_count> fns = {{
            +[](const NaNBoxedVariant* v, Visitor&& vis2)->R { using T = typename nth_type<0, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct_value<T>()); }
#if (alternatives_count > 1)
            ,+[](const NaNBoxedVariant* v, Visitor&& vis2)->R { using T = typename nth_type<1, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct_value<T>()); }
#endif
#if (alternatives_count > 2)
            ,+[](const NaNBoxedVariant* v, Visitor&& vis2)->R { using T = typename nth_type<2, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct_value<T>()); }
#endif
#if (alternatives_count > 3)
            ,+[](const NaNBoxedVariant* v, Visitor&& vis2)->R { using T = typename nth_type<3, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct_value<T>()); }
#endif
#if (alternatives_count > 4)
            ,+[](const NaNBoxedVariant* v, Visitor&& vis2)->R { using T = typename nth_type<4, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct_value<T>()); }
#endif
#if (alternatives_count > 5)
            ,+[](const NaNBoxedVariant* v, Visitor&& vis2)->R { using T = typename nth_type<5, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct_value<T>()); }
#endif
#if (alternatives_count > 6)
            ,+[](const NaNBoxedVariant* v, Visitor&& vis2)->R { using T = typename nth_type<6, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct_value<T>()); }
#endif
#if (alternatives_count > 7)
            ,+[](const NaNBoxedVariant* v, Visitor&& vis2)->R { using T = typename nth_type<7, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct_value<T>()); }
#endif
        }};
        return fns[index_](this, std::forward<Visitor>(vis));
    }

    template <typename Visitor, std::size_t... Is>
    MEOW_ALWAYS_INLINE decltype(auto) visit_impl_const(Visitor&& vis, std::index_sequence<Is...>) const {
        using R = decltype(std::declval<Visitor>()(reconstruct_value<typename nth_type<0, flat_list>::type>()));
        using fn_t = R(*)(const NaNBoxedVariant*, Visitor&&);
        std::array<fn_t, alternatives_count> fns = {{
            +[](const NaNBoxedVariant* v, Visitor&& vis2)->R { using T = typename nth_type<0, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct_value<T>()); }
#if (alternatives_count > 1)
            ,+[](const NaNBoxedVariant* v, Visitor&& vis2)->R { using T = typename nth_type<1, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct_value<T>()); }
#endif
#if (alternatives_count > 2)
            ,+[](const NaNBoxedVariant* v, Visitor&& vis2)->R { using T = typename nth_type<2, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct_value<T>()); }
#endif
#if (alternatives_count > 3)
            ,+[](const NaNBoxedVariant* v, Visitor&& vis2)->R { using T = typename nth_type<3, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct_value<T>()); }
#endif
#if (alternatives_count > 4)
            ,+[](const NaNBoxedVariant* v, Visitor&& vis2)->R { using T = typename nth_type<4, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct_value<T>()); }
#endif
#if (alternatives_count > 5)
            ,+[](const NaNBoxedVariant* v, Visitor&& vis2)->R { using T = typename nth_type<5, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct_value<T>()); }
#endif
#if (alternatives_count > 6)
            ,+[](const NaNBoxedVariant* v, Visitor&& vis2)->R { using T = typename nth_type<6, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct_value<T>()); }
#endif
#if (alternatives_count > 7)
            ,+[](const NaNBoxedVariant* v, Visitor&& vis2)->R { using T = typename nth_type<7, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct_value<T>()); }
#endif
        }};
        return fns[index_](this, std::forward<Visitor>(vis));
    }
};

// non-member swap (fast)
template <typename... Ts>
MEOW_ALWAYS_INLINE void swap(NaNBoxedVariant<Ts...>& a, NaNBoxedVariant<Ts...>& b) noexcept { a.swap(b); }

// ---------------------- public Variant alias ----------------------
template <typename... Args>
class Variant : public NaNBoxedVariant<Args...> {
    using base_t = NaNBoxedVariant<Args...>;
public:
    using base_t::base_t;
    using base_t::operator=;
    Variant() noexcept = default;
    ~Variant() noexcept = default;

    using base_t::index;
    [[nodiscard]] bool valueless() const noexcept { return base_t::valueless(); }

    template <typename T>
    [[nodiscard]] bool holds() const noexcept { return base_t::template holds<T>(); }
    template <typename T>
    [[nodiscard]] bool is() const noexcept { return holds<T>(); }

    template <typename T>
    [[nodiscard]] T& get() noexcept { return base_t::template get<T>(); }
    template <typename T>
    [[nodiscard]] const T& get() const noexcept { return base_t::template get<T>(); }

    template <typename T>
    [[nodiscard]] T& safe_get() { return base_t::template safe_get<T>(); }
    template <typename T>
    [[nodiscard]] const T& safe_get() const { return base_t::template safe_get<T>(); }

    template <typename T>
    [[nodiscard]] T* get_if() noexcept { return base_t::template get_if<T>(); }
    template <typename T>
    [[nodiscard]] const T* get_if() const noexcept { return base_t::template get_if<T>(); }

    template <typename Visitor>
    decltype(auto) visit(Visitor&& vis) { return base_t::template visit<Visitor>(std::forward<Visitor>(vis)); }
    template <typename Visitor>
    decltype(auto) visit(Visitor&& vis) const { return base_t::template visit<Visitor>(std::forward<Visitor>(vis)); }

    using base_t::swap;
    using base_t::emplace_or_assign;
    using base_t::get_raw_bits;
    using base_t::set_raw_bits;
    using base_t::peek_payload;
    using base_t::raw_tag;
    using base_t::as_int48;
    using base_t::as_ptr;
};

// non-member swap for Variant<>
template <typename... Ts>
MEOW_ALWAYS_INLINE void swap(Variant<Ts...>& a, Variant<Ts...>& b) noexcept { a.swap(b); }

} // namespace meow::utils

