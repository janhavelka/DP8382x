#pragma once

#include "Check.h"
#include <DP8382x/Config.h>

namespace test {

// Each registration starts a fresh process. The simulated identity words
// remain independent of the declaration supplied to bind.
inline DP8382x::Variant declaredVariant = DP8382x::Variant::DP83826I;

inline bool selectVariant(int argc, char** argv) {
  if (argc == 2) return true;
  if (argc != 3) return false;
  if (std::strcmp(argv[2], "--dp83826e") == 0) declaredVariant = DP8382x::Variant::DP83826E;
  else if (std::strcmp(argv[2], "--dp83826ai") == 0) declaredVariant = DP8382x::Variant::DP83826AI;
  else if (std::strcmp(argv[2], "--dp83826ae") == 0) declaredVariant = DP8382x::Variant::DP83826AE;
  else return false;
  return true;
}

// SNLS647I tables 8-21/22 and SNLS783A tables 8-22/23.
inline bool aGrade() {
  return declaredVariant == DP8382x::Variant::DP83826AI || declaredVariant == DP8382x::Variant::DP83826AE;
}
inline uint16_t basicId2() { return aGrade() ? 0xA114 : 0xA111; }
inline uint16_t enhancedId2() { return aGrade() ? 0xA134 : 0xA131; }

template<std::size_t N>
int runVariantSuites(int argc, char** argv, const Suite (&suites)[N]) {
  if (!selectVariant(argc, argv)) return 2;
  return runSuites(2, argv, suites);
}

} // namespace test
