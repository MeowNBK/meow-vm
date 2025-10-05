// variant_last_breath.h
// Drop-in high-performance Variant for meow::utils::Variant
// - Aggressive NaN: enabled by defining MEOW_AGGRESSIVE_NAN=1 at compile time (x86_64/aarch64 little-endian).
// - Conservative NaN: default NaN-box with 48-bit payload (3 tag bits).
// - Packed64 fallback: 8-byte payload + 1-byte tag.
// WARNING: aggressive mode trades portability and pointer coverage for speed.

#pragma once

#include "common/pch.h"

namespace meow::utils {

// -------------------- minimal type_list + helpers --------------------
template <typename... Ts> struct type_list {};

template <typename List, typename T> struct tl_append;
template <typename... Ts, typename T>
struct tl_append<type_list<Ts...>, T> { using type = type_list<Ts..., T>; };

template <typename T, typename List> struct tl_contains;
template <typename T> struct tl_contains<T, type_list<>> : std::false_type {};
template <typename T, typename Head, typename... Tail>
struct tl_contains<T, type_list<Head, Tail...>>
    : std::conditional_t<std::is_same<T, Head>::value, std::true_type, tl_contains<T, type_list<Tail...>>> {};

template <typename List, typename T>
struct tl_append_unique {
    using type = std::conditional_t<tl_contains<T, List>::value, List, typename tl_append<List, T>::type>;
};

template <std::size_t I, typename List> struct nth_type;
template <std::size_t I, typename Head, typename... Tail>
struct nth_type<I, type_list<Head, Tail...>> : nth_type<I - 1, type_list<Tail...>> {};
template <typename Head, typename... Tail>
struct nth_type<0, type_list<Head, Tail...>> { using type = Head; };

template <typename List> struct tl_length;
template <typename... Ts> struct tl_length<type_list<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)> {};

template <typename T, typename List> struct tl_index_of;
template <typename T> struct tl_index_of<T, type_list<>> { static constexpr std::size_t value = static_cast<std::size_t>(-1); };
template <typename T, typename Head, typename... Tail>
struct tl_index_of<T, type_list<Head, Tail...>> {
    static constexpr std::size_t next = tl_index_of<T, type_list<Tail...>>::value;
    static constexpr std::size_t value = std::is_same<T, Head>::value ? 0 : (next == static_cast<std::size_t>(-1) ? static_cast<std::size_t>(-1) : 1 + next);
};

// flatten nested variants (simple and fast for our usecases - expands nested Variants)
template <typename...> struct flatten_list_impl;
template <> struct flatten_list_impl<> { using type = type_list<>; };

template <typename Head, typename... Tail>
struct flatten_list_impl<Head, Tail...> {
private:
    using tail_flat = typename flatten_list_impl<Tail...>::type;
    template <typename H> struct expand_nonvariant {
        using with_head = typename tl_append_unique<type_list<>, H>::type;
        template <typename Src, typename Acc> struct merge_tail;
        template <typename Acc2> struct merge_tail<type_list<>, Acc2> { using type = Acc2; };
        template <typename T0, typename... Ts, typename Acc2>
        struct merge_tail<type_list<T0, Ts...>, Acc2> {
            using next_acc = typename tl_append_unique<Acc2, T0>::type;
            using type = typename merge_tail<type_list<Ts...>, next_acc>::type;
        };
        using type = typename merge_tail<tail_flat, with_head>::type;
    };

    template <typename H>
    struct expand_head { using type = typename expand_nonvariant<H>::type; };

    // If H is Variant<Inner...>, we will expand - SFINAE detection via is_variant below
public:
    using type = typename expand_head<std::decay_t<Head>>::type;
};

template <typename... Ts>
using flattened_unique_t = typename flatten_list_impl<Ts...>::type;


// -------------------- small trait helpers --------------------
template <typename T>
struct is_small_trivial {
    static constexpr bool value = std::is_trivially_copyable<T>::value && sizeof(T) <= 8 && alignof(T) <= 8;
};

