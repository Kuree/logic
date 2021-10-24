#ifndef LOGIC_LOGIC_HH
#define LOGIC_LOGIC_HH

#include <array>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace logic {

template <uint64_t size>
struct big_num;

// some constants
constexpr uint64_t big_num_threshold = sizeof(uint64_t) * 8;

namespace util {
// compute the holder type
static constexpr bool in_range(uint64_t a, uint64_t min, uint64_t max) {
    return a >= min and a <= max;
}

static constexpr bool gte(uint64_t a, uint64_t min) { return a >= min; }

static constexpr uint64_t abs_diff(uint64_t a, uint64_t b) {
    return (a > b)? (a - b): (b - a);
}

template <uint64_t s, typename enable = void>
struct get_golder_type;
template <uint64_t s>
struct get_golder_type<s, typename std::enable_if<in_range(s, 1, 8)>::type> {
    using type = uint8_t;
};
template <uint64_t s>
struct get_golder_type<s, typename std::enable_if<in_range(s, 9, 16)>::type> {
    using type = uint16_t;
};

template <uint64_t s>
struct get_golder_type<s, typename std::enable_if<in_range(s, 17, 32)>::type> {
    using type = uint32_t;
};

template <uint64_t s>
struct get_golder_type<s, typename std::enable_if<in_range(s, 33, 64)>::type> {
    using type = uint64_t;
};

template <uint64_t s>
struct get_golder_type<s, typename std::enable_if<gte(s, 65)>::type> {
    using type = big_num<s>;
};
}  // namespace util

template <uint64_t size>
struct big_num {
public:
    constexpr static auto s = (size + 1) / big_num_threshold;
    // use uint64_t as a holder
    std::array<uint64_t, s> values;

    bool inline operator[](uint64_t idx) const {
        if (idx < size) [[likely]] {
            auto a = idx / big_num_threshold;
            auto b = idx % big_num_threshold;
            return (values[a] >> b) & 1;
        } else {
            return false;
        }
    }

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


    explicit big_num(const std::string_view &v) {
        std::fill(values.begin(), values.end(), 0);
        auto iter = v.rbegin();
        for (auto i = 0u; i < size; i++) {
            // from right to left
            auto &value = values[i / big_num_threshold];
            auto c = *iter;
            if (c == '1') {
                value |= 1u << (i % big_num_threshold);
            }
            iter++;
            if (iter == v.rend()) break;
        }
    }

    template <uint64_t a, uint64_t b>
    big_num<util::abs_diff(a, b) + 1> inline slice() const {
        big_num<util::abs_diff(a, b) + 1> res;
        auto max = a, min = b;
        if (min > max) std::swap(min, max);
        for (uint64_t i = min; i <= max; i++) {
            auto idx = i - min;
            res.set(idx, this->operator[](i));
        }
        return res;
    }

    big_num() = default;
};

template <uint64_t size>
struct logic;

template <uint64_t size>
struct bits {
private:
public:
    static_assert(size > 0, "0 sized logic not allowed");
    using T = typename util::get_golder_type<size>::type;

    bool signed_ = false;
    T value;

    bool inline operator[](uint64_t idx) const {
        if constexpr (size <= big_num_threshold) {
            return (value >> idx) & 1;
        } else {
            return value[idx];
        }
    }

    template <uint64_t a, uint64_t b>
    bits<util::abs_diff(a, b) + 1> inline slice() const {
        // assume the import has type-checked properly, e.g. by a compiler
        uint64_t max = a, min = b;
        if (min > max) std::swap(max, min);
        bits<util::abs_diff(a, b) + 1>result;
        if constexpr (size <= big_num_threshold) {
            auto const default_mask = std::numeric_limits<T>::max();
            auto mask = default_mask << min;
            mask &= (default_mask >> (size - max));
            result.value = value & mask;
            result.value >>= min;
        } else {
            auto res = value.template slice<a, b>();
            if constexpr (util::abs_diff(a, b) < big_num_threshold) {
                // shrink it back to normal variable
                result.value = res.values[0];
            } else {
                result = res;
            }
        }

        return result;
    }

    explicit bits(const std::string_view &v) {
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

    explicit bits(T v): value(v) {}
    bits() = default;
};

template <uint64_t size>
struct logic {
    using T = typename util::get_golder_type<size>::type;
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

    // by default everything is x
    logic() {
        std::fill(x_mask.begin(), x_mask.end(), true);
        std::fill(z_mask.begin(), z_mask.end(), false);
    }

    explicit logic(T value) : value(bits<size>(value)) {
        std::fill(x_mask.begin(), x_mask.end(), false);
        std::fill(z_mask.begin(), z_mask.end(), false);
    }

    explicit logic(const std::string_view &v): value(bits<size>(v)) {
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

}  // namespace logic
#endif  // LOGIC_LOGIC_HH
