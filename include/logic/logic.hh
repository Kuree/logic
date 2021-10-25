#ifndef LOGIC_LOGIC_HH
#define LOGIC_LOGIC_HH

#include <array>
#include <limits>
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
        dst[i] = src[i + start];
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

    template <typename T>
    requires std::is_arithmetic_v<T>
    [[maybe_unused]] explicit big_num(T v) : values({v}) {}

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

    // single bit
    bool inline operator[](uint64_t idx) const {
        if constexpr (size <= big_num_threshold) {
            return (value >> idx) & 1;
        } else {
            return value[idx];
        }
    }

    template <uint64_t idx>
    requires(idx < size && native_num) [[nodiscard]] bool inline get() const {
        return (value >> idx) & 1;
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

    bits() = default;
};

template <uint64_t size, bool signed_>
struct logic {
    using T = typename util::get_holder_type<size, signed_>::type;
    bits<size> value;
    // by default every thing is x
    std::array<bool, size> x_mask;
    std::array<bool, size> z_mask;

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
                r.x_mask[idx] = true;
            } else if (z_mask[idx]) [[unlikely]] {
                r.z_mask[idx] = true;
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
        if (x_mask[idx]) [[unlikely]] {
            r.x_mask[idx] = true;
        } else if (z_mask[idx]) [[unlikely]] {
            r.z_mask[idx] = true;
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

        logic<util::abs_diff(a, b) + 1, false> result;
        result.value = value.template slice<a, b>();
        // copy over masks
        constexpr auto max = util::max(a, b);
        constexpr auto min = util::min(a, b);
        util::copy(result.x_mask, x_mask, min, max);
        util::copy(result.z_mask, z_mask, min, max);

        return result;
    }

    /*
     * constructors
     */
    // by default everything is x
    logic() {
        std::fill(x_mask.begin(), x_mask.end(), true);
        std::fill(z_mask.begin(), z_mask.end(), false);
    }

    explicit constexpr logic(T value) requires(size <= big_num_threshold)
        : value(bits<size>(value)) {
        std::fill(x_mask.begin(), x_mask.end(), false);
        std::fill(z_mask.begin(), z_mask.end(), false);
    }

    template <typename K>
    requires(std::is_arithmetic_v<K> &&size > big_num_threshold) explicit constexpr logic(K value)
        : value(bits<size>(value)) {
        std::fill(x_mask.begin(), x_mask.end(), false);
        std::fill(z_mask.begin(), z_mask.end(), false);
    }

    constexpr explicit logic(const char *str) : logic(std::string_view(str)) {}
    explicit logic(std::string_view v) : value(bits<size>(v)) {
        std::fill(x_mask.begin(), x_mask.end(), false);
        std::fill(z_mask.begin(), z_mask.end(), false);
        auto iter = v.rbegin();
        for (auto i = 0u; i < size; i++) {
            // from right to left
            auto c = *iter;
            switch (c) {
                case 'x':
                    x_mask[i] = true;
                    break;
                case 'z':
                    z_mask[i] = true;
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
