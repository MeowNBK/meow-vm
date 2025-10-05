#pragma once

#include "common/pch.h"

namespace meow::utils {

// -------------------- minimal meta helpers (kept, slightly cleaned) --------------------
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

// flatten nested variants (kept same algorithm)
template <typename...> struct flatten_list_impl;
template <> struct flatten_list_impl<> { using type = type_list<>; };

template <typename Head, typename... Tail>
struct flatten_list_impl<Head, Tail...> {
private:
    using tail_flat = typename flatten_list_impl<Tail...>::type;

    template <typename H>
    struct expand_head_nonvariant {
        using with_head = typename tl_append_unique<type_list<>, H>::type;
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
    struct expand_head<decltype((void)0, Head)> { using type = typename expand_head_nonvariant<Head>::type; };

    template <typename... Inner>
    struct expand_head<type_list<Inner...>> { using type = typename expand_head_nonvariant<type_list<Inner...>>::type; };

    template <typename... Inner>
    struct expand_head<type_list<Inner...>&&> { using type = typename expand_head_nonvariant<type_list<Inner...>>::type; };

    template <typename... Inner>
    struct expand_head<type_list<Inner...> const> { using type = typename expand_head_nonvariant<type_list<Inner...>>::type; };

    // Specialization for Variant<T...> will be handled below by a different overload:
    template <typename... Inner>
    struct expand_head<class Variant<Inner...>> {
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


// -------------------- detection helpers --------------------
template <typename, typename = void>
struct has_eq : std::false_type {};

template <typename T>
struct has_eq<T, std::void_t<decltype(std::declval<const T&>() == std::declval<const T&>())>> : std::true_type {};

// is_variant
template <typename T> struct is_variant : std::false_type {};
template <typename... Us> struct is_variant<class Variant<Us...>> : std::true_type {};


// -------------------- ops tables (unchanged approach but compact) --------------------
template <typename... Ts>
struct ops {
    using destroy_fn_t = void(*)(void*) noexcept;
    using copy_ctor_fn_t = void(*)(void* dst, const void* src);
    using move_ctor_fn_t = void(*)(void* dst, void* src);
    using copy_assign_fn_t = void(*)(void* dst, const void* src);
    using move_assign_fn_t = void(*)(void* dst, void* src);

    template <typename T>
    static void destroy_impl(void* p) noexcept {
        if constexpr (!std::is_trivially_destructible<T>::value) {
            reinterpret_cast<T*>(p)->~T();
        } else {
            (void)p;
        }
    }

    template <typename T>
    static void copy_ctor_impl(void* dst, const void* src) {
        if constexpr (std::is_trivially_copyable<T>::value && sizeof(T) <= sizeof(void*) ) {
            // fast path for small trivially-copyable: memcpy of pointer-sized chunk (good for Int/Real/Pointer)
            std::memcpy(dst, src, sizeof(T));
        } else if constexpr (std::is_trivially_copy_constructible<T>::value) {
            std::memcpy(dst, src, sizeof(T));
        } else {
            new (dst) T(*reinterpret_cast<const T*>(src));
        }
    }

    template <typename T>
    static void move_ctor_impl(void* dst, void* src) {
        if constexpr (std::is_trivially_move_constructible<T>::value && std::is_trivially_copyable<T>::value) {
            std::memcpy(dst, src, sizeof(T));
        } else {
            new (dst) T(std::move(*reinterpret_cast<T*>(src)));
        }
    }

    template <typename T>
    static void copy_assign_impl(void* dst, const void* src) {
        if constexpr (std::is_copy_assignable<T>::value) {
            *reinterpret_cast<T*>(dst) = *reinterpret_cast<const T*>(src);
        } else if constexpr (std::is_trivially_copyable<T>::value) {
            std::memcpy(dst, src, sizeof(T));
        } else {
            reinterpret_cast<T*>(dst)->~T();
            new (dst) T(*reinterpret_cast<const T*>(src));
        }
    }

