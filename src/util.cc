#include "logic/util.hh"

namespace logic::util {

constexpr std::pair<char, uint64_t> get_base(std::string_view value) {
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
    }

    return {base, pos};
}

constexpr uint64_t parse_raw_str_(std::string_view value, char base) {
    uint64_t result = 0;
    switch (base) {
        case 'd':
        case 'D': {
            // since x in decimal is not allowed. we use system default parsing functions
            result = std::stoull(value.data());
            break;
        }
        case 'b':
        case 'B': {
            // need to take care of 1 and z
            for (auto i = 0ull; i < value.size(); i++) {
                auto c = value[value.size() - i - 1];
                if (c == '1' || c == 'z') {
                    result |= 1ull << i;
                }
            }
            break;
        }
        case 'o':
        case 'O': {
            // same for x
            for (auto i = 0ull; i < value.size(); i++) {
                auto c = value[value.size() - i - 1];
                uint64_t s;
                // this will be taken care of by the xz mask parsing
                if (c == 'x' || c == 'X')
                    continue;
                else if (c == 'z' || c == 'Z')
                    s = 0b111;
                else
                    s = c - '0';
                result |= s << (i * 3ull);
            }
            break;
        }
        case 'h':
        case 'H': {
            for (auto i = 0ull; i < value.size(); i++) {
                auto c = value[value.size() - i - 1];
                uint64_t s = 0;
                // this will be taken care of by the xz mask parsing
                if (c == 'x' || c == 'X')
                    continue;
                else if (c == 'z' || c == 'Z')
                    s = 0b1111;
                else if (c >= '0' && c <= '9')
                    s = c - '0';
                else if (c >= 'a' && c <= 'f')
                    s = c - 'a';
                else if (c >= 'A' && c <= 'F')
                    s = c - 'A' + 10;
                result |= s << (i * 4ull);
            }
            break;
        }
        default:;
    }

    return result;
}

uint64_t parse_raw_str(std::string_view value) {
    auto [base, start_pos] = get_base(value);
    return parse_raw_str_(value.substr(start_pos), base);
}

uint64_t parse_xz_raw_str_(std::string_view value, char base) {
    uint64_t result = 0;

    switch (base) {
        case 'b':
        case 'B': {
            for (auto i = 0u; i < value.size(); i++) {
                auto c = value[value.size() - i - 1];
                if (c == 'x' || c == 'z') {
                    result |= 1u << i;
                }
            }
            break;
        }
        case 'o':
        case 'O': {
            // same for x
            for (auto i = 0u; i < value.size(); i++) {
                auto c = value[value.size() - i - 1];
                uint64_t s;
                // this will be taken care of by the xz mask parsing
                if (c == 'x' || c == 'X' || c == 'z' || c == 'Z') {
                    s = 0b111;
                    result |= s << (i * 3);
                }
            }
            break;
        }
        case 'h':
        case 'H': {
            for (auto i = 0u; i < value.size(); i++) {
                auto c = value[value.size() - i - 1];
                uint64_t s = 0;
                // this will be taken care of by the xz mask parsing
                if (c == 'x' || c == 'X' || c == 'z' || c == 'Z') {
                    s = 0b1111;
                    result |= s << (i * 4);
                }
            }
        }
        default:;
    }

    return result;
}

uint64_t parse_xz_raw_str(std::string_view value) {
    auto [base, start_pos] = get_base(value);
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
            return 4;
        default:
            return 1;
    }
}

void parse_raw_str(std::string_view value, uint64_t size, uint64_t* ptr) {
    // use existing functions to parse single uint64_t values, except for decimals.
    // big number decimals are just inefficient, and I don't know why would anyone use it.
    auto [base, start_pos] = get_base(value);
    value = value.substr(start_pos);
    auto remaining_size = value.size();
    auto end = value.size();
    auto const stride = get_stride(base);
    auto const batch_size = 64 / stride;
    uint64_t idx = 0;
    while (remaining_size > 0 && idx < size) {
        auto begin = remaining_size >= batch_size ? remaining_size - batch_size : 0;
        ptr[idx] = parse_raw_str_(value.substr(begin, end), base);

        idx++;
        end = begin;
        remaining_size -= batch_size;
    }
}

void parse_xz_raw_str(std::string_view value, uint64_t size, uint64_t* ptr) {
    // use existing functions to parse single uint64_t values, except for decimals.
    // big number decimals are just inefficient, and I don't know why would anyone use it.
    auto [base, start_pos] = get_base(value);
    value = value.substr(start_pos);
    auto remaining_size = value.size();
    auto end = value.size();
    auto const stride = get_stride(base);
    auto const batch_size = 64 / stride;
    uint64_t idx = 0;
    while (remaining_size > 0 && idx < size) {
        auto begin = remaining_size >= batch_size ? remaining_size - batch_size : 0;
        ptr[idx] = parse_xz_raw_str_(value.substr(begin, end), base);

        idx++;
        end = begin;
        remaining_size -= batch_size;
    }
}

}  // namespace logic::util