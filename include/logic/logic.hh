#ifndef LOGIC_LOGIC_HH
#define LOGIC_LOGIC_HH

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

// some constants
constexpr uint64_t big_num_threshold = sizeof(uint64_t) * 8;

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

using big_num_holder_type = uint64_t;

template <uint64_t size, bool signed_>
struct big_num {
public:
    // Notice that big num is always big endian, as in MSB >= LSB
    constexpr static auto s =
        (size % big_num_threshold) == 0 ? size / big_num_threshold : (size / big_num_threshold) + 1;
    // use uint64_t as a holder
    std::array<big_num_holder_type, s> values;

    /*
     * single bit
     */
    bool inline operator[](uint64_t idx) const {
        if (idx < size) [[likely]] {
            auto a = idx / big_num_threshold;
            auto b = idx % big_num_threshold;
            return (values[a] >> b) & 1;
        } else {
            return false;
        }
    }

    template <uint64_t idx>
    requires(idx < size) void set(bool value) {
        auto constexpr a = idx / big_num_threshold;
        auto constexpr b = idx % big_num_threshold;
        if (value) {
            values[a] |= 1ull << b;
        } else {
            values[a] &= ~(1ull << b);
        }
    }

    template <uint64_t idx, bool value>
    requires(idx < size) void set() {
        auto constexpr a = idx / big_num_threshold;
        auto constexpr b = idx % big_num_threshold;
        if constexpr (value) {
            values[a] |= 1ull << b;
        } else {
            values[a] &= ~(1ull << b);
        }
    }

    template <uint64_t idx>
    requires(idx < size) [[nodiscard]] bool inline get() const {
        auto a = idx / big_num_threshold;
        auto b = idx % big_num_threshold;
        return (values[a] >> b) & 1;
    }

    // single bit
    void set(uint64_t idx, bool value) {
        if (idx < size) [[likely]] {
            auto a = idx / big_num_threshold;
            auto b = idx % big_num_threshold;
            if (value) {
                values[a] |= 1ull << b;
            } else {
                values[a] &= ~(1ull << b);
            }
        }
    }

    [[nodiscard]] bool negative() const requires(signed_) { return this->operator[](size - 1); }

    template <uint64_t a, uint64_t b>
    requires(util::max(a, b) < size) big_num<util::abs_diff(a, b) + 1, false>
    inline slice() const {
        if constexpr (size <= util::max(a, b)) {
            // out of bound access
            return big_num<util::abs_diff(a, b) + 1>(0);
        }

        big_num<util::abs_diff(a, b) + 1, false> res;
        constexpr auto max = util::max(a, b);
        constexpr auto min = util::min(a, b);
        for (uint64_t i = min; i <= max; i++) {
            auto idx = i - min;
            res.set(idx, this->operator[](i));
        }
        return res;
    }

    template <uint64_t op_size>
    big_num<op_size, signed_> extend() const {
        if constexpr (op_size > size) {
            big_num<op_size, signed_> result;
            for (auto i = 0ul; i < s; i++) result.values[i] = values[i];
            // based on the sign, need to fill out leading ones
            if constexpr (signed_) {
                for (uint64_t i = size; i < op_size; i++) {
                    result.set(i, true);
                }
            }
            return result;
        } else {
            // just a copy
            return *this;
        }
    }

    // doesn't matter about the sign, but endianness must match
    template <uint64_t ss, bool num_signed>
    auto concat(const big_num<ss, num_signed> &num) const {
        auto constexpr final_size = size + ss;
        big_num<final_size, false> result;
        // copy over the num one since it's on the LSB
        for (auto i = 0u; i < num.s; i++) {
            result.values[i] = num.values[i];
        }
        // then copy over the current on
        for (auto i = ss; i < final_size; i++) {
            auto idx = i - ss;
            result.set(i, get(idx));
        }

        return result;
    }

    [[maybe_unused]] [[nodiscard]] uint64_t popcount() const {
        return std::accumulate(values.begin(), values.end(), std::popcount);
    }

    void mask_off() {
        // mask off bits that's excessive bits
        auto constexpr amount = s * 64 - size;
        auto constexpr mask = std::numeric_limits<uint64_t>::max() >> amount;
        values[s - 1] &= mask;
    }

    /*
     * boolean operators
     */
    explicit operator bool() const { return any_set(); }

    /*
     * bitwise operators
     */
    big_num<size, signed_> operator~() const {
        big_num<size, signed_> result;
        for (uint i = 0; i < s; i++) {
            result.values[i] = ~values[i];
        }
        // mask off the top bits, if any
        for (auto i = size; i < s * big_num_threshold; i++) {
            result.set(i, false);
        }
        return result;
    }

    // need to conform to LRM 11.6.1
    // During the evaluation of an expression,
    // interim results shall take the size of the largest operand (in case of an
    // assignment, this also includes the left-hand side).
    // Care has to be taken to prevent loss of a significant bit during expression evaluation.
    template <uint64_t op_size, bool op_signed>
    requires(op_size != size) auto operator&(const big_num<op_size, op_signed> &op) const {
        return this->template and_<util::max(size, op_size), op_size, op_signed>(op);
    }

    template <bool op_signed>
    big_num<size, util::signed_result(signed_, op_signed)> operator&(
        const big_num<size, op_signed> &op) const {
        big_num<size, util::signed_result(signed_, op_signed)> result;
        for (uint i = 0; i < s; i++) {
            result.values[i] = values[i] & op.values[i];
        }
        result.mask_off();
        return result;
    }

    template <uint64_t target_size, uint64_t op_size, bool op_signed>
    requires(target_size >=
             util::max(size, op_size)) auto and_(const big_num<op_size, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l & r;
        return result;
    }

    template <uint64_t op_size, bool op_signed>
    big_num<size, util::signed_result(signed_, op_signed)> &operator&=(
        const big_num<op_size, op_signed> &op) {
        auto v = (*this) & op;
        if constexpr (op_size > size) {
            // need to size it down
            v = v.template slice<size>();
        }
        this->values = v.values;
        mask_off();
        return *this;
    }

    template <uint64_t op_size, bool op_signed>
    requires(op_size != size) auto operator^(const big_num<op_size, op_signed> &op) const {
        return this->template xor_<util::max(size, op_size), op_size, op_signed>(op);
    }

    template <bool op_signed>
    big_num<size, util::signed_result(signed_, op_signed)> operator^(
        const big_num<size, op_signed> &op) const {
        big_num<size, util::signed_result(signed_, op_signed)> result;
        for (uint i = 0; i < s; i++) {
            result.values[i] = values[i] ^ op.values[i];
        }
        return result;
    }

    template <uint64_t target_size, uint64_t op_size, bool op_signed>
    requires(target_size >=
             util::max(size, op_size)) auto xor_(const big_num<op_size, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l ^ r;
        return result;
    }

    template <uint64_t op_size, bool op_signed>
    big_num<size, util::signed_result(signed_, op_signed)> &operator^=(
        const big_num<op_size, op_signed> &op) {
        auto v = (*this) ^ op;
        if constexpr (op_size > size) {
            // need to size it down
            v = v.template slice<size>();
        }
        this->values = v.values;
        mask_off();
        return *this;
    }

    template <uint64_t op_size, bool op_signed>
    auto operator|(const big_num<op_size, op_signed> &op) const {
        return this->template or_<util::max(size, op_size), op_size, op_signed>(op);
    }

    template <bool op_signed>
    big_num<size, util::signed_result(signed_, op_signed)> operator|(
        const big_num<size, op_signed> &op) const {
        big_num<size, util::signed_result(signed_, op_signed)> result;
        for (uint i = 0; i < s; i++) {
            result.values[i] = values[i] | op.values[i];
        }
        return result;
    }

    template <uint64_t target_size, uint64_t op_size, bool op_signed>
    requires(target_size >=
             util::max(size, op_size)) auto or_(const big_num<op_size, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l | r;
        return result;
    }

    template <uint64_t op_size, bool op_signed>
    big_num<size, util::signed_result(signed_, op_signed)> &operator|=(
        const big_num<op_size, op_signed> &op) const {
        auto v = (*this) | op;
        if constexpr (op_size > size) {
            // need to size it down
            v = v.template slice<size>();
        }
        this->values = v.values;
        mask_off();
        return *this;
    }

    // reduction
    [[maybe_unused]] [[nodiscard]] bool r_and() const {
        // only if all the bits are set
        auto b = std::all_of(values.begin(), values.begin() + s - 1, [](auto const v) {
            return v == std::numeric_limits<uint64_t>::max();
        });
        if constexpr (values.size() * 8 == size) {
            return b;
        } else {
            if (b) return false;

            for (uint64_t i = (values.size() - 1); i < s; i++) {
                if (!get(i)) return false;
            }
        }
        return true;
    }

    [[maybe_unused]] [[nodiscard]] bool r_xor() const {
        bool b = get<0>();
        for (auto i = 1u; i < size; i++) {
            b = b ^ get(i);
        }
        return b;
    }

    big_num<size, signed_> operator>>(uint64_t amount) const {
        // we will implement logic shifts regardless of the sign
        big_num<size, signed_> res;
        if (amount > size) [[unlikely]]
            return res;
        for (uint64_t i = amount; i < size; i++) {
            auto dst = i - amount;
            res.set(dst, get(i));
        }
        return res;
    }

