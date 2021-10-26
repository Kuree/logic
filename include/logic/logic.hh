#ifndef LOGIC_LOGIC_HH
#define LOGIC_LOGIC_HH

#include <algorithm>
#include <array>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>
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
    for (auto i = 0; i <= end - start; i++) {
        dst.set(i, src[i + start]);
    }
}

static constexpr bool gte(uint64_t a, uint64_t min) { return a >= min; }

constexpr uint64_t abs_diff(uint64_t a, uint64_t b) { return (a > b) ? (a - b) : (b - a); }

constexpr bool native_num(uint64_t size) { return size <= big_num_threshold; }

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
            return bits<util::abs_diff(a, b) + 1, false>(0);
        }

        big_num<util::abs_diff(a, b) + 1> res;
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
        big_num<new_size, signed_> result;
        util::copy(result.values, values, 0, s);
        // based on the sign, need to fill out leading ones
        if constexpr (signed_) {
            for (uint64_t i = size; i < new_size; i++) {
                set<i>(true);
            }
        }
    }

    template <uint64_t ss>
    auto concat(const big_num<ss> &num) const {
        auto constexpr final_size = size + ss;
        big_num<final_size> result;
        // copy over the num one since it's on the LSB
        // TODO: refactor code to support big/little endianness
        for (auto i = 0u; i < num.s; i++) {
            result.values[i] = num.values[i];
        }
        // then copy over the current on
        for (auto i = ss; i < final_size; i++) {
            auto idx = i - ss;
            result.set(i, get(i));
        }

        return result;
    }

    /*
     * mask related stuff
     */
    [[nodiscard]] bool has_set() const {
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

template <uint64_t size, bool signed_ = false>
struct logic;

template <uint64_t size, bool signed_ = false>
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
    inline slice() const {
        // assume the import has type-checked properly, e.g. by a compiler
        constexpr auto max = util::max(a, b);
        constexpr auto min = util::min(a, b);
        bits<util::abs_diff(a, b) + 1, false> result;
        auto const default_mask = std::numeric_limits<T>::max();
        auto mask = default_mask << min;
        mask &= (default_mask >> (size - max));
        result.value = value & mask;
        result.value >>= min;

        return result;
    }

    /*
     * big number but small slice
     */
    template <uint64_t a, uint64_t b>
    requires(util::max(a, b) < size && !native_num &&
             util::native_num(util::abs_diff(a, b) + 1)) bits<util::abs_diff(a, b) + 1, false>
    inline slice() const {
        bits<util::abs_diff(a, b) + 1, false> result;
        auto res = value.template slice<a, b>();
        // need to shrink it down
        result.value = res.values[0];
        return result;
    }

    /*
     * big number and big slice
     */
    template <uint64_t a, uint64_t b>
    requires(util::max(a, b) < size && !native_num &&
             !util::native_num(util::abs_diff(a, b) + 1)) bits<util::abs_diff(a, b) + 1, false>
    inline slice() const {
        bits<util::abs_diff(a, b) + 1, false> result;
        result.value = value.template slice<a, b>();
        return result;
    }

    // extension
    // notice that we need to implement signed extension, for the result that's native holder
    // C++ will handle that for us already.
    template <uint64_t new_size>
    requires(new_size > size && util::native_num(new_size)) bits<new_size> extend()
    const { return bits<new_size>(value); }

    // same size, just use the default copy constructor
    template <uint64_t new_size>
    requires(new_size == size) bits<new_size> extend()
    const { return *this; }

    // new size is smaller. this is just a slice
    template <uint64_t new_size>
    requires(new_size < size) bits<new_size> extend()
    const { return slice<new_size - 1, 0>(); }

    // native number, but extend to a big number
    template <uint64_t new_size>
    requires(new_size > size && !util::native_num(new_size) && native_num) bits<new_size> extend()
    const {
        bits<new_size> result;
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
    requires(new_size > size && !util::native_num(new_size) && !native_num) bits<new_size> extend()
    const {
        bits<new_size> result;

        return result;
    }

    /*
     * concatenation
     */
    template <uint64_t arg0_size>
    bits<size + arg0_size> concat(const bits<arg0_size> &arg0) {
        auto constexpr final_size = size + arg0_size;
        if constexpr (native_num) {
            return bits<final_size>(value << arg0_size | arg0.value);
        } else {
            auto res = bits<final_size>();
            if constexpr (arg0.native_num) {
                // convert to big num
                big_num<arg0_size> b(arg0.value);
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
};

template <uint64_t size, bool signed_>
struct logic {
    using T = typename util::get_holder_type<size, signed_>::type;
    bits<size> value;
    // by default every thing is x
    bits<size> x_mask;
    bits<size> z_mask;

    // basic formatting
    [[nodiscard]] std::string str() const {
        std::stringstream ss;
        for (auto i = 0; i < size; i++) {
            uint64_t idx = size - i - 1;
            if (x_mask[idx]) {
                ss << 'x';
            } else if (z_mask[idx]) {
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
            if (x_mask[idx]) [[unlikely]] {
                r.x_mask.set(idx, true);
            } else if (z_mask[idx]) [[unlikely]] {
                r.z_mask.set(idx, true);
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
        if (x_mask.template get<idx>()) [[unlikely]] {
            r.x_mask.set<idx, true>();
        } else if (z_mask.template get<idx>) [[unlikely]] {
            r.z_mask.set<idx, true>();
        } else {
            r.value = value.template get<idx>();
        }
        return r;
    }

    /*
     * slices
     */
    template <uint64_t a, uint64_t b>
    requires(util::max(a, b) < size) logic<util::abs_diff(a, b) + 1, false>
    inline slice() const {
        if constexpr (size <= util::max(a, b)) {
            // out of bound access
            return logic<util::abs_diff(a, b) + 1, false>(0);
        }

        constexpr auto result_size = util::abs_diff(a, b) + 1u;
        logic<result_size, false> result;
        result.value = value.template slice<a, b>();
        // copy over masks
        result.x_mask = x_mask.template slice<a, b>();
        result.z_mask = z_mask.template slice<a, b>();

        return result;
    }

    /*
     * concatenation
     */
    template <uint64_t arg0_size>
    logic<size + arg0_size> concat(const logic<arg0_size> &arg0) {
        auto constexpr final_size = size + arg0_size;
        auto res = logic<final_size>();
        res.value = value.concat(arg0.value);
        // concat masks as well
        res.x_mask = x_mask.concat(arg0.x_mask);
        res.z_mask = z_mask.concat(arg0.z_mask);
        return res;
    }

    template <typename U, typename... Ts>
    auto concat(U arg0, Ts... args) {
        return concat(arg0).concat(args...);
    }

    /*
     * constructors
     */
    // by default everything is x
    logic() { x_mask.mask(); }

    explicit constexpr logic(T value) requires(size <= big_num_threshold)
        : value(bits<size>(value)) {}

    template <typename K>
    requires(std::is_arithmetic_v<K> &&size > big_num_threshold) explicit constexpr logic(K value)
        : value(bits<size>(value)) {}

    constexpr explicit logic(const char *str) : logic(std::string_view(str)) {}
    explicit logic(std::string_view v) : value(bits<size>(v)) {
        auto iter = v.rbegin();
        for (auto i = 0u; i < size; i++) {
            // from right to left
            auto c = *iter;
            switch (c) {
                case 'x':
                    x_mask.set(i, true);
                    break;
                case 'z':
                    z_mask.set(i, true);
                    break;
                default:;
            }
            iter++;
            if (iter == v.rend()) break;
        }
    }

private:
};

inline namespace literals {
// constexpr to parse SystemVerilog literals
// per SV standard, if size not specified, the default size is 32
constexpr logic<32> operator""_logic(unsigned long long value) { return logic<32>(value); }
constexpr logic<64> operator""_logic64(unsigned long long value) { return logic<64>(value); }
}  // namespace literals

}  // namespace logic
#endif  // LOGIC_LOGIC_HH