template <typename T>
struct is_pointer_like { static constexpr bool value = std::is_pointer<T>::value && sizeof(T) <= 8; };
template <typename T>
struct is_integral_like { static constexpr bool value = std::is_integral<T>::value && sizeof(T) <= 8; };
template <typename T>
struct is_double_like { static constexpr bool value = std::is_floating_point<T>::value && sizeof(T) == 8; };
template <typename T>
struct is_monostate_like { static constexpr bool value = std::is_same<std::decay_t<T>, std::monostate>::value; };
template <typename T>
struct is_bool_like { static constexpr bool value = std::is_same<std::decay_t<T>, bool>::value; };

template <typename List> struct all_small_trivial_impl;
template <> struct all_small_trivial_impl<type_list<>> : std::true_type {};
template <typename H, typename... Ts>
struct all_small_trivial_impl<type_list<H, Ts...>> : std::integral_constant<bool, is_small_trivial<H>::value && all_small_trivial_impl<type_list<Ts...>>::value> {};

template <typename List> struct all_nanboxable_impl;
template <> struct all_nanboxable_impl<type_list<>> : std::true_type {};
template <typename H, typename... Ts>
struct all_nanboxable_impl<type_list<H, Ts...>> : std::integral_constant<bool,
    (is_double_like<H>::value || is_integral_like<H>::value || is_pointer_like<H>::value || is_monostate_like<H>::value || is_bool_like<H>::value)
    && all_nanboxable_impl<type_list<Ts...>>::value> {};


// -------------------- platform detection --------------------
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
#  define MEOW_BYTE_ORDER __BYTE_ORDER__
#  define MEOW_ORDER_LITTLE __ORDER_LITTLE_ENDIAN__
#endif

#if (defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)) && (defined(MEOW_BYTE_ORDER) && MEOW_BYTE_ORDER == MEOW_ORDER_LITTLE) /* && (sizeof(void*) == 8 */)
#  define MEOW_LITTLE_64 1
#else
#  define MEOW_LITTLE_64 0
#endif

// -------------------- NaN-box constants --------------------
#if MEOW_LITTLE_64
static constexpr uint64_t EXP_MASK = 0x7FF0000000000000ULL; // exponent ones
static constexpr uint64_t QNAN_BIT = 0x0008000000000000ULL; // mantissa MSB (bit51)
static constexpr uint64_t EXP_QNAN_PREFIX = (EXP_MASK | QNAN_BIT); // pattern with exponent all ones and qNaN bit set
#endif

// -------------------- Aggressive NaN config --------------------
// Aggressive mode: larger tag (8-bit) stored in mantissa top bits [44..51] (8 bits), payload 44 bits (0..43).
// Tag value uses high bit (0x80) set to guarantee qNaN bit (bit51) semantics.
// This lets us support up to 127 distinct non-double types (index) while keeping 44-bit payload.
// POINTERS MUST FIT IN 44 BITS — this is risky but allowed in aggressive mode.
#if MEOW_LITTLE_64
static constexpr unsigned AGG_TAG_SHIFT = 44;
static constexpr uint64_t AGG_TAG_MASK = (0xFFULL << AGG_TAG_SHIFT);
static constexpr uint64_t AGG_PAYLOAD_MASK = ((1ULL << AGG_TAG_SHIFT) - 1ULL);
static constexpr uint64_t AGG_QNAN_PREFIX = EXP_MASK | QNAN_BIT; // exponent=all1 and qNaN bit set
#endif

// -------------------- Conservative NaN config --------------------
// earlier conservative: tag 3 bits at shift 48, payload 48 bits
#if MEOW_LITTLE_64
static constexpr unsigned CONS_TAG_SHIFT = 48;
static constexpr uint64_t CONS_TAG_MASK = (0x7ULL << CONS_TAG_SHIFT);
static constexpr uint64_t CONS_PAYLOAD_MASK = ((1ULL << CONS_TAG_SHIFT) - 1ULL);
static constexpr uint64_t CONS_QNAN_PREFIX = EXP_MASK | QNAN_BIT;
#endif