    template <typename T>
    static void move_assign_impl(void* dst, void* src) {
        if constexpr (std::is_move_assignable<T>::value) {
            *reinterpret_cast<T*>(dst) = std::move(*reinterpret_cast<T*>(src));
        } else if constexpr (std::is_trivially_copyable<T>::value) {
            std::memcpy(dst, src, sizeof(T));
        } else {
            reinterpret_cast<T*>(dst)->~T();
            new (dst) T(std::move(*reinterpret_cast<T*>(src)));
        }
    }

    static constexpr std::array<destroy_fn_t, sizeof...(Ts)> make_destroy_table() { return { &destroy_impl<Ts>... }; }
    static constexpr std::array<copy_ctor_fn_t, sizeof...(Ts)> make_copy_ctor_table() { return { &copy_ctor_impl<Ts>... }; }
    static constexpr std::array<move_ctor_fn_t, sizeof...(Ts)> make_move_ctor_table() { return { &move_ctor_impl<Ts>... }; }
    static constexpr std::array<copy_assign_fn_t, sizeof...(Ts)> make_copy_assign_table() { return { &copy_assign_impl<Ts>... }; }
    static constexpr std::array<move_assign_fn_t, sizeof...(Ts)> make_move_assign_table() { return { &move_assign_impl<Ts>... }; }

    static inline constexpr auto destroy_table = make_destroy_table();
    static inline constexpr auto copy_ctor_table = make_copy_ctor_table();
    static inline constexpr auto move_ctor_table = make_move_ctor_table();
    static inline constexpr auto copy_assign_table = make_copy_assign_table();
    static inline constexpr auto move_assign_table = make_move_assign_table();
};


// ========================= Optimized Variant =========================
template <typename... Args>
class Variant {
public:
    using flat_list = flattened_unique_t<Args...>;
    static constexpr std::size_t alternatives_count = tl_length<flat_list>::value;
    using inner_types = type_list<Args...>;

    // compact index type: use uint8_t when alternatives small -> smaller footprint & better codegen
    using index_t = std::conditional_t<(alternatives_count <= 0xFF), uint8_t, std::size_t>;
    static constexpr index_t npos = static_cast<index_t>(-1);

    Variant() noexcept : index_(npos) {}
    ~Variant() noexcept { destroy_current(); }

    Variant(const Variant& other) : index_(npos) {
        if (other.index_ != npos) {
            copy_from_index(other.index_, other.storage_ptr());
            index_ = other.index_;
        }
    }

    Variant(Variant&& other) noexcept : index_(npos) {
        if (other.index_ != npos) {
            move_from_index(other.index_, other.storage_ptr());
            index_ = other.index_;
            other.destroy_current();
            other.index_ = npos;
        }
    }

    template <typename T, typename U = std::decay_t<T>,
              typename = std::enable_if_t<tl_index_of<U, flat_list>::value != static_cast<std::size_t>(-1)>>
    Variant(T&& v) noexcept(std::is_nothrow_constructible<U, T&&>::value) : index_(npos) {
        construct_by_type<U>(std::forward<T>(v));
        index_ = static_cast<index_t>(tl_index_of<U, flat_list>::value);
    }

    template <typename T, typename... CArgs, typename U = std::decay_t<T>,
              typename = std::enable_if_t<tl_index_of<U, flat_list>::value != static_cast<std::size_t>(-1)>>
    explicit Variant(std::in_place_type_t<T>, CArgs&&... args) noexcept(std::is_nothrow_constructible<U, CArgs...>::value) : index_(npos) {
        construct_by_type<U>(std::forward<CArgs>(args)...);
        index_ = static_cast<index_t>(tl_index_of<U, flat_list>::value);
    }

    template <std::size_t I, typename... CArgs>
    explicit Variant(std::in_place_index_t<I>, CArgs&&... args) noexcept {
        static_assert(I < alternatives_count, "in_place_index out of range");
        using U = typename nth_type<I, flat_list>::type;
        construct_by_type<U>(std::forward<CArgs>(args)...);
        index_ = static_cast<index_t>(I);
    }

