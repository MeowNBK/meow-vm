#pragma once

#include "common/pch.h"

namespace meow::utils {

// ------------------- meta helpers (flatten + type_list + lookup) -------------------
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

// flatten nested Variant types (left-to-right, unique)
template <typename...> struct flatten_impl;
template <> struct flatten_impl<> { using type = type_list<>; };

template <typename Head, typename... Tail>
struct flatten_impl<Head, Tail...> {
private:
    using tail_flat = typename flatten_impl<Tail...>::type;
    template <typename H>
    struct expand_nonvariant {
        using with_head = typename tl_append_unique<type_list<>, H>::type;
        template <typename Src, typename Acc> struct merge;
        template <typename Acc2>
        struct merge<type_list<>, Acc2> { using type = Acc2; };
        template <typename T0, typename... Ts, typename Acc2>
        struct merge<type_list<T0, Ts...>, Acc2> {
            using next_acc = typename tl_append_unique<Acc2, T0>::type;
            using type = typename merge<type_list<Ts...>, next_acc>::type;
        };
        using type = typename merge<tail_flat, with_head>::type;
    };

    // detect if Head is Variant<...> by SFINAE: forward-declare Variant later, and check is_variant
    template <typename H> struct expand_head { using type = typename expand_nonvariant<H>::type; };

    // forward-declare is_variant
    template <typename> struct is_variant : std::false_type {};
    template <typename... U> struct is_variant<class Variant<U...>> : std::true_type {};

    template <typename H>
    struct expand_head< H, std::enable_if_t<is_variant<std::decay_t<H>>::value, void> >; // forward; we'll specialize afterwards

public:
    using type = typename expand_head<std::decay_t<Head>>::type;
};

// Provide the specialization for when Head is Variant<Inner...>
// We'll re-open flatten_impl for a partial specialization trick:
template <typename Head, typename... Tail>
struct flatten_impl<Head, Tail...>; // declared above; now provide actual definition specialized using is_variant detection below

// To avoid complicated partial specialization order, simpler: implement a separate flatten_list_impl that directly matches Variant when declared below
// We'll implement a lightweight flattener that handles the expected use-case (Variants possibly nested) with recursion.

template <typename... Ts> struct flatten_list_impl;
template <> struct flatten_list_impl<> { using type = type_list<>; };

template <typename Head, typename... Tail>
struct flatten_list_impl<Head, Tail...> {
private:
    using tail_flat = typename flatten_list_impl<Tail...>::type;

    // if Head is a Variant<...> we'll expand inner, otherwise append Head
    template <typename H, bool IsVar = false> struct expand_head_impl { using type = typename tl_append_unique<tail_flat, H>::type; };