// -------------------- Variant implementations --------------------

// Forward declare Variant
template <typename... Args> class Variant;

// --- Aggressive NaN-Boxed Variant (x86_64 little-endian) ---
#if MEOW_LITTLE_64
template <typename... Args>
class AggressiveNaNVariant {
private:
    using flat_list = flattened_unique_t<Args...>;
    static constexpr std::size_t alternatives_count = tl_length<flat_list>::value;
    static constexpr bool all_nanboxable = all_nanboxable_impl<flat_list>::value;
    static constexpr bool eligible = (alternatives_count > 0) && (alternatives_count <= 127) && all_nanboxable;

    static_assert(eligible, "AggressiveNaNVariant not eligible: use Packed64Variant fallback or disable aggressive mode.");

public:
    using inner_types = type_list<Args...>;
    using index_t = uint8_t;
    static constexpr index_t npos = static_cast<index_t>(-1);

    AggressiveNaNVariant() noexcept {
        if constexpr (std::is_same<typename nth_type<0, flat_list>::type, std::monostate>::value) {
            index_ = 0;
            encode_non_double(0, 0ull);
        } else {
            index_ = npos;
            bits_ = 0;
        }
    }
    ~AggressiveNaNVariant() noexcept = default;
    AggressiveNaNVariant(const AggressiveNaNVariant& o) noexcept { bits_ = o.bits_; index_ = o.index_; }
    AggressiveNaNVariant(AggressiveNaNVariant&& o) noexcept { bits_ = o.bits_; index_ = o.index_; o.index_ = npos; }

    template <typename T, typename U = std::decay_t<T>,
              typename = std::enable_if_t<(tl_index_of<U, flat_list>::value != static_cast<std::size_t>(-1))>>
    AggressiveNaNVariant(T&& v) noexcept {
        using VT = std::decay_t<T>;
        constexpr std::size_t idx = tl_index_of<VT, flat_list>::value;
        assign_impl<VT>(idx, std::forward<T>(v));
    }

    AggressiveNaNVariant& operator=(const AggressiveNaNVariant& o) noexcept { bits_ = o.bits_; index_ = o.index_; return *this; }
    AggressiveNaNVariant& operator=(AggressiveNaNVariant&& o) noexcept { bits_ = o.bits_; index_ = o.index_; o.index_ = npos; return *this; }

    template <typename T, typename U = std::decay_t<T>,
              typename = std::enable_if_t<(tl_index_of<U, flat_list>::value != static_cast<std::size_t>(-1))>>
    AggressiveNaNVariant& operator=(T&& v) noexcept {
        using VT = std::decay_t<T>;
        constexpr std::size_t idx = tl_index_of<VT, flat_list>::value;
        assign_impl<VT>(idx, std::forward<T>(v));
        return *this;
    }

    template <typename T>
    bool holds() const noexcept {
        constexpr std::size_t idx = tl_index_of<std::decay_t<T>, flat_list>::value;
        if (idx == static_cast<std::size_t>(-1)) return false;
        return index_ == static_cast<index_t>(idx);
    }

    template <typename T>
    T safe_get() const {
        if (!holds<T>()) throw std::bad_variant_access();
        return reconstruct<T>();
    }

