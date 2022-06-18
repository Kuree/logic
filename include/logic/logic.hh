#ifndef LOGIC_LOGIC_HH
#define LOGIC_LOGIC_HH

#include "bit.hh"

namespace logic {

template <int msb, int lsb, bool signed_, bool array>
struct logic {
public:
    static auto constexpr size = util::abs_diff(msb, lsb) + 1;
    constexpr static auto big_endian = msb >= lsb;
    constexpr static auto is_signed = signed_;
    constexpr static bool is_4state = true;
    bit<msb, lsb, signed_> value;
    // To reduce memory footprint, we use the following encoding scheme
    // if xz_mask is off, value is the actual integer value
    // if xz_mask is on, 0 in value means x and 1 means z
    bit<msb, lsb> xz_mask;

    // make sure that if we use native number, it's an arithmetic type
    static_assert((size <= 64 && std::is_arithmetic_v<typename bit<msb, lsb, signed_>::T>) ||
                      (size > 64),
                  "Native number holder");

    // basic formatting
    [[nodiscard]] std::string str(std::string_view fmt = "b") const {
        if constexpr (signed_) {
            auto is_negative = value.negative();
            if (is_negative && util::decimal_fmt(fmt)) {
                auto value_ = value.negate().value;
                if constexpr (util::native_num(size)) {
                    uint64_t xz = xz_mask.value;
                    return util::to_string(fmt, size, value_, xz, true);
                } else {
                    return util::to_string(fmt, size, value_.values.data(),
                                           xz_mask.value.values.data(), true);
                }
            }
        }
        if constexpr (util::native_num(size)) {
            uint64_t v = value.value;
            uint64_t xz = xz_mask.value;
            return util::to_string(fmt, size, v, xz, false);
        } else {
            return util::to_string(fmt, size, value.value.values.data(),
                                   xz_mask.value.values.data(), false);
        }
    }

    /*
     * single bit
     */
    inline logic<0> operator[](uint64_t idx) const requires(!array) {
        if (idx < size) [[likely]] {
            logic<0> r;
            if (x_set(idx)) [[unlikely]] {
                r.set_x(idx);
            } else if (z_set(idx)) [[unlikely]] {
                r.set_z(idx);
            } else {
                r.value = value[idx];
                r.xz_mask.value = false;
            }
            return r;
        } else {
            // return X
            return logic<0>{};
        }
    }

    template <uint64_t idx>
    requires(idx < size) [[nodiscard]] inline logic<0> get() const requires(!array) {
        logic<0> r{false};
        if (this->template x_set<idx>()) [[unlikely]] {
            r.set_x<idx>();
        } else if (this->template z_set<idx>()) [[unlikely]] {
            r.set_z<idx>();
        } else {
            r.value = value.template get<idx>();
        }
        return r;
    }

    template <int op_msb, int op_lsb, bool op_signed, bool op_array>
    logic<0> inline get(const logic<op_msb, op_lsb, op_signed, op_array> &op) const
        requires(!array) {
        uint64_t index;
        if constexpr (util::native_num(util::total_size(msb, lsb))) {
            index = static_cast<uint64_t>(op.value - util::min(msb, lsb));
        } else {
            if (op.fit_in_64() && !op.xz_mask.any_set()) {
                index = static_cast<uint64_t>(op.value.value[0] - util::min(msb, lsb));
            } else {
                return x_();
            }
        }
        return operator[](index);
    }

    template <int op_msb, int op_lsb, bool op_signed, bool op_array>
    logic<0> inline get(const bit<op_msb, op_lsb, op_signed, op_array> &op) const requires(!array) {
        uint64_t index;
        if constexpr (util::native_num(util::total_size(msb, lsb))) {
            index = static_cast<uint64_t>(op.value - util::min(msb, lsb));
        } else {
            if (op.fit_in_64()) {
                index = static_cast<uint64_t>(op.value.value[0] - util::min(msb, lsb));
            } else {
                return x_();
            }
        }
        return operator[](index);
    }

