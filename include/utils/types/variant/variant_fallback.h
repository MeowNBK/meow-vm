// fallback.h - FallbackVariant implementation that reuses meow::utils helpers
#pragma once

// Include the utils header you provided earlier (typelist, flatten, overload,
// ...)
#include "variant_utils.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <new>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

namespace meow::utils {

// Forward declare FallbackVariant and is_variant trait
template <typename...>
class FallbackVariant;
template <typename T>
struct is_variant : std::false_type {};
template <typename... Us>
struct is_variant<FallbackVariant<Us...>> : std::true_type {};

// ---------- ops table for types ----------
template <typename... Ts>
struct ops {
    using destroy_fn_t = void (*)(void*) noexcept;
    using copy_ctor_fn_t = void (*)(void* dst, const void* src);
    using move_ctor_fn_t = void (*)(void* dst, void* src);
    using copy_assign_fn_t = void (*)(void* dst, const void* src);
    using move_assign_fn_t = void (*)(void* dst, void* src);

    template <typename T>
    static void destroy_impl(void* p) noexcept {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            reinterpret_cast<T*>(p)->~T();
        } else {
            (void)p;
        }
    }

    template <typename T>
    static void copy_ctor_impl(void* dst, const void* src) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(dst, src, sizeof(T));
        } else if constexpr (std::is_trivially_copy_constructible_v<T>) {
            std::memcpy(dst, src, sizeof(T));
        } else {
            new (dst) T(*reinterpret_cast<const T*>(src));
        }
    }

    template <typename T>
    static void move_ctor_impl(void* dst, void* src) {
        if constexpr (std::is_trivially_move_constructible_v<T> && std::is_trivially_copyable_v<T>) {
            std::memcpy(dst, src, sizeof(T));
        } else {
            new (dst) T(std::move(*reinterpret_cast<T*>(src)));
        }
    }

    template <typename T>
    static void copy_assign_impl(void* dst, const void* src) {
        if constexpr (std::is_copy_assignable_v<T>) {
            *reinterpret_cast<T*>(dst) = *reinterpret_cast<const T*>(src);
        } else if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(dst, src, sizeof(T));
        } else {
            reinterpret_cast<T*>(dst)->~T();
            new (dst) T(*reinterpret_cast<const T*>(src));
        }
    }

    template <typename T>
    static void move_assign_impl(void* dst, void* src) {
        if constexpr (std::is_move_assignable_v<T>) {
            *reinterpret_cast<T*>(dst) = std::move(*reinterpret_cast<T*>(src));
        } else if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(dst, src, sizeof(T));
        } else {
            reinterpret_cast<T*>(dst)->~T();
            new (dst) T(std::move(*reinterpret_cast<T*>(src)));
        }
    }

    static constexpr std::array<destroy_fn_t, sizeof...(Ts)> make_destroy_table() {
        return {&destroy_impl<Ts>...};
    }
    static constexpr std::array<copy_ctor_fn_t, sizeof...(Ts)> make_copy_ctor_table() {
        return {&copy_ctor_impl<Ts>...};
    }
    static constexpr std::array<move_ctor_fn_t, sizeof...(Ts)> make_move_ctor_table() {
        return {&move_ctor_impl<Ts>...};
    }
    static constexpr std::array<copy_assign_fn_t, sizeof...(Ts)> make_copy_assign_table() {
        return {&copy_assign_impl<Ts>...};
    }
    static constexpr std::array<move_assign_fn_t, sizeof...(Ts)> make_move_assign_table() {
        return {&move_assign_impl<Ts>...};
    }

    static inline constexpr auto destroy_table = make_destroy_table();
    static inline constexpr auto copy_ctor_table = make_copy_ctor_table();
    static inline constexpr auto move_ctor_table = make_move_ctor_table();
    static inline constexpr auto copy_assign_table = make_copy_assign_table();
    static inline constexpr auto move_assign_table = make_move_assign_table();
};

// visitor_returns: ensure a visitor returns the same type for all alternatives
// (mimic std::variant checks)
template <typename Visitor, typename List, int Mode = 0>
struct visitor_returns_impl;