    template <typename Visitor>
    decltype(auto) visit(Visitor&& vis) const {
        if (index_ == npos) throw std::bad_variant_access();
        // Jump table using pack expansion (inlined lambdas for each alternative)
        using R = decltype(std::declval<Visitor>()(reconstruct<typename nth_type<0, flat_list>::type>()));
        using fn_t = R(*)(const AggressiveNaNVariant*, Visitor&&);
        // build array with compile-time entries (unrolled up to alternatives_count)
        std::array<fn_t, alternatives_count> table = {{
            [](const AggressiveNaNVariant* v, Visitor&& vis2)->R {
                using T = typename nth_type<0, flat_list>::type;
                return std::forward<Visitor>(vis2)(v->reconstruct<T>());
            }
#if (alternatives_count > 1)
            ,[](const AggressiveNaNVariant* v, Visitor&& vis2)->R{
                using T = typename nth_type<1, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct<T>()); }
#endif
#if (alternatives_count > 2)
            ,[](const AggressiveNaNVariant* v, Visitor&& vis2)->R{
                using T = typename nth_type<2, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct<T>()); }
#endif
#if (alternatives_count > 3)
            ,[](const AggressiveNaNVariant* v, Visitor&& vis2)->R{
                using T = typename nth_type<3, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct<T>()); }
#endif
#if (alternatives_count > 4)
            ,[](const AggressiveNaNVariant* v, Visitor&& vis2)->R{
                using T = typename nth_type<4, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct<T>()); }
#endif
#if (alternatives_count > 5)
            ,[](const AggressiveNaNVariant* v, Visitor&& vis2)->R{
                using T = typename nth_type<5, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct<T>()); }
#endif
#if (alternatives_count > 6)
            ,[](const AggressiveNaNVariant* v, Visitor&& vis2)->R{
                using T = typename nth_type<6, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct<T>()); }
#endif
#if (alternatives_count > 7)
            ,[](const AggressiveNaNVariant* v, Visitor&& vis2)->R{
                using T = typename nth_type<7, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct<T>()); }
#endif
            // expands up to alternatives_count entries
        }};
        return table[index_](this, std::forward<Visitor>(vis));
    }

    void swap(AggressiveNaNVariant& o) noexcept { std::swap(bits_, o.bits_); std::swap(index_, o.index_); }

    bool operator==(const AggressiveNaNVariant& o) const noexcept { return bits_ == o.bits_ && index_ == o.index_; }
    bool operator!=(const AggressiveNaNVariant& o) const noexcept { return !(*this == o); }

    static constexpr std::size_t alternatives() noexcept { return alternatives_count; }
    static constexpr std::size_t storage_size_bytes() noexcept { return sizeof(bits_); }

