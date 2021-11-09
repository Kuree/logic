#ifndef LOGIC_ARRAY_HH
#define LOGIC_ARRAY_HH

#include "logic.hh"

namespace logic {

template <bool signed_, uint64_t ...nums>
class packed_array {
public:
    static constexpr auto size = util::total_size<nums...>();
    
};

}

#endif  // LOGIC_ARRAY_HH
