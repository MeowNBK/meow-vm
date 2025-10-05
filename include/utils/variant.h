/**
 * @file variant.h
 * @author GPT-5
 * @brief Upgraded flattened Variant:
 *   - Flatten nested Variant types into unique left-to-right alternatives
 *   - Strong exception-safety for copy-assignment
 *   - in_place_type / in_place_index construction (emplace)
 *   - visit(visitor) for single-variant visitation
 *   - swap / std::swap support
 *   - index_of<T>(), holds<T>(), get_if<T>() and nested-Variant-aware get<T>()
 *
 * Notes:
 *  - Target: C++17 and newer
 *  - Design keeps the original flattened-list meta programming style
 *  - The implementation uses recursive constexpr helpers and an aligned storage buffer
 *
 * Copyright (c) 2025 LazyPaws & GPT-5
 * License: All rights reserved.
 */

#pragma once

#include <type_traits>
#include <utility>
#include <new>
#include <optional>
#include <cstddef>
#include <stdexcept>
#include <utility>   // std::swap
#include <cassert>

namespace meow::utils {

// Forward declare Variant for meta traits
template <typename... Args>
class Variant;

// -------------------- Type-list meta helpers --------------------
template <typename... Ts> struct type_list {};

template <typename List, typename T> struct tl_append;
template <typename... Ts, typename T>
struct tl_append<type_list<Ts...>, T> { using type = type_list<Ts..., T>; };

template <typename A, typename B> struct tl_concat;
template <typename... A, typename... B>
struct tl_concat<type_list<A...>, type_list<B...>> { using type = type_list<A..., B...>; };

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

// -------------------- is_variant trait --------------------
template <typename T> struct is_variant : std::false_type {};
template <typename... Us> struct is_variant<Variant<Us...>> : std::true_type {};

// -------------------- Flattening nested Variants --------------------
template <typename...> struct flatten_list_impl;

template <>
struct flatten_list_impl<> { using type = type_list<>; };

template <typename Head, typename... Tail>
struct flatten_list_impl<Head, Tail...> {
private:
    using tail_flat = typename flatten_list_impl<Tail...>::type;

    template <typename H>
    struct expand_head_nonvariant {
        using with_head = typename tl_append_unique<type_list<>, H>::type;
        // merge tail_flat into with_head preserving uniqueness
        template <typename Src, typename Acc> struct merge_tail;
        template <typename Acc2>
        struct merge_tail<type_list<>, Acc2> { using type = Acc2; };
        template <typename T0, typename... Ts, typename Acc2>
        struct merge_tail<type_list<T0, Ts...>, Acc2> {
            using next_acc = typename tl_append_unique<Acc2, T0>::type;
            using type = typename merge_tail<type_list<Ts...>, next_acc>::type;
        };
        using type = typename merge_tail<tail_flat, with_head>::type;
    };

    template <typename H> struct expand_head { using type = typename expand_head_nonvariant<H>::type; };

    template <typename... Inner>
    struct expand_head<Variant<Inner...>> {
        using inner_flat = typename flatten_list_impl<Inner...>::type;
        template <typename Src, typename Acc> struct merge_src;
        template <typename Acc2>
        struct merge_src<type_list<>, Acc2> { using type = Acc2; };
        template <typename S0, typename... Ss, typename Acc2>
        struct merge_src<type_list<S0, Ss...>, Acc2> {
            using next_acc = typename tl_append_unique<Acc2, S0>::type;
            using type = typename merge_src<type_list<Ss...>, next_acc>::type;
        };
        using merged_with_inner = typename merge_src<inner_flat, type_list<>>::type;
        using merged_all = typename merge_src<tail_flat, merged_with_inner>::type;
        using type = merged_all;
    };

public:
    using type = typename expand_head<std::decay_t<Head>>::type;
};

template <typename... Ts>
using flattened_unique_t = typename flatten_list_impl<Ts...>::type;

// -------------------- in_place tags --------------------
template <typename T> struct in_place_type_t { explicit in_place_type_t() = default; };
template <std::size_t I> struct in_place_index_t { explicit in_place_index_t() = default; };
template <typename T> inline constexpr in_place_type_t<T> in_place_type{};
template <std::size_t I> inline constexpr in_place_index_t<I> in_place_index{};

// -------------------- The Variant class --------------------
template <typename... Args>
class Variant {
public:
    using flat_list = flattened_unique_t<Args...>;
    static constexpr std::size_t alternatives_count = tl_length<flat_list>::value;
    using inner_types = type_list<Args...>;