private:
    uint64_t bits_ = 0;
    index_t index_ = npos;

    static inline uint64_t d2u(double d) noexcept { uint64_t r; std::memcpy(&r, &d, sizeof(r)); return r; }
    static inline double u2d(uint64_t u) noexcept { double r; std::memcpy(&r, &u, sizeof(r)); return r; }

    static inline bool is_raw_double(uint64_t u) noexcept { return ( (u & EXP_MASK) != EXP_MASK ); }

    static inline uint64_t encode_payload_ptr(const void* p) noexcept {
        uint64_t tmp = 0; std::memcpy(&tmp, &p, sizeof(void*)); return tmp & AGG_PAYLOAD_MASK;
    }

    template <typename T>
    static inline uint64_t encode_payload_value(const T& v) noexcept {
        if constexpr (is_pointer_like<T>::value) {
            return encode_payload_ptr(reinterpret_cast<const void*>(v));
        } else if constexpr (is_integral_like<T>::value) {
            int64_t iv = static_cast<int64_t>(v);
            return (static_cast<uint64_t>(iv) & AGG_PAYLOAD_MASK);
        } else if constexpr (is_bool_like<T>::value) {
            return static_cast<uint64_t>(v ? 1ULL : 0ULL);
        } else {
            return 0ULL;
        }
    }

    inline void encode_non_double(std::size_t idx, uint64_t payload) noexcept {
        // tag stored as idx+1 with high bit set to guarantee qNaN bit semantics
        uint64_t tagval = (static_cast<uint64_t>(idx + 1) | 0x80ULL); // ensures top bit of the 8-bit tag is 1
        bits_ = (AGG_QNAN_PREFIX & EXP_MASK) | (tagval << AGG_TAG_SHIFT) | (payload & AGG_PAYLOAD_MASK);
    }

    template <typename T>
    inline T reconstruct() const noexcept {
        uint64_t u = bits_;
        if (is_raw_double(u)) {
            double d = u2d(u);
            if constexpr (is_double_like<T>::value) return static_cast<T>(d);
            else return T{};
        } else {
            // non-double boxed: decode payload
            uint64_t payload = u & AGG_PAYLOAD_MASK;
            if constexpr (is_pointer_like<T>::value) {
                void* p = nullptr; uint64_t tmp = payload; std::memcpy(&p, &tmp, sizeof(void*)); return reinterpret_cast<T>(p);
            } else if constexpr (is_integral_like<T>::value) {
                // sign-extend from 44 bits
                uint64_t mask = (1ULL << AGG_TAG_SHIFT) - 1ULL;
                uint64_t val = payload & mask;
                if (val & (1ULL << (AGG_TAG_SHIFT - 1))) {
                    uint64_t se = (~mask);
                    uint64_t full = val | se;
                    return static_cast<T>(static_cast<int64_t>(full));
                } else {
                    return static_cast<T>(static_cast<int64_t>(val));
                }
            } else if constexpr (is_bool_like<T>::value) {
                return static_cast<T>(payload != 0);
            } else if constexpr (is_monostate_like<T>::value) {
                return T{};
            } else {
                return T{};
            }
        }
    }

    template <typename T>
    inline void assign_impl(std::size_t idx, T&& v) noexcept {
        using U = std::decay_t<T>;
        if constexpr (is_double_like<U>::value) {
            double d = static_cast<double>(v);
            if (!std::isnan(d)) {
                bits_ = d2u(d);
                index_ = static_cast<index_t>(idx);
            } else {
                // canonical NaN boxed value (tag 0 reserved)
                bits_ = AGG_QNAN_PREFIX;
                index_ = static_cast<index_t>(idx);
            }
        } else {
            uint64_t payload = encode_payload_value<U>(v);
            encode_non_double(idx, payload);
            index_ = static_cast<index_t>(idx);
        }
    }
};
#endif // MEOW_LITTLE_64 Aggressive

// --- Conservative NaNBoxedVariant (3-bit tag, 48-bit payload) ---
#if MEOW_LITTLE_64
template <typename... Args>
class NaNBoxedVariant {
private:
    using flat_list = flattened_unique_t<Args...>;
    static constexpr std::size_t alternatives_count = tl_length<flat_list>::value;
    static constexpr bool all_nan_ok = all_nanboxable_impl<flat_list>::value;
    static constexpr bool eligible = (alternatives_count > 0) && (alternatives_count <= 8) && all_nan_ok;

    static_assert(eligible, "NaNBoxedVariant not eligible: use Packed64Variant instead.");

public:
    using inner_types = type_list<Args...>;
    using index_t = uint8_t;
    static constexpr index_t npos = static_cast<index_t>(-1);

    NaNBoxedVariant() noexcept {
        if constexpr (std::is_same<typename nth_type<0, flat_list>::type, std::monostate>::value) {
            index_ = 0;
            encode_non_double(0, 0ull);
        } else {
            index_ = npos;
            bits_ = 0;
        }
    }
    ~NaNBoxedVariant() noexcept = default;
    NaNBoxedVariant(const NaNBoxedVariant& o) noexcept { bits_ = o.bits_; index_ = o.index_; }
    NaNBoxedVariant(NaNBoxedVariant&& o) noexcept { bits_ = o.bits_; index_ = o.index_; o.index_ = npos; }

    template <typename T, typename U = std::decay_t<T>,
              typename = std::enable_if_t<(tl_index_of<U, flat_list>::value != static_cast<std::size_t>(-1))>>
    NaNBoxedVariant(T&& v) noexcept {
        using VT = std::decay_t<T>;
        constexpr std::size_t idx = tl_index_of<VT, flat_list>::value;
        assign_impl<VT>(idx, std::forward<T>(v));
    }

    NaNBoxedVariant& operator=(const NaNBoxedVariant& o) noexcept { bits_ = o.bits_; index_ = o.index_; return *this; }
    NaNBoxedVariant& operator=(NaNBoxedVariant&& o) noexcept { bits_ = o.bits_; index_ = o.index_; o.index_ = npos; return *this; }

