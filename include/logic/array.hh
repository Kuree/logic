#ifndef LOGIC_ARRAY_HH
#define LOGIC_ARRAY_HH

#include "logic.hh"

namespace logic {

namespace util {
template <typename T, int msb, int lsb, bool is_logic>
struct get_base_array_type;

template <typename T, int msb, int lsb>
struct get_base_array_type<T, msb, lsb, true> {
    using type = logic<util::total_size(msb, lsb) * T::size - 1, 0, T::is_signed, true>;
};

template <typename T, int msb, int lsb>
struct get_base_array_type<T, msb, lsb, false> {
    using type = bit<util::total_size(msb, lsb) * T::size - 1, 0, T::is_signed, true>;
};
}  // namespace util

template <typename T, int msb, int lsb>
class packed_array : public util::get_base_array_type<T, msb, lsb, T::is_4state>::type {
    // compute the base size
    static constexpr auto base_size = T::size;
};

}  // namespace logic

#endif  // LOGIC_ARRAY_HH
