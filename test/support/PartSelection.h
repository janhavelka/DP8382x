#pragma once

#include "Check.h"
#include <DP8382x/Config.h>

namespace test {

// Each registration starts a fresh process. The simulated identity words
// remain independent of the declaration supplied to bind.
inline DP8382x::Variant declaredVariant = DP8382x::Variant::DP83826I;

inline bool selectVariant(int argc, char** argv) {
  if (argc == 2) return true;
  if (argc != 3 || std::strcmp(argv[2], "--dp83826e") != 0) return false;
  declaredVariant = DP8382x::Variant::DP83826E;
  return true;
}

template<std::size_t N>
int runVariantSuites(int argc, char** argv, const Suite (&suites)[N]) {
  if (!selectVariant(argc, argv)) return 2;
  return runSuites(2, argv, suites);
}

} // namespace test
