#include "logic/util.hh"

#include <bitset>
#include <cmath>
#include <optional>

namespace logic::util {

constexpr std::pair<char, uint64_t> get_input_base(std::string_view value) {
    char base = 'b';
    uint64_t pos = 0;

    auto p = value.find_first_of('\'');
    if (p != std::string::npos) {
        // need to find the next base values
        value = value.substr(p + 1);
        auto b = value.find_first_not_of("0123456789");
        if (b != std::string::npos) {
            base = value[b];
            pos = b + 1 + p + 1;
        }
    } else {
        base = 's';
    }

    return {base, pos};
}

char get_output_base(std::string_view fmt) {
    auto p = fmt.find_first_not_of("0123456789");
    if (p == std::string::npos) {
        return 'b';
    } else {
        return fmt[p];
    }
}

constexpr uint64_t parse_raw_str_(std::string_view value, char base) {
    uint64_t result = 0;
    switch (base) {
        case 'd':
        case 'D': {
            uint64_t base10 = 1;
            for (auto i = 0ull; i < value.size(); i++) {
                auto c = value[value.size() - i - 1];
                if (c >= '0' && c <= '9') {
                    uint64_t v = c - '0';
                    result += v * base10;
                    base10 *= 10;
                }
            }
            break;
        }
        case 'b':
        case 'B': {
            // need to take care of 1 and z
            uint64_t idx = 0;
            for (auto i = 0ull; i < value.size(); i++) {
                auto c = value[value.size() - i - 1];
                if (c == '1' || c == 'z') {
                    result |= 1ull << idx;
                }
                if (c != '_') idx++;
            }
            break;
        }
        case 'o':
        case 'O': {
            // same for x
            uint64_t idx = 0;
            for (auto i = 0ull; i < value.size(); i++) {
                auto c = value[value.size() - i - 1];
                uint64_t s;
                // this will be taken care of by the xz mask parsing
                if (c == 'x' || c == 'X' || c == '_')
                    s = 0;
                else if (c == 'z' || c == 'Z')
                    s = 0b111;
                else
                    s = c - '0';
                result |= s << (idx * 3ull);
                if (c != '_') idx++;
            }
            break;
        }
        case 'h':
        case 'H': {
            uint64_t idx = 0;
            for (auto i = 0ull; i < value.size(); i++) {
                auto c = value[value.size() - i - 1];
                uint64_t s = 0;
                // this will be taken care of by the xz mask parsing
                if (c == 'x' || c == 'X' || c == '_')
                    s = 0;
                else if (c == 'z' || c == 'Z')
                    s = 0b1111;
                else if (c >= '0' && c <= '9')
                    s = c - '0';
                else if (c >= 'a' && c <= 'f')
                    s = c - 'a';
                else if (c >= 'A' && c <= 'F')
                    s = c - 'A' + 10;
                result |= s << (idx * 4ull);
                if (c != '_') idx++;
            }
            break;
        }
        case 's':
        case 'S': {
            for (auto i = 0u; i < std::min(8ul, value.size()); i++) {
                uint64_t c = static_cast<uint8_t>(value[value.size() - i - 1]);
                result |= c << (i * 8);
            }
        }
        default:;
    }

    return result;
}

uint64_t parse_raw_str(std::string_view value) {
    auto [base, start_pos] = get_input_base(value);
    return parse_raw_str_(value.substr(start_pos), base);
}

uint64_t parse_xz_raw_str_(std::string_view value, char base) {
    uint64_t result = 0;

    switch (base) {
        case 'b':
        case 'B': {
            uint64_t idx = 0;
            for (auto i = 0u; i < value.size(); i++) {
                auto c = value[value.size() - i - 1];
                if (c == 'x' || c == 'z') {
                    result |= 1u << idx;
                }
                if (c != '_') idx++;
            }
            break;
        }
        case 'o':
        case 'O': {
            // same for x
            uint64_t idx = 0;
            for (auto i = 0u; i < value.size(); i++) {
                auto c = value[value.size() - i - 1];
                uint64_t s;
                // this will be taken care of by the xz mask parsing
                if (c == 'x' || c == 'X' || c == 'z' || c == 'Z') {
                    s = 0b111;
                    result |= s << (idx * 3);
                }
                if (c != '_') idx++;
            }
            break;
        }
        case 'h':
        case 'H': {
            uint64_t idx = 0;
            for (auto i = 0u; i < value.size(); i++) {
                auto c = value[value.size() - i - 1];
                uint64_t s;
                // this will be taken care of by the xz mask parsing
                if (c == 'x' || c == 'X' || c == 'z' || c == 'Z') {
                    s = 0b1111;
                    result |= s << (idx * 4);
                }
                if (c != '_') idx++;
            }
        }
        default:;
    }

    return result;
}

uint64_t parse_xz_raw_str(std::string_view value) {
    auto [base, start_pos] = get_input_base(value);
    return parse_xz_raw_str_(value.substr(start_pos), base);
}

uint64_t get_stride(char base) {
    switch (base) {
        case 'b':
        case 'B':
            return 1;
        case 'o':
        case 'O':
            return 3;
        case 'h':
        case 'H':
        case 'x':
        case 'X':
            return 4;
        case 's':
        case 'S':
            return 8;
        default:
            return 1;
    }
}

void parse_raw_str(std::string_view value, uint64_t size, uint64_t* ptr) {
    // use existing functions to parse single uint64_t values, except for decimals.
    // big number decimals are just inefficient, and I don't know why would anyone use it.
    auto [base, start_pos] = get_input_base(value);
    value = value.substr(start_pos);
    auto remaining_size = value.size();
    auto end = value.size();
    auto const stride = get_stride(base);
    auto const batch_size = 64 / stride;

    auto num_array = (size % 64 == 0) ? size / 64 : (size / 64 + 1);
    uint64_t idx = 0;
    while (remaining_size > 0 && idx < num_array) {
        auto begin = remaining_size >= batch_size ? remaining_size - batch_size : 0;
        ptr[idx] = parse_raw_str_(value.substr(begin, end - begin), base);

        idx++;
        end = begin;
        remaining_size = remaining_size > batch_size ? remaining_size - batch_size : 0;
    }
}

void parse_xz_raw_str(std::string_view value, uint64_t size, uint64_t* ptr) {
    // use existing functions to parse single uint64_t values, except for decimals.
    // big number decimals are just inefficient, and I don't know why would anyone use it.
    auto [base, start_pos] = get_input_base(value);
    value = value.substr(start_pos);
    auto remaining_size = value.size();
    auto end = value.size();
    auto const stride = get_stride(base);
    auto const batch_size = 64 / stride;

    auto num_array = (size % 64 == 0) ? size / 64 : (size / 64 + 1);
    uint64_t idx = 0;
    while (remaining_size > 0 && idx < num_array) {
        auto begin = remaining_size >= batch_size ? remaining_size - batch_size : 0;
        ptr[idx] = parse_xz_raw_str_(value.substr(begin, end - begin), base);

        idx++;
        end = begin;
        remaining_size = remaining_size > batch_size ? remaining_size - batch_size : 0;
    }
}

constexpr std::array<uint64_t, 128> compute_decimal_size() {
    // we precompute 128 entries of decimal sizes. if larger than that we need to
    // recompute each time. no caching allowed for the sakes of concurrency since
    // we need to protect the cache with a mutex if that is the case
    std::array<uint64_t, 128> result{};
    // zero is ill-defined
    result[0] = 1;

    for (auto i = 1u; i < 128; i++) {
        __uint128_t value = 0;
        value = ~value;
        // shift based on the size
        auto q = value >> (128 - i);
        auto r = 0u;
        while (q != 0) {
            q = q / 10;
            r++;
        }
        result[i] = r;
    }

    return result;
}

static constexpr auto decimal_size_table = compute_decimal_size();

void parse_fmt(const std::string_view& fmt, uint64_t size, char& base, uint64_t& actual_size,
               bool& padding) {
    // if base not specified, we treat it as a raw string
    base = 's';
    padding = true;  // per LRM 21.2.1.3, we need to perform padding by default
    // need to find the base
    auto pos = fmt.find_first_not_of("0123456789");
    if (pos != std::string::npos) {
        base = fmt[pos];
    }
    // now that we now the base, need to see what's requested size
    std::optional<uint64_t> requested_size;
    if (pos != 0) {
        // don't copy here?
        requested_size = std::stoul(std::string(fmt.substr(0, pos)));
    }

    // compute actual size
    uint64_t possible_size;
    switch (base) {
        case 'b':
        case 'B': {
            possible_size = size;
            break;
        }
        case 'o':
        case 'O': {
            possible_size = static_cast<uint64_t>(ceil(static_cast<double>(size) / 3.0));
            break;
        }
        case 'd':
        case 'D': {
            // give up if the size is larger than 128
            // will implement this in the future
            if (size > 128) {
                possible_size = size;
            } else {
                possible_size = decimal_size_table[size];
            }
            break;
        }
        case 'x':
        case 'X':
        case 'h':
        case 'H': {
            possible_size = static_cast<uint64_t>(ceil(static_cast<double>(size) / 4.0));
            break;
        }
        case 's':
        case 'S': {
            possible_size = static_cast<uint64_t>(ceil(static_cast<double>(size) / 8.0));
            break;
        }
        default:
            possible_size = size;
    }

    actual_size = possible_size;
    if (requested_size) {
        // need to compute the actual size and do a max, unless it's 0, which we don't do any
        // padding
        if (*requested_size == 0) {
            padding = false;
        } else {
            actual_size = std::max(actual_size, *requested_size);
        }
    }
}

std::string pad_result(bool is_negative, char base, uint64_t actual_size, bool padding,
                       const std::stringstream& ss) {
    auto result = ss.str();
    if (is_negative && (base == 'd' || base == 'D')) {
        result.append("-");
        actual_size++;
    }
    if (padding) {
        if (result.size() < actual_size) {
            auto const num_pad = actual_size - result.size();
            auto pad_char = (base == 'd' || base == 'D' || base == 's' || base == 'S') ? " " : "0";
            for (auto i = 0u; i < num_pad; i++) {
                result.append(pad_char);
            }
        }
    }
    std::reverse(result.begin(), result.end());
    return result;
}

std::string format_value_2power(char base, uint64_t value) {
    std::ostringstream ss;
    switch (base) {
        case 'o':
        case 'O': {
            ss << std::oct << value;
            break;
        }
        case 'x':
        case 'h':
        case 'X':
        case 'H': {
            ss << std::hex << value;
            break;
        }
        default:;
    }
    return ss.str();
}

std::string to_string(std::string_view fmt, uint64_t size, uint64_t value, bool is_negative) {
    char base;
    uint64_t actual_size;
    bool padding;
    parse_fmt(fmt, size, base, actual_size, padding);

    // now print out the format
    std::stringstream ss;
    switch (base) {
        case 'b':
        case 'B': {
            auto b = std::bitset<64>(value);
            auto s = b.to_string().substr(64 - size);
            // need to reverse it
            std::reverse(s.begin(), s.end());
            ss << s;
            break;
        }
        case 'o':
        case 'O':
        case 'x':
        case 'h':
        case 'X':
        case 'H': {
            ss << format_value_2power(base, value);
            break;
        }
        case 's':
        case 'S':
            for (auto i = 0u; i < size; i += 8) {
                auto c = value >> (i * 8) & 0xFF;
                ss << c;
            }
        default:;
    }
    return pad_result(is_negative, base, actual_size, padding, ss);
}

uint64_t get_mask(uint64_t stride) { return std::numeric_limits<uint64_t>::max() >> (64 - stride); }

std::string fmt_value(char base, uint64_t size, uint64_t value, uint64_t xz_mask) {
    auto stride = get_stride(base);
    const auto mask = get_mask(stride);
    std::stringstream ss;
    for (auto i = 0u; i < size; i += stride) {
        auto v = (value >> i) & mask;
        auto xz = (xz_mask >> i) & mask;
        if (xz != 0) {
            // need to see if it's all x or all z
            if (xz == mask) {
                if (v == 0) {
                    ss << 'x';
                } else if (v == mask) {
                    ss << 'z';
                } else {
                    ss << 'X';
                }
            } else {
                // need to check each one
                // we have a mix. need to see if it contains x
                bool has_x = false;
                for (auto j = 0u; j < stride; j++) {
                    auto bit = (v >> j) & 1;
                    auto bit_xz = (xz >> j) & 1;
                    if (bit_xz && !bit) {
                        ss << 'X';
                        has_x = true;
                        break;
                    }
                }
                if (!has_x) {
                    ss << 'Z';
                }
            }
        } else {
            auto str = format_value_2power(base, v);
            ss << str;
        }
    }
    return ss.str();
}

std::string fmt_decimal(uint64_t size, uint64_t value, uint64_t xz_mask) {
    if (xz_mask != 0) {
        // need to test if all the bits are x/z
        auto mask = std::numeric_limits<uint64_t>::max() >> (64 - size);
        if (xz_mask == mask) {
            // all x or zero
            if (value == 0) {
                return "x";
            } else if (value == mask) {
                return "z";
            } else {
                return "X";
            }
        } else {
            // some of them are actual digits
            // need to check if it contains x or not
            for (auto i = 0u; i < size; i++) {
                auto v = (value >> i) & 1;
                auto xz = (xz_mask >> i) & 1;
                if (xz && !v) {
                    return "X";
                }
            }
            return "Z";
        }
    } else {
        std::stringstream ss;
        if (value == 0) {
            ss << 0;
        } else {
            while (value > 0) {
                auto r = value % 10;
                ss << r;
                value /= 10;
            }
        }
        return ss.str();
    }
}

std::string fmt_char(uint64_t size, uint64_t value) {
    std::stringstream ss;
    auto num_chunk = static_cast<uint64_t>(std::ceil(static_cast<double>(size) / 8));
    for (auto i = 0u; i < num_chunk; i++) {
        auto c = static_cast<char>(static_cast<uint8_t>(value >> (8 * i) & 0xFF));
        ss << c;
    }
    return ss.str();
}

std::string to_string_(std::string_view fmt, uint64_t size, uint64_t value, uint64_t xz_mask,
                       bool is_negative, bool use_padding) {
    char base;
    uint64_t actual_size;
    bool padding;
    parse_fmt(fmt, size, base, actual_size, padding);

    std::stringstream ss;

    // need to obey the rule of LRM 21.2.1.4
    switch (base) {
        case 'b':
        case 'B': {
            for (auto i = 0u; i < size; i++) {
                auto c = (value >> i) & 1;
                auto xz = (xz_mask >> i) & 1;
                if (xz == 0) {
                    ss << c;
                } else {
                    ss << (c ? 'z' : 'x');
                }
            }
            break;
        }
        case 'o':
        case 'O':
        case 'h':
        case 'H':
        case 'x':
        case 'X': {
            ss << fmt_value(base, size, value, xz_mask);
            break;
        }
        // decimal is special
        case 'd':
        case 'D': {
            ss << fmt_decimal(size, value, xz_mask);
            break;
        }
        case 's':
        case 'S': {
            // the LRM does not specify the behavior if there is an x or z
            ss << fmt_char(size, value);
            break;
        }
        default:
            break;
    }

    if (use_padding)
        return pad_result(is_negative, base, actual_size, padding, ss);
    else
        return ss.str();
}

std::string to_string(std::string_view fmt, uint64_t size, uint64_t value, uint64_t xz_mask,
                      bool is_negative) {
    return to_string_(fmt, size, value, xz_mask, is_negative, true);
}

std::string to_string_(std::string_view fmt, uint64_t size, const uint64_t* value, bool is_negative,
                       bool use_padding) {
    char base;
    uint64_t actual_size;
    bool padding;
    parse_fmt(fmt, size, base, actual_size, padding);

    auto num_array = (size % 64 == 0) ? size / 64 : (size / 64 + 1);

    std::stringstream ss;
    if (base != 'd' && base != 'D') {
        // notice that because in oct format, 3 is not a power of 2, we need to be extra careful
        if (base == 'o' || base == 'O') {
            auto num_chunks = static_cast<uint64_t>(std::ceil(static_cast<double>(size) / 3));
            for (auto i = 0u; i < num_chunks; i++) {
                auto start = i * 3;
                auto value_idx = start / 64;
                auto start_idx = start % 64;
                // if it's across the 64-bit boundary
                uint64_t v;
                if ((64 - start_idx) < 3) {
                    v = (value[value_idx] >> start_idx) & 0x7;
                    v |= (value[value_idx + 1]) & (0x7 >> (64 - start_idx));
                } else {
                    v = (value[value_idx] >> start_idx) & 0x7;
                }
                auto s = to_string(fmt, 3, v, false);
                ss << s;
            }
        } else {
            for (auto i = 0u; i < num_array - 1; i++) {
                ss << to_string(fmt, 64, value[i], false);
            }
            if (size % 64) {
                ss << to_string(fmt, size % 64, value[num_array - 1], false);
            }
        }
    } else {
        // FIXME
        // give up. display the least significant two parts
        auto size_ = size > 128 ? 128 : size;
        __uint128_t v = 0;
        if (size_ > 64) {
            v |= value[1];
            v = v << 64;
        }
        v |= value[0];
        auto q = v;
        while (q != 0) {
            auto r = q % 10;
            ss << static_cast<uint64_t>(r);
            q = q / 10;
        }
    }

    if (use_padding)
        return pad_result(is_negative, base, actual_size, padding, ss);
    else
        return ss.str();
}

std::string to_string(std::string_view fmt, uint64_t size, const uint64_t* value,
                      bool is_negative) {
    return to_string_(fmt, size, value, is_negative, true);
}

std::string to_string(std::string_view fmt, uint64_t size, const uint64_t* value,
                      const uint64_t* xz_mask, bool is_negative) {
    char base;
    uint64_t actual_size;
    bool padding;
    parse_fmt(fmt, size, base, actual_size, padding);

    auto num_array = (size % 64 == 0) ? size / 64 : (size / 64 + 1);

    std::stringstream ss;
    if (base != 'd' && base != 'D') {
        // notice that because in oct format, 3 is not a power of 2, we need to be extra careful
        if (base == 'o' || base == 'O') {
            auto num_chunks = static_cast<uint64_t>(std::ceil(static_cast<double>(size) / 3));
            for (auto i = 0u; i < num_chunks; i++) {
                auto start = i * 3;
                auto value_idx = start / 64;
                auto start_idx = start % 64;
                // if it's across the 64-bit boundary
                uint64_t v;
                uint64_t xz;
                if ((64 - start_idx) < 3) {
                    v = (value[value_idx] >> start_idx) & 0x7;
                    v |= (value[value_idx + 1]) & (0x7 >> (64 - start_idx));
                    xz = (xz_mask[value_idx] >> start_idx) & 0x7;
                    xz |= (xz_mask[value_idx + 1]) & (0x7 >> (64 - start_idx));
                } else {
                    v = (value[value_idx] >> start_idx) & 0x7;
                    xz = (xz_mask[value_idx] >> start_idx) & 0x7;
                }
                auto s = to_string_(fmt, 3, v, xz, false, false);
                ss << s;
            }
        } else {
            for (auto i = 0u; i < num_array - 1; i++) {
                ss << to_string_(fmt, 64, value[i], xz_mask[i], false, false);
            }
            if (size % 64) {
                ss << to_string_(fmt, size % 64, value[num_array - 1], xz_mask[num_array - 1],
                                 false, false);
            }
        }

    } else {
        // FIXME
        // give up. display the least significant two parts
        auto size_ = size > 128 ? 128 : size;
        __uint128_t v = 0;
        __uint128_t xz = 0;
        if (size_ > 64) {
            v |= value[1];
            v = v << 64;
            xz |= xz_mask[1];
            xz = xz << 64;
        }
        v |= value[0];
        xz |= xz_mask[0];
        if (xz != 0) {
            // contains x/z
            if ((~xz) == 0) {
                // all x or z
                if (v == 0) {
                    ss << 'x';
                } else {
                    if ((~v) == 0) {
                        // all z
                        ss << 'z';
                    } else {
                        ss << 'X';
                    }
                }
            } else {
                // has some real digits
                bool has_x = false;
                for (auto i = 0u; i < size_; i++) {
                    auto x = (xz >> i) & 1;
                    if (x) {
                        auto vv = (v >> i) & 1;
                        if (vv == 0) {
                            has_x = true;
                            break;
                        }
                    }
                }
                if (has_x)
                    ss << "X";
                else
                    ss << "Z";
            }
        } else {
            auto q = v;
            while (q != 0) {
                auto r = q % 10;
                ss << static_cast<uint64_t>(r);
                q = q / 10;
            }
        }
    }

    return pad_result(is_negative, base, actual_size, padding, ss);
}

bool decimal_fmt(std::string_view fmt) {
    auto base = get_output_base(fmt);
    return base == 'd' || base == 'D';
}

}  // namespace logic::util