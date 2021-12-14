#ifndef LOGIC_BIG_NUM_HH
#define LOGIC_BIG_NUM_HH

#include <bit>

#include "util.hh"

namespace logic {
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
    [[nodiscard]] bool inline get() const {
        if constexpr (idx >= size) {
            return false;
        } else {
            auto a = idx / big_num_threshold;
            auto b = idx % big_num_threshold;
            return (values[a] >> b) & 1;
        }
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

    // this is inefficient since we don't know what the values are during runtime
    template <uint32_t target_size>
    big_num<target_size, false> slice(uint64_t a, uint64_t b) const {
        auto start = util::min(a, b);
        auto end = util::max(a, b);
        big_num<target_size, false> result;

        for (auto i = start; i <= end; i++) {
            result.set(i - start, get(i));
        }
        return result;
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
        // mask off bit that's excessive bit
        auto constexpr amount = s * 64 - size;
        auto constexpr mask = std::numeric_limits<uint64_t>::max() >> amount;
        values[s - 1] &= mask;
    }

    void clear() { std::fill(values.begin(), values.end(), 0); }

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
        // mask off the top bit, if any
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
        // only if all the bit are set
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
        // if the big number amount actually uses more than 1 uint64 and high bit set,
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
        // if the big number amount actually uses more than 1 uint64 and high bit set,
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
            // set high bit
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
        // if the big number amount actually uses more than 1 uint64 and high bit set,
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

    template <uint64_t target_size, uint64_t op_size, bool op_signed>
    requires(target_size >=
             util::max(size, op_size)) auto power(const big_num<op_size, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        // power by squaring
        // https://en.wikipedia.org/wiki/Exponentiation_by_squaring
        // TODO:
        //  IMPLEMENT LRM Table 11-4
        big_num<target_size, util::signed_result(signed_, op_signed)> result;
        if (op.is_one()) {
            return l;
        } else if (!any_set()) {
            // ill-defined. use 0
            // or raise zero to some number, which is zero as well
            return result;
        } else if (!op.any_set()) {
            result.values[0] = 1u;
            return result;
        }

        if (op.values[0] % 2 == 0) {
            return (l * l).power(op / 2);
        } else {
            // recursion
            return (*this) * (l * l).power((op - 1) / 2);
        }
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
        // also all unused bit are set to 0 by default
        auto r = std::reduce(values.begin(), values.end(), 0, [](auto a, auto b) { return a | b; });
        return r != 0;
    }

    [[maybe_unused]] void mask() {
        for (auto i = 0u; i < (s - 1); i++) {
            values[i] = std::numeric_limits<big_num_holder_type>::max();
        }
        // last bit
        values[s - 1] =
            std::numeric_limits<big_num_holder_type>::max() >> (s * big_num_threshold - size);
    }

    [[maybe_unused]] void unmask() { std::fill(values.begin(), values.end(), 0); }

    [[nodiscard]] bool all_set() const {
        auto v = std::reduce(values.begin(), values.begin() + s - 1, 0,
                             [](auto a, auto b) { return a & b; });
        auto constexpr max =
            std::numeric_limits<big_num_holder_type>::max() >> (s * big_num_threshold - size);
        if constexpr (s == 1) {
            return values[0] == max;
        } else {
            return v == (std::numeric_limits<uint64_t>::max()) && (values[s - 1] == max);
        }
    }

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
            // shift r to the correct bit, could be too big
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

    /*
     * value conversions
     */
    [[nodiscard]] uint64_t to_uint64() const { return values[0]; }

    // constructors
    constexpr explicit big_num(std::string_view v) { util::parse_raw_str(v, size, values.data()); }

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
    constexpr big_num() { std::fill(values.begin(), values.end(), 0ull); }

    // implicit conversion between signed and unsigned
    template <uint64_t op_size, bool op_signed>
    requires(op_size == size && op_signed != signed_)
        [[maybe_unused]] big_num(const big_num<op_size, op_signed> &op)  // NOLINT
        : values(op.values) {}

private:
    [[nodiscard]] bool get(uint64_t idx) const { return operator[](idx); }
};
}  // namespace logic

#endif  // LOGIC_BIG_NUM_HH
