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

template <uint64_t size, bool signed_ = false, bool big_endian = true>
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

constexpr uint64_t abs_diff(uint64_t a, uint64_t b) { return (a > b) ? (a - b) : (b - a); }

constexpr bool native_num(uint64_t size) { return size <= big_num_threshold; }

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

template <uint64_t size, bool signed_, bool big_endian>
struct big_num {
public:
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

    template <uint64_t a, uint64_t b>
    requires(util::max(a, b) < size) big_num<util::abs_diff(a, b) + 1, false>
    inline slice() const {
        if constexpr (size <= util::max(a, b)) {
            // out of bound access
            return bits<util::abs_diff(a, b) + 1, false, big_endian>(0);
        }

        big_num<util::abs_diff(a, b) + 1, false, big_endian> res;
        constexpr auto max = util::max(a, b);
        constexpr auto min = util::min(a, b);
        for (uint64_t i = min; i <= max; i++) {
            auto idx = i - min;
            res.set(idx, this->operator[](i));
        }
        return res;
    }

    template <uint64_t new_size>
    requires(new_size > size) big_num<new_size, signed_> extend()
    const {
        big_num<new_size, signed_, big_endian> result;
        util::copy(result.values, values, 0, s);
        // based on the sign, need to fill out leading ones
        if constexpr (signed_) {
            for (uint64_t i = size; i < new_size; i++) {
                set<i>(true);
            }
        }
    }

    // doesn't matter about the sign, but endianness must match
    template <uint64_t ss, bool num_signed>
    auto concat(const big_num<ss, num_signed, big_endian> &num) const {
        auto constexpr final_size = size + ss;
        big_num<final_size, false, big_endian> result;
        if constexpr (big_endian) {
            // copy over the num one since it's on the LSB
            for (auto i = 0u; i < num.s; i++) {
                result.values[i] = num.values[i];
            }
            // then copy over the current on
            for (auto i = ss; i < final_size; i++) {
                auto idx = i - ss;
                result.set(i, get(idx));
            }
        } else {
            // little endian
            for (auto i = 0u; i < s; i++) {
                result.values[i] = values[i];
            }
            // copy over the new one
            for (auto i = size; i < final_size; i++) {
                auto idx = i - size;
                result.set(i, get(idx));
            }
        }

        return result;
    }

    [[maybe_unused]] [[nodiscard]] uint64_t popcount() const {
        return std::accumulate(values.begin(), values.end(), std::popcount);
    }

    /*
     * boolean operators
     */
    explicit operator bool() const { return any_set(); }

    /*
     * bitwise operators
     */
    big_num<size, signed_, big_endian> operator~() const {
        big_num<size, signed_, big_endian> result;
        for (uint i = 0; i < s; i++) {
            result.values[i] = ~values[i];
        }
        // mask off the top bits, if any
        for (auto i = size; i < s * big_num_threshold; i++) {
            set(i, false);
        }
        return result;
    }

    big_num<size, signed_, big_endian> operator&(
        const big_num<size, signed_, big_endian> &op) const {
        big_num<size, signed_, big_endian> result;
        for (uint i = 0; i < s; i++) {
            result.values[i] = values[i] & op.values[i];
        }
        return result;
    }

    big_num<size, signed_, big_endian> &operator&=(
        const big_num<size, signed_, big_endian> &op) const {
        for (uint i = 0; i < s; i++) {
            values[i] &= op.values[i];
        }
        return *this;
    }

    big_num<size, signed_, big_endian> operator^(
        const big_num<size, signed_, big_endian> &op) const {
        big_num<size, signed_, big_endian> result;
        for (uint i = 0; i < s; i++) {
            result.values[i] = values[i] ^ op.values[i];
        }
        return result;
    }

    big_num<size, signed_, big_endian> &operator^=(
        const big_num<size, signed_, big_endian> &op) const {
        for (uint i = 0; i < s; i++) {
            values[i] ^= op.values[i];
        }
        return *this;
    }