    template <uint64_t op_size, bool op_signed>
    big_num<size, util::signed_result(signed_, op_signed)> operator>>(
        const big_num<op_size, op_signed> &amount) const {
        // we will implement logic shifts regardless of the sign
        // we utilize the property that, since the max size of the logic is 2^64,
        // if the big number amount actually uses more than 1 uint64 and high bits set,
        // the result has to be zero
        for (auto i = 1u; i < amount.s; i++) {
            if (amount.values[i]) return big_num<size, util::signed_result(signed_, op_signed)>{};
        }
        // now we reduce the problem to a normal shift amount problem
        return *(this) >> amount.values[0];
    }

    big_num<size, signed_> operator<<(uint64_t amount) const {
        // we will implement logic shifts regardless of the sign
        big_num<size, signed_> res;
        if (amount > size) [[unlikely]]
            return res;
        for (uint64_t i = 0u; i < (size - amount); i++) {
            auto dst = i + amount;
            res.set(dst, get(i));
        }
        return res;
    }

    template <uint64_t op_size, bool op_signed>
    big_num<size, util::signed_result(signed_, op_signed)> operator<<(
        const big_num<op_size, op_signed> &amount) const {
        // we will implement logic shifts regardless of the sign
        // we utilize the property that, since the max size of the logic is 2^64,
        // if the big number amount actually uses more than 1 uint64 and high bits set,
        // the result has to be zero
        for (auto i = 1u; i < amount.s; i++) {
            if (amount.values[i]) return big_num<size, signed_>{};
        }
        // now we reduce the problem to a normal shift amount problem
        return *(this) << amount.values[0];
    }

    [[nodiscard]] big_num<size, signed_> ashr(uint64_t amount) const {
        if constexpr (signed_) {
            // we will implement logic shifts regardless of the sign
            big_num<size, signed_> res;
            // notice that we need size - 1 since the top bit is always signed bit
            if (amount > (size - 1)) [[unlikely]] {
                if (negative()) [[unlikely]] {
                    res.mask();
                }
                return res;
            }
            // set high bits
            bool negative = this->negative();
            for (uint64_t i = 0; i < amount; i++) {
                auto dst = size - i - 1;
                res.set(dst, negative);
            }
            for (uint64_t i = amount; i < (size - 1); i++) {
                auto dst = i - amount;
                res.set(dst, get(i));
            }
            return res;
        } else {
            return (*this) >> amount;
        }
    }

    template <uint64_t op_size, bool op_signed>
    [[maybe_unused]] big_num<size, util::signed_result(signed_, op_signed)> ashr(
        const big_num<op_size, op_signed> &amount) const {
        // we will implement logic shifts regardless of the sign
        // we utilize the property that, since the max size of the logic is 2^64,
        // if the big number amount actually uses more than 1 uint64 and high bits set,
        // the result has to be zero
        big_num<size, util::signed_result(signed_, op_signed)> res;
        for (auto i = 1u; i < amount.s; i++) {
            if (amount.values[i]) {
                if constexpr (signed_) {
                    if (negative()) [[unlikely]] {
                        res.mask();
                    }
                }
                return res;
            }
        }
        // now we reduce the problem to a normal shift amount problem
        return this->ashr(amount.values[0]);
    }

    // arithmetic shift right is the same as logical shift right
    [[maybe_unused]] [[nodiscard]] big_num<size, signed_> ashl(uint64_t amount) const {
        return (*this) << amount;
    }

    template <uint64_t op_size, bool op_signed>
    [[maybe_unused]] big_num<size, util::signed_result(signed_, op_signed)> ashl(
        const big_num<op_size, op_signed> &amount) const {
        return (*this) << amount;
    }

    /*
     * comparators
     */
    template <uint64_t op_size, bool op_signed>
    bool operator==(const big_num<op_size, op_signed> &op) const {
        if constexpr (signed_ && op_signed) {
            // doing signed comparison
            if constexpr (op_size > size) {
                // extend itself
                auto t = this->template extend<op_size>();
                auto constexpr op_s = big_num<op_size, op_signed>::s;
                for (auto i = 0u; i < op_s; i++) {
                    if (t.values[i] != op.values[i]) return false;
                }
                return true;
            } else {
                auto op_t = op->template extend<size>();
                for (auto i = 0u; i < size; i++) {
                    if (values[i] != op_t.values[i]) return false;
                }
                return true;
            }
        }
        auto constexpr op_s = big_num<op_size, op_signed>::s;
        if constexpr (s >= op_s) {
            for (auto i = 0u; i < op_s; i++) {
                if (values[i] != op.values[i]) return false;
            }
            if constexpr (s == op_s) {
                return true;
            } else {
                // the rest has to be zero
                return std::reduce(values.begin() + op_s, values.end(), 0,
                                   [](auto a, auto b) { return a | b; }) == 0;
            }
        } else {
            for (auto i = 0u; i < s; i++) {
                if (values[i] != op.values[i]) return false;
            }
            // the rest has to be zero
            return std::reduce(op.values.begin() + s, op.values.end(), 0,
                               [](auto a, auto b) { return a | b; }) == 0;
        }
        return true;
    }

    template <typename T>
    requires(std::is_arithmetic_v<T>) bool operator==(T v) const {
        auto v_casted = static_cast<uint64_t>(v);
        if (v_casted != values[0]) return false;
        return std::all_of(values.begin() + 1, values.end(), [](auto c) { return c == 0; });
    }

    template <uint64_t op_size, bool op_signed>
    bool operator>(const big_num<op_size, op_signed> &op) const {
        if constexpr (signed_ && op_signed) {
            if (negative() && !op.negative()) {
                return false;
            } else if (!negative() && op.negative()) {
                return true;
            }
            // if it's both positive or both negative, we fall back to the normal comparison
            // per LRM, signed comparison is only used when both operands are signed
            // notice that if both of them are negative, we need to revert the result
            // since the smaller number appears to be bigger due to 2's complement
            else if (negative() && negative()) {
                return negate() < op.negate();
            }
        }
        // from top to bottom comparison
        auto top = util::max(s, big_num<op_size, op_signed>::s);
        for (auto i = 0u; i < top; i++) {
            auto idx = top - i - 1;
            if (idx >= s) {
                if (op.values[idx] != 0) return false;
            } else if (idx >= big_num<op_size, op_signed>::s) {
                if (values[idx] != 0) return true;
            } else {
                if (values[idx] != op.values[idx]) {
                    return values[idx] > op.values[idx];
                }
            }
        }
        // it means they are the same, return false
        return false;
    }

    template <typename T>
    requires(std::is_arithmetic_v<T>) bool operator>(T v) const {
        auto constexpr is_signed = std::is_signed_v<T>();
        auto op = big_num<sizeof(T) * 8, is_signed>();
        op.values[0] = static_cast<uint64_t>(v);
        return (*this) > op;
    }

    template <uint64_t op_size, bool op_signed>
    bool operator<(const big_num<op_size, op_signed> &op) const {
        return op > (*this);
    }

    template <typename T>
    requires(std::is_arithmetic_v<T>) bool operator<=(T v) const {
        auto constexpr is_signed = std::is_signed_v<T>();
        auto op = big_num<sizeof(T) * 8, is_signed>();
        op.values[0] = static_cast<uint64_t>(v);
        return (*this) <= op;
    }

    template <uint64_t op_size, bool op_signed>
    bool operator>=(const big_num<op_size, op_signed> &op) const {
        return (*this) > op || (*this) == op;
    }

    template <typename T>
    requires(std::is_arithmetic_v<T>) bool operator>=(T v) const {
        auto constexpr is_signed = std::is_signed_v<T>();
        auto op = big_num<sizeof(T) * 8, is_signed>();
        op.values[0] = static_cast<uint64_t>(v);
        return (*this) >= op;
    }

    template <uint64_t op_size, bool op_signed>
    bool operator<=(const big_num<op_size, op_signed> &op) const {
        return op >= (*this);
    }

    /*
     * arithmetic operators: + - * / %
     */

    template <uint64_t op_size, bool op_signed>
    requires(op_size != size) auto operator+(const big_num<op_size, op_signed> &op) const {
        return this->template add<util::max(size, op_size), op_size, op_signed>(op);
    }

    template <bool op_signed>
    big_num<size, util::signed_result(signed_, op_signed)> operator+(
        const big_num<size, op_signed> &op) const {
        big_num<size, util::signed_result(signed_, op_signed)> result;
        // based on the compiler explorer, using 128 bit is the "best" implementation
        // "best" as in the fewest instructions
        __uint128_t carry = 0;
        for (auto i = 0u; i < s; i++) {
            auto v = carry + values[i] + op.values[i];
            result.values[i] = (v << 64) >> 64;
            carry = v >> 64;
        }
        return result;
    }

    template <uint64_t target_size, uint64_t op_size, bool op_signed>
    requires(target_size >=
             util::max(size, op_size)) auto add(const big_num<op_size, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l + r;
        return result;
    }

    template <uint64_t op_size, bool op_signed>
    requires(op_size != size) auto operator-(const big_num<op_size, op_signed> &op) const {
        return this->template minus<util::max(size, op_size), op_size, op_signed>(op);
    }