    template <typename... Inner>
    struct expand_head_impl<class Variant<Inner...>, true> {
        using inner_flat = typename flatten_list_impl<Inner...>::type;
        template <typename Src, typename Acc> struct merge;
        template <typename Acc2>
        struct merge<type_list<>, Acc2> { using type = Acc2; };
        template <typename S0, typename... Ss, typename Acc2>
        struct merge<type_list<S0, Ss...>, Acc2> {
            using next_acc = typename tl_append_unique<Acc2, S0>::type;
            using type = typename merge<type_list<Ss...>, next_acc>::type;
        };
        using merged_with_inner = typename merge<inner_flat, type_list<>>::type;
        using merged_all = typename merge<tail_flat, merged_with_inner>::type;
        using type = merged_all;
    };

public:
    using type = typename expand_head_impl<std::decay_t<Head>, std::is_class<std::decay_t<Head>>::value>::type;
};

template <typename... Ts>
using flattened_unique_t = typename flatten_list_impl<Ts...>::type;


// ------------------- helper: check small trivially-copyable <=8 -------------------
template <typename T>
struct is_small_trivial_8 {
    static constexpr bool value = std::is_trivially_copyable<T>::value && sizeof(T) <= 8 && alignof(T) <= 8;
};

template <typename List> struct all_small_trivial_8_impl;
template <> struct all_small_trivial_8_impl<type_list<>> : std::true_type {};
template <typename H, typename... Ts>
struct all_small_trivial_8_impl<type_list<H, Ts...>> : std::integral_constant<bool, is_small_trivial_8<H>::value && all_small_trivial_8_impl<type_list<Ts...>>::value> {};


// ------------------- Primary: Packed 64-bit Variant specialization (fast path) -------------------
template <typename... Args>
class Variant
    // enable only when all flattened unique types are small trivially-copyable and count <= 255
{
private:
    using flat_list = flattened_unique_t<Args...>;
    static constexpr std::size_t alternatives_count_static = tl_length<flat_list>::value;

    static constexpr bool eligible =
        (alternatives_count_static > 0) &&
        (alternatives_count_static <= 255) &&
        all_small_trivial_8_impl<flat_list>::value;

    static_assert(eligible, "Variant<...>: types are not eligible for packed-64 fast specialization. Use a generic Variant fallback (not provided here).");

public:
    static constexpr std::size_t alternatives_count = alternatives_count_static;
    using inner_types = type_list<Args...>;
    using index_t = uint8_t;
    static constexpr index_t npos = static_cast<index_t>(-1);

    Variant() noexcept {
        // default-initialize to first alternative if it's std::monostate / Null, else valueless (npos)
        // We'll prefer valueless unless the first alternative is std::monostate
        if constexpr (std::is_same<typename nth_type<0, flat_list>::type, std::monostate>::value) {
            index_ = 0;
            std::memset(&storage_, 0, sizeof(storage_));
        } else {
            index_ = npos;
            storage_ = 0;
        }
    }
    ~Variant() noexcept = default; // trivially copyable types -> no destructor work

    // copy/move are raw mem ops (very fast)
    Variant(const Variant& o) noexcept { storage_ = o.storage_; index_ = o.index_; }
    Variant(Variant&& o) noexcept { storage_ = o.storage_; index_ = o.index_; o.index_ = npos; }

    // construct from T
    template <typename T, typename U = std::decay_t<T>,
              typename = std::enable_if_t<(tl_index_of<U, flat_list>::value != static_cast<std::size_t>(-1))>>
    Variant(T&& v) noexcept {
        constexpr std::size_t idx = tl_index_of<U, flat_list>::value;
        index_ = static_cast<index_t>(idx);
        // raw store
        store_value_impl<U>(std::forward<T>(v));
    }

    // in_place_type
    template <typename T, typename... CArgs, typename U = std::decay_t<T>,
              typename = std::enable_if_t<(tl_index_of<U, flat_list>::value != static_cast<std::size_t>(-1))>>
    explicit Variant(std::in_place_type_t<T>, CArgs&&... args) noexcept {
        constexpr std::size_t idx = tl_index_of<U, flat_list>::value;
        index_ = static_cast<index_t>(idx);
        U tmp(std::forward<CArgs>(args)...);
        store_value_impl<U>(std::move(tmp));
    }

    // in_place_index
    template <std::size_t I, typename... CArgs>
    explicit Variant(std::in_place_index_t<I>, CArgs&&... args) noexcept {
        static_assert(I < alternatives_count, "in_place_index out of range");
        using U = typename nth_type<I, flat_list>::type;
        index_ = static_cast<index_t>(I);
        U tmp(std::forward<CArgs>(args)...);
        store_value_impl<U>(std::move(tmp));
    }

    Variant& operator=(const Variant& o) noexcept {
        if (this == &o) return *this;
        storage_ = o.storage_;
        index_ = o.index_;
        return *this;
    }
    Variant& operator=(Variant&& o) noexcept {
        if (this == &o) return *this;
        storage_ = o.storage_;
        index_ = o.index_;
        o.index_ = npos;
        return *this;
    }

    // assignment from T
    template <typename T, typename U = std::decay_t<T>,
              typename = std::enable_if_t<(tl_index_of<U, flat_list>::value != static_cast<std::size_t>(-1))>>
    Variant& operator=(T&& v) noexcept {
        constexpr std::size_t idx = tl_index_of<U, flat_list>::value;
        index_ = static_cast<index_t>(idx);
        store_value_impl<U>(std::forward<T>(v));
        return *this;
    }

    // emplace
    template <typename T, typename... CArgs, typename U = std::decay_t<T>,
              typename = std::enable_if_t<(tl_index_of<U, flat_list>::value != static_cast<std::size_t>(-1))>>
    void emplace(CArgs&&... args) {
        constexpr std::size_t idx = tl_index_of<U, flat_list>::value;
        index_ = static_cast<index_t>(idx);
        U tmp(std::forward<CArgs>(args)...);
        store_value_impl<U>(std::move(tmp));
    }

    template <std::size_t I, typename... CArgs>
    void emplace_index(CArgs&&... args) {
        static_assert(I < alternatives_count, "emplace_index out of range");
        using U = typename nth_type<I, flat_list>::type;
        index_ = static_cast<index_t>(I);
        U tmp(std::forward<CArgs>(args)...);
        store_value_impl<U>(std::move(tmp));
    }

    bool valueless_by_exception() const noexcept { return index_ == npos; }
    std::size_t index() const noexcept { return static_cast<std::size_t>(index_); }

    template <typename T>
    static constexpr std::size_t index_of() noexcept { return tl_index_of<std::decay_t<T>, flat_list>::value; }

    // get / get_if / try_get
    template <typename T>
    [[nodiscard]] const T& get() const noexcept {
        constexpr std::size_t idx = tl_index_of<T, flat_list>::value;
        // NOTE: caller must ensure holds<T>()
        return *reinterpret_cast<const T*>(&storage_);
    }
    template <typename T>
    [[nodiscard]] T& get() noexcept {
        constexpr std::size_t idx = tl_index_of<T, flat_list>::value;
        return *reinterpret_cast<T*>(&storage_);
    }

    template <typename T>
    const T* get_if() const noexcept {
        constexpr std::size_t idx = tl_index_of<T, flat_list>::value;
        if (idx == static_cast<std::size_t>(-1)) return nullptr;
        if (index_ != static_cast<index_t>(idx)) return nullptr;
        return reinterpret_cast<const T*>(&storage_);
    }
    template <typename T>
    T* get_if() noexcept {
        constexpr std::size_t idx = tl_index_of<T, flat_list>::value;
        if (idx == static_cast<std::size_t>(-1)) return nullptr;
        if (index_ != static_cast<index_t>(idx)) return nullptr;
        return reinterpret_cast<T*>(&storage_);
    }

    // unchecked fast getter for hot path
    template <typename T>
    const T& unsafe_get_unchecked() const noexcept { return *reinterpret_cast<const T*>(&storage_); }
    template <typename T>
    T& unsafe_get_unchecked() noexcept { return *reinterpret_cast<T*>(&storage_); }

    template <typename T>
    T* try_get() noexcept { return get_if<T>(); }
    template <typename T>
    const T* try_get() const noexcept { return get_if<T>(); }

    template <typename T>
    const T& safe_get() const {
        if (!holds<T>()) throw std::bad_variant_access();
        return get<T>();
    }
    template <typename T>
    T& safe_get() {
        if (!holds<T>()) throw std::bad_variant_access();
        return get<T>();
    }

    template <typename T>
    bool holds() const noexcept {
        constexpr std::size_t idx = tl_index_of<std::decay_t<T>, flat_list>::value;
        if (idx == static_cast<std::size_t>(-1)) return false;
        return index_ == static_cast<index_t>(idx);
    }

    // holds any of (for nested variant types)
    template <typename T>
    bool holds_any_of() const noexcept {
        return holds_any_of_impl<T, 0>();
    }

    // visit: build per-Visitor jump-table for O(1) call
    template <typename Visitor>
    decltype(auto) visit(Visitor&& vis) {
        if (index_ == npos) throw std::bad_variant_access();
        return visit_impl(std::forward<Visitor>(vis), std::make_index_sequence<alternatives_count>{});
    }

    template <typename Visitor>
    decltype(auto) visit(Visitor&& vis) const {
        if (index_ == npos) throw std::bad_variant_access();
        return visit_impl_const(std::forward<Visitor>(vis), std::make_index_sequence<alternatives_count>{});
    }

    // swap
    void swap(Variant& other) noexcept {
        if (this == &other) return;
        std::swap(storage_, other.storage_);
        std::swap(index_, other.index_);
    }

    // operator==
    bool operator==(const Variant& o) const noexcept {
        if (index_ != o.index_) return false;
        if (index_ == npos) return true;
        return storage_ == o.storage_;
    }
    bool operator!=(const Variant& o) const noexcept { return !(*this == o); }

    static constexpr std::size_t alternatives() noexcept { return alternatives_count; }
    static constexpr std::size_t storage_size_bytes() noexcept { return sizeof(storage_); }

private:
    uint64_t storage_ = 0;
    index_t index_ = npos;

    // store helpers: fast memcpy into 8-byte payload (we assume sizeof(T) <= 8 and trivially copyable)
    template <typename T, typename V>
    inline void store_value_impl(V&& v) noexcept {
        static_assert(std::is_trivially_copyable<T>::value && sizeof(T) <= 8 && alignof(T) <= 8, "store_value_impl requirements");
        // write directly into storage_ bytes
        // for speed: use direct aliasing (minimal overhead). This may technically be UB for strict-aliasing on some compilers;
        // if you want strictly-conforming code, replace with std::memcpy(&storage_, &v, sizeof(T));
        std::memset(&storage_, 0, sizeof(storage_));
        std::memcpy(&storage_, &v, sizeof(T));
    }

    // visit impl
    template <typename Visitor, std::size_t... Is>
    decltype(auto) visit_impl(Visitor&& vis, std::index_sequence<Is...>) {
        using R = decltype(std::invoke(std::declval<Visitor>(), *reinterpret_cast<typename nth_type<0, flat_list>::type*>(nullptr)));
        using fn_t = R(*)(uint64_t const&, Visitor&&);
        static fn_t table[] = {
            +[](uint64_t const& raw, Visitor&& v) -> R {
                using T = typename nth_type<Is, flat_list>::type;
                T tmp;
                std::memcpy(&tmp, &raw, sizeof(T));
                return std::invoke(std::forward<Visitor>(v), tmp);
            }...
        };
        return table[index_](storage_, std::forward<Visitor>(vis));
    }

    template <typename Visitor, std::size_t... Is>
    decltype(auto) visit_impl_const(Visitor&& vis, std::index_sequence<Is...>) const {
        using R = decltype(std::invoke(std::declval<Visitor>(), *reinterpret_cast<const typename nth_type<0, flat_list>::type*>(nullptr)));
        using fn_t = R(*)(uint64_t const&, Visitor&&);
        static fn_t table[] = {
            +[](uint64_t const& raw, Visitor&& v) -> R {
                using T = typename nth_type<Is, flat_list>::type;
                T tmp;
                std::memcpy(&tmp, &raw, sizeof(T));
                return std::invoke(std::forward<Visitor>(v), tmp);
            }...
        };
        return table[index_](storage_, std::forward<Visitor>(vis));
    }

    // holds_any_of_impl for nested Variant inner_types compatibility
    template <typename InnerList, std::size_t I = 0>
    bool holds_any_of_impl() const noexcept {
        if constexpr (I >= tl_length<InnerList>::value) return false;
        else {
            using InnerT = typename nth_type<I, InnerList>::type;
            constexpr std::size_t idx = tl_index_of<InnerT, flat_list>::value;
            if (idx != static_cast<std::size_t>(-1) && index_ == static_cast<index_t>(idx)) return true;
            return holds_any_of_impl<InnerList, I + 1>();
        }
    }
};

// non-member swap
template <typename... Ts>
inline void swap(Variant<Ts...>& a, Variant<Ts...>& b) noexcept(noexcept(a.swap(b))) { a.swap(b); }

} // namespace meow::utils