    template <typename T, typename U = std::decay_t<T>,
              typename = std::enable_if_t<(tl_index_of<U, flat_list>::value != static_cast<std::size_t>(-1))>>
    NaNBoxedVariant& operator=(T&& v) noexcept {
        using VT = std::decay_t<T>;
        constexpr std::size_t idx = tl_index_of<VT, flat_list>::value;
        assign_impl<VT>(idx, std::forward<T>(v));
        return *this;
    }

    template <typename T>
    bool holds() const noexcept {
        constexpr std::size_t idx = tl_index_of<std::decay_t<T>, flat_list>::value;
        if (idx == static_cast<std::size_t>(-1)) return false;
        return index_ == static_cast<index_t>(idx);
    }

    template <typename T>
    T safe_get() const {
        if (!holds<T>()) throw std::bad_variant_access();
        return reconstruct<T>();
    }

    template <typename Visitor>
    decltype(auto) visit(Visitor&& vis) const {
        if (index_ == npos) throw std::bad_variant_access();
        using R = decltype(std::declval<Visitor>()(reconstruct<typename nth_type<0, flat_list>::type>()));
        using fn_t = R(*)(const NaNBoxedVariant*, Visitor&&);
        std::array<fn_t, alternatives_count> table = {{
            [](const NaNBoxedVariant* v, Visitor&& vis2)->R{ using T = typename nth_type<0, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct<T>()); }
#if (alternatives_count > 1)
            ,[](const NaNBoxedVariant* v, Visitor&& vis2)->R{ using T = typename nth_type<1, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct<T>()); }
#endif
#if (alternatives_count > 2)
            ,[](const NaNBoxedVariant* v, Visitor&& vis2)->R{ using T = typename nth_type<2, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct<T>()); }
#endif
#if (alternatives_count > 3)
            ,[](const NaNBoxedVariant* v, Visitor&& vis2)->R{ using T = typename nth_type<3, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct<T>()); }
#endif
#if (alternatives_count > 4)
            ,[](const NaNBoxedVariant* v, Visitor&& vis2)->R{ using T = typename nth_type<4, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct<T>()); }
#endif
#if (alternatives_count > 5)
            ,[](const NaNBoxedVariant* v, Visitor&& vis2)->R{ using T = typename nth_type<5, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct<T>()); }
#endif
#if (alternatives_count > 6)
            ,[](const NaNBoxedVariant* v, Visitor&& vis2)->R{ using T = typename nth_type<6, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct<T>()); }
#endif
#if (alternatives_count > 7)
            ,[](const NaNBoxedVariant* v, Visitor&& vis2)->R{ using T = typename nth_type<7, flat_list>::type; return std::forward<Visitor>(vis2)(v->reconstruct<T>()); }
#endif
        }};
        return table[index_](this, std::forward<Visitor>(vis));
    }

    void swap(NaNBoxedVariant& o) noexcept { std::swap(bits_, o.bits_); std::swap(index_, o.index_); }

    bool operator==(const NaNBoxedVariant& o) const noexcept { return bits_ == o.bits_ && index_ == o.index_; }
    bool operator!=(const NaNBoxedVariant& o) const noexcept { return !(*this == o); }

    static constexpr std::size_t alternatives() noexcept { return alternatives_count; }
    static constexpr std::size_t storage_size_bytes() noexcept { return sizeof(bits_); }