    void set(uint64_t idx, bool v) requires(!array) {
        value.set(idx, v);
        // unmask bit
        unmask_bit(idx);
    }

    template <uint64_t idx>
    void set(bool v) requires(!array) {
        value.template set<idx>(v);
        this->template unmask_bit<idx>();
    }

    template <uint64_t idx, bool v>
    void set() requires(!array) {
        value.template set<idx, v>();
        this->template unmask_bit<idx>();
    }

    template <bool l_signed>
    void set(uint64_t idx, const logic<0, 0, l_signed> &l) requires(!array) {
        set_(idx, l);
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
    requires(util::max(a, b) < size) constexpr logic<util::abs_diff(a, b)> inline slice() const
        requires(!array) {
        return this->template slice_<a, b>();
    }

    // used for runtime slice (only used in packed array per SV LRM)
    template <uint32_t target_size>
    logic<target_size - 1, 0> slice(int a, int b) const {
        logic<target_size - 1, 0> result;
        result.value = value.template slice<target_size>(a, b);
        result.xz_mask = xz_mask.template slice<target_size>(a, b);
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

    template <int arg0_msb, int arg0_lsb, bool arg0_signed>
    constexpr auto concat(const bit<arg0_msb, arg0_lsb, arg0_signed> &arg0) {
        auto constexpr final_size = size + bit<arg0_msb, arg0_lsb, arg0_signed>::size;
        auto res = logic<final_size - 1, 0, signed_>();
        res.value = value.concat(arg0);
        // concat masks as well
        res.xz_mask = xz_mask.template extend<final_size>();
        return res;
    }

    template <typename U, typename... Ts>
    constexpr auto concat(U arg0, Ts... args) {
        return this->concat(arg0).concat(args...);
    }

    /*
     * bit unpacking
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

    template <int op_msb, int op_lsb, bool op_signed>
    requires((util::abs_diff(op_msb, op_lsb) + 1) != size) auto operator&(
        const bit<op_msb, op_lsb, op_signed> &op) const {
        return this->template operator&(logic(op));
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

    template <int op_msb, int op_lsb, bool op_signed>
    requires((util::abs_diff(op_msb, op_lsb) + 1) != size) auto operator|(
        const bit<op_msb, op_lsb, op_signed> &op) const {
        return this->template operator|(logic(op));
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

    template <int op_msb, int op_lsb, bool op_signed>
    requires((util::abs_diff(op_msb, op_lsb) + 1) != size) auto operator^(
        const bit<op_msb, op_lsb, op_signed> &op) const {
        return this->template operator^(logic(op));
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

    auto &operator++() {
        (*this) = (*this) + one_();
        return *this;
    }

    auto &operator--() {
        (*this) = (*this) - one_();
        return *this;
    }

    // NOLINTNEXTLINE
    logic<size - 1, 0, signed_> operator++(int) {
        logic<size - 1, 0, signed_> v = *this;
        this->operator++();
        return v;
    }

    // NOLINTNEXTLINE
    logic<size - 1, 0, signed_> operator--(int) {
        logic<size - 1, 0, signed_> v = *this;
        this->operator--();
        return v;
    }

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
    requires((util::abs_diff(op_msb, op_lsb) + 1) != size) auto operator>>(
        const bit<op_msb, op_lsb, op_signed> &op) const {
        return this->template operator>>(logic(op));
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
    requires((util::abs_diff(op_msb, op_lsb) + 1) != size) auto operator<<(
        const bit<op_msb, op_lsb, op_signed> &op) const {
        return this->template operator<<(logic(op));
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

    template <int op_msb, int op_lsb, bool op_signed>
    auto ashl(const bit<op_msb, op_lsb, op_signed> &amount) const {
        return this->template ashl(logic(amount));
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
    logic<0> operator==(const bit<op_msb, op_lsb, op_signed> &target) const {
        return this->template operator==(logic(target));
    }

    template <int op_msb, int op_lsb, bool op_signed>
    logic<0> operator!=(const logic<op_msb, op_lsb, op_signed> &target) const {
        if (xz_mask.any_set() || target.xz_mask.any_set()) return x_();
        return value != target.value ? one_() : zero_();
    }

    template <int op_msb, int op_lsb, bool op_signed>
    logic<0> operator!=(const bit<op_msb, op_lsb, op_signed> &target) const {
        return this->template operator!=(logic(target));
    }

    template <int op_msb, int op_lsb, bool op_signed>
    [[nodiscard]] bool match(const logic<op_msb, op_lsb, op_signed> &op) const {
        return value == op.value && xz_mask == op.xz_mask;
    }

    template <int op_msb, int op_lsb, bool op_signed>
    [[nodiscard]] bool match(const bit<op_msb, op_lsb, op_signed> &op) const {
        return this->template match(logic(op));
    }

    template <int op_msb, int op_lsb, bool op_signed>
    [[nodiscard]] bool nmatch(const logic<op_msb, op_lsb, op_signed> &op) const {
        return !match(op);
    }

    template <int op_msb, int op_lsb, bool op_signed>
    [[nodiscard]] bool nmatch(const bit<op_msb, op_lsb, op_signed> &op) const {
        return !match(op);
    }

    template <int op_msb, int op_lsb, bool op_signed>
    logic<0> operator>(const logic<op_msb, op_lsb, op_signed> &target) const {
        if (xz_mask.any_set() || target.xz_mask.any_set()) return x_();
        return value > target.value ? one_() : zero_();
    }

    template <int op_msb, int op_lsb, bool op_signed>
    logic<0> operator>(const bit<op_msb, op_lsb, op_signed> &target) const {
        return this->template operator>(logic(target));
    }

    template <int op_msb, int op_lsb, bool op_signed>
    logic<0> operator<(const logic<op_msb, op_lsb, op_signed> &target) const {
        if (xz_mask.any_set() || target.xz_mask.any_set()) return x_();
        return value < target.value ? one_() : zero_();
    }

    template <int op_msb, int op_lsb, bool op_signed>
    logic<0> operator<(const bit<op_msb, op_lsb, op_signed> &target) const {
        return this->template operator<(logic(target));
    }

    template <int op_msb, int op_lsb, bool op_signed>
    logic<0> operator>=(const logic<op_msb, op_lsb, op_signed> &target) const {
        if (xz_mask.any_set() || target.xz_mask.any_set()) return x_();
        return value >= target.value ? one_() : zero_();
    }

    template <int op_msb, int op_lsb, bool op_signed>
    logic<0> operator>=(const bit<op_msb, op_lsb, op_signed> &target) const {
        return this->template operator>=(logic(target));
    }

    template <int op_msb, int op_lsb, bool op_signed>
    logic<0> operator<=(const logic<op_msb, op_lsb, op_signed> &target) const {
        if (xz_mask.any_set() || target.xz_mask.any_set()) return x_();
        return value <= target.value ? one_() : zero_();
    }

    template <int op_msb, int op_lsb, bool op_signed>
    logic<0> operator<=(const bit<op_msb, op_lsb, op_signed> &target) const {
        return this->template operator<=(logic(target));
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

    template <int op_msb, int op_lsb, bool op_signed>
    auto operator+(const bit<op_msb, op_lsb, op_signed> &target) const {
        return this->template operator+(logic(target));
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

    template <int op_msb, int op_lsb, bool op_signed>
    auto operator-(const bit<op_msb, op_lsb, op_signed> &target) const {
        return this->template operator-(logic(target));
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

    template <int op_msb, int op_lsb, bool op_signed>
    auto operator*(const bit<op_msb, op_lsb, op_signed> &target) const {
        return this->template operator*(logic(target));
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

    template <int op_msb, int op_lsb, bool op_signed>
    auto operator/(const bit<op_msb, op_lsb, op_signed> &target) const {
        return this->template operator/(logic(target));
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

    template <int op_msb, int op_lsb, bool op_signed>
    auto operator%(const bit<op_msb, op_lsb, op_signed> &target) const {
        return this->template operator%(logic(target));
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

    [[nodiscard]] constexpr logic<size - 1, 0, true> to_signed() const {
        logic<size - 1, 0, true> result;
        result.value = value.to_signed();
        result.xz_mask = xz_mask;
        return result;
    }

    [[nodiscard]] constexpr logic<size - 1, 0, false> to_unsigned() const {
        logic<size - 1, 0, false> result;
        result.value = value.to_unsigned();
        result.xz_mask = xz_mask;
        return result;
    }

    /*
     * update using slice
     */
    // setting values
    template <int hi, int lo = hi, int op_hi, int op_lo, bool op_signed>
    requires(!array) void update(const logic<op_hi, op_lo, op_signed> &op) requires(
        hi < size && lo < size && util::match_endian(hi, lo, msb, lsb)) {
        this->template update_<hi, lo>(op);
    }

    /*
     * value conversions
     */
    [[nodiscard]] uint64_t to_uint64() const {
        if (xz_mask.any_set())
            return 0;
        else
            return value.to_uint64();
    }

    // automatic conversion
    [[nodiscard]] int8_t to_num() const requires(signed_ &&size <= 8) {
        if (xz_mask.any_set())
            return 0;
        else
            return value.to_num();
    }

    [[nodiscard]] uint8_t to_num() const requires(!signed_ && size <= 8) {
        if (xz_mask.any_set())
            return 0;
        else
            return value.to_num();
    }

    [[nodiscard]] int16_t to_num() const requires(signed_ &&size > 8 && size <= 16) {
        if (xz_mask.any_set())
            return 0;
        else
            return value.to_num();
    }

    [[nodiscard]] uint16_t to_num() const requires(!signed_ && size > 8 && size <= 16) {
        if (xz_mask.any_set())
            return 0;
        else
            return value.to_num();
    }

    [[nodiscard]] int32_t to_num() const requires(signed_ &&size > 16 && size <= 32) {
        if (xz_mask.any_set())
            return 0;
        else
            return value.to_num();
    }

    [[nodiscard]] uint32_t to_num() const requires(!signed_ && size > 16 && size <= 32) {
        if (xz_mask.any_set())
            return 0;
        else
            return value.to_num();
    }

    [[nodiscard]] int64_t to_num() const requires(signed_ &&size > 32 && size <= 64) {
        if (xz_mask.any_set())
            return 0;
        else
            return value.to_num();
    }

    [[nodiscard]] uint64_t to_num() const requires(!signed_ && size > 32 && size <= 64) {
        if (xz_mask.any_set())
            return 0;
        else
            return value.to_num();
    }

    /*
     * constructors
     */
    // by default everything is x
    constexpr logic() { xz_mask.mask(); }

    template <typename T>
    constexpr logic(T value) requires(std::is_arithmetic_v<T>)  // NOLINT
        : value(bit<msb, lsb, signed_>(value)) {}

    explicit logic(const char *str) : logic(std::string_view(str)) {}
    explicit constexpr logic(std::string_view v) {
        if constexpr (util::native_num(size)) {
            value.value = util::parse_raw_str(v);
            xz_mask.value = util::parse_xz_raw_str(v);
        } else {
            util::parse_raw_str(v, size, value.value.values.data());
            util::parse_xz_raw_str(v, size, xz_mask.value.values.data());
        }
    }

    // conversion from bit to logic
    constexpr logic(const bit<msb, lsb, signed_> &b) : value(b) {}

    // shifting msb and lsb
    template <int new_msb, int new_lsb, bool new_signed>
    requires(util::abs_diff(new_msb, new_lsb) ==
             util::abs_diff(msb,
                            lsb)) explicit logic(const logic<new_msb, new_lsb, new_signed> &target)
        : value(target.value), xz_mask(target.xz_mask) {}

    // implicit conversion via assignment
    template <int new_msb, int new_lsb, bool new_signed>
    logic &operator=(const logic<new_msb, new_lsb, new_signed> &b) {
        value = b.value;
        xz_mask = b.xz_mask;
        return *this;
    }

    template <typename T>
    logic &operator=(T v) requires(std::is_arithmetic_v<T>) {
        value = v;
        xz_mask = 0;
        return *this;
    }

    template <int new_msb, int new_lsb>
    constexpr logic(const logic<new_msb, new_lsb, true> &b) {  // NOLINT
        value = b.value;
        xz_mask = b.xz_mask;
    }

    // useful constants
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

    template <bool l_signed>
    void set_(uint64_t idx, const logic<0, 0, l_signed> &l) {
        value.set(idx, l.value.value);
        xz_mask.set(idx, l.xz_mask.value);
    }

protected:
    template <int a, int b>
    requires(util::max(a, b) < size) constexpr logic<util::abs_diff(a, b)> inline slice_() const {
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

    template <int hi, int lo = hi, int op_hi, int op_lo, bool op_signed>
    void update_(const logic<op_hi, op_lo, op_signed> &op) requires(hi < size && lo < size &&
                                                                    util::match_endian(hi, lo, msb,
                                                                                       lsb)) {
        auto constexpr start = util::min(hi, lo);
        auto constexpr end = util::max(hi, lo) + 1;
        for (auto i = start; i < end; i++) {
            // notice that if it is out of range, 0 will be returned
            // we need to revert the index accessing scheme here, if the endian doesn't match
            uint64_t idx;
            if constexpr (logic<op_hi, op_lo, op_signed>::big_endian ^ big_endian) {
                idx = util::max(op_lo, op_hi) - i;
            } else {
                idx = i;
            }
            idx = idx - start;
            logic<0, 0> b;
            if (idx > util::max(op_lo, op_hi) || idx < util::min(op_lo, op_hi)) {
                // out of bound access, set to 0
                b = zero_();
            } else {
                b = op.operator[](idx);
            }
            set_(i, b);
        }
    }

    template <int op_hi, int op_lo, bool op_signed>
    void update_(int hi, int lo, const logic<op_hi, op_lo, op_signed> &op) {
        auto start = util::min(hi, lo);
        auto end = util::max(hi, lo) + 1;
        for (auto i = start; i < end; i++) {
            // notice that if it is out of range, 0 will be returned
            // we need to revert the index accessing scheme here, if the endian doesn't match
            uint64_t idx;
            if constexpr (logic<op_hi, op_lo, op_signed>::big_endian ^ big_endian) {
                idx = util::max(op_lo, op_hi) - i;
            } else {
                idx = i;
            }
            idx = idx - start;
            logic<0, 0> b;
            if (idx > util::max(op_lo, op_hi) || idx < util::min(op_lo, op_hi)) {
                // out of bound access, set to 0
                b = zero_();
            } else {
                b = op.operator[](idx);
            }
            set_(i, b);
        }
    }
};

inline namespace literals {
// constexpr to parse SystemVerilog literals
// per LRM, if size not specified, the default size is 32
//     it will be signed as well
constexpr logic<31, 0, true> operator""_logic(unsigned long long value) {
    return logic<31, 0, true>(static_cast<int32_t>(value)).extend<32>();
}
constexpr logic<63, 0, true> operator""_logic64(unsigned long long value) {
    return logic<63, 0, true>(static_cast<int64_t>(value)).extend<64>();
}
}  // namespace literals

// helper functions
template <typename U, typename... Ts>
auto concat(U arg0, Ts... args) {
    return arg0.concat(args...);
}

}  // namespace logic
#endif  // LOGIC_LOGIC_HH