    Variant& operator=(const Variant& other) {
        if (this == &other) return *this;
        if (other.index_ == npos) { destroy_current(); index_ = npos; return *this; }
        if (index_ == other.index_) {
            full_ops::copy_assign_table[index_](storage_ptr(), other.storage_ptr());
            return *this;
        }
        Variant tmp(other);
        swap(tmp);
        return *this;
    }

    Variant& operator=(Variant&& other) noexcept {
        if (this == &other) return *this;
        if (other.index_ == npos) { destroy_current(); index_ = npos; return *this; }
        destroy_current();
        move_from_index(other.index_, other.storage_ptr());
        index_ = other.index_;
        other.destroy_current(); other.index_ = npos;
        return *this;
    }

    template <typename T, typename U = std::decay_t<T>,
              typename = std::enable_if_t<tl_index_of<U, flat_list>::value != static_cast<std::size_t>(-1)>>
    Variant& operator=(T&& v) noexcept(std::is_nothrow_constructible<U, T&&>::value) {
        constexpr std::size_t idx = tl_index_of<U, flat_list>::value;
        if (index_ == static_cast<index_t>(idx)) {
            full_ops::copy_assign_table[idx](storage_ptr(), &v);
        } else {
            destroy_current();
            construct_by_type<U>(std::forward<T>(v));
            index_ = static_cast<index_t>(idx);
        }
        return *this;
    }

    template <typename T, typename... CArgs, typename U = std::decay_t<T>,
              typename = std::enable_if_t<tl_index_of<U, flat_list>::value != static_cast<std::size_t>(-1)>>
    void emplace(CArgs&&... args) {
        destroy_current();
        construct_by_type<U>(std::forward<CArgs>(args)...);
        index_ = static_cast<index_t>(tl_index_of<U, flat_list>::value);
    }

    template <std::size_t I, typename... CArgs>
    void emplace_index(CArgs&&... args) {
        static_assert(I < alternatives_count, "emplace_index out of range");
        destroy_current();
        using U = typename nth_type<I, flat_list>::type;
        construct_by_type<U>(std::forward<CArgs>(args)...);
        index_ = static_cast<index_t>(I);
    }

    bool valueless_by_exception() const noexcept { return index_ == npos; }
    std::size_t index() const noexcept { return static_cast<std::size_t>(index_); }

    template <typename T>
    static constexpr std::size_t index_of() noexcept { return tl_index_of<std::decay_t<T>, flat_list>::value; }

    template <typename T>
    [[nodiscard]] const T& get() const noexcept { return *reinterpret_cast<const T*>(storage_ptr()); }
    template <typename T>
    [[nodiscard]] T& get() noexcept { return *reinterpret_cast<T*>(storage_ptr()); }

    template <typename T>
    const T* get_if() const noexcept {
        constexpr std::size_t idx = tl_index_of<T, flat_list>::value;
        if (idx == static_cast<std::size_t>(-1) || index_ != static_cast<index_t>(idx)) return nullptr;
        return reinterpret_cast<const T*>(storage_ptr());
    }
    template <typename T>
    T* get_if() noexcept {
        constexpr std::size_t idx = tl_index_of<T, flat_list>::value;
        if (idx == static_cast<std::size_t>(-1) || index_ != static_cast<index_t>(idx)) return nullptr;
        return reinterpret_cast<T*>(storage_ptr());
    }

    // unchecked getter for hot paths
    template <typename T>
    const T& unsafe_get_unchecked() const noexcept { return *reinterpret_cast<const T*>(storage_ptr()); }
    template <typename T>
    T& unsafe_get_unchecked() noexcept { return *reinterpret_cast<T*>(storage_ptr()); }

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
    inline std::enable_if_t<is_variant<std::decay_t<T>>::value, std::decay_t<T>>
    get() const noexcept {
        using TV = std::decay_t<T>;
        return construct_nested_variant_from_index<TV>(static_cast<std::size_t>(index_));
    }

    template <typename T>
    bool holds() const noexcept {
        using U = std::decay_t<T>;
        if constexpr (is_variant<U>::value) {
            return holds_any_of<typename U::inner_types>();
        } else {
            constexpr std::size_t idx = tl_index_of<U, flat_list>::value;
            if (idx == static_cast<std::size_t>(-1)) return false;
            return index_ == static_cast<index_t>(idx);
        }
    }