private:
    uint64_t bits_ = 0;
    uint8_t index_ = npos;

    static inline uint64_t d2u(double d) noexcept { uint64_t r; std::memcpy(&r, &d, sizeof(r)); return r; }
    static inline double u2d(uint64_t u) noexcept { double r; std::memcpy(&r, &u, sizeof(r)); return r; }
    static inline bool is_raw_double(uint64_t u) noexcept { return ( (u & EXP_MASK) != EXP_MASK ); }

    template <typename T>
    static inline uint64_t encode_payload(const T& v) noexcept {
        if constexpr (is_pointer_like<T>::value) {
            uint64_t tmp = 0; std::memcpy(&tmp, &v, sizeof(v)); return tmp & CONS_PAYLOAD_MASK;
        } else if constexpr (is_integral_like<T>::value) {
            return (static_cast<uint64_t>(static_cast<int64_t>(v)) & CONS_PAYLOAD_MASK);
        } else if constexpr (is_bool_like<T>::value) {
            return static_cast<uint64_t>(v ? 1ULL : 0ULL);
        } else {
            return 0ULL;
        }
    }

    inline void encode_non_double(std::size_t idx, uint64_t payload) noexcept {
        uint64_t tagval = static_cast<uint64_t>(idx + 1) & 0x7ULL; // 3 bits
        bits_ = CONS_QNAN_PREFIX | (tagval << CONS_TAG_SHIFT) | (payload & CONS_PAYLOAD_MASK);
    }

    template <typename T>
    inline void assign_impl(std::size_t idx, T&& v) noexcept {
        using U = std::decay_t<T>;
        if constexpr (is_double_like<U>::value) {
            double d = static_cast<double>(v);
            if (!std::isnan(d)) {
                bits_ = d2u(d);
            } else {
                bits_ = CONS_QNAN_PREFIX; // canonical NaN
            }
            index_ = static_cast<uint8_t>(idx);
        } else {
            uint64_t payload = encode_payload<U>(v);
            encode_non_double(idx, payload);
            index_ = static_cast<uint8_t>(idx);
        }
    }

    template <typename T>
    inline T reconstruct() const noexcept {
        uint64_t u = bits_;
        if (is_raw_double(u)) {
            double d = u2d(u);
            if constexpr (is_double_like<T>::value) return static_cast<T>(d);
            else return T{};
        } else {
            uint64_t payload = u & CONS_PAYLOAD_MASK;
            if constexpr (is_pointer_like<T>::value) {
                void* p = nullptr; uint64_t tmp = payload; std::memcpy(&p, &tmp, sizeof(void*));
                return reinterpret_cast<T>(p);
            } else if constexpr (is_integral_like<T>::value) {
                uint64_t mask = CONS_PAYLOAD_MASK;
                uint64_t val = payload & mask;
                if (val & (1ULL << (CONS_TAG_SHIFT - 1))) {
                    uint64_t se = ~mask; uint64_t full = val | se; return static_cast<T>(static_cast<int64_t>(full));
                } else return static_cast<T>(static_cast<int64_t>(val));
            } else if constexpr (is_bool_like<T>::value) {
                return static_cast<T>(payload != 0);
            } else return T{};
        }
    }
};
#endif // MEOW_LITTLE_64 conservative NaN

// --- Packed64Variant fallback (fast) ---
template <typename... Args>
class Packed64Variant {
private:
    using flat_list = flattened_unique_t<Args...>;
    static constexpr std::size_t alternatives_count = tl_length<flat_list>::value;
public:
    using inner_types = type_list<Args...>;
    using index_t = uint8_t;
    static constexpr index_t npos = static_cast<index_t>(-1);

    Packed64Variant() noexcept {
        if constexpr (std::is_same<typename nth_type<0, flat_list>::type, std::monostate>::value) {
            index_ = 0; storage_ = 0;
        } else { index_ = npos; storage_ = 0; }
    }
    ~Packed64Variant() noexcept = default;
    Packed64Variant(const Packed64Variant& o) noexcept { storage_ = o.storage_; index_ = o.index_; }
    Packed64Variant(Packed64Variant&& o) noexcept { storage_ = o.storage_; index_ = o.index_; o.index_ = npos; }

    template <typename T, typename U = std::decay_t<T>,
              typename = std::enable_if_t<(tl_index_of<U, flat_list>::value != static_cast<std::size_t>(-1))>>
    Packed64Variant(T&& v) noexcept {
        constexpr std::size_t idx = tl_index_of<std::decay_t<T>, flat_list>::value;
        index_ = static_cast<index_t>(idx);
        store_impl<std::decay_t<T>>(std::forward<T>(v));
    }

