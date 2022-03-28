#ifndef LOGIC_BIT_HH
#define LOGIC_BIT_HH

#include "big_num.hh"

namespace logic {

template <int msb = 0, int lsb = 0, bool signed_ = false, bool array = false>
struct logic;

template <int msb = 0, int lsb = 0, bool signed_ = false, bool array = false>
struct bit {
public:
    constexpr static auto size = util::abs_diff(msb, lsb) + 1;
    constexpr static auto big_endian = msb >= lsb;
    static_assert(size > 0, "0 sized logic not allowed");
    static constexpr bool native_num = util::native_num(size);
    constexpr static auto is_signed = signed_;
    constexpr static bool is_4state = false;
    using T = typename util::get_holder_type<size, signed_>::type;

    T value;

    // basic formatting
    [[nodiscard]] std::string str(std::string_view fmt = "b") const {
        if constexpr (native_num) {
            if (signed_ && negative() && util::decimal_fmt(fmt)) {
                return util::to_string(fmt, size, negate().value, true);
            } else {
                return util::to_string(fmt, size, value, false);
            }
        } else {
            if (signed_ && negative() && util::decimal_fmt(fmt)) {
                auto neg = negate();
                return util::to_string(fmt, size, neg.value.values.data(), true);
            } else {
                return util::to_string(fmt, size, value.values.data(), false);
            }
        }
    }

    // single bit
    bit<0> inline operator[](uint64_t idx) const requires(!array) { return get_(idx); }

    template <uint64_t idx>
    requires(idx < size && native_num) [[nodiscard]] bit<0> inline get() const requires(!array) {
        return this->operator[](idx);
    }