template <typename Visitor, typename... Ts, int Mode>
struct visitor_returns_impl<Visitor, detail::type_list<Ts...>, Mode> {
   private:
    static constexpr bool empty = (sizeof...(Ts) == 0);
    template <typename T>
    using param_t = std::conditional_t<Mode == 1, const T&, std::conditional_t<Mode == 2, T&&, T&>>;
    using results_tuple = std::tuple<std::invoke_result_t<Visitor, param_t<Ts>>...>;

   public:
    using first_t = std::conditional_t<empty, void, std::tuple_element_t<0, results_tuple>>;
    static constexpr bool value = empty || ((std::is_same_v<first_t, std::invoke_result_t<Visitor, param_t<Ts>>> && ...));
    using first = first_t;
};

template <typename Visitor, typename List, int Mode = 0>
using visitor_returns = visitor_returns_impl<Visitor, List, Mode>;

// ---------- FallbackVariant ----------
template <typename... Args>
class FallbackVariant {
   public:
    using flat_list = detail::flattened_unique_t<Args...>;
    static constexpr std::size_t alternatives_count = detail::type_list_length<flat_list>::value;
    using inner_types = detail::type_list<Args...>;

    using index_t = std::conditional_t<(alternatives_count <= 0xFFu), uint8_t, std::size_t>;
    static constexpr index_t npos = static_cast<index_t>(-1);

    FallbackVariant() noexcept : index_(npos) {}
    ~FallbackVariant() noexcept { destroy_current(); }

    FallbackVariant(const FallbackVariant& other) : index_(npos) {
        if (other.index_ != npos) {
            copy_from_index(static_cast<std::size_t>(other.index_), other.storage_ptr());
            index_ = other.index_;
        }
    }

    FallbackVariant(FallbackVariant&& other) noexcept : index_(npos) {
        if (other.index_ != npos) {
            move_from_index(static_cast<std::size_t>(other.index_), other.storage_ptr());
            index_ = other.index_;
            other.destroy_current();
            other.index_ = npos;
        }
    }

    // construct from alternative type (implicit)
    template <typename T, typename U = std::decay_t<T>, typename = std::enable_if_t<detail::type_list_index_of<U, flat_list>::value != detail::invalid_index>>
    FallbackVariant(T&& v) noexcept(std::is_nothrow_constructible_v<U, T&&>) : index_(npos) {
        construct_by_type<U>(std::forward<T>(v));
        index_ = static_cast<index_t>(detail::type_list_index_of<U, flat_list>::value);
    }

    // in_place_type
    template <typename T, typename... CArgs, typename U = std::decay_t<T>, typename = std::enable_if_t<detail::type_list_index_of<U, flat_list>::value != detail::invalid_index>>
    explicit FallbackVariant(std::in_place_type_t<T>, CArgs&&... args) noexcept(std::is_nothrow_constructible_v<U, CArgs...>) : index_(npos) {
        construct_by_type<U>(std::forward<CArgs>(args)...);
        index_ = static_cast<index_t>(detail::type_list_index_of<U, flat_list>::value);
    }

    template <std::size_t I, typename... CArgs>
    explicit FallbackVariant(std::in_place_index_t<I>, CArgs&&... args) noexcept {
        static_assert(I < alternatives_count, "in_place_index out of range");
        using U = typename detail::nth_type<I, flat_list>::type;
        construct_by_type<U>(std::forward<CArgs>(args)...);
        index_ = static_cast<index_t>(I);
    }

    // copy/move assign
    FallbackVariant& operator=(const FallbackVariant& other) {
        if (this == &other) return *this;
        if (other.index_ == npos) {
            destroy_current();
            index_ = npos;
            return *this;
        }
        if (index_ == other.index_) {
            full_ops::copy_assign_table[index_](storage_ptr(), other.storage_ptr());
            return *this;
        }
        FallbackVariant tmp(other);
        swap(tmp);
        return *this;
    }

    FallbackVariant& operator=(FallbackVariant&& other) noexcept {
        if (this == &other) return *this;
        if (other.index_ == npos) {
            destroy_current();
            index_ = npos;
            return *this;
        }
        destroy_current();
        move_from_index(static_cast<std::size_t>(other.index_), other.storage_ptr());
        index_ = other.index_;
        other.destroy_current();
        other.index_ = npos;
        return *this;
    }