    Packed64Variant& operator=(const Packed64Variant& o) noexcept { storage_ = o.storage_; index_ = o.index_; return *this; }
    Packed64Variant& operator=(Packed64Variant&& o) noexcept { storage_ = o.storage_; index_ = o.index_; o.index_ = npos; return *this; }

    template <typename T>
    bool holds() const noexcept {
        constexpr std::size_t idx = tl_index_of<std::decay_t<T>, flat_list>::value;
        if (idx == static_cast<std::size_t>(-1)) return false;
        return index_ == static_cast<index_t>(idx);
    }

    template <typename T>
    T safe_get() const {
        if (!holds<T>()) throw std::bad_variant_access();
        T tmp; std::memcpy(&tmp, &storage_, sizeof(T)); return tmp;
    }

    template <typename Visitor>
    decltype(auto) visit(Visitor&& vis) const {
        if (index_ == npos) throw std::bad_variant_access();
        return visit_impl<Visitor, 0>(std::forward<Visitor>(vis));
    }

    void swap(Packed64Variant& o) noexcept { std::swap(storage_, o.storage_); std::swap(index_, o.index_); }

    bool operator==(const Packed64Variant& o) const noexcept { return storage_ == o.storage_ && index_ == o.index_; }

    static constexpr std::size_t alternatives() noexcept { return alternatives_count; }
private:
    uint64_t storage_;
    uint8_t index_;

    template <typename T>
    inline void store_impl(T&& v) noexcept {
        std::memset(&storage_, 0, sizeof(storage_));
        std::memcpy(&storage_, &v, sizeof(T));
    }

    template <typename Visitor, std::size_t I>
    decltype(auto) visit_impl(Visitor&& vis) const {
        if constexpr (I < alternatives_count) {
            using T = typename nth_type<I, flat_list>::type;
            if (index_ == static_cast<uint8_t>(I)) {
                T tmp; std::memcpy(&tmp, &storage_, sizeof(T));
                return std::invoke(std::forward<Visitor>(vis), tmp);
            }
            return visit_impl<Visitor, I + 1>(std::forward<Visitor>(vis));
        } else {
            throw std::bad_variant_access();
        }
    }
};


// -------------------- Variant chooser --------------------
template <typename... Args>
struct choose_variant {
private:
    using flat = flattened_unique_t<Args...>;
    static constexpr std::size_t cnt = tl_length<flat>::value;
    static constexpr bool all_small = all_small_trivial_impl<flat>::value;
    static constexpr bool all_nan_ok = all_nanboxable_impl<flat>::value;
    static constexpr bool conservative_nan_possible = MEOW_LITTLE_64 && cnt > 0 && (cnt <= 8) && all_nan_ok;
    static constexpr bool aggressive_nan_possible = MEOW_LITTLE_64 && cnt > 0 && (cnt <= 127) && all_nan_ok;
public:
#if MEOW_LITTLE_64
#if defined(MEOW_AGGRESSIVE_NAN) && (MEOW_AGGRESSIVE_NAN == 1)
    using type = std::conditional_t<aggressive_nan_possible, AggressiveNaNVariant<Args...>,
                std::conditional_t<conservative_nan_possible, NaNBoxedVariant<Args...>, Packed64Variant<Args...>>>;
#else
    using type = std::conditional_t<conservative_nan_possible, NaNBoxedVariant<Args...>, Packed64Variant<Args...>>;
#endif
#else
    using type = Packed64Variant<Args...>;
#endif
};

template <typename... Args>
class Variant : public choose_variant<Args...>::type {
    using base_t = typename choose_variant<Args...>::type;
public:
    using base_t::base_t;
    using base_t::operator=;
    // inherited API (holds, safe_get, visit, swap, operator==)
};

} // namespace meow::utils
