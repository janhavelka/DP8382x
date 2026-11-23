#pragma once

#include "Check.h"
#include <DP8382x/DP8382x.h>

namespace test {

/// Own a controller and a driver bound to its default configuration.
/// The driver borrows the controller until driver destruction. Member order
/// keeps that controller alive through scoped reentry and teardown.
template<typename Controller>
struct BoundDriver {
  Controller bus;
  ::DP8382x::DP8382x driver;

  BoundDriver() { CHECK(driver.bind(bus.config()).ok()); }
};

} // namespace test