    template <bool op_signed>
    big_num<size, util::signed_result(signed_, op_signed)> operator-(
        const big_num<size, op_signed> &op) const {
        auto neg = op.negate();
        return (*this) + neg;
    }

    template <uint64_t target_size, uint64_t op_size, bool op_signed>
    requires(target_size >=
             util::max(size, op_size)) auto minus(const big_num<op_size, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l - r;
        return result;
    }

    template <uint64_t op_size, bool op_signed>
    requires(op_size != size) auto operator*(const big_num<op_size, op_signed> &op) const {
        return this->template multiply<util::max(size, op_size), op_size, op_signed>(op);
    }

    template <bool op_signed>
    big_num<size, util::signed_result(signed_, op_signed)> operator*(
        const big_num<size, op_signed> &op) const {
        big_num<size, util::signed_result(signed_, op_signed)> result;
        // if it fits in uint64 then we're gold
        if (fit_in_64() && op.fit_in_64()) {
            __uint128_t a = values[0];
            __uint128_t b = op.values[0];
            __uint128_t c = a * b;
            result.values[0] = (c << 64) >> 64;
            result.values[1] = c >> 64;
            return result;
        }
        // we're not using Karatsuba's algorithm since it can get infinitely recursion or underflow
        // easily given our current setup
        // using text book version of multiply, which is O(n^2)
        // we use __uint128 for speed up
        big_num<size, false> temp;
        for (uint64_t i = 0; i < s; i++) {
            if (values[i] == 0) continue;
            for (uint j = 0; j < s; j++) {
                // deal with special cases
                if (op.values[j] == 0) continue;
                __uint128_t a = values[i];
                __uint128_t b = op.values[j];
                __uint128_t c = a * b;
                uint64_t hi = c >> 64u;
                uint64_t lo = (c << 64u) >> 64;

                if ((i + j) < size) {
                    temp.values[i + j] = lo;
                    result = result + temp;
                    temp.values[i + j] = 0;
                }
                if ((i + j + 1) < size) {
                    temp.values[i + j + 1] = hi;
                    result = result + temp;
                    temp.values[i + j + 1] = 0;
                }
            }
        }
        auto res = result;
        return res;
    }

    template <uint64_t target_size, uint64_t op_size, bool op_signed>
    requires(target_size >=
             util::max(size, op_size)) auto multiply(const big_num<op_size, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l * r;
        return result;
    }

    template <uint64_t op_size, bool op_signed>
    requires(op_size != size) auto operator/(const big_num<op_size, op_signed> &op) const {
        return this->template divide<util::max(size, op_size), op_size, op_signed>(op);
    }

    template <bool op_signed>
    big_num<size, util::signed_result(signed_, op_signed)> operator/(
        const big_num<size, op_signed> &op) const {
        return div_mod(op).first;
    }

    template <uint64_t target_size, uint64_t op_size, bool op_signed>
    requires(target_size >=
             util::max(size, op_size)) auto divide(const big_num<op_size, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l / r;
        return result;
    }

    template <uint64_t op_size, bool op_signed>
    requires(op_size != size) auto operator%(const big_num<op_size, op_signed> &op) const {
        return this->template mod<util::max(size, op_size), op_size, op_signed>(op);
    }

    template <bool op_signed>
    big_num<size, util::signed_result(signed_, op_signed)> operator%(
        const big_num<size, op_signed> &op) const {
        return div_mod(op).second;
    }

    template <uint64_t target_size, uint64_t op_size, bool op_signed>
    requires(target_size >=
             util::max(size, op_size)) auto mod(const big_num<op_size, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l % r;
        return result;
    }

    [[nodiscard]] big_num<size, signed_> negate() const {
        // 2's complement
        big_num<size, signed_> result = ~(*this);
        constexpr static auto v = big_num<size>(1ul);
        result = result + v;
        return result;
    }

    [[nodiscard]] big_num<size, true> to_signed() const {
        big_num<size, true> res;
        res.values = values;
        return res;
    }

    constexpr big_num<size, signed_> operator-() const { return negate(); }
    constexpr big_num<size, signed_> operator+() const { return *this; }

    /*
     * mask related stuff
     */
    [[nodiscard]] bool any_set() const {
        // we use the fact that SIMD instructions are faster than compare and branch
        // so summation is faster than any_of in theory
        // also all unused bits are set to 0 by default
        auto r = std::reduce(values.begin(), values.end(), 0, [](auto a, auto b) { return a | b; });
        return r != 0;
    }

    [[maybe_unused]] void mask() {
        for (auto i = 0u; i < (s - 1); i++) {
            values[i] = std::numeric_limits<big_num_holder_type>::max();
        }
        // last bits
        values[s - 1] =
            std::numeric_limits<big_num_holder_type>::max() >> (s * big_num_threshold - size);
    }

    [[maybe_unused]] void unmask() { std::fill(values.begin(), values.end(), 0); }

    /*
     * helper functions
     */

    [[nodiscard]] bool is_one() const {
        if constexpr (s == 1) {
            return values[0] == 1u;
        } else {
            return values[0] == 1u && (std::reduce(values.begin() + 1, values.end(), 0,
                                                   [](auto a, auto b) { return a | b; }) == 0u);
        }
    }

    [[nodiscard]] bool fit_in_64() const {
        return std::reduce(values.begin() + 1, values.end(), 0,
                           [](auto a, auto b) { return a | b; }) == 0;
    }

    template <bool op_signed>
    std::pair<big_num<size, util::signed_result(signed_, op_signed)>,
              big_num<size, util::signed_result(signed_, op_signed)>>
    div_mod(const big_num<size, op_signed> &op) const {
        // we only do unsigned division
        constexpr bool result_signed = util::signed_result(signed_, op_signed);
        // notice that if we divide by zero, we return 0, per convention,
        // since we are not allowed to throw exception in the simulation
        big_num<size, result_signed> q;
        big_num<size, result_signed> r;
        if (!op.any_set()) {
            return {q, r};
        }
        if constexpr (result_signed) {
            // this implies that both of them are signed numbers
            auto op_pos = op.negative() ? op.negate() : op;
            auto this_pos = negative() ? negate() : *this;
            std::tie(q, r) = this_pos.div_mode_(op);
            // if both of them are negative or positive, the sign will cancel out
            if (op.negative() ^ negative()) {
                q = q.negate();
                r = r.negate();
            }
        } else {
            std::tie(q, r) = div_mod_unsigned(op);
        }
        return {q, r};
    }

    std::pair<big_num<size, false>, big_num<size, false>> div_mod_unsigned(
        const big_num<size, false> &op) const requires(!signed_) {
        // deal with some special cases
        if (op.is_one()) {
            return std::make_pair(*this, big_num<size, false>{0u});
        } else if ((*this) == op) {
            return std::make_pair(big_num<size, false>{1u}, big_num<size, false>{0u});
        } else if ((*this) < op) {
            return std::make_pair(big_num<size, false>{0u}, *this);
        } else if (fit_in_64() && op.fit_in_64()) {
            auto q = values[0] / op.values[0];
            auto r = values[0] % op.values[0];
            return std::make_pair(big_num<size, false>{q}, big_num<size, false>{r});
        }

        // code to figure out the set bit
        auto const this_hi_bit = highest_bit();
        auto const op_hi_bit = op.highest_bit();
        auto const diff_bits = this_hi_bit - op_hi_bit;

        big_num<size, false> q;
        big_num<size, false> r = *this;
        for (auto i = 0u; i <= diff_bits; i++) {
            // shift r to the correct bits, could be too big
            auto t = op << (diff_bits - i);
            if (r >= t) {
                r = r - t;
                q.set(diff_bits - i, true);
            }
        }

        return std::make_pair(q, r);
    }

    [[nodiscard]] uint64_t highest_bit() const {
        for (auto i = 0u; i < s; i++) {
            auto const v = values[s - i - 1];
            if (v != 0) {
                auto r = __builtin_clzll(v);
                return (s - i) * 64 - r - 1;
            }
        }
        // this is undefined behavior
        return s;
    }

    // constructors
    constexpr explicit big_num(std::string_view v) {
        std::fill(values.begin(), values.end(), 0);
        auto iter = v.rbegin();
        for (auto i = 0u; i < size; i++) {
            // from right to left
            auto &value = values[i / big_num_threshold];
            auto c = *iter;
            if (c == '1') {
                value |= 1ull << (i % big_num_threshold);
            }
            iter++;
            if (iter == v.rend()) break;
        }
    }

    template <typename T>
    requires std::is_arithmetic_v<T>
    explicit constexpr big_num(T v) : values({v}) {
        if constexpr (signed_) {
            if (v < 0) {
                for (auto i = 1u; i < s; i++) {
                    values[i] = std::numeric_limits<big_num_holder_type>::max();
                }
                mask_off();
            }
        }
    }

    // set values to 0 when initialized
    constexpr big_num() { std::fill(values.begin(), values.end(), 0); }

    // implicit conversion between signed and unsigned
    template <uint64_t op_size, bool op_signed>
    requires(op_size == size && op_signed != signed_)
        [[maybe_unused]] big_num(const big_num<op_size, op_signed> &op)  // NOLINT
        : values(op.values) {}

private:
    [[nodiscard]] bool get(uint64_t idx) const { return operator[](idx); }
};