    Variant() noexcept : index_(npos) {}

    ~Variant() { destroy_current(); }

    // copy ctor
    Variant(const Variant& other) : index_(npos) {
        if (other.index_ != npos) {
            copy_from_index(other.index_, other.storage_ptr());
            index_ = other.index_;
        }
    }

    // move ctor
    Variant(Variant&& other) noexcept(std::is_nothrow_move_constructible<typename nth_type<0, flat_list>::type>::value) : index_(npos) {
        if (other.index_ != npos) {
            move_from_index(other.index_, other.storage_ptr());
            index_ = other.index_;
            other.destroy_current();
            other.index_ = npos;
        }
    }

    // construct from any allowed flattened alternative
    template <typename T, typename U = std::decay_t<T>,
              typename = std::enable_if_t<tl_index_of<U, flat_list>::value != static_cast<std::size_t>(-1)>>
    Variant(T&& value) noexcept(std::is_nothrow_constructible<U, T&&>::value) : index_(npos) {
        construct_by_type<U>(std::forward<T>(value));
        index_ = tl_index_of<U, flat_list>::value;
    }

    // in-place type
    template <typename T, typename... CtArgs,
              typename U = std::decay_t<T>,
              typename = std::enable_if_t<tl_index_of<U, flat_list>::value != static_cast<std::size_t>(-1)> >
    explicit Variant(in_place_type_t<T>, CtArgs&&... args) noexcept(std::is_nothrow_constructible<U, CtArgs...>::value) : index_(npos) {
        construct_by_type<U>(std::forward<CtArgs>(args)...);
        index_ = tl_index_of<U, flat_list>::value;
    }

    // in-place index
    template <std::size_t I, typename... CtArgs>
    explicit Variant(in_place_index_t<I>, CtArgs&&... args) noexcept {
        static_assert(I < alternatives_count, "in_place_index out of range");
        using U = typename nth_type<I, flat_list>::type;
        construct_by_type<U>(std::forward<CtArgs>(args)...);
        index_ = I;
    }

    // copy assignment - strong exception safety via copy-and-swap
    Variant& operator=(const Variant& other) {
        if (this == &other) return *this;
        if (other.index_ == npos) {
            destroy_current();
            index_ = npos;
            return *this;
        }
        if (index_ == other.index_) {
            assign_same_index(other.index_, other.storage_ptr());
            return *this;
        }
        // strong exception safety: copy-construct a temp, then swap
        Variant tmp(other);
        swap(tmp);
        return *this;
    }

    // move assignment
    Variant& operator=(Variant&& other) noexcept {
        if (this == &other) return *this;
        if (other.index_ == npos) {
            destroy_current();
            index_ = npos;
            return *this;
        }
        destroy_current();
        move_from_index(other.index_, other.storage_ptr());
        index_ = other.index_;
        other.destroy_current();
        other.index_ = npos;
        return *this;
    }

    // assign from T (allowed types)
    template <typename T, typename U = std::decay_t<T>,
              typename = std::enable_if_t<tl_index_of<U, flat_list>::value != static_cast<std::size_t>(-1)>>
    Variant& operator=(T&& value) noexcept(std::is_nothrow_constructible<U, T&&>::value) {
        constexpr std::size_t idx = tl_index_of<U, flat_list>::value;
        if (index_ == idx) {
            get_ref_by_index<U>() = std::forward<T>(value);
        } else {
            destroy_current();
            construct_by_type<U>(std::forward<T>(value));
            index_ = idx;
        }
        return *this;
    }

    // emplace by type
    template <typename T, typename... CtArgs,
              typename U = std::decay_t<T>,
              typename = std::enable_if_t<tl_index_of<U, flat_list>::value != static_cast<std::size_t>(-1)>>
    void emplace(CtArgs&&... args) {
        destroy_current();
        construct_by_type<U>(std::forward<CtArgs>(args)...);
        index_ = tl_index_of<U, flat_list>::value;
    }

    // emplace by index
    template <std::size_t I, typename... CtArgs>
    void emplace_index(CtArgs&&... args) {
        static_assert(I < alternatives_count, "emplace_index out of range");
        destroy_current();
        using U = typename nth_type<I, flat_list>::type;
        construct_by_type<U>(std::forward<CtArgs>(args)...);
        index_ = I;
    }

