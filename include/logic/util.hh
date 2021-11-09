#ifndef LOGIC_UTIL_HH
#define LOGIC_UTIL_HH

#include <algorithm>
#include <array>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

namespace logic {
template <uint64_t size, bool signed_ = false>
struct big_num;

constexpr uint64_t big_num_threshold = sizeof(uint64_t) * 8;
using big_num_holder_type = uint64_t;

namespace util {
// compute the holder type
constexpr bool in_range(uint64_t a, uint64_t min, uint64_t max) { return a >= min and a <= max; }

template <typename T>
constexpr T max(T a, T b) {
    return a > b ? a : b;
}

template <typename T>
constexpr T min(T a, T b) {
    return a > b ? b : a;
}

template <typename T, typename K>
void copy(T &dst, const K &src, uint64_t start, uint64_t end) {
    for (auto i = 0u; i <= end - start; i++) {
        dst.set(i, src[i + start]);
    }
}

static constexpr bool gte(uint64_t a, uint64_t min) { return a >= min; }

template <typename T>
constexpr T abs_diff(T a, T b) {
    return (a > b) ? (a - b) : (b - a);
}

constexpr bool native_num(uint64_t size) { return size <= big_num_threshold; }

// LRM 11.8.1
// For any non-self-determined expression, the result is signed if both operands are signed.
constexpr bool signed_result(bool signed_op1, bool signed_op2) { return signed_op1 && signed_op2; }

template <typename T_R, typename T = std::remove_reference_t<T_R>, auto N = std::tuple_size_v<T>>
constexpr auto reverse_tuple(T_R &&t) {
    return [&t]<auto... I>(std::index_sequence<I...>) {
        constexpr std::array is{(N - 1 - I)...};
        return std::tuple<std::tuple_element_t<is[I], T>...>{
            std::get<is[I]>(std::forward<T_R>(t))...};
    }
    (std::make_index_sequence<N>{});
}

template <uint64_t s, bool signed_, typename enable = void>
struct get_holder_type;

template <uint64_t s, bool signed_>
struct get_holder_type<s, signed_, typename std::enable_if<in_range(s, 1, 1) && !signed_>::type> {
    using type = bool;
};

template <uint64_t s, bool signed_>
struct get_holder_type<s, signed_, typename std::enable_if<in_range(s, 1, 1) && signed_>::type> {
    using type = bool;
};

template <uint64_t s, bool signed_>
struct get_holder_type<s, signed_, typename std::enable_if<in_range(s, 2, 8) && !signed_>::type> {
    using type = uint8_t;
};

template <uint64_t s, bool signed_>
struct get_holder_type<s, signed_, typename std::enable_if<in_range(s, 2, 8) && signed_>::type> {
    using type = int8_t;
};

template <uint64_t s, bool signed_>
struct get_holder_type<s, signed_, typename std::enable_if<in_range(s, 9, 16) && !signed_>::type> {
    using type = uint16_t;
};

template <uint64_t s, bool signed_>
struct get_holder_type<s, signed_, typename std::enable_if<in_range(s, 9, 16) && signed_>::type> {
    using type = int16_t;
};

template <uint64_t s, bool signed_>
struct get_holder_type<s, signed_, typename std::enable_if<in_range(s, 17, 32) && !signed_>::type> {
    using type = uint32_t;
};

template <uint64_t s, bool signed_>
struct get_holder_type<s, signed_, typename std::enable_if<in_range(s, 17, 32) && signed_>::type> {
    using type = int32_t;
};

template <uint64_t s, bool signed_>
struct get_holder_type<s, signed_, typename std::enable_if<in_range(s, 33, 64) && !signed_>::type> {
    using type = uint64_t;
};

template <uint64_t s, bool signed_>
struct get_holder_type<s, signed_, typename std::enable_if<in_range(s, 33, 64) && signed_>::type> {
    using type = int64_t;
};

template <uint64_t s, bool signed_>
struct get_holder_type<s, signed_, typename std::enable_if<gte(s, 65)>::type> {
    using type = big_num<s, signed_>;
};
}  // namespace util
}  // namespace logic

#endif  // LOGIC_UTIL_HH