    // assign from alternative type
    template <typename T, typename U = std::decay_t<T>, typename = std::enable_if_t<detail::type_list_index_of<U, flat_list>::value != detail::invalid_index>>
    FallbackVariant& operator=(T&& v) noexcept(std::is_nothrow_constructible_v<U, T&&>) {
        constexpr std::size_t idx = detail::type_list_index_of<U, flat_list>::value;
        if (index_ == static_cast<index_t>(idx)) {
            full_ops::copy_assign_table[idx](storage_ptr(), &v);
        } else {
            destroy_current();
            construct_by_type<U>(std::forward<T>(v));
            index_ = static_cast<index_t>(idx);
        }
        return *this;
    }

    // emplace by type or index
    template <typename T, typename... CArgs, typename U = std::decay_t<T>, typename = std::enable_if_t<detail::type_list_index_of<U, flat_list>::value != detail::invalid_index>>
    void emplace(CArgs&&... args) {
        destroy_current();
        construct_by_type<U>(std::forward<CArgs>(args)...);
        index_ = static_cast<index_t>(detail::type_list_index_of<U, flat_list>::value);
    }

    template <std::size_t I, typename... CArgs>
    void emplace_index(CArgs&&... args) {
        static_assert(I < alternatives_count, "emplace_index out of range");
        destroy_current();
        using U = typename detail::nth_type<I, flat_list>::type;
        construct_by_type<U>(std::forward<CArgs>(args)...);
        index_ = static_cast<index_t>(I);
    }

    bool valueless() const noexcept {
        return index_ == npos;
    }
    std::size_t index() const noexcept {
        return static_cast<std::size_t>(index_);
    }

    template <typename T>
    static constexpr std::size_t index_of() noexcept {
        return detail::type_list_index_of<std::decay_t<T>, flat_list>::value;
    }

    // get / get_if (typed)
    template <typename T>
    const T& get() const noexcept {
        return unsafe_get_unchecked<T>();
    }
    template <typename T>
    T& get() noexcept {
        return unsafe_get_unchecked<T>();
    }

    template <typename T>
    T* get_if() noexcept {
        constexpr std::size_t idx = detail::type_list_index_of<T, flat_list>::value;
        if (idx == detail::invalid_index || index_ != static_cast<index_t>(idx)) return nullptr;
        return reinterpret_cast<T*>(storage_ptr());
    }
    template <typename T>
    const T* get_if() const noexcept {
        constexpr std::size_t idx = detail::type_list_index_of<T, flat_list>::value;
        if (idx == detail::invalid_index || index_ != static_cast<index_t>(idx)) return nullptr;
        return reinterpret_cast<const T*>(storage_ptr());
    }

    template <typename T>
    const T& unsafe_get_unchecked() const noexcept {
        return *reinterpret_cast<const T*>(storage_ptr());
    }
    template <typename T>
    T& unsafe_get_unchecked() noexcept {
        return *reinterpret_cast<T*>(storage_ptr());
    }

    // safe get (throws if holds different type)
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

    // If T is a variant type, construct nested variant from current index
    template <typename T>
    std::enable_if_t<is_variant<std::decay_t<T>>::value, std::decay_t<T>> get() const noexcept {
        using TV = std::decay_t<T>;
        return construct_nested_variant_from_index<TV>(static_cast<std::size_t>(index_));
    }

    template <typename T>
    bool holds() const noexcept {
        using U = std::decay_t<T>;
        if constexpr (is_variant<U>::value) {
            return holds_any_of<typename U::inner_types>();
        } else {
            constexpr std::size_t idx = detail::type_list_index_of<U, flat_list>::value;
            if (idx == detail::invalid_index) return false;
            return index_ == static_cast<index_t>(idx);
        }
    }

    // visit (lvalue & const-lvalue)
    template <typename Visitor>
    decltype(auto) visit(Visitor&& vis) {
        if (index_ == npos) throw std::bad_variant_access();
        static_assert(visitor_returns<Visitor, flat_list, 0>::value,
                      "FallbackVariant::visit: visitor must return same type for "
                      "all alternatives (or use void)");
        return visit_impl(std::forward<Visitor>(vis), std::make_index_sequence<alternatives_count>{});
    }