    // queries
    bool valueless_by_exception() const noexcept { return index_ == npos; }
    std::size_t index() const noexcept { return index_; }

    // static index_of helper
    template <typename T>
    static constexpr std::size_t index_of() noexcept { return tl_index_of<std::decay_t<T>, flat_list>::value; }

    // raw get for exact flattened types (returns reference)
    template <typename T>
    [[nodiscard]] inline const T& get() const noexcept {
        return *reinterpret_cast<const T*>(storage_ptr());
    }
    template <typename T>
    [[nodiscard]] inline T& get() noexcept {
        return *reinterpret_cast<T*>(storage_ptr());
    }

    // nested Variant get: returns nested Variant constructed from current held value
    template <typename T>
    inline std::enable_if_t<is_variant<std::decay_t<T>>::value, std::decay_t<T>>
    get() const noexcept {
        using TV = std::decay_t<T>;
        return construct_nested_variant_from_index<TV>(index_);
    }

    // safe_get for exact types (throws if not holds)
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

    // safe_get for nested Variant types (throws if not holds)
    template <typename T>
    inline std::enable_if_t<is_variant<std::decay_t<T>>::value, std::decay_t<T>>
    safe_get() const {
        using TV = std::decay_t<T>;
        if (!holds<TV>()) throw std::bad_variant_access();
        return get<TV>();
    }

    // get_if for exact types: returns pointer or nullptr
    template <typename T>
    [[nodiscard]] inline const T* get_if() const noexcept {
        constexpr std::size_t idx = tl_index_of<T, flat_list>::value;
        if (idx == static_cast<std::size_t>(-1) || idx != index_) return nullptr;
        return reinterpret_cast<const T*>(storage_ptr());
    }
    template <typename T>
    [[nodiscard]] inline T* get_if() noexcept {
        constexpr std::size_t idx = tl_index_of<T, flat_list>::value;
        if (idx == static_cast<std::size_t>(-1) || idx != index_) return nullptr;
        return reinterpret_cast<T*>(storage_ptr());
    }

    // get_if for nested Variant types: returns optional<T> (value copy) or nullopt
    template <typename T>
    inline std::enable_if_t<is_variant<std::decay_t<T>>::value, std::optional<std::decay_t<T>>>
    get_if() const noexcept {
        using TV = std::decay_t<T>;
        if (!holds<TV>()) return std::nullopt;
        return std::optional<TV>(get<TV>());
    }

    // holds<T>() supports:
    // - if T is flattened alternative: true iff current index matches that exact type
    // - if T is a Variant<...>: true iff current stored type belongs to that Variant's inner_types
    template <typename T>
    [[nodiscard]] inline bool holds() const noexcept {
        using U = std::decay_t<T>;
        if constexpr (is_variant<U>::value) {
            return holds_any_of<typename U::inner_types>();
        } else {
            constexpr std::size_t idx = tl_index_of<U, flat_list>::value;
            if (idx == static_cast<std::size_t>(-1)) return false;
            return index_ == idx;
        }
    }

    template <typename T>
    [[nodiscard]] inline bool is() const noexcept {
        return holds<T>();
    }

    // visit: single-variant visitation. visitor should be callable with the exact held type.
    template <typename Visitor>
    decltype(auto) visit(Visitor&& vis) {
        if (index_ == npos) throw std::bad_variant_access();
        return visit_impl<std::decay_t<Visitor>, 0>(std::forward<Visitor>(vis));
    }

    template <typename Visitor>
    decltype(auto) visit(Visitor&& vis) const {
        if (index_ == npos) throw std::bad_variant_access();
        return visit_impl_const<std::decay_t<Visitor>, 0>(std::forward<Visitor>(vis));
    }

