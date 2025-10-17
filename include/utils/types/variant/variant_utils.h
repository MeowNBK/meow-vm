// variant/variant_utils.h
#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

namespace meow::utils {

// Namespace `detail` chứa các công cụ siêu lập trình (metaprogramming)
namespace detail {

// ---------- tiny typelist (danh sách kiểu) ----------
template <typename... Ts> struct type_list {};

// Nối một kiểu vào cuối danh sách
template <typename List, typename T> struct type_list_append;
template <typename... Ts, typename T>
struct type_list_append<type_list<Ts...>, T> { using type = type_list<Ts..., T>; };

// Kiểm tra xem một kiểu có trong danh sách không
template <typename T, typename List> struct type_list_contains;
template <typename T> struct type_list_contains<T, type_list<>> : std::false_type {};
template <typename T, typename Head, typename... Tail>
struct type_list_contains<T, type_list<Head, Tail...>>
    : std::conditional_t<std::is_same_v<T, Head>, std::true_type, type_list_contains<T, type_list<Tail...>>> {};

// Nối một kiểu vào danh sách nếu nó chưa tồn tại
template <typename List, typename T>
struct type_list_append_unique {
    using type = std::conditional_t<type_list_contains<T, List>::value, List, typename type_list_append<List, T>::type>;
};

// Lấy kiểu ở vị trí thứ I
template <std::size_t I, typename List> struct nth_type;
template <std::size_t I, typename Head, typename... Tail>
struct nth_type<I, type_list<Head, Tail...>> : nth_type<I - 1, type_list<Tail...>> {};
template <typename Head, typename... Tail>
struct nth_type<0, type_list<Head, Tail...>> { using type = Head; };

// Lấy độ dài danh sách
template <typename List> struct type_list_length;
template <typename... Ts> struct type_list_length<type_list<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)> {};

// Lấy chỉ số của một kiểu trong danh sách
inline constexpr std::size_t invalid_index = static_cast<std::size_t>(-1);
template <typename T, typename List> struct type_list_index_of;
template <typename T> struct type_list_index_of<T, type_list<>> { static constexpr std::size_t value = invalid_index; };
template <typename T, typename Head, typename... Tail>
struct type_list_index_of<T, type_list<Head, Tail...>> {
private:
    static constexpr std::size_t next = type_list_index_of<T, type_list<Tail...>>::value;
public:
    static constexpr std::size_t value = std::is_same_v<T, Head> ? 0u : (next == invalid_index ? invalid_index : 1u + next);
};

// Trait to detect whether T supports operator== (const T& == const T&)
template <typename T, typename = void>
struct has_eq : std::false_type {};

template <typename T>
struct has_eq<T, std::void_t<
    decltype(std::declval<const T&>() == std::declval<const T&>())
>> : std::true_type {};

// "Làm phẳng" các variant lồng nhau thành một danh sách duy nhất các kiểu
template <typename...> struct flatten_list_implement;
template <> struct flatten_list_implement<> { using type = type_list<>; };

template <typename Head, typename... Tail>
struct flatten_list_implement<Head, Tail...> {
private:
    using tail_flat = typename flatten_list_implement<Tail...>::type;

    template <typename H> struct head_to_list { using type = type_list<H>; };
    template <typename... Inner> struct head_to_list<type_list<Inner...>> { using type = type_list<Inner...>; };

    using head_list = typename head_to_list<Head>::type;

    template <typename Src, typename Acc> struct merge_src;
    template <typename Acc> struct merge_src<type_list<>, Acc> { using type = Acc; };
    template <typename S0, typename... Ss, typename Acc>
    struct merge_src<type_list<S0, Ss...>, Acc> {
        using next_acc = typename type_list_append_unique<Acc, S0>::type;
        using type = typename merge_src<type_list<Ss...>, next_acc>::type;
    };

    using merged_with_head = typename merge_src<head_list, type_list<>>::type;
    using merged_all = typename merge_src<tail_flat, merged_with_head>::type;

public:
    using type = merged_all;
};

template <typename... Ts>
using flattened_unique_t = typename flatten_list_implement<Ts...>::type;

} // namespace detail

// Tiện ích overload cho visit
template <class... Fs>
struct overload : Fs... { using Fs::operator()...; };
template <class... Fs> overload(Fs...) -> overload<Fs...>;

} // namespace meow::utils
