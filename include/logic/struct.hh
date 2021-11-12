#ifndef LOGIC_STRUCT_HH
#define LOGIC_STRUCT_HH

#include "logic.hh"

namespace logic {

// since it's packed, based on LRM it's just a huge chunk of logic
template <uint64_t size, bool is_4state = true>
struct packed_struct {};

template <uint64_t size>
struct packed_struct<size, true> {
    using type = logic<size - 1, 0>;
};

template <uint64_t size>
struct packed_struct<size, false> {
    using type = bit<size - 1, 0>;
};

// this is just a normal struct
struct unpacked_struct {};

}  // namespace logic

#endif  // LOGIC_STRUCT_HH