    // swap member
    void swap(Variant& other) noexcept(std::is_nothrow_move_constructible<typename nth_type<0, flat_list>::type>::value
                                       && std::is_nothrow_move_assignable<typename nth_type<0, flat_list>::type>::value) {
        if (this == &other) return;
        if (index_ == other.index_) {
            if (index_ == npos) return;
            // same alternative type: swap the contained values via std::swap
            swap_same_index(index_, other);
            return;
        }
        // different types: move both to temporaries then place back
        Variant tmp_self;
        Variant tmp_other;

        // move-construct temps
        if (index_ != npos) { tmp_self.move_from_index(index_, storage_ptr()); tmp_self.index_ = index_; }
        if (other.index_ != npos) { tmp_other.move_from_index(other.index_, other.storage_ptr()); tmp_other.index_ = other.index_; }

        // destroy originals
        destroy_current();
        other.destroy_current();

        // move back
        if (tmp_other.index_ != npos) { move_from_index(tmp_other.index_, tmp_other.storage_ptr()); index_ = tmp_other.index_; }
        if (tmp_self.index_ != npos) { other.move_from_index(tmp_self.index_, tmp_self.storage_ptr()); other.index_ = tmp_self.index_; }
    }

private:
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    // compute max sizeof / align for flat_list
    template <typename List> struct max_sizeof;
    template <> struct max_sizeof<type_list<>> : std::integral_constant<std::size_t, 0> {};
    template <typename H, typename... Ts>
    struct max_sizeof<type_list<H, Ts...>> {
        static constexpr std::size_t next = max_sizeof<type_list<Ts...>>::value;
        static constexpr std::size_t value = (sizeof(H) > next ? sizeof(H) : next);
    };
    template <typename List> struct max_alignof;
    template <> struct max_alignof<type_list<>> : std::integral_constant<std::size_t, 1> {};
    template <typename H, typename... Ts>
    struct max_alignof<type_list<H, Ts...>> {
        static constexpr std::size_t next = max_alignof<type_list<Ts...>>::value;
        static constexpr std::size_t value = (alignof(H) > next ? alignof(H) : next);
    };

    static constexpr std::size_t storage_size = max_sizeof<flat_list>::value;
    static constexpr std::size_t storage_align = max_alignof<flat_list>::value;
    alignas(storage_align) unsigned char storage_[storage_size ? storage_size : 1];
    std::size_t index_ = npos;

    void* storage_ptr() noexcept { return static_cast<void*>(storage_); }
    const void* storage_ptr() const noexcept { return static_cast<const void*>(storage_); }

    void destroy_current() noexcept {
        if (index_ == npos) return;
        destroy_by_index(index_, storage_ptr());
        index_ = npos;
    }

    void copy_from_index(std::size_t idx, const void* src) {
        copy_construct_by_index(idx, src, storage_ptr());
    }
    void move_from_index(std::size_t idx, void* src) {
        move_construct_by_index(idx, src, storage_ptr());
    }

    // destroy by index (recursive)
    template <std::size_t I = 0>
    void destroy_by_index(std::size_t idx, const void* ptr) noexcept {
        if constexpr (I < alternatives_count) {
            using T = typename nth_type<I, flat_list>::type;
            if (idx == I) {
                reinterpret_cast<const T*>(ptr)->~T();
                return;
            } else {
                destroy_by_index<I + 1>(idx, ptr);
            }
        } else {
            // unreachable
        }
    }

    template <std::size_t I = 0>
    void copy_construct_by_index(std::size_t idx, const void* src, void* dst) {
        if constexpr (I < alternatives_count) {
            using T = typename nth_type<I, flat_list>::type;
            if (idx == I) {
                new (dst) T(*reinterpret_cast<const T*>(src));
                return;
            } else {
                copy_construct_by_index<I + 1>(idx, src, dst);
            }
        } else {
            throw std::runtime_error("Variant: bad copy_construct_by_index");
        }
    }

    template <std::size_t I = 0>
    void move_construct_by_index(std::size_t idx, void* src, void* dst) {
        if constexpr (I < alternatives_count) {
            using T = typename nth_type<I, flat_list>::type;
            if (idx == I) {
                new (dst) T(std::move(*reinterpret_cast<T*>(src)));
                return;
            } else {
                move_construct_by_index<I + 1>(idx, src, dst);
            }
        } else {
            throw std::runtime_error("Variant: bad move_construct_by_index");
        }
    }

    template <typename T, typename... CtArgs>
    void construct_by_type(CtArgs&&... args) {
        new (storage_ptr()) T(std::forward<CtArgs>(args)...);
    }

    template <std::size_t I = 0>
    void assign_same_index(std::size_t idx, const void* src) {
        if constexpr (I < alternatives_count) {
            using T = typename nth_type<I, flat_list>::type;
            if (idx == I) {
                *reinterpret_cast<T*>(storage_ptr()) = *reinterpret_cast<const T*>(src);
                return;
            } else {
                assign_same_index<I + 1>(idx, src);
            }
        } else {
            // no-op
        }
    }

