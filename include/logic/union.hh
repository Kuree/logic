#ifndef LOGIC_UNION_HH
#define LOGIC_UNION_HH

#include "logic.hh"

namespace logic {

// this is the same concept as packed struct
// since the underlying data type is continuous specified by LRM
template <uint64_t size, bool is_4state = true>
struct union_ {};

template <uint64_t size>
struct union_<size, true> {
    using type = logic<size - 1, 0>;
};

template <uint64_t size>
struct union_<size, false> {
    using type = bit<size - 1, 0>;
};

}  // namespace logic

#endif  // LOGIC_UNION_HH