    template <typename Visitor>
    decltype(auto) visit(Visitor&& vis) const {
        if (index_ == npos) throw std::bad_variant_access();
        static_assert(visitor_returns<Visitor, flat_list, 1>::value,
                      "FallbackVariant::visit(const): visitor must return same "
                      "type for all alternatives (or use void)");
        return visit_impl_const(std::forward<Visitor>(vis), std::make_index_sequence<alternatives_count>{});
    }

    // convenience visit with multiple callables
    template <typename... Fs>
    decltype(auto) visit(Fs&&... fs) {
        auto ov = overload<std::decay_t<Fs>...>{std::forward<Fs>(fs)...};
        return visit(std::move(ov));
    }
    template <typename... Fs>
    decltype(auto) visit(Fs&&... fs) const {
        auto ov = overload<std::decay_t<Fs>...>{std::forward<Fs>(fs)...};
        return visit(std::move(ov));
    }

    // optimized swap
    void swap(FallbackVariant& other) noexcept {
        if (this == &other) return;
        if (index_ == other.index_) {
            if (index_ == npos) return;
            swap_same_index(index_, other);
            return;
        }
        FallbackVariant tmp_self;
        FallbackVariant tmp_other;
        if (index_ != npos) {
            tmp_self.move_from_index(static_cast<std::size_t>(index_), storage_ptr());
            tmp_self.index_ = index_;
        }
        if (other.index_ != npos) {
            tmp_other.move_from_index(static_cast<std::size_t>(other.index_), other.storage_ptr());
            tmp_other.index_ = other.index_;
        }
        destroy_current();
        other.destroy_current();
        if (tmp_other.index_ != npos) {
            move_from_index(static_cast<std::size_t>(tmp_other.index_), tmp_other.storage_ptr());
            index_ = tmp_other.index_;
        }
        if (tmp_self.index_ != npos) {
            other.move_from_index(static_cast<std::size_t>(tmp_self.index_), tmp_self.storage_ptr());
            other.index_ = tmp_self.index_;
        }
    }

    template <typename T, typename... CArgs>
    void emplace_or_assign(CArgs&&... args) {
        using U = std::decay_t<T>;
        constexpr std::size_t idx = detail::type_list_index_of<U, flat_list>::value;
        static_assert(idx != detail::invalid_index, "Type not in variant");
        if (index_ == static_cast<index_t>(idx)) {
            U tmp(std::forward<CArgs>(args)...);
            full_ops::copy_assign_table[idx](storage_ptr(), &tmp);
        } else {
            destroy_current();
            construct_by_type<U>(std::forward<CArgs>(args)...);
            index_ = static_cast<index_t>(idx);
        }
    }

    template <typename T>
    T* try_get() noexcept {
        return get_if<T>();
    }
    template <typename T>
    const T* try_get() const noexcept {
        return get_if<T>();
    }

    const void* get_by_index(std::size_t idx) const noexcept {
        if (idx == static_cast<std::size_t>(index_) && index_ != npos) return storage_ptr();
        return nullptr;
    }
    void* get_by_index(std::size_t idx) noexcept {
        if (idx == static_cast<std::size_t>(index_) && index_ != npos) return storage_ptr();
        return nullptr;
    }

    static constexpr std::size_t alternatives() noexcept {
        return alternatives_count;
    }

    bool operator==(const FallbackVariant& other) const noexcept {
        if (index_ != other.index_) return false;
        if (index_ == npos) return true;
        return equal_same_index(static_cast<std::size_t>(index_), other);
    }
    bool operator!=(const FallbackVariant& other) const noexcept {
        return !(*this == other);
    }

    // raw bits helper (debug)
    uint64_t get_raw_bits() const noexcept {
        uint64_t out = 0;
        uint8_t idx_byte = (index_ == npos) ? static_cast<uint8_t>(0xFFu) : static_cast<uint8_t>(index_);
        std::size_t copy_n = std::min<std::size_t>(sizeof(storage_), 7);
        for (std::size_t i = 0; i < copy_n; ++i) {
            uint8_t b = static_cast<uint8_t>(storage_[i]);
            out |= (uint64_t(b) << (8 * i));
        }
        out |= (uint64_t(idx_byte) << 56);
        return out;
    }

