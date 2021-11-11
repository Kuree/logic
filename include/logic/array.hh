#ifndef LOGIC_ARRAY_HH
#define LOGIC_ARRAY_HH

#include "logic.hh"

namespace logic {

template <typename T, int msb, int lsb>
class packed_array {};

template <int msb, int lsb, int target_msb, int target_lsb = target_msb, bool target_signed = false>
class packed_array_bits
    : public logic<util::total_size(msb, lsb) * util::total_size(target_msb, target_lsb),
                   target_signed, true>,
      public packed_array<packed_array_bits<msb, lsb, target_msb, target_lsb, target_signed>, msb,
                          lsb> {
public:
};

template <int msb, int lsb, int target_msb, int target_lsb = target_msb, bool target_signed = false>
class packed_array_logic
    : public logic<util::total_size(msb, lsb) * util::total_size(target_msb, target_lsb),
                   target_signed, true>,
      public packed_array<packed_array_bits<msb, lsb, target_msb, target_lsb, target_signed>, msb,
                          lsb> {
public:
};

}  // namespace logic

#endif  // LOGIC_ARRAY_HH