    // -------- visit: optimized via jump-table generated per-Visitor type (O(1) dispatch) -------
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

    // swap optimized
    void swap(Variant& other) noexcept {
        if (this == &other) return;
        if (index_ == other.index_) {
            if (index_ == npos) return;
            swap_same_index(index_, other);
            return;
        }
        Variant tmp_self;
        Variant tmp_other;
        if (index_ != npos) { tmp_self.move_from_index(index_, storage_ptr()); tmp_self.index_ = index_; }
        if (other.index_ != npos) { tmp_other.move_from_index(other.index_, other.storage_ptr()); tmp_other.index_ = other.index_; }
        destroy_current(); other.destroy_current();
        if (tmp_other.index_ != npos) { move_from_index(tmp_other.index_, tmp_other.storage_ptr()); index_ = tmp_other.index_; }
        if (tmp_self.index_  != npos) { other.move_from_index(tmp_self.index_, tmp_self.storage_ptr()); other.index_ = tmp_self.index_; }
    }

    // emplace_or_assign optimized
    template <typename T, typename... CArgs>
    void emplace_or_assign(CArgs&&... args) {
        using U = std::decay_t<T>;
        constexpr std::size_t idx = tl_index_of<U, flat_list>::value;
        static_assert(idx != static_cast<std::size_t>(-1), "Type not in variant");
        if (index_ == static_cast<index_t>(idx)) {
            U tmp(std::forward<CArgs>(args)...);
            full_ops::copy_assign_table[idx](storage_ptr(), &tmp);
        } else {
            destroy_current();
            construct_by_type<U>(std::forward<CArgs>(args)...);
            index_ = static_cast<index_t>(idx);
        }
    }

    // try_get (alias)
    template <typename T> T* try_get() noexcept { return get_if<T>(); }
    template <typename T> const T* try_get() const noexcept { return get_if<T>(); }

    // get by numeric index (checked)
    const void* get_by_index(std::size_t idx) const noexcept {
        if (idx == static_cast<std::size_t>(index_) && index_ != npos) return storage_ptr();
        return nullptr;
    }
    void* get_by_index(std::size_t idx) noexcept {
        if (idx == static_cast<std::size_t>(index_) && index_ != npos) return storage_ptr();
        return nullptr;
    }

    static constexpr std::size_t alternatives() noexcept { return alternatives_count; }

    // operator==
    bool operator==(const Variant& other) const noexcept {
        if (index_ != other.index_) return false;
        if (index_ == npos) return true;
        return equal_same_index(static_cast<std::size_t>(index_), other);
    }
    bool operator!=(const Variant& other) const noexcept { return !(*this == other); }

private:
    template <typename List> struct ops_resolver;
    template <typename... Ts> struct ops_resolver<type_list<Ts...>> { using type = ops<Ts...>; };
    using full_ops = typename ops_resolver<flat_list>::type;

    // compute storage size & align
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
    index_t index_ = npos;

    void* storage_ptr() noexcept { return static_cast<void*>(storage_); }
    const void* storage_ptr() const noexcept { return static_cast<const void*>(storage_); }

    void destroy_current() noexcept {
        if (index_ == npos) return;
        full_ops::destroy_table[index_](storage_ptr());
        index_ = npos;
    }

    void copy_from_index(std::size_t idx, const void* src) {
        full_ops::copy_ctor_table[idx](storage_ptr(), src);
    }
    void move_from_index(std::size_t idx, void* src) {
        full_ops::move_ctor_table[idx](storage_ptr(), src);
    }

    template <typename T, typename... CArgs>
    void construct_by_type(CArgs&&... args) {
        new (storage_ptr()) T(std::forward<CArgs>(args)...);
    }

    template <std::size_t I = 0>
    void swap_same_index(std::size_t idx, Variant& other) {
        if constexpr (I < alternatives_count) {
            using T = typename nth_type<I, flat_list>::type;
            if (idx == I) { using std::swap; swap(*reinterpret_cast<T*>(storage_ptr()), *reinterpret_cast<T*>(other.storage_ptr())); return; }
            swap_same_index<I + 1>(idx, other);
        }
    }