    template <int op_msb, int op_lsb, bool op_signed, bool op_array>
    bool inline get(const bit<op_msb, op_lsb, op_signed, op_array> &op) const requires(!array) {
        uint64_t index;
        if constexpr (bit<op_msb, op_lsb>::native_num) {
            index = static_cast<uint64_t>(op.value - util::min(msb, lsb));
        } else {
            if (op.fit_in_64()) {
                index = static_cast<uint64_t>(op.value.value[0] - util::min(msb, lsb));
            } else {
                return false;
            }
        }
        return get_(index);
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
    void set(bool v) requires(!array) {
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

    void inline set(uint64_t idx, bit<0> v) { value.set(idx, v.value); }

    template <uint64_t idx>
    void set(bit<0> v) requires(!array) {
        this->template set<idx>(v.value);
    }

    template <uint64_t idx, bool v>
    void set() requires(!array) {
        constexpr auto i = !big_endian ? lsb - idx : idx;
        if constexpr (native_num) {
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
    [[nodiscard]] bool negative() const {
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
    requires(util::max(a, b) < size && native_num && b >= lsb) bit<util::abs_diff(a, b)>
    constexpr inline slice() const requires(!array) { return this->template slice_<a, b>(); }

    /*
     * big number but small slice
     */
    template <int a, int b>
    requires(util::max(a, b) < size && !native_num && util::native_num(util::abs_diff(a, b)) &&
             b >= lsb) constexpr bit<util::abs_diff(a, b)> inline slice() const requires(!array) {
        bit<util::abs_diff(a, b), false> result;
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
             b >= lsb) constexpr bit<util::abs_diff(a, b)> inline slice() const requires(!array) {
        bit<util::abs_diff(a, b), false> result;
        result.value = value.template slice<a, b>();
        return result;
    }

    template <uint32_t size>
    [[nodiscard]] auto slice(int a, int b) const {
        return this->template slice_<size>(a, b);
    }

    // extension
    // notice that we need to implement signed extension, for the result that's native holder
    // C++ will handle that for us already.
    template <uint64_t target_size>
    requires(target_size > size && util::native_num(target_size))
        [[nodiscard]] constexpr bit<target_size - 1, 0, signed_> extend() const {
        return bit<target_size - 1, 0, signed_>(value);
    }

    template <uint64_t target_size>
    requires(target_size == size)
        [[nodiscard]] constexpr bit<target_size - 1, 0, signed_> extend() const {
        return *this;
    }

    // new size is smaller. this is just a slice
    template <uint64_t target_size>
    requires(target_size < size)
        [[nodiscard]] constexpr bit<target_size - 1, 0, signed_> extend() const {
        return slice<target_size - 1, 0>();
    }

    // native number, but extend to a big number
    template <uint64_t target_size>
    requires(target_size > size && !util::native_num(target_size) && native_num)
        [[nodiscard]] constexpr bit<target_size - 1, 0, signed_> extend() const {
        bit<target_size - 1, 0, signed_> result;
        // use C++ default semantics to cast. if it's a negative number
        // the upper bit will be set properly
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
        [[nodiscard]] constexpr bit<target_size - 1, 0, signed_> extend() const {
        bit<target_size - 1, 0, signed_> res;
        res.value = value.template extend<target_size>();
        return res;
    }

    /*
     * concatenation
     */
    template <int arg0_msb, int arg0_lsb, bool arg0_signed>
    auto concat(const bit<arg0_msb, arg0_lsb, arg0_signed> &arg0) {
        auto constexpr arg0_size = bit<arg0_msb, arg0_lsb>::size;
        auto constexpr final_size = size + arg0_size;
        if constexpr (final_size < big_num_threshold) {
            return bit<final_size - 1>(value << arg0_size | arg0.value);
        } else {
            auto res = bit<final_size - 1>();
            if constexpr (bit<arg0_msb, arg0_lsb>::native_num) {
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
     * bit unpacking
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

    void clear() {
        if constexpr (native_num) {
            value = 0;
        } else {
            value.clear();
        }
    }

    /*
     * bitwise operators
     */
    bit<size - 1, 0, false> operator~() const {
        bit<size - 1, 0, false> result;
        if constexpr (size == 1) {
            result.value = !value;
        } else {
            result.value = ~value;
        }
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    requires((util::abs_diff(op_msb, op_lsb) + 1) != size) auto operator&(
        const bit<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, bit<op_msb, op_lsb>::size);
        return this->template and_<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator&(const bit<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        bit<size - 1, 0, util::signed_result(signed_, op_signed)> result;
        result.value = value & op.value;
        return result;
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, bit<op_msb, op_lsb>::size)) auto and_(
        const bit<op_msb, op_lsb, op_signed> &op) const {
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l & r;
        result.mask_off();
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    bit<size - 1, 0, util::signed_result(signed_, op_signed)> &operator&=(
        const bit<op_msb, op_lsb, op_signed> &op) {
        auto res = (*this) & op;
        value = res.value;
        mask_off();
        return *this;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    requires((util::abs_diff(op_msb, op_lsb) + 1) != size) auto operator^(
        const bit<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, bit<op_msb, op_lsb>::size);
        return this->template xor_<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator^(const bit<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        bit<size - 1, 0, util::signed_result(signed_, op_signed)> result;
        result.value = value ^ op.value;
        return result;
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, bit<op_msb, op_lsb>::size)) auto xor_(
        const bit<op_msb, op_lsb, op_signed> &op) const {
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l ^ r;
        result.mask_off();
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    bit<size - 1, 0, util::signed_result(signed_, op_signed)> &operator^=(
        const bit<op_msb, op_lsb, op_signed> &op) {
        auto res = (*this) ^ op;
        value = res.value;
        mask_off();
        return *this;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    requires((util::abs_diff(op_msb, op_lsb) + 1) != size) auto operator|(
        const bit<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, bit<op_msb, op_lsb>::size);
        return this->template or_<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator|(const bit<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        bit<size - 1, 0, util::signed_result(signed_, op_signed)> result;
        result.value = value | op.value;
        return result;
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, bit<op_msb, op_lsb>::size)) auto or_(
        const bit<op_msb, op_lsb, op_signed> &op) const {
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l | r;
        result.mask_off();
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    bit<size - 1, 0, util::signed_result(signed_, op_signed)> &operator|=(
        const bit<op_msb, op_lsb, op_signed> &op) {
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
    auto operator>>(const bit<op_msb, op_lsb, op_signed> &amount) const {
        bit<size - 1, 0, util::signed_result(signed_, op_signed)> res;
        // couple cases
        // 1. both of them are native number
        if constexpr (native_num && bit<op_msb, op_lsb>::native_num) {
            // make sure to mask off any bit dur to signed extension
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

    auto &operator++() {
        (*this) = (*this) + one_();
        return *this;
    }

    auto &operator--() {
        (*this) = (*this) - one_();
        return *this;
    }

    // NOLINTNEXTLINE
    bit<size - 1, 0, signed_> operator++(int) {
        bit<size - 1, 0, signed_> v = *this;
        this->operator++();
        return v;
    }

    // NOLINTNEXTLINE
    bit<size - 1, 0, signed_> operator--(int) {
        bit<size - 1, 0, signed_> v = *this;
        this->operator--();
        return v;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    bit<size - 1, 0, util::signed_result(signed_, op_signed)> &operator>>=(
        const bit<op_msb, op_lsb, op_signed> &amount) {
        auto res = (*this) >> amount;
        value = res.value;
        return *this;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    auto operator<<(const bit<op_msb, op_lsb, op_signed> &amount) const {
        bit<size - 1, 0, util::signed_result(signed_, op_signed)> res;
        // couple cases
        // 1. both of them are native number
        // clang doesn't allow accessing native_num as amount.native_num
        // see https://stackoverflow.com/a/44996066
        if constexpr (native_num && bit<op_msb, op_lsb>::native_num) {
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
    bit<size - 1, 0, util::signed_result(signed_, op_signed)> &operator<<=(
        const bit<op_msb, op_lsb, op_signed> &amount) {
        auto res = (*this) << amount;
        value = res.value;
        return *this;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    auto ashr(const bit<op_msb, op_lsb, op_signed> &amount) const {
        bit<size - 1, 0, util::signed_result(signed_, op_signed)> res;
        // couple cases
        // 1. both of them are native number
        if constexpr (native_num && bit<op_msb, op_lsb>::native_num) {
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
    auto ashl(const bit<op_msb, op_lsb, op_signed> &amount) const {
        // this is the same as bitwise shift left
        return (*this) << amount;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    bool operator==(const bit<op_msb, op_lsb, op_signed> &v) const {
        if constexpr (native_num && bit<op_msb, op_lsb>::native_num) {
            return value == static_cast<T>(v.value);
        } else {
            return value == v.value;
        }
    }

    template <int op_msb, int op_lsb, bool op_signed>
    bool operator!=(const bit<op_msb, op_lsb, op_signed> &v) const {
        if constexpr (native_num && bit<op_msb, op_lsb>::native_num) {
            return value != static_cast<T>(v.value);
        } else {
            return value != v.value;
        }
    }

    template <int op_msb, int op_lsb, bool op_signed>
    [[nodiscard]] bool match(const bit<op_msb, op_lsb, op_signed> &op) const {
        return (*this) == op;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    [[nodiscard]] bool nmatch(const bit<op_msb, op_lsb, op_signed> &op) const {
        return !match(op);
    }

    template <int op_msb, int op_lsb, bool op_signed>
    bool operator>(const bit<op_msb, op_lsb, op_signed> &v) const {
        if constexpr (native_num && bit<op_msb, op_lsb>::native_num) {
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
    bool operator<(const bit<op_msb, op_lsb, op_signed> &v) const {
        if constexpr (native_num && bit<op_msb, op_lsb>::native_num) {
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
    bool operator>=(const bit<op_msb, op_lsb, op_signed> &v) const {
        if constexpr (native_num && bit<op_msb, op_lsb>::native_num) {
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
    bool operator<=(const bit<op_msb, op_lsb, op_signed> &v) const {
        if constexpr (native_num && bit<op_msb, op_lsb>::native_num) {
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

    constexpr bit<msb, lsb, signed_> operator-() const {
        if constexpr (native_num) {
            return bit<msb, lsb, signed_>(-value);
        } else {
            return bit<msb, lsb, signed_>(value.negate());
        }
    }

    constexpr auto operator+() const { return *this; }

    /*
     * arithmetic operators: + - * / %
     */

    template <int op_msb, int op_lsb, bool op_signed>
    requires(bit<op_msb, op_lsb>::size != size) auto operator+(
        const bit<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, bit<op_msb, op_lsb>::size);
        return this->template add<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator+(const bit<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        bit<size - 1, 0, util::signed_result(signed_, op_signed)> result;
        result.value = value + op.value;
        return result;
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, bit<op_msb, op_lsb>::size)) auto add(
        const bit<op_msb, op_lsb, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l + r;
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    requires(bit<op_msb, op_lsb>::size != size) auto operator-(
        const bit<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, bit<op_msb, op_lsb>::size);
        return this->template minus<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator-(const bit<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        bit<size - 1, 0, util::signed_result(signed_, op_signed)> result;
        result.value = value - op.value;
        return result;
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, bit<op_msb, op_lsb>::size)) auto minus(
        const bit<op_msb, op_lsb, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l - r;
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    requires(bit<op_msb, op_lsb>::size != size) auto operator*(
        const bit<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, bit<op_msb, op_lsb>::size);
        return this->template multiply<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator*(const bit<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        bit<size - 1, 0, util::signed_result(signed_, op_signed)> result;
        result.value = value * op.value;
        return result;
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, bit<op_msb, op_lsb>::size)) auto multiply(
        const bit<op_msb, op_lsb, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l * r;
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    requires(bit<op_msb, op_lsb>::size != size) auto operator%(
        const bit<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, bit<op_msb, op_lsb>::size);
        return this->template mod<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator%(const bit<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        bit<size - 1, 0, util::signed_result(signed_, op_signed)> result;
        result.value = value % op.value;
        return result;
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, bit<op_msb, op_lsb>::size)) auto mod(
        const bit<op_msb, op_lsb, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l % r;
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    requires(bit<op_msb, op_lsb>::size != size) auto operator/(
        const bit<op_msb, op_lsb, op_signed> &op) const {
        auto constexpr target_size = util::max(size, bit<op_msb, op_lsb>::size);
        return this->template divide<target_size, op_msb, op_lsb, op_signed>(op);
    }

    template <int op_lsb, bool op_signed>
    auto operator/(const bit<size - 1 + op_lsb, op_lsb, op_signed> &op) const {
        bit<size - 1, 0, util::signed_result(signed_, op_signed)> result;
        result.value = value / op.value;
        return result;
    }

    template <uint64_t target_size, int op_msb, int op_lsb, bool op_signed>
    requires(target_size >= util::max(size, bit<op_msb, op_lsb>::size)) auto divide(
        const bit<op_msb, op_lsb, op_signed> &op) const {
        // resize things to target size
        auto l = this->template extend<target_size>();
        auto r = op.template extend<target_size>();
        auto result = l / r;
        return result;
    }

    [[nodiscard]] bit<size - 1, 0, true> to_signed() const {
        bit<size - 1, 0, true> res;
        if constexpr (native_num) {
            if constexpr (size == 1) {
                res.value = value;
            } else {
                res.value = static_cast<decltype(bit<size - 1, 0, true>::value)>(value);
            }
        } else {
            res.value = value.to_signed();
        }
        return res;
    }

    [[nodiscard]] bit<size - 1, 0, false> to_unsigned() const {
        bit<size - 1, 0, false> res;
        if constexpr (native_num) {
            if constexpr (size == 1) {
                res.value = value;
            } else {
                res.value = static_cast<decltype(bit<size - 1, 0, false>::value)>(value);
            }
        } else {
            // big number is always unsigned
            res.value = value;
        }
        return res;
    }

    /*
     * mask related stuff
     */
    [[nodiscard]] bool any_set() const requires(native_num) {
        // unused bit are always set to 0
        return value != 0;
    }

    [[nodiscard]] bool any_set() const requires(!native_num) { return value.any_set(); }

    void mask() {
        if constexpr (native_num) {
            if constexpr (size == 1) {
                value = true;
            } else {
                value = std::numeric_limits<T>::max() >> (sizeof(T) * 8 - size);
            }
        } else {
            value.mask();
        }
    }

    [[nodiscard]] uint64_t highest_bit() const {
        if constexpr (native_num) {
            if (value == 0)
                return size;
            else {
                auto r = __builtin_clzll(static_cast<uint64_t>(value));
                return 64 - r - 1;
            }
        } else {
            return value.highest_bit();
        }
    }

    void negate() {
        if constexpr (native_num) {
            value = (~value) + 1;
            mask_off();
        } else {
            return value.negate();
        }
    }

    [[nodiscard]] bit<size - 1, 0> negate() const {
        bit<size - 1, 0> res;
        if constexpr (native_num) {
            res.value = (~value) + 1;
            res.mask_off();
        } else {
            res.value = value.negate();
        }
        return res;
    }

    [[nodiscard]] bool all_set() const {
        if constexpr (native_num) {
            auto constexpr max = value_mask(size);
            return value == max;
        } else {
            return value.all_set();
        }
    }

    /*
     * update using slice
     */
    // setting values
    template <uint64_t hi, uint64_t lo = hi, uint64_t op_hi, uint64_t op_lo, bool op_signed>
    void update(const bit<op_hi, op_lo, op_signed> &op) requires(hi < size && lo < size) {
        auto constexpr start = util::min(hi, lo);
        auto constexpr end = util::max(hi, lo) + 1;
        for (auto i = start; i < end; i++) {
            // notice that if it is out of range, 0 will be returned
            // we need to revert the index accessing scheme here, if the endian doesn't match
            uint64_t idx;
            if constexpr (bit<op_hi, op_lo>::big_endian ^ big_endian) {
                idx = util::max(op_lo, op_hi) - i;
            } else {
                idx = i;
            }
            auto b = op.operator[](idx);
            set(i, b);
        }
    }

    /*
     * value conversions
     */
    [[nodiscard]] uint64_t to_uint64() const {
        if constexpr (native_num) {
            return value;
        } else {
            return value.to_uint64();
        }
    }

    [[nodiscard]] int8_t to_num() const requires(signed_ &&size <= 8) { return value; }

    [[nodiscard]] uint8_t to_num() const requires(!signed_ && size <= 8) { return value; }

    [[nodiscard]] int16_t to_num() const requires(signed_ &&size > 8 && size <= 16) {
        return value;
    }

    [[nodiscard]] uint16_t to_num() const requires(!signed_ && size > 8 && size <= 16) {
        return value;
    }

    [[nodiscard]] int32_t to_num() const requires(signed_ &&size > 16 && size <= 32) {
        return value;
    }

    [[nodiscard]] uint32_t to_num() const requires(!signed_ && size > 16 && size <= 32) {
        return value;
    }

    [[nodiscard]] int64_t to_num() const requires(signed_ &&size > 32 && size <= 64) {
        return value;
    }

    [[nodiscard]] uint64_t to_num() const requires(!signed_ && size > 32 && size <= 64) {
        return value;
    }

    /*
     * constructors
     */
    explicit constexpr bit(std::string_view v) {
        if constexpr (native_num) {
            value = static_cast<T>(util::parse_raw_str(v));
        } else {
            util::parse_raw_str(v, size, value.values.data());
        }
    }

    explicit constexpr bit(T v) requires(util::native_num(size)) : value(v) {}
    template <typename K>
    requires(std::is_arithmetic_v<K> && !util::native_num(size)) constexpr bit(K v)  // NOLINT
        : value(v) {}

    template <uint64_t new_size, bool new_signed>
    explicit bit(const big_num<new_size, new_signed> &big_num) requires(size <= big_num_threshold)
        : value(big_num.values[0]) {}

    template <uint64_t op_hi, uint64_t op_lo, bool op_signed>
    constexpr bit(logic<op_hi, op_lo, op_signed> &&logic) {
        *this = logic.value;
    }

    bit() {
        if constexpr (native_num) {
            // init to 0
            value = 0;
        }
        // for big number it's already set to 0
    }

    // implicit conversion via assignment
    template <int new_msb, int new_lsb, bool new_signed>
    bit &operator=(const bit<new_msb, new_lsb, new_signed> &b) {
        value = b.value;
        return *this;
    }

    template <typename T>
    bit &operator=(T v) requires(std::is_arithmetic<T>::value) {
        value = v;
        return *this;
    }

    template <int new_msb, int new_lsb>
    constexpr bit(const bit<new_msb, new_lsb, true> &b) {  // NOLINT
        value = b.value;
    }

    static constexpr bit<0> one_() {
        bit<0> r;
        r.value = true;
        return r;
    }

private:
    /*
     * unpacking, which is basically slicing as syntax sugars
     */
    template <int base, int arg0_msb, int arg0_lsb, bool arg0_signed>
    requires(base < size) void unpack_(bit<arg0_msb, arg0_lsb, arg0_signed> &arg0) const {
        auto constexpr arg0_size = bit<arg0_msb, arg0_lsb, arg0_signed>::size;
        auto constexpr upper_bound = util::min(size - 1, arg0_size + base - 1);
        arg0.value = this->template slice<base, upper_bound>();
    }

    template <int base, int arg0_msb, int arg0_lsb, bool arg0_signed>
    requires(base < size) void unpack_(logic<arg0_msb, arg0_lsb, arg0_signed> &arg0) const {
        auto constexpr arg0_size = bit<arg0_msb, arg0_lsb, arg0_signed>::size;
        auto constexpr upper_bound = util::min(size - 1, arg0_size + base - 1);
        arg0.value = this->template slice<base, upper_bound>();
        arg0.xz_mask.clear();
    }

    template <int base = 0, int arg0_msb, int arg0_lsb, bool arg0_signed, typename... Ts>
    [[maybe_unused]] auto unpack_(bit<arg0_msb, arg0_lsb, arg0_signed> &arg0, Ts &...args) const {
        auto constexpr arg0_size = bit<arg0_msb, arg0_lsb, arg0_signed>::size;
        this->template unpack_<base, arg0_msb, arg0_lsb, arg0_signed>(arg0);
        this->template unpack_<base + arg0_size>(args...);
    }

    template <int base = 0, int arg0_msb, int arg0_lsb, bool arg0_signed, typename... Ts>
    [[maybe_unused]] auto unpack_(logic<arg0_msb, arg0_lsb, arg0_signed> &arg0, Ts &...args) const {
        auto constexpr arg0_size = bit<arg0_msb, arg0_lsb, arg0_signed>::size;
        this->template unpack_<base, arg0_msb, arg0_lsb, arg0_signed>(arg0);
        this->template unpack_<base + arg0_size>(args...);
    }

    [[nodiscard]] T value_mask(uint64_t requested_size) const {
        auto mask = std::numeric_limits<uint64_t>::max();
        mask = mask >> (64ull - requested_size);
        return static_cast<T>(mask);
    }

    [[nodiscard]] inline bit<0> get_(int idx) const {
        if constexpr (!big_endian) {
            idx = lsb - idx;
        }
        if constexpr (native_num) {
            return bit<0>((value >> idx) & 1);
        } else {
            return bit<0>(value[idx]);
        }
    }

    template <int idx>
    [[nodiscard]] inline bit<0> get_() const {
        if constexpr (!big_endian) {
            constexpr auto idx_ = lsb - idx;
            if constexpr (native_num) {
                return bit<0>((value >> idx_) & 1);
            } else {
                return bit<0>(value.template get<idx_>());
            }
        } else {
            if constexpr (native_num) {
                return bit<0>((value >> idx) & 1);
            } else {
                return bit<0>(value.template get<idx>());
            }
        }
    }

protected:
    template <int a, int b>
    requires(util::max(a, b) < size && native_num && b >= lsb) bit<util::abs_diff(a, b)>
    constexpr inline slice_() const {
        // assume the import has type-checked properly, e.g. by a compiler
        constexpr auto base = util::min(msb, lsb);
        constexpr auto max = util::max(a, b) - base;
        constexpr auto min = util::min(a, b) - base;
        constexpr auto t_size = sizeof(T) * 8;
        constexpr auto target_size = util::abs_diff(a, b) + 1;
        bit<target_size - 1, false> result;
        if constexpr (target_size == 1 && size == 1) {
            result.value = value != 0;
        } else {
            constexpr auto default_mask = std::numeric_limits<T>::max();
            uint64_t mask = default_mask << min;
            uint64_t shmnt;
            if constexpr (signed_) {
                shmnt = 2;
            } else {
                shmnt = 1;
            }
            mask &= (default_mask >> (t_size - max - shmnt));
            result.value = (value & mask) >> min;
        }
        return result;
    }

    template <uint32_t target_size>
    [[nodiscard]] auto slice_(int a, int b) const {
        bit<target_size - 1, false> result;
        if constexpr (native_num) {
            // assume the import has type-checked properly, e.g. by a compiler
            auto base = util::min(msb, lsb);
            auto max = util::max(a, b) - base;
            auto min = util::min(a, b) - base;
            auto t_size = sizeof(T) * 8;
            constexpr auto default_mask = std::numeric_limits<T>::max();
            uint64_t mask = default_mask << min;
            mask &= (default_mask >> (t_size - max - 1));
            result.value = (value & mask) >> min;

            return result;
        } else {
            // may need to shrink
            auto v = value.template slice<target_size>(a, b);
            if constexpr (util::native_num(target_size)) {
                result.value = static_cast<decltype(result.value)>(v.values[0]);
            } else {
                result.value = v;
            }
        }
        return result;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    void update_(int hi, int lo, const bit<op_msb, op_lsb, op_signed> &op) {
        auto start = util::min(hi, lo);
        auto end = util::max(hi, lo) + 1;
        for (auto i = start; i < end; i++) {
            // notice that if it is out of range, 0 will be returned
            // we need to revert the index accessing scheme here, if the endian doesn't match
            uint64_t idx;
            if constexpr (bit<op_msb, op_lsb>::big_endian ^ big_endian) {
                idx = util::max(op_lsb, op_msb) - i;
            } else {
                idx = i;
            }
            auto b = op.operator[](idx - start);
            set(i, b);
        }
    }
};

inline namespace literals {
// constexpr to parse SystemVerilog literals
// per LRM, if size not specified, the default size is 32
//     it will be signed as well
constexpr bit<31, 0, true> operator""_bit(unsigned long long value) {
    return bit<31, 0, true>(static_cast<int32_t>(value)).extend<32>();
}
constexpr bit<63, 0, true> operator""_bit64(unsigned long long value) {
    return bit<63, 0, true>(static_cast<int64_t>(value)).extend<64>();
}
}  // namespace literals

}  // namespace logic

#endif  // LOGIC_BIT_HH