    static FallbackVariant from_raw_bits(uint64_t bits) noexcept {
        FallbackVariant v;
        uint8_t idx = static_cast<uint8_t>((bits >> 56) & 0xFFu);
        if (idx == 0xFFu)
            v.index_ = npos;
        else
            v.index_ = static_cast<index_t>(idx);
        std::size_t copy_n = std::min<std::size_t>(sizeof(v.storage_), 7);
        std::memset(v.storage_, 0, sizeof(v.storage_));
        for (std::size_t i = 0; i < copy_n; ++i) {
            v.storage_[i] = static_cast<unsigned char>((bits >> (8 * i)) & 0xFFu);
        }
        return v;
    }

   private:
    // resolve ops for flattened list
    template <typename List>
    struct ops_resolver;
    template <typename... Ts>
    struct ops_resolver<detail::type_list<Ts...>> {
        using type = ops<Ts...>;
    };
    using full_ops = typename ops_resolver<flat_list>::type;

    // compute max sizeof/alignof for storage
    template <typename List>
    struct max_sizeof;
    template <>
    struct max_sizeof<detail::type_list<>> : std::integral_constant<std::size_t, 0> {};
    template <typename H, typename... Ts>
    struct max_sizeof<detail::type_list<H, Ts...>> {
        static constexpr std::size_t next = max_sizeof<detail::type_list<Ts...>>::value;
        static constexpr std::size_t value = (sizeof(H) > next ? sizeof(H) : next);
    };
    template <typename List>
    struct max_alignof;
    template <>
    struct max_alignof<detail::type_list<>> : std::integral_constant<std::size_t, 1> {};
    template <typename H, typename... Ts>
    struct max_alignof<detail::type_list<H, Ts...>> {
        static constexpr std::size_t next = max_alignof<detail::type_list<Ts...>>::value;
        static constexpr std::size_t value = (alignof(H) > next ? alignof(H) : next);
    };

    static constexpr std::size_t storage_size = max_sizeof<flat_list>::value;
    static constexpr std::size_t storage_align = max_alignof<flat_list>::value;
    alignas(storage_align) unsigned char storage_[storage_size ? storage_size : 1];
    index_t index_ = npos;

    void* storage_ptr() noexcept {
        return static_cast<void*>(storage_);
    }
    const void* storage_ptr() const noexcept {
        return static_cast<const void*>(storage_);
    }

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

    // swap when both hold same index
    void swap_same_index(index_t idx, FallbackVariant& other) {
        swap_same_index_impl<0>(idx, other);
    }
    template <std::size_t I>
    void swap_same_index_impl(index_t idx, FallbackVariant& other) {
        if constexpr (I < alternatives_count) {
            using T = typename detail::nth_type<I, flat_list>::type;
            if (idx == static_cast<index_t>(I)) {
                using std::swap;
                swap(*reinterpret_cast<T*>(storage_ptr()), *reinterpret_cast<T*>(other.storage_ptr()));
                return;
            }
            swap_same_index_impl<I + 1>(idx, other);
        }
    }

    // visit_impl jump-table (non-const)
    template <typename Visitor, std::size_t... Is>
    decltype(auto) visit_impl(Visitor&& vis, std::index_sequence<Is...>) {
        using R = std::invoke_result_t<Visitor, typename detail::nth_type<0, flat_list>::type&>;
        using fn_t = R (*)(void*, Visitor&&);
        static fn_t table[] = {+[](void* storage, Visitor&& v) -> R {
            using T = typename detail::nth_type<Is, flat_list>::type;
            return std::invoke(std::forward<Visitor>(v), *reinterpret_cast<T*>(storage));
        }...};
        return table[index_](storage_ptr(), std::forward<Visitor>(vis));
    }