    // visit_impl: build per-Visitor jump table (array of function pointers) using index_sequence expansion
    template <typename Visitor, std::size_t... Is>
    decltype(auto) visit_impl(Visitor&& vis, std::index_sequence<Is...>) {
        // deduce return type from first alternative (unsafe if different alternatives return different types)
        using R = decltype(std::invoke(std::declval<Visitor>(), *reinterpret_cast<typename nth_type<0, flat_list>::type*>(storage_ptr())));
        using fn_t = R(*)(void*, Visitor&&);
        // create jump table of inline functions (non-capturing lambdas convertible to function pointers)
        static fn_t table[] = {
            +[](void* storage, Visitor&& v) -> R {
                using T = typename nth_type<Is, flat_list>::type;
                return std::invoke(std::forward<Visitor>(v), *reinterpret_cast<T*>(storage));
            }...
        };
        return table[index_](storage_ptr(), std::forward<Visitor>(vis));
    }

    template <typename Visitor, std::size_t... Is>
    decltype(auto) visit_impl_const(Visitor&& vis, std::index_sequence<Is...>) const {
        using R = decltype(std::invoke(std::declval<Visitor>(), *reinterpret_cast<const typename nth_type<0, flat_list>::type*>(storage_ptr())));
        using fn_t = R(*)(const void*, Visitor&&);
        static fn_t table[] = {
            +[](const void* storage, Visitor&& v) -> R {
                using T = typename nth_type<Is, flat_list>::type;
                return std::invoke(std::forward<Visitor>(v), *reinterpret_cast<const T*>(storage));
            }...
        };
        return table[index_](storage_ptr(), std::forward<Visitor>(vis));
    }

    template <typename InnerList>
    bool holds_any_of() const noexcept { return holds_any_of_impl<InnerList, 0>(); }
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

    // nested construct
    template <typename TV>
    std::enable_if_t<is_variant<TV>::value, TV> construct_nested_variant_from_index(std::size_t idx) const noexcept {
        return construct_nested_variant_from_index_impl<TV, 0>(idx);
    }
    template <typename TV, std::size_t I>
    std::enable_if_t<(I < alternatives_count), TV> construct_nested_variant_from_index_impl(std::size_t idx) const noexcept {
        using U = typename nth_type<I, flat_list>::type;
        constexpr std::size_t in_nested = tl_index_of<U, typename TV::inner_types>::value;
        if (idx == I && in_nested != static_cast<std::size_t>(-1)) {
            return TV(get<U>());
        }
        return construct_nested_variant_from_index_impl<TV, I + 1>(idx);
    }
    template <typename TV, std::size_t I>
    std::enable_if_t<(I >= alternatives_count), TV> construct_nested_variant_from_index_impl(std::size_t) const noexcept {
        return TV{};
    }

    // equality helper
    bool equal_same_index(std::size_t idx, const Variant& other) const noexcept {
        return equal_same_index_impl<0>(idx, other);
    }
    template <std::size_t I>
    bool equal_same_index_impl(std::size_t idx, const Variant& other) const noexcept {
        if constexpr (I < alternatives_count) {
            using T = typename nth_type<I, flat_list>::type;
            if (idx == I) {
                if constexpr (std::is_trivially_copyable<T>::value) {
                    return std::memcmp(storage_ptr(), other.storage_ptr(), sizeof(T)) == 0;
                } else if constexpr (has_eq<T>::value) {
                    return *reinterpret_cast<const T*>(storage_ptr()) == *reinterpret_cast<const T*>(other.storage_ptr());
                } else {
                    // fallback: not comparable, return false conservatively (could throw but we keep noexcept)
                    return false;
                }
            }
            return equal_same_index_impl<I + 1>(idx, other);
        }
        return false;
    }
};

// non-member swap
template <typename... Ts>
inline void swap(Variant<Ts...>& a, Variant<Ts...>& b) noexcept(noexcept(a.swap(b))) { a.swap(b); }

} // namespace meow::utils

// End of variant_fast_opt.h
