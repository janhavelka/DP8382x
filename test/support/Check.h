#pragma once
#include <cstdio>
#include <cstring>
#include <cstddef>
namespace test {
inline unsigned checks = 0;
inline unsigned failures = 0;
inline void check(bool condition, const char* expression, int line) {
  ++checks;
  if (!condition) { ++failures; std::fprintf(stderr, "line %d: %s\n", line, expression); }
}
struct Suite { const char* name; void (*run)(); };
template<std::size_t N>
int runSuites(int argc, char** argv, const Suite (&suites)[N]) {
  if (argc != 2) { return 2; }
  bool found = false;
  if (std::strcmp(argv[1], "--verify-failure-path") == 0) {
    check(false, "Intentional Release failure probe", __LINE__);
    found = true;
  } else {
    for (const auto& suite : suites) {
      if (std::strcmp(argv[1], suite.name) == 0) { suite.run(); found = true; break; }
    }
  }
  if (!found) { return 2; }
  std::printf("%u checks; %u failures\n", checks, failures);
  return failures == 0U ? 0 : 1;
}
}
#define CHECK(expression) test::check(static_cast<bool>(expression), #expression, __LINE__)