template <int msb = 0, int lsb = 0, bool signed_ = false>
struct logic;

template <int msb = 0, int lsb = 0, bool signed_ = false>
struct bits {
private:
public:
    constexpr static auto size = util::abs_diff(msb, lsb) + 1;
    constexpr static auto big_endian = msb > lsb;
    static_assert(size > 0, "0 sized logic not allowed");
    static constexpr bool native_num = util::native_num(size);
    using T = typename util::get_holder_type<size, signed_>::type;

    T value;

    // basic formatting
    [[nodiscard]] std::string str() const {
        std::stringstream ss;
        for (auto i = 0; i < size; i++) {
            if (operator[](i)) {
                ss << '1';
            } else {
                ss << '0';
            }
        }
        return ss.str();
    }

    // single bit
    bool inline operator[](uint64_t idx) const {
        if constexpr (native_num) {
            return (value >> idx) & 1;
        } else {
            if constexpr (!big_endian) {
                idx = lsb - idx;
            }
            return value[idx];
        }
    }

    template <uint64_t idx>
    requires(idx < size && native_num) [[nodiscard]] bool inline get() const {
        return this->operator[](idx);
    }

    void inline set(uint64_t idx, bool v) {
        if constexpr (!big_endian) {
            idx = lsb - idx;
        }
        if constexpr (native_num) {
            if (v) {
                value |= 1ull << idx;
            } else {
                value &= ~(1ull << idx);
            }
        } else {
            value.set(idx, v);
        }
    }

    template <uint64_t idx>
    void set(bool v) {
        constexpr auto i = !big_endian ? lsb - idx : idx;
        if constexpr (native_num) {
            if (v) {
                value |= 1ull << i;
            } else {
                value &= ~(1ull << i);
            }
        } else {
            value.template set<i>(v);
        }
    }

    template <uint64_t idx, bool v>
    void set() {
        constexpr auto i = !big_endian ? lsb - idx : idx;
        if constexpr (big_endian) {
            if constexpr (v) {
                value |= 1ull << i;
            } else {
                value &= ~(1ull << i);
            }
        } else {
            value.template set<i, v>();
        }
    }

    /*
     * relates to whether it's negative or not
     */
    [[nodiscard]] bool negative() const requires(signed_) {
        if constexpr (native_num) {
            return value < 0;
        } else {
            return value.negative();
        }
    }

    /*
     * Slice always produce unsigned result
     */
    /*
     * native holder always produce native numbers
     */
    template <int a, int b>
    requires(util::max(a, b) < size && native_num && b >= lsb) bits<util::abs_diff(a, b)>
    constexpr inline slice() const {
        // assume the import has type-checked properly, e.g. by a compiler
        constexpr auto base = util::min(msb, lsb);
        constexpr auto max = util::max(a, b) - base;
        constexpr auto min = util::min(a, b) - base;
        constexpr auto t_size = sizeof(T) * 8;
        bits<util::abs_diff(a, b), false> result;
        constexpr auto default_mask = std::numeric_limits<T>::max();
        uint64_t mask = default_mask << min;
        mask &= (default_mask >> (t_size - max - 1));
        result.value = (value & mask) >> min;

        return result;
    }

    /*
     * big number but small slice
     */
    template <int a, int b>
    requires(util::max(a, b) < size && !native_num && util::native_num(util::abs_diff(a, b)) &&
             b >= lsb) constexpr bits<util::abs_diff(a, b)> inline slice() const {
        bits<util::abs_diff(a, b), false> result;
        auto res = value.template slice<a, b>();
        // need to shrink it down
        result.value = res.values[0];
        return result;
    }

    /*
     * big number and big slice
     */
    template <int a, int b>
    requires(util::max(a, b) < size && !native_num && !util::native_num(util::abs_diff(a, b)) &&
             b >= lsb) constexpr bits<util::abs_diff(a, b)> inline slice() const {
        bits<util::abs_diff(a, b), false> result;
        result.value = value.template slice<a, b>();
        return result;
    }

    // extension
    // notice that we need to implement signed extension, for the result that's native holder
    // C++ will handle that for us already.
    template <uint64_t target_size>
    requires(target_size > size && util::native_num(target_size))
        [[nodiscard]] constexpr bits<target_size - 1, 0, signed_> extend() const {
        return bits<target_size - 1, 0, signed_>(value);
    }

    template <uint64_t target_size>
    requires(target_size == size)
        [[nodiscard]] constexpr bits<target_size - 1, 0, signed_> extend() const {
        return *this;
    }

    // new size is smaller. this is just a slice
    template <uint64_t target_size>
    requires(target_size < size)
        [[nodiscard]] constexpr bits<target_size - 1, 0, signed_> extend() const {
        return slice<target_size - 1, 0>();
    }

    // native number, but extend to a big number
    template <uint64_t target_size>
    requires(target_size > size && !util::native_num(target_size) && native_num)
        [[nodiscard]] constexpr bits<target_size - 1, 0, signed_> extend() const {
        bits<target_size - 1, 0, signed_> result;
        // use C++ default semantics to cast. if it's a negative number
        // the upper bits will be set properly
        result.value.values[0] = static_cast<big_num_holder_type>(value);
        if constexpr (signed_) {
            if (negative()) {
                for (auto i = 1u; i < result.value.s; i++) {
                    // sign extension
                    result.value.values[i] = std::numeric_limits<big_num_holder_type>::max();
                }
                result.mask_off();
            }
        }
        // if not we're fine since we set everything to zero when we start
        return result;
    }

    // big number and gets extended to a big number
    template <uint64_t target_size>
    requires(target_size > size && !util::native_num(target_size) && !native_num)
        [[nodiscard]] constexpr bits<target_size - 1, 0, signed_> extend() const {
        bits<target_size - 1, 0, signed_> res;
        res.value = value.template extend<target_size>();
        return res;
    }

    /*
     * concatenation
     */
    template <int arg0_msb, int arg0_lsb, bool arg0_signed>
    auto concat(const bits<arg0_msb, arg0_lsb, arg0_signed> &arg0) {
        auto constexpr arg0_size = bits<arg0_msb, arg0_lsb>::size;
        auto constexpr final_size = size + arg0_size;
        if constexpr (final_size < big_num_threshold) {
            return bits<final_size - 1>(value << arg0_size | arg0.value);
        } else {
            auto res = bits<final_size - 1>();
            if constexpr (bits<arg0_msb, arg0_lsb>::native_num) {
                // convert to big num
                big_num<arg0_size, false> b(arg0.value);
                auto big_num = value.concat(b);
                res.big_num = big_num;
            } else {
                auto big_num = value.concat(arg0.value);
                res.big_num = big_num;
            }
            return res;
        }
    }

    template <typename U, typename... Ts>
    auto concat(U arg0, Ts... args) {
        return concat(arg0).concat(args...);
    }

    /*
     * bits unpacking
     */
    template <typename... Ts>
    [[maybe_unused]] auto unpack(Ts &...args) const {
        if constexpr (big_endian) {
            auto tuples = std::forward_as_tuple(args...);
            auto reversed = util::reverse_tuple(tuples);
            std::apply([this](auto &&...args) { this->template unpack_(args...); }, reversed);
        } else {
            return this->template unpack_(args...);
        }
    }

    /*
     * boolean operators
     */
    explicit operator bool() const { return any_set(); }
    bool operator!() const { return !any_set(); }

    [[nodiscard]] uint64_t popcount() const {
        if constexpr (native_num) {
            return std::popcount(value);
        } else {
            return value.popcount();
        }
    }

    void mask_off() {
        if constexpr (native_num) {
            if constexpr (size > 1) {
                auto constexpr amount = sizeof(T) * 8 - size;
                auto constexpr mask_max = std::numeric_limits<T>::max();
                auto mask = mask_max >> amount;
                value &= mask;
            }
        } else {
            value.mask_off();
        }
    }