    // visit_impl jump-table (const)
    template <typename Visitor, std::size_t... Is>
    decltype(auto) visit_impl_const(Visitor&& vis, std::index_sequence<Is...>) const {
        using R = std::invoke_result_t<Visitor, const typename detail::nth_type<0, flat_list>::type&>;
        using fn_t = R (*)(const void*, Visitor&&);
        static fn_t table[] = {+[](const void* storage, Visitor&& v) -> R {
            using T = typename detail::nth_type<Is, flat_list>::type;
            return std::invoke(std::forward<Visitor>(v), *reinterpret_cast<const T*>(storage));
        }...};
        return table[index_](storage_ptr(), std::forward<Visitor>(vis));
    }

    // holds_any_of for nested variant type checks
    template <typename InnerList>
    bool holds_any_of() const noexcept {
        return holds_any_of_impl<InnerList, 0>();
    }

    template <typename InnerList, std::size_t I = 0>
    bool holds_any_of_impl() const noexcept {
        if constexpr (I >= detail::type_list_length<InnerList>::value)
            return false;
        else {
            using InnerT = typename detail::nth_type<I, InnerList>::type;
            constexpr std::size_t idx = detail::type_list_index_of<InnerT, flat_list>::value;
            if (idx != detail::invalid_index && index_ == static_cast<index_t>(idx)) return true;
            return holds_any_of_impl<InnerList, I + 1>();
        }
    }

    // construct nested variant from index when TV is a variant
    template <typename TV>
    std::enable_if_t<is_variant<TV>::value, TV> construct_nested_variant_from_index(std::size_t idx) const noexcept {
        return construct_nested_variant_from_index_impl<TV, 0>(idx);
    }

    template <typename TV, std::size_t I>
    std::enable_if_t<(I < alternatives_count), TV> construct_nested_variant_from_index_impl(std::size_t idx) const noexcept {
        using U = typename detail::nth_type<I, flat_list>::type;
        constexpr std::size_t in_nested = detail::type_list_index_of<U, typename TV::inner_types>::value;
        if (idx == I && in_nested != detail::invalid_index) {
            return TV(get<U>());
        }
        return construct_nested_variant_from_index_impl<TV, I + 1>(idx);
    }

    template <typename TV, std::size_t I>
    std::enable_if_t<(I >= alternatives_count), TV> construct_nested_variant_from_index_impl(std::size_t) const noexcept {
        return TV{};
    }

    // equality when indices equal
    bool equal_same_index(std::size_t idx, const FallbackVariant& other) const noexcept {
        return equal_same_index_impl<0>(idx, other);
    }

    template <std::size_t I>
    bool equal_same_index_impl(std::size_t idx, const FallbackVariant& other) const noexcept {
        if constexpr (I < alternatives_count) {
            using T = typename detail::nth_type<I, flat_list>::type;
            if (idx == I) {
                if constexpr (std::is_trivially_copyable_v<T>) {
                    return std::memcmp(storage_ptr(), other.storage_ptr(), sizeof(T)) == 0;
                } else if constexpr (detail::has_eq<T>::value) {
                    return *reinterpret_cast<const T*>(storage_ptr()) == *reinterpret_cast<const T*>(other.storage_ptr());
                } else {
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
inline void swap(FallbackVariant<Ts...>& a, FallbackVariant<Ts...>& b) noexcept(noexcept(a.swap(b))) {
    a.swap(b);
}

// Free visit helpers (convenience)
template <typename... Ts, typename... Fs>
decltype(auto) visit(FallbackVariant<Ts...>& v, Fs&&... fs) {
    auto ov = overload<std::decay_t<Fs>...>{std::forward<Fs>(fs)...};
    return v.visit(std::move(ov));
}

template <typename... Ts, typename... Fs>
decltype(auto) visit(const FallbackVariant<Ts...>& v, Fs&&... fs) {
    auto ov = overload<std::decay_t<Fs>...>{std::forward<Fs>(fs)...};
    return v.visit(std::move(ov));
}

template <typename... Ts, typename... Fs>
decltype(auto) visit(FallbackVariant<Ts...>&& v, Fs&&... fs) {
    auto ov = overload<std::decay_t<Fs>...>{std::forward<Fs>(fs)...};
    return std::move(v).visit(std::move(ov));
}

}  // namespace meow::utils