    template <typename T>
    T& get_ref_by_index() {
        return *reinterpret_cast<T*>(storage_ptr());
    }
    template <typename T>
    const T& get_ref_by_index() const {
        return *reinterpret_cast<const T*>(storage_ptr());
    }

    // holds_any_of: checks if current stored type is any type in InnerList
    template <typename InnerList>
    bool holds_any_of() const noexcept {
        return holds_any_of_impl<InnerList, 0>();
    }

    template <typename InnerList, std::size_t I = 0>
    bool holds_any_of_impl() const noexcept {
        if constexpr (I >= tl_length<InnerList>::value) return false;
        else {
            using InnerT = typename nth_type<I, InnerList>::type;
            constexpr std::size_t idx = tl_index_of<InnerT, flat_list>::value;
            if (idx != static_cast<std::size_t>(-1) && index_ == idx) return true;
            return holds_any_of_impl<InnerList, I + 1>();
        }
    }

    // Construct nested Variant<TV> from current stored value (by index)
    template <typename TV>
    std::enable_if_t<is_variant<TV>::value, TV>
    construct_nested_variant_from_index(std::size_t idx) const noexcept {
        return construct_nested_variant_from_index_impl<TV, 0>(idx);
    }

    template <typename TV, std::size_t I>
    std::enable_if_t<(I < alternatives_count), TV>
    construct_nested_variant_from_index_impl(std::size_t idx) const noexcept {
        using U = typename nth_type<I, flat_list>::type;
        constexpr std::size_t in_nested = tl_index_of<U, typename TV::inner_types>::value;
        if (idx == I && in_nested != static_cast<std::size_t>(-1)) {
            return TV(get<U>()); // copy construct nested TV from held U
        }
        return construct_nested_variant_from_index_impl<TV, I + 1>(idx);
    }

    template <typename TV, std::size_t I>
    std::enable_if_t<(I >= alternatives_count), TV>
    construct_nested_variant_from_index_impl(std::size_t) const noexcept {
        return TV{}; // fallback default-constructed
    }

    // visit implementation (non-const)
    template <typename Visitor, std::size_t I>
    decltype(auto) visit_impl(Visitor&& vis) {
        if constexpr (I < alternatives_count) {
            using T = typename nth_type<I, flat_list>::type;
            if (index_ == I) return std::invoke(std::forward<Visitor>(vis), get<T>());
            return visit_impl<Visitor, I + 1>(std::forward<Visitor>(vis));
        } else {
            throw std::bad_variant_access();
        }
    }

    template <typename Visitor, std::size_t I>
    decltype(auto) visit_impl_const(Visitor&& vis) const {
        if constexpr (I < alternatives_count) {
            using T = typename nth_type<I, flat_list>::type;
            if (index_ == I) return std::invoke(std::forward<Visitor>(vis), get<T>());
            return visit_impl_const<Visitor, I + 1>(std::forward<Visitor>(vis));
        } else {
            throw std::bad_variant_access();
        }
    }

    // swap_same_index: swap the actual stored values if same type
    void swap_same_index(std::size_t idx, Variant& other) {
        swap_same_index_impl<0>(idx, other);
    }
    template <std::size_t I = 0>
    void swap_same_index_impl(std::size_t idx, Variant& other) {
        if constexpr (I < alternatives_count) {
            using T = typename nth_type<I, flat_list>::type;
            if (idx == I) {
                using std::swap;
                swap(get_ref_by_index<T>(), other.get_ref_by_index<T>());
                return;
            } else {
                swap_same_index_impl<I + 1>(idx, other);
            }
        } else {
            // no-op
        }
    }

}; // class Variant

// non-member swap
template <typename... Ts>
inline void swap(Variant<Ts...>& a, Variant<Ts...>& b) noexcept(noexcept(a.swap(b))) {
    a.swap(b);
}

} // namespace meow::utils

// -------------------- USAGE EXAMPLES (quick) --------------------
/*
using V1 = meow::utils::Variant<int, float, char>;
using V2 = meow::utils::Variant<V1, double, long>;

V2 v = 3.14; // holds double
if (v.holds<double>()) {
    double d = v.safe_get<double>();
}

// nested get: construct V1 from V2 if holds alternative of V1
if (v.holds<V1>()) {
    V1 nested = v.get<V1>();
}

// emplace
v.emplace<double>(2.71);

// visit
v.visit([](auto&& x) {
    using U = std::decay_t<decltype(x)>;
    // do something with x
});

// swap
V2 a = 1.0; V2 b = 42L; swap(a, b);
*/