    /*
     * bitwise operators
     */
    bits<size - 1, 0, false> operator~() const {
        bits<size - 1, 0, false> result;
        if constexpr (size == 1) {
            result.value = !value;
        } else {
            result.value = ~value;
        }
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    requires((util::abs_diff(op_msb, op_lsb) + 1) != size) auto operator&(
        const bits<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, bits<op_msb, op_lsb>::size);
        return this->template and_<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator&(const bits<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        bits<size - 1, 0, util::signed_result(signed_, op_signed)> result;
        result.value = value & op.value;
        return result;
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, bits<op_msb, op_lsb>::size)) auto and_(
        const bits<op_msb, op_lsb, op_signed> &op) const {
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l & r;
        result.mask_off();
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    bits<size - 1, 0, util::signed_result(signed_, op_signed)> &operator&=(
        const bits<op_msb, op_lsb, op_signed> &op) {
        auto res = (*this) & op;
        value = res.value;
        mask_off();
        return *this;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    requires((util::abs_diff(op_msb, op_lsb) + 1) != size) auto operator^(
        const bits<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, bits<op_msb, op_lsb>::size);
        return this->template xor_<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator^(const bits<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        bits<size - 1, 0, util::signed_result(signed_, op_signed)> result;
        result.value = value ^ op.value;
        return result;
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, bits<op_msb, op_lsb>::size)) auto xor_(
        const bits<op_msb, op_lsb, op_signed> &op) const {
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l ^ r;
        result.mask_off();
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    bits<size - 1, 0, util::signed_result(signed_, op_signed)> &operator^=(
        const bits<op_msb, op_lsb, op_signed> &op) {
        auto res = (*this) ^ op;
        value = res.value;
        mask_off();
        return *this;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    requires((util::abs_diff(op_msb, op_lsb) + 1) != size) auto operator|(
        const bits<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, bits<op_msb, op_lsb>::size);
        return this->template or_<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator|(const bits<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        bits<size - 1, 0, util::signed_result(signed_, op_signed)> result;
        result.value = value | op.value;
        return result;
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, bits<op_msb, op_lsb>::size)) auto or_(
        const bits<op_msb, op_lsb, op_signed> &op) const {
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l | r;
        result.mask_off();
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    bits<size - 1, 0, util::signed_result(signed_, op_signed)> &operator|=(
        const bits<op_msb, op_lsb, op_signed> &op) {
        auto res = (*this) | op;
        value = res.value;
        mask_off();
        return *this;
    }

    // reduction
    [[nodiscard]] bool r_and() const requires(native_num) {
        auto constexpr max = std::numeric_limits<T>::max() >> (sizeof(T) * 8 - size);
        return value == max;
    }

    [[nodiscard]] bool r_and() const requires(!native_num) { return value.r_xor(); }

    [[maybe_unused]] [[nodiscard]] bool r_nand() const { return !r_and(); }

    [[nodiscard]] bool r_or() const { return any_set(); }

    [[maybe_unused]] [[nodiscard]] bool r_nor() const { return !r_or(); }

    [[nodiscard]] bool r_xor() const requires(native_num) {
        bool b = get<0>();
        for (auto i = 1; i < size; i++) {
            b = b ^ get(i);
        }
        return b;
    }

    [[nodiscard]] bool r_xor() const requires(!native_num) { return value.r_xor(); }

    [[maybe_unused]] [[nodiscard]] bool r_xnor() const { return !r_xor(); }

    template <int op_msb, int op_lsb, bool op_signed>
    auto operator>>(const bits<op_msb, op_lsb, op_signed> &amount) const {
        bits<size - 1, 0, util::signed_result(signed_, op_signed)> res;
        // couple cases
        // 1. both of them are native number
        if constexpr (native_num && bits<op_msb, op_lsb>::native_num) {
            // make sure to mask off any bits dur to signed extension
            res.value = (value >> static_cast<T>(amount.value)) & value_mask(size - amount.value);
        } else if constexpr ((!native_num)) {
            res.value = value >> amount.value;
        } else {
            // itself is a native number but amount is not
            // convert to itself as a big number
            big_num<size, signed_> big_num;
            big_num.values[0] = static_cast<uint64_t>(value);
            auto res_num = big_num >> amount;
            res.value = static_cast<T>(res_num.values[0]);
        }

        return res;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    bits<size - 1, 0, util::signed_result(signed_, op_signed)> &operator>>=(
        const bits<op_msb, op_lsb, op_signed> &amount) {
        auto res = (*this) >> amount;
        value = res.value;
        return *this;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    auto operator<<(const bits<op_msb, op_lsb, op_signed> &amount) const {
        bits<size - 1, 0, util::signed_result(signed_, op_signed)> res;
        // couple cases
        // 1. both of them are native number
        // clang doesn't allow accessing native_num as amount.native_num
        // see https://stackoverflow.com/a/44996066
        if constexpr (native_num && bits<op_msb, op_lsb>::native_num) {
            res.value = value << static_cast<T>(amount.value);
        } else if constexpr ((!native_num)) {
            res.value = value << amount.value;
        } else {
            // itself is a native number but amount is not
            // convert to itself as a big number
            big_num<size> big_num;
            big_num.values[0] = static_cast<uint64_t>(value);
            auto res_num = big_num << amount;
            res.value = static_cast<T>(res_num.values[0]);
        }

        return res;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    bits<size - 1, 0, util::signed_result(signed_, op_signed)> &operator<<=(
        const bits<op_msb, op_lsb, op_signed> &amount) {
        auto res = (*this) << amount;
        value = res.value;
        return *this;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    auto ashr(const bits<op_msb, op_lsb, op_signed> &amount) const {
        bits<size - 1, 0, util::signed_result(signed_, op_signed)> res;
        // couple cases
        // 1. both of them are native number
        if constexpr (native_num && bits<op_msb, op_lsb>::native_num) {
            // no need to mask since C++ does arithmetic shifts by default
            res.value = value >> static_cast<T>(amount.value);
        } else if constexpr ((!native_num)) {
            res.value = value.ashr(amount.value);
        } else {
            // itself is a native number but amount is not
            // convert to itself as a big number
            big_num<size> big_num;
            big_num.values[0] = static_cast<uint64_t>(value);
            auto res_num = big_num.ashr(amount);
            res.value = static_cast<T>(res_num.values[0]);
        }

        return res;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    auto ashl(const bits<op_msb, op_lsb, op_signed> &amount) const {
        // this is the same as bitwise shift left
        return (*this) << amount;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    bool operator==(const bits<op_msb, op_lsb, op_signed> &v) const {
        if constexpr (native_num && bits<op_msb, op_lsb>::native_num) {
            return value == static_cast<T>(v.value);
        } else {
            return value == v.value;
        }
    }

    template <int op_msb, int op_lsb, bool op_signed>
    bool operator>(const bits<op_msb, op_lsb, op_signed> &v) const {
        if constexpr (native_num && bits<op_msb, op_lsb>::native_num) {
            // LRM specifies that on ly if both operands are signed we do signed comparison
            if constexpr (signed_ && op_signed) {
                return value > v.value;
            } else {
                return static_cast<uint64_t>(value) > static_cast<uint64_t>(v.value);
            }
        } else if constexpr (!native_num) {
            return value > v.value;
        }
    }

    template <int op_msb, int op_lsb, bool op_signed>
    bool operator<(const bits<op_msb, op_lsb, op_signed> &v) const {
        if constexpr (native_num && bits<op_msb, op_lsb>::native_num) {
            // LRM specifies that on ly if both operands are signed we do signed comparison
            if constexpr (signed_ && op_signed) {
                return value < v.value;
            } else {
                return static_cast<uint64_t>(value) < static_cast<uint64_t>(v.value);
            }
        } else if constexpr (!native_num) {
            return value < v.value;
        }
    }

    template <int op_msb, int op_lsb, bool op_signed>
    bool operator>=(const bits<op_msb, op_lsb, op_signed> &v) const {
        if constexpr (native_num && bits<op_msb, op_lsb>::native_num) {
            // LRM specifies that on ly if both operands are signed we do signed comparison
            if constexpr (signed_ && op_signed) {
                return value >= v.value;
            } else {
                return static_cast<uint64_t>(value) >= static_cast<uint64_t>(v.value);
            }
        } else if constexpr (!native_num) {
            return value >= v.value;
        }
    }

    template <int op_msb, int op_lsb, bool op_signed>
    bool operator<=(const bits<op_msb, op_lsb, op_signed> &v) const {
        if constexpr (native_num && bits<op_msb, op_lsb>::native_num) {
            // LRM specifies that on ly if both operands are signed we do signed comparison
            if constexpr (signed_ && op_signed) {
                return value <= v.value;
            } else {
                return static_cast<uint64_t>(value) <= static_cast<uint64_t>(v.value);
            }
        } else if constexpr (!native_num) {
            return value <= v.value;
        }
    }

    constexpr bits<msb, lsb, signed_> operator-() const {
        if constexpr (native_num) {
            return bits<msb, lsb, signed_>(-value);
        } else {
            return bits<msb, lsb, signed_>(value.negate());
        }
    }

    constexpr auto operator+() const { return *this; }

    /*
     * arithmetic operators: + - * / %
     */

    template <int op_msb, int op_lsb, bool op_signed>
    requires(bits<op_msb, op_lsb>::size != size) auto operator+(
        const bits<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, bits<op_msb, op_lsb>::size);
        return this->template add<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator+(const bits<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        bits<size - 1, 0, util::signed_result(signed_, op_signed)> result;
        result.value = value + op.value;
        return result;
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, bits<op_msb, op_lsb>::size)) auto add(
        const bits<op_msb, op_lsb, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l + r;
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    requires(bits<op_msb, op_lsb>::size != size) auto operator-(
        const bits<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, bits<op_msb, op_lsb>::size);
        return this->template minus<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator-(const bits<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        bits<size - 1, 0, util::signed_result(signed_, op_signed)> result;
        result.value = value - op.value;
        return result;
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, bits<op_msb, op_lsb>::size)) auto minus(
        const bits<op_msb, op_lsb, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l - r;
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    requires(bits<op_msb, op_lsb>::size != size) auto operator*(
        const bits<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, bits<op_msb, op_lsb>::size);
        return this->template multiply<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator*(const bits<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        bits<size - 1, 0, util::signed_result(signed_, op_signed)> result;
        result.value = value * op.value;
        return result;
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, bits<op_msb, op_lsb>::size)) auto multiply(
        const bits<op_msb, op_lsb, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l * r;
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    requires(bits<op_msb, op_lsb>::size != size) auto operator%(
        const bits<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, bits<op_msb, op_lsb>::size);
        return this->template mod<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator%(const bits<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        bits<size - 1, 0, util::signed_result(signed_, op_signed)> result;
        result.value = value % op.value;
        return result;
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, bits<op_msb, op_lsb>::size)) auto mod(
        const bits<op_msb, op_lsb, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l % r;
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    requires(bits<op_msb, op_lsb>::size != size) auto operator/(
        const bits<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, bits<op_msb, op_lsb>::size);
        return this->template divide<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator/(const bits<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        bits<size - 1, 0, util::signed_result(signed_, op_signed)> result;
        result.value = value / op.value;
        return result;
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, bits<op_msb, op_lsb>::size)) auto divide(
        const bits<op_msb, op_lsb, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l / r;
        return result;
    }

    [[nodiscard]] bits<size - 1, 0, true> to_signed() const {
        bits<size - 1, 0, true> res;
        if constexpr (native_num) {
            res.value = static_cast<decltype(bits<size - 1, 0, true>::value)>(value);
        } else {
            res.value = value.to_signed();
        }
        return res;
    }

    /*
     * mask related stuff
     */
    [[nodiscard]] bool any_set() const requires(native_num) {
        // unused bits are always set to 0
        return value != 0;
    }

    [[nodiscard]] bool any_set() const requires(!native_num) { return value.any_set(); }

    void mask() {
        if constexpr (native_num) {
            value = std::numeric_limits<T>::max() >> (sizeof(T) * 8 - size);
        } else {
            value.mask();
        }
    }

    /*
     * constructors
     */
    explicit bits(std::string_view v) {
        if constexpr (size <= big_num_threshold) {
            // normal number
            value = 0;
            auto iter = v.rbegin();
            for (auto i = 0u; i < size; i++) {
                // from right to left
                auto c = *iter;
                if (c == '1') {
                    value |= 1ull << i;
                }
                iter++;
                if (iter == v.rend()) break;
            }
        } else {
            // big num
            value = big_num<size, signed_>(v);
        }
    }

    explicit constexpr bits(T v) requires(util::native_num(size)) : value(v) {}
    template <typename K>
    requires(std::is_arithmetic_v<K> && !util::native_num(size)) explicit constexpr bits(K v)
        : value(v) {}

    template <uint64_t new_size, bool new_signed>
    explicit bits(const big_num<new_size, new_signed> &big_num) requires(size <= big_num_threshold)
        : value(big_num.values[0]) {}

    bits() {
        if constexpr (native_num) {
            // init to 0
            value = 0;
        }
        // for big number it's already set to 0
    }

    // implicit conversion via assignment
    template <int new_msb, int new_lsb, bool new_signed>
    bits<msb, lsb, signed_> &operator=(const bits<new_msb, new_lsb, new_signed> &b) {
        value = b.value;
        return *this;
    }

private:
    /*
     * unpacking, which is basically slicing as syntax sugars
     */
    template <int base, int arg0_msb, int arg0_lsb, bool arg0_signed>
    requires(base < size) void unpack_(bits<arg0_msb, arg0_lsb, arg0_signed> &arg0) const {
        auto constexpr arg0_size = bits<arg0_msb, arg0_lsb, arg0_signed>::size;
        auto constexpr upper_bound = util::min(size - 1, arg0_size + base - 1);
        arg0.value = this->template slice<base, upper_bound>();
    }

    template <int base = 0, int arg0_msb, int arg0_lsb, bool arg0_signed, typename... Ts>
    [[maybe_unused]] auto unpack_(bits<arg0_msb, arg0_lsb, arg0_signed> &arg0, Ts &...args) const {
        auto constexpr arg0_size = bits<arg0_msb, arg0_lsb, arg0_signed>::size;
        this->template unpack_<base, arg0_msb, arg0_lsb, arg0_signed>(arg0);
        this->template unpack_<base + arg0_size>(args...);
    }

    [[nodiscard]] T value_mask(uint64_t requested_size) const {
        auto mask = std::numeric_limits<uint64_t>::max();
        mask = mask >> (64ull - requested_size);
        return static_cast<T>(mask);
    }
};

template <int msb, int lsb, bool signed_>
struct logic {
    static auto constexpr size = util::abs_diff(msb, lsb) + 1;
    constexpr static auto big_endian = msb > lsb;
    bits<msb, lsb, signed_> value;
    // To reduce memory footprint, we use the following encoding scheme
    // if xz_mask is off, value is the actual integer value
    // if xz_mask is on, 0 in value means x and 1 means z
    bits<msb, lsb> xz_mask;

    // make sure that if we use native number, it's an arithmetic type
    static_assert((size <= 64 && std::is_arithmetic_v<typename bits<msb, lsb, signed_>::T>) ||
                      (size > 64),
                  "Native number holder");

    // basic formatting
    [[nodiscard]] std::string str() const {
        std::stringstream ss;
        for (auto i = 0u; i < size; i++) {
            uint64_t idx = size - i - 1;
            if (x_set(idx)) {
                ss << 'x';
            } else if (z_set(idx)) {
                ss << 'z';
            } else {
                if (value[idx]) {
                    ss << '1';
                } else {
                    ss << '0';
                }
            }
        }
        return ss.str();
    }

    /*
     * single bit
     */
    inline logic operator[](uint64_t idx) const {
        if (idx < size) [[likely]] {
            logic r;
            if (x_set(idx)) [[unlikely]] {
                r.set_x(idx);
            } else if (z_set(idx)) [[unlikely]] {
                r.set_z(idx);
            } else {
                r.value = value[idx];
            }
            return r;
        } else {
            return logic(false);
        }
    }

    template <uint64_t idx>
    requires(idx < size) [[nodiscard]] inline logic get() const {
        logic r;
        if (this->template x_set<idx>()) [[unlikely]] {
            r.set_x<idx>();
        } else if (this->template z_set<idx>()) [[unlikely]] {
            r.set_z<idx>();
        } else {
            r.value = value.template get<idx>();
        }
        return r;
    }

    void set(uint64_t idx, bool v) {
        value.set(idx, v);
        // unmask bit
        unmask_bit(idx);
    }

    template <uint64_t idx>
    void set(bool v) {
        value.template set<idx>(v);
        this->template unmask_bit<idx>();
    }

    template <uint64_t idx, bool v>
    void set() {
        value.template set<idx, v>();
        this->template unmask_bit<idx>();
    }

    void set(uint64_t idx, const logic &l) {
        value.set(idx, l.value[idx]);
        xz_mask.set(idx, l.xz_mask[idx]);
    }

    [[nodiscard]] inline bool x_set(uint64_t idx) const { return xz_mask[idx] && !value[idx]; }

    template <uint64_t idx>
    [[nodiscard]] inline bool x_set() const {
        return xz_mask.template get<idx>() && !value.template get<idx>();
    }

    [[nodiscard]] inline bool z_set(uint64_t idx) const { return xz_mask[idx] && value[idx]; }

    template <uint64_t idx>
    [[nodiscard]] inline bool z_set() const {
        return xz_mask.template get<idx>() && value.template get<idx>();
    }

    inline void set_x(uint64_t idx) {
        xz_mask.set(idx, true);
        value.set(idx, false);
    }

    template <uint64_t idx>
    inline void set_x() {
        xz_mask.template set<idx, true>();
        value.template set<idx, false>();
    }

    inline void set_z(uint64_t idx) {
        xz_mask.set(idx, true);
        value.set(idx, true);
    }

    template <uint64_t idx>
    inline void set_z() {
        xz_mask.template set<idx, true>();
        value.template set<idx, true>();
    }

    /*
     * slices
     */
    template <int a, int b>
    requires(util::max(a, b) < size) constexpr logic<util::abs_diff(a, b)> inline slice() const {
        if constexpr (size <= util::max(a, b)) {
            // out of bound access will be X
            return logic<util::abs_diff(a, b)>{};
        }

        logic<util::abs_diff(a, b), 0, false> result;
        result.value = value.template slice<a, b>();
        // copy over masks
        result.xz_mask = xz_mask.template slice<a, b>();

        return result;
    }

    /*
     * extend
     */
    template <uint64_t target_size>
    requires(target_size >= size)
        [[nodiscard]] constexpr logic<target_size - 1, 0, signed_> extend() const {
        logic<target_size - 1, 0, signed_> result;
        result.value = value.template extend<target_size>();
        result.xz_mask = xz_mask.template extend<target_size>();
        return result;
    }

    /*
     * concatenation
     */
    template <int arg0_msb, int arg0_lsb, bool arg0_signed>
    constexpr auto concat(const logic<arg0_msb, arg0_lsb, arg0_signed> &arg0) {
        auto constexpr final_size = size + logic<arg0_msb, arg0_lsb, arg0_signed>::size;
        auto res = logic<final_size - 1, 0, signed_>();
        res.value = value.concat(arg0.value);
        // concat masks as well
        res.xz_mask = xz_mask.concat(arg0.xz_mask);
        return res;
    }

    template <typename U, typename... Ts>
    constexpr auto concat(U arg0, Ts... args) {
        return concat(arg0).concat(args...);
    }

    /*
     * bits unpacking
     */
    template <typename... Ts>
    auto unpack(Ts &...args) const {
        if constexpr (big_endian) {
            auto tuples = std::forward_as_tuple(args...);
            auto reversed = util::reverse_tuple(tuples);
            std::apply([this](auto &&...args) { this->template unpack_(args...); }, reversed);
        } else {
            return this->template unpack_(args...);
        }
    }

    /*
     * boolean operators
     */
    explicit operator bool() const {
        // if there is any x or z
        return (!xz_mask.any_set()) && value.any_set();
    }

    logic<0> operator!() const {
        return xz_mask.any_set() ? x_() : (value.any_set() ? zero_() : one_());
    }

    /*
     * bitwise operators
     */

    template <int op_msb, int op_lsb, bool op_signed>
    requires((util::abs_diff(op_msb, op_lsb) + 1) != size) auto operator&(
        const logic<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, logic<op_msb, op_lsb>::size);
        return this->template and_<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator&(const logic<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        // this is the truth table
        //   0 1 x z
        // 0 0 0 0 0
        // 1 0 1 x x
        // x 0 x x x
        // z 0 x x x
        logic<size - 1, 0, util::signed_result(signed_, op_signed)> result;
        result.value = value & op.value;
        result.xz_mask = xz_mask & op.xz_mask;
        // we have taken care of the top left and bottom case
        // i.e.
        // 0 0 0 0
        // 0 1
        // 0   x x
        // 0   x x
        // need to take care of the rest
        // only happens between 1 and x/z. Notice that 1 is encoded as (1, 0) and x is (, 1)
        //
        auto mask = (((xz_mask ^ op.xz_mask) & ~xz_mask) & value) |     // 1   & x/z
                    (((op.xz_mask ^ xz_mask) & ~op.xz_mask) & ~value);  // x/z & 1
        result.value &= ~mask;
        result.xz_mask |= mask;

        return result;
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, logic<op_msb, op_lsb>::size)) auto and_(
        const logic<op_msb, op_lsb, op_signed> &op) const {
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l & r;
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    logic<size - 1, 0, util::signed_result(signed_, op_signed)> &operator&=(
        const logic<op_msb, op_lsb, op_signed> &op) {
        auto const res = (*this) & op;
        this->value = res.value;
        this->xz_mask = res.xz_mask;
        return *this;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    requires((util::abs_diff(op_msb, op_lsb) + 1) != size) auto operator|(
        const logic<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, logic<op_msb, op_lsb>::size);
        return this->template or_<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator|(const logic<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        // this is the truth table
        //   0 1 x z
        // 0 0 1 x x
        // 1 1 1 1 1
        // x x 1 x x
        // z x 1 x x
        logic<size - 1, 0, util::signed_result(signed_, op_signed)> result;
        result.value = value | op.value;
        result.xz_mask = xz_mask | op.xz_mask;
        // we have taken care of the only top left case
        // i.e.
        // 0 0
        // 0 1
        //
        //
        // notice that for the rest of the empty cells, xz_mask is set properly
        // we use that to create a msk of change everything into x
        result.value &= ~result.xz_mask;
        // now we have something like this
        //   0 1 x z
        // 0 0 1 x x
        // 1 1 1 x x
        // x x x x x
        // z x x x x
        // compute the mask for case // 1 & x/z and x/z & 1
        auto mask = (((xz_mask ^ op.xz_mask) & ~xz_mask) & value) |     // 1   | x/z
                    (((op.xz_mask ^ xz_mask) & ~op.xz_mask) & ~value);  // x/z | 1
        result.value |= mask;
        result.xz_mask &= ~mask;
        return result;
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, logic<op_msb, op_lsb>::size)) auto or_(
        const logic<op_msb, op_lsb, op_signed> &op) const {
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l | r;
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    logic<size - 1, 0, util::signed_result(signed_, op_signed)> &operator|=(
        const logic<op_msb, op_lsb, op_signed> &op) {
        auto const res = (*this) | op;
        this->value = res.value;
        this->xz_mask = res.xz_mask;
        return *this;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    requires((util::abs_diff(op_msb, op_lsb) + 1) != size) auto operator^(
        const logic<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, logic<op_msb, op_lsb>::size);
        return this->template xor_<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator^(const logic<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        // this is the truth table
        //   0 1 x z
        // 0 1 0 x x
        // 1 0 1 x x
        // x x x x x
        // z x x x x
        logic<size - 1, 0, util::signed_result(signed_, op_signed)> result;
        result.value = value ^ op.value;
        result.xz_mask = xz_mask | op.xz_mask;
        // we have taken care of the only top left case
        // i.e.
        // 1 0 x z
        // 0 0 z x
        // x z x z
        // z x z x
        // notice that for the rest of the empty cells, xz_mask is set properly
        // we use that to create a msk of change everything into x
        result.value &= ~result.xz_mask;
        return result;
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, logic<op_msb, op_lsb>::size)) auto xor_(
        const logic<op_msb, op_lsb, op_signed> &op) const {
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l | r;
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    logic<size - 1, 0, util::signed_result(signed_, op_signed)> &operator^=(
        const logic<op_msb, op_lsb, op_signed> &op) {
        auto const res = (*this) ^ op;
        this->value = res.value;
        this->xz_mask = res.xz_mask;
        return *this;
    }

    logic<size - 1> operator~() const {
        // this is the truth table
        //   0 1 x z
        //   1 0 x x
        logic<size - 1> result;
        result.value = ~value;
        result.xz_mask = xz_mask;
        // change anything has xz_mask on to x
        result.value &= ~result.xz_mask;
        return result;
    }

    // reduction
    [[nodiscard]] logic<0> r_and() const {
        // zero trump everything
        // brute force way to compute
        for (auto i = 0u; i < size; i++) {
            auto b = value[i];
            auto m = xz_mask[i];
            if (!b && !m)
                return zero_();
            else if (m)
                return x_();
        }
        return one_();
    }

    [[nodiscard]] logic<0> r_nand() const { return !r_and(); }

    [[nodiscard]] logic<0> r_or() const {
        for (auto i = 0u; i < size; i++) {
            auto b = value[i];
            auto m = xz_mask[i];
            if (b && !m)
                return one_();
            else if (m)
                return x_();
        }
        return zero_();
    }

    [[nodiscard]] logic<0> r_nor() const { return !r_or(); }

    [[nodiscard]] logic<0> r_xor() const {
        // this is the truth table
        //   0 1 x z
        // 0 1 0 x x
        // 1 0 1 x x
        // x x x x x
        // z x x x x
        // if x/z is set, it's always x
        if (xz_mask.any_set()) return x_();
        auto bits_count = value.popcount();
        auto zeros = size - bits_count;
        return (zeros % 2) ? one_() : zero_();
    }

    [[nodiscard]] logic<0> r_xnor() const { return !r_xor(); }

    template <int op_msb, int op_lsb, bool op_signed>
    auto operator>>(const logic<op_msb, op_lsb, op_signed> &amount) const {
        logic<size - 1, 0, util::signed_result(signed_, op_signed)> res;
        if (amount.xz_mask.any_set() || xz_mask.any_set()) [[unlikely]] {
            // return all x
            res.xz_mask.mask();
        } else {
            res.value = value >> amount.value;
            res.xz_mask = xz_mask >> amount.value;
        }
        return res;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    logic<size - 1, 0, util::signed_result(signed_, op_signed)> &operator>>=(
        const logic<op_msb, op_lsb, op_signed> &amount) {
        auto res = (*this) >> amount;
        value = res.value;
        xz_mask = res.xz_mask;
        return *this;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    auto operator<<(const logic<op_msb, op_lsb, op_signed> &amount) const {
        logic<size - 1, 0, util::signed_result(signed_, op_signed)> res;
        if (amount.xz_mask.any_set() || xz_mask.any_set()) [[unlikely]] {
            // return all x
            res.xz_mask.mask();
        } else {
            res.value = value << amount.value;
            res.xz_mask = xz_mask << amount.value;
        }
        return res;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    logic<size - 1, 0, util::signed_result(signed_, op_signed)> &operator<<=(
        const logic<op_msb, op_lsb, op_signed> &amount) {
        auto res = (*this) << amount;
        value = res.value;
        xz_mask = res.xz_mask;
        return *this;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    auto ashr(const logic<op_msb, op_lsb, op_signed> &amount) const {
        logic<size - 1, 0, util::signed_result(signed_, op_signed)> res;
        if (amount.xz_mask.any_set() || xz_mask.any_set()) [[unlikely]] {
            // return all x
            res.xz_mask.mask();
        } else {
            res.value = value.ashr(amount.value);
            res.xz_mask = xz_mask.ashr(amount.value);
        }
        return res;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    auto ashl(const logic<op_msb, op_lsb, op_signed> &amount) const {
        logic<size - 1, 0, util::signed_result(signed_, op_signed)> res;
        if (amount.xz_mask.any_set() || xz_mask.any_set()) [[unlikely]] {
            // return all x
            res.xz_mask.mask();
        } else {
            res.value = value.ashl(amount.value);
            res.xz_mask = xz_mask.ashl(amount.value);
        }
        return res;
    }

    /*
     * comparators
     */
    template <int op_msb, int op_lsb, bool op_signed>
    logic<0> operator==(const logic<op_msb, op_lsb, op_signed> &target) const {
        if (xz_mask.any_set() || target.xz_mask.any_set()) return x_();
        return value == target.value ? one_() : zero_();
    }

    template <int op_msb, int op_lsb, bool op_signed>
    logic<0> operator>(const logic<op_msb, op_lsb, op_signed> &target) const {
        if (xz_mask.any_set() || target.xz_mask.any_set()) return x_();
        return value > target.value ? one_() : zero_();
    }

    template <int op_msb, int op_lsb, bool op_signed>
    logic<0> operator<(const logic<op_msb, op_lsb, op_signed> &target) const {
        if (xz_mask.any_set() || target.xz_mask.any_set()) return x_();
        return value < target.value ? one_() : zero_();
    }

    template <int op_msb, int op_lsb, bool op_signed>
    logic<0> operator>=(const logic<op_msb, op_lsb, op_signed> &target) const {
        if (xz_mask.any_set() || target.xz_mask.any_set()) return x_();
        return value >= target.value ? one_() : zero_();
    }

    template <int op_msb, int op_lsb, bool op_signed>
    logic<0> operator<=(const logic<op_msb, op_lsb, op_signed> &target) const {
        if (xz_mask.any_set() || target.xz_mask.any_set()) return x_();
        return value <= target.value ? one_() : zero_();
    }

    constexpr auto operator-() const {
        if (xz_mask.any_set()) return logic<msb, lsb, signed_>();
        logic<msb, lsb, signed_> result(0);
        static_assert(!std::is_same_v<int, decltype(value)>);
        result.value = -value;
        return result;
    }

    constexpr auto operator+() const {
        if (xz_mask.any_set()) return logic<msb, lsb, signed_>();
        return *this;
    }

    /*
     * arithmetic operators: + - * / %
     */

    template <int op_msb, int op_lsb, bool op_signed>
    requires(logic<op_msb, op_lsb>::size != size) auto operator+(
        const logic<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, logic<op_msb, op_lsb>::size);
        return this->template add<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator+(const logic<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        if (xz_mask.any_set() || op.xz_mask.any_set()) [[unlikely]] {
            return logic<size - 1, 0, util::signed_result(signed_, op_signed)>();
        } else {
            return logic<size - 1, 0, util::signed_result(signed_, op_signed)>{value + op.value};
        }
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, logic<op_msb, op_lsb>::size)) auto add(
        const logic<op_msb, op_lsb, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l + r;
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    requires(logic<op_msb, op_lsb>::size != size) auto operator-(
        const logic<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, logic<op_msb, op_lsb>::size);
        return this->template minus<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator-(const logic<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        if (xz_mask.any_set() || op.xz_mask.any_set()) [[unlikely]] {
            return logic<size - 1, 0, util::signed_result(signed_, op_signed)>();
        } else {
            return logic<size - 1, 0, util::signed_result(signed_, op_signed)>{value - op.value};
        }
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, logic<op_msb, op_lsb>::size)) auto minus(
        const logic<op_msb, op_lsb, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l - r;
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    requires(logic<op_msb, op_lsb>::size != size) auto operator*(
        const logic<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, logic<op_msb, op_lsb>::size);
        return this->template multiply<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator*(const logic<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        if (xz_mask.any_set() || op.xz_mask.any_set()) [[unlikely]] {
            return logic<size - 1, 0, util::signed_result(signed_, op_signed)>();
        } else {
            return logic<size - 1, 0, util::signed_result(signed_, op_signed)>{value * op.value};
        }
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, logic<op_msb, op_lsb>::size)) auto multiply(
        const logic<op_msb, op_lsb, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l * r;
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    requires(logic<op_msb, op_lsb>::size != size) auto operator/(
        const logic<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, logic<op_msb, op_lsb>::size);
        return this->template divide<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator/(const logic<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        // if the op is 0, return x
        if (xz_mask.any_set() || op.xz_mask.any_set() || !op.value.any_set()) [[unlikely]] {
            return logic<size - 1, 0, util::signed_result(signed_, op_signed)>();
        } else {
            return logic<size - 1, 0, util::signed_result(signed_, op_signed)>{value / op.value};
        }
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, logic<op_msb, op_lsb>::size)) auto divide(
        const logic<op_msb, op_lsb, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l / r;
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    requires(logic<op_msb, op_lsb>::size != size) auto operator%(
        const logic<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, logic<op_msb, op_lsb>::size);
        return this->template mod<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator%(const logic<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        // if the op is 0, return x
        if (xz_mask.any_set() || op.xz_mask.any_set() || !op.value.any_set()) [[unlikely]] {
            return logic<size - 1, 0, util::signed_result(signed_, op_signed)>();
        } else {
            return logic<size - 1, 0, util::signed_result(signed_, op_signed)>{value % op.value};
        }
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, logic<op_msb, op_lsb>::size)) auto mod(
        const logic<op_msb, op_lsb, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l % r;
        return result;
    }

    [[nodiscard]] logic<size - 1, 0, true> to_signed() const {
        logic<size - 1, 0, true> result;
        result.value = value.to_signed();
        result.xz_mask = xz_mask;
        return result;
    }

    /*
     * constructors
     */
    // by default everything is x
    constexpr logic() { xz_mask.mask(); }

    template <typename T>
    requires(size <= big_num_threshold) explicit constexpr logic(T value)
        : value(bits<msb, lsb, signed_>(value)) {}

    template <typename K>
    requires(std::is_arithmetic_v<K> &&size > big_num_threshold) explicit constexpr logic(K value)
        : value(bits<msb, lsb, signed_>(value)) {}

    explicit logic(const char *str) : logic(std::string_view(str)) {}
    explicit logic(std::string_view v) : value(bits<msb, lsb, signed_>(v)) {
        auto iter = v.rbegin();
        for (auto i = 0u; i < size; i++) {
            // from right to left
            auto c = *iter;
            switch (c) {
                case 'x':
                    set_x(i);
                    break;
                case 'z':
                    set_z(i);
                    break;
                default:;
            }
            iter++;
            if (iter == v.rend()) break;
        }
    }

    // conversion from bits to logic
    constexpr explicit logic(bits<msb, lsb, signed_> &&b) : value(std::move(b)) {}
    // shifting msb and lsb
    template <int new_msb, int new_lsb, bool new_signed>
    requires(util::abs_diff(new_msb, new_lsb) ==
             util::abs_diff(msb,
                            lsb)) explicit logic(const logic<new_msb, new_lsb, new_signed> &target)
        : value(target.value), xz_mask(target.xz_mask) {}
    // safe conversions

private:
    void unmask_bit(uint64_t idx) { xz_mask.set(idx, false); }

    template <uint64_t idx>
    void unmask_bit() {
        xz_mask.template set<idx, false>();
    }

    /*
     * unpacking, which is basically slicing as syntax sugars
     */
    template <int base, int arg0_msb, int arg0_lsb, int arg0_signed>
    requires(base < size) void unpack_(logic<arg0_msb, arg0_lsb, arg0_signed> &arg0) const {
        auto constexpr arg0_size = logic<arg0_msb, arg0_lsb, arg0_signed>::size;
        auto constexpr upper_bound = util::min(size - 1, arg0_size + base - 1);
        arg0.value = value.template slice<base, upper_bound>();
        arg0.xz_mask = xz_mask.template slice<base, upper_bound>();
    }

    template <int base = lsb, int arg0_msb, int arg0_lsb, bool arg0_signed, typename... Ts>
    [[maybe_unused]] auto unpack_(logic<arg0_msb, arg0_lsb, arg0_signed> &arg0, Ts &...args) const {
        auto constexpr arg0_size = logic<arg0_msb, arg0_lsb, arg0_signed>::size;
        this->template unpack_<base, arg0_msb, arg0_lsb, arg0_signed>(arg0);
        this->template unpack_<base + arg0_size>(args...);
    }

    template <int>
    [[maybe_unused]] void unpack_() const {
        // noop to keep gcc happy
    }

    static constexpr logic<0> x_() {
        logic<0> r;
        r.value.value = false;
        r.xz_mask.value = true;
        return r;
    }

    static constexpr logic<0> one_() {
        logic<0> r;
        r.value.value = true;
        r.xz_mask.value = false;
        return r;
    }

    static constexpr logic<0> zero_() {
        logic<0> r;
        r.value.value = false;
        r.xz_mask.value = false;
        return r;
    }
};

inline namespace literals {
// constexpr to parse SystemVerilog literals
// per LRM, if size not specified, the default size is 32
//     it will be signed as well
// in C++ we only allow unsigned integer. as a result, we will create a number that's
// 1 bit less to account for negative number
constexpr logic<31, 0, true> operator""_logic(unsigned long long value) {
    return logic<30, 0, true>(static_cast<int32_t>(value)).extend<32>();
}
constexpr logic<63, 0, true> operator""_logic64(unsigned long long value) {
    return logic<62, 0, true>(static_cast<int64_t>(value)).extend<64>();
}
}  // namespace literals

}  // namespace logic
#endif  // LOGIC_LOGIC_HH