    big_num<size, signed_, big_endian> operator|(
        const big_num<size, signed_, big_endian> &op) const {
        big_num<size, signed_, big_endian> result;
        for (uint i = 0; i < s; i++) {
            result.values[i] = values[i] | op.values[i];
        }
        return result;
    }

    big_num<size, signed_, big_endian> &operator|=(
        const big_num<size, signed_, big_endian> &op) const {
        for (uint i = 0; i < s; i++) {
            values[i] |= op.values[i];
        }
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

    /*
     * mask related stuff
     */
    [[nodiscard]] bool any_set() const {
        // we use the fact that SIMD instructions are faster than compare and branch
        // so summation is faster than any_of in theory
        // also all unused bits are set to 0 by default
        auto r = std::accumulate(values.begin(), values.end(), 0);
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

    // constructors
    explicit big_num(std::string_view v) {
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
    [[maybe_unused]] explicit big_num(T v) : values({v}) {}

    // set values to 0 when initialized
    big_num() { std::fill(values.begin(), values.end(), 0); }
};

template <uint64_t size, bool signed_ = false, bool big_endian = true>
struct logic;

template <uint64_t size, bool signed_ = false, bool big_endian = true>
struct bits {
private:
public:
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
            return value[idx];
        }
    }

    template <uint64_t idx>
    requires(idx < size && native_num) [[nodiscard]] bool inline get() const {
        return (value >> idx) & 1;
    }

    void inline set(uint64_t idx, bool v) {
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
        if constexpr (idx < size) {
            if (v) {
                value |= 1ull << idx;
            } else {
                value &= ~(1ull << idx);
            }
        } else {
            value.template set<idx>(v);
        }
    }

    template <uint64_t idx, bool v>
    void set() {
        if constexpr (idx < size) {
            if constexpr (v) {
                value |= 1ull << idx;
            } else {
                value &= ~(1ull << idx);
            }
        } else {
            value.template set<idx, v>();
        }
    }

    /*
     * Slice always produce unsigned result
     */
    /*
     * native holder always produce native numbers
     */
    template <uint64_t a, uint64_t b>
    requires(util::max(a, b) < size && native_num) bits<util::abs_diff(a, b) + 1, false>
    constexpr inline slice() const {
        // assume the import has type-checked properly, e.g. by a compiler
        constexpr auto max = util::max(a, b);
        constexpr auto min = util::min(a, b);
        constexpr auto t_size = sizeof(T) * 8;
        bits<util::abs_diff(a, b) + 1, false, big_endian> result;
        constexpr auto default_mask = std::numeric_limits<T>::max();
        uint64_t mask = default_mask << min;
        mask &= (default_mask >> (t_size - max - 1));
        result.value = (value & mask) >> min;

        return result;
    }

    /*
     * big number but small slice
     */
    template <uint64_t a, uint64_t b>
    requires(
        util::max(a, b) < size && !native_num &&
        util::native_num(util::abs_diff(a, b) +
                         1)) constexpr bits<util::abs_diff(a, b) + 1, false> inline slice() const {
        bits<util::abs_diff(a, b) + 1, false, big_endian> result;
        auto res = value.template slice<a, b>();
        // need to shrink it down
        result.value = res.values[0];
        return result;
    }

    /*
     * big number and big slice
     */
    template <uint64_t a, uint64_t b>
    requires(
        util::max(a, b) < size && !native_num &&
        !util::native_num(util::abs_diff(a, b) +
                          1)) constexpr bits<util::abs_diff(a, b) + 1, false> inline slice() const {
        bits<util::abs_diff(a, b) + 1, false, big_endian> result;
        result.value = value.template slice<a, b>();
        return result;
    }

    // extension
    // notice that we need to implement signed extension, for the result that's native holder
    // C++ will handle that for us already.
    template <uint64_t new_size>
    requires(new_size > size &&
             util::native_num(new_size)) bits<new_size, signed_, big_endian> extend()
    const { return bits<new_size, signed_, big_endian>(value); }

    // same size, just use the default copy constructor
    template <uint64_t new_size>
    requires(new_size == size) bits<new_size, signed_, big_endian> extend()
    const { return *this; }

    // new size is smaller. this is just a slice
    template <uint64_t new_size>
    requires(new_size < size) bits<new_size, signed_, big_endian> extend()
    const { return slice<new_size - 1, 0>(); }

    // native number, but extend to a big number
    template <uint64_t new_size>
    requires(new_size > size && !util::native_num(new_size) &&
             native_num) bits<new_size, signed_, big_endian> extend()
    const {
        bits<new_size, signed_, big_endian> result;
        // use C++ default semantics to cast
        result.value.values[0] = static_cast<big_num_holder_type>(value);
        if constexpr (signed_) {
            for (auto i = 1; i < result.value.s; i++) {
                // sign extension
                result.value.valuies[1] = std::numeric_limits<big_num_holder_type>::max();
            }
        }
        // if not we're fine since we set everything to zero when we start
        return result;
    }

    // big number and gets extended to a big number
    template <uint64_t new_size>
    requires(new_size > size && !util::native_num(new_size) &&
             !native_num) bits<new_size, signed_, big_endian> extend()
    const { return value.template extend<new_size>(); }

    /*
     * concatenation
     */
    template <uint64_t arg0_size>
    bits<size + arg0_size, false, big_endian> concat(const bits<arg0_size> &arg0) {
        auto constexpr final_size = size + arg0_size;
        if constexpr (final_size < big_num_threshold) {
            return bits<final_size, false, big_endian>(value << arg0_size | arg0.value);
        } else {
            auto res = bits<final_size>();
            if constexpr (arg0.native_num) {
                // convert to big num
                big_num<arg0_size, false, big_endian> b(arg0.value);
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

    /*
     * bitwise operators
     */
    bits<size, signed_, big_endian> operator~() const {
        bits<size, signed_, big_endian> result;
        if constexpr (size == 1) {
            result.value = !value;
        } else {
            result.value = ~value;
        }
        return result;
    }

    bits<size, signed_, big_endian> operator&(const bits<size, signed_, big_endian> &op) const {
        bits<size, signed_, big_endian> result;
        result.value = value & op.value;
        return result;
    }

    bits<size, signed_, big_endian> &operator&=(const bits<size, signed_, big_endian> &op) {
        bits<size, signed_, big_endian> result;
        value &= op.value;
        return *this;
    }

    bits<size, signed_, big_endian> operator^(const bits<size, signed_, big_endian> &op) const {
        bits<size, signed_, big_endian> result;
        result.value = value ^ op.value;
        return result;
    }

    bits<size, signed_, big_endian> &operator^=(const bits<size, signed_, big_endian> &op) {
        bits<size, signed_, big_endian> result;
        value ^= op.value;
        return *this;
    }

    bits<size, signed_, big_endian> operator|(const bits<size, signed_, big_endian> &op) const {
        bits<size, signed_, big_endian> result;
        result.value = value | op.value;
        return result;
    }

    bits<size, signed_, big_endian> &operator|=(const bits<size, signed_, big_endian> &op) {
        bits<size, signed_, big_endian> result;
        value |= op.value;
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
    constexpr explicit bits(const char *str) : bits(std::string_view(str)) {}
    explicit bits(std::string_view v) {
        if constexpr (size <= big_num_threshold) {
            // normal number
            value = 0;
            auto iter = v.rbegin();
            for (auto i = 0u; i < size; i++) {
                // from right to left
                auto c = *iter;
                if (c == '1') {
                    value |= 1u << i;
                }
                iter++;
                if (iter == v.rend()) break;
            }
        } else {
            // big num
            value = big_num<size>(v);
        }
    }

    explicit constexpr bits(T v) requires(util::native_num(size)) : value(v) {}
    template <typename K>
    requires(std::is_arithmetic_v<K> && !util::native_num(size)) explicit constexpr bits(K v)
        : value(v) {}

    bits() {
        if constexpr (native_num) {
            // init to 0
            value = 0;
        }
        // for big number it's already set to 0
    }

private:
    /*
     * unpacking, which is basically slicing as syntax sugars
     */
    template <uint64_t base, uint64_t arg0_size, bool arg0_signed>
    requires(base < size) void unpack_(bits<arg0_size, arg0_signed, big_endian> &target) const {
        auto constexpr upper_bound = util::min(size - 1, arg0_size + base - 1);
        target.value = this->template slice<base, upper_bound>();
    }

    template <uint64_t base = 0, uint64_t arg0_size, bool arg0_signed, typename... Ts>
    [[maybe_unused]] auto unpack_(bits<arg0_size, arg0_signed, big_endian> &bit0,
                                  Ts &...args) const {
        this->template unpack_<base, arg0_size>(bit0);
        this->template unpack_<base + arg0_size>(args...);
    }
};

template <uint64_t size, bool signed_, bool big_endian>
struct logic {
    using T = typename util::get_holder_type<size, signed_>::type;
    bits<size, signed_, big_endian> value;
    // To reduce memory footprint, we use the following encoding scheme
    // if xz_mask is off, value is the actual integer value
    // if xz_mask is on, 0 in value means x and 1 means z
    bits<size, false, big_endian> xz_mask;

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
    inline logic<1> operator[](uint64_t idx) const {
        if (idx < size) [[likely]] {
            logic<1> r;
            if (x_set(idx)) [[unlikely]] {
                r.set_x(idx);
            } else if (z_set(idx)) [[unlikely]] {
                r.set_z(idx);
            } else {
                r.value = value[idx];
            }
            return r;
        } else {
            return logic<1>(false);
        }
    }

    template <uint64_t idx>
    requires(idx < size) [[nodiscard]] inline logic<1> get() const {
        logic<1> r;
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

    void set(uint64_t idx, const logic<1> &l) {
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
    template <uint64_t a, uint64_t b>
    requires(
        util::max(a, b) <
        size) constexpr logic<util::abs_diff(a, b) + 1, false, big_endian> inline slice() const {
        if constexpr (size <= util::max(a, b)) {
            // out of bound access
            return logic<util::abs_diff(a, b) + 1, false, big_endian>(0);
        }

        constexpr auto result_size = util::abs_diff(a, b) + 1u;
        logic<result_size, false, big_endian> result;
        result.value = value.template slice<a, b>();
        // copy over masks
        result.xz_mask = xz_mask.template slice<a, b>();

        return result;
    }

    /*
     * concatenation
     */
    template <uint64_t arg0_size>
    constexpr logic<size + arg0_size, false, big_endian> concat(const logic<arg0_size> &arg0) {
        auto constexpr final_size = size + arg0_size;
        auto res = logic<final_size, false, big_endian>();
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

    logic<1> operator!() const {
        return xz_mask.any_set() ? x_() : (value.any_set() ? zero_() : one_());
    }

    /*
     * bitwise operators
     */
    logic<size, signed_, big_endian> operator&(const logic<size, signed_, big_endian> &op) const {
        // this is the truth table
        //   0 1 x z
        // 0 0 0 0 0
        // 1 0 1 x x
        // x 0 x x x
        // z 0 x x x
        logic<size, signed_, big_endian> result;
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

    logic<size, signed_, big_endian> operator&=(const logic<size, signed_, big_endian> &op) {
        auto const res = (*this) & op;
        this->value = res.value;
        this->xz_mask = res.xz_mask;
        return *this;
    }

    logic<size, signed_, big_endian> operator|(const logic<size, signed_, big_endian> &op) const {
        // this is the truth table
        //   0 1 x z
        // 0 0 1 x x
        // 1 1 1 1 1
        // x x 1 x x
        // z x 1 x x
        logic<size, signed_, big_endian> result;
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

    logic<size, signed_, big_endian> operator|=(const logic<size, signed_, big_endian> &op) {
        auto const res = (*this) | op;
        this->value = res.value;
        this->xz_mask = res.xz_mask;
        return *this;
    }

    logic<size, signed_, big_endian> operator^(const logic<size, signed_, big_endian> &op) const {
        // this is the truth table
        //   0 1 x z
        // 0 1 0 x x
        // 1 0 1 x x
        // x x x x x
        // z x x x x
        logic<size, signed_, big_endian> result;
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

    logic<size, signed_, big_endian> operator^=(const logic<size, signed_, big_endian> &op) {
        auto const res = (*this) ^ op;
        this->value = res.value;
        this->xz_mask = res.xz_mask;
        return *this;
    }

    logic<size, signed_, big_endian> operator~() const {
        // this is the truth table
        //   0 1 x z
        //   1 0 x x
        logic<size, signed_, big_endian> result;
        result.value = ~value;
        result.xz_mask = xz_mask;
        // change anything has xz_mask on to x
        result.value &= ~result.xz_mask;
        return result;
    }

    // reduction
    [[nodiscard]] logic<1> r_and() const {
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

    [[maybe_unused]] [[nodiscard]] logic<1> r_nand() const { return !r_and(); }

    [[nodiscard]] logic<1> r_or() const {
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

    [[nodiscard]] logic<1> r_nor() const { return !r_or(); }

    [[nodiscard]] logic<1> r_xor() const {
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

    [[nodiscard]] logic<1> r_xnor() const { return !r_xor(); }

    /*
     * constructors
     */
    // by default everything is x
    constexpr logic() { xz_mask.mask(); }

    explicit constexpr logic(T value) requires(size <= big_num_threshold)
        : value(bits<size>(value)) {}

    template <typename K>
    requires(std::is_arithmetic_v<K> &&size > big_num_threshold) explicit constexpr logic(K value)
        : value(bits<size>(value)) {}

    explicit logic(const char *str) : logic(std::string_view(str)) {}
    explicit logic(std::string_view v) : value(bits<size>(v)) {
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
    constexpr explicit logic(const bits<size, signed_> &b) : value(b) {}

private:
    void unmask_bit(uint64_t idx) { xz_mask.set(idx, false); }

    template <uint64_t idx>
    void unmask_bit() {
        xz_mask.template set<idx, false>();
    }

    /*
     * unpacking, which is basically slicing as syntax sugars
     */
    template <uint64_t base, uint64_t arg0_size, bool arg0_signed>
    requires(base < size) void unpack_(logic<arg0_size, arg0_signed, big_endian> &target) const {
        auto constexpr upper_bound = util::min(size - 1, arg0_size + base - 1);
        target.value = value.template slice<base, upper_bound>();
        target.xz_mask = xz_mask.template slice<base, upper_bound>();
    }

    template <uint64_t base = 0, uint64_t arg0_size, bool arg0_signed, typename... Ts>
    [[maybe_unused]] auto unpack_(logic<arg0_size, arg0_signed, big_endian> &logic0,
                                  Ts &...args) const {
        this->template unpack_<base, arg0_size>(logic0);
        this->template unpack_<base + arg0_size>(args...);
    }

    static constexpr logic<1> x_() {
        logic<1> r;
        r.value.value = false;
        r.xz_mask.value = true;
        return r;
    }

    static constexpr logic<1> one_() {
        logic<1> r;
        r.value.value = true;
        r.xz_mask.value = false;
        return r;
    }

    static constexpr logic<1> zero_() {
        logic<1> r;
        r.value.value = false;
        r.xz_mask.value = false;
        return r;
    }
};

inline namespace literals {
// constexpr to parse SystemVerilog literals
// per SV standard, if size not specified, the default size is 32
constexpr logic<32> operator""_logic(unsigned long long value) { return logic<32>(value); }
constexpr logic<64> operator""_logic64(unsigned long long value) { return logic<64>(value); }
}  // namespace literals

}  // namespace logic
#endif  // LOGIC_LOGIC_HH
