#ifndef LOGIC_ARRAY_HH
#define LOGIC_ARRAY_HH

#include "logic.hh"

namespace logic {

namespace util {
template <typename T, int msb, int lsb, bool is_logic>
struct get_array_type;

template <typename T, int msb, int lsb>
struct get_array_type<T, msb, lsb, true> {
    using type = logic<util::total_size(msb, lsb) * T::size - 1, 0, T::is_signed, true>;
};

template <typename T, int msb, int lsb>
struct get_array_type<T, msb, lsb, false> {
    using type = bit<util::total_size(msb, lsb) * T::size - 1, 0, T::is_signed, true>;
};

template <typename T, int size, bool is_logic>
struct get_array_base_type;

template <typename T, int size>
struct get_array_base_type<T, size, true> {
    using type = logic<size - 1, 0, false>;
};

template <typename T, int size>
struct get_array_base_type<T, size, false> {
    using type = bit<size - 1, 0, false>;
};

}  // namespace util

template <typename T, int msb, int lsb>
struct packed_array : public util::get_array_type<T, msb, lsb, T::is_4state>::type {
private:
    constexpr static int get_slice_size(int hi, int lo) {
        return base_size * util::total_size(hi, lo);
    }
    using VT = typename util::get_array_type<T, msb, lsb, T::is_4state>::type;

public:
    // compute the base size
    static constexpr auto base_size = T::size;
    static constexpr auto size = base_size * (util::abs_diff(msb, lsb) + 1);
    static constexpr auto native_num = util::native_num(size);
    static constexpr bool is_4state = T::is_4state;

    template <int hi, int lo>
    auto slice() {
        // typename util::get_array_base_type<T, get_slice_size(hi, lo), T::is_4state>::type result;
        // copy values over
        // maybe use SIMD to copy?
        // need to compute base address
        auto constexpr min_idx = util::min(hi, lo) - util::min(msb, lsb);
        auto constexpr max_idx = util::max(hi, lo) - util::min(msb, lsb);
        auto constexpr start_addr = min_idx * base_size;
        auto constexpr end_addr = (max_idx + 1) * base_size;

        return VT::value.template slice<end_addr - 1, start_addr>();
    }

    template <int hi, int lo, typename V>
    auto update(const V v) {
        // need to compute the base
        auto constexpr min_idx = util::min(hi, lo) - util::min(msb, lsb);
        auto constexpr max_idx = util::max(hi, lo) - util::min(msb, lsb);
        auto constexpr start_addr = min_idx * base_size;
        auto constexpr end_addr = (max_idx + 1) * base_size;
        // use the underlying logic implementation
        VT::template update<end_addr - 1, start_addr>(v);
    }

    template <typename IT>
    requires(std::is_arithmetic_v<IT>) explicit packed_array(IT v) : VT(v) {}
    explicit packed_array(std::string_view v) : VT(v) {}

    packed_array() = default;
};

template <typename T, int msb, int lsb>
struct unpacked_array {
public:
    static constexpr auto size = util::total_size(msb, lsb);
    std::array<T, size> value;

    const T &operator[](int idx) const {
        const static T x_ = T{};
        auto i = static_cast<uint32_t>(idx - util::min(msb, lsb));
        if (i >= size) {
            return x_;
        } else {
            return value[i];
        }
    }

    template <int idx, typename V>
    auto update(const V v) {
        if (idx >= size) return;
        // need to compute the base
        auto constexpr i = idx -  util::min(msb, lsb);
        // use the underlying logic implementation
        value[i] = v;
    }

private:

};

}  // namespace logic

#endif  // LOGIC_ARRAY_HH
