/// @file Dp8382xCliFeatures.h
/// @brief Common typed feature dispatch through portable and native owners.
#pragma once

#include "Dp8382xCli.h"
#include "Dp8382xCliDiagnostics.h"
#include "Dp8382xCliControls.h"
#include "Dp8382xCliPins.h"
#include <type_traits>

namespace DP8382x::Cli::owner_detail {
/// @cond INTERNAL

inline bool featureMaintenance(Feature feature) noexcept {
  const auto mode = featureMode(feature);
  return mode == FeatureMode::WRITE || mode == FeatureMode::START || mode == FeatureMode::RESET;
}

// Only typed core member calls are shared with the SDK bridge. The native
// equivalent applies the stopped gate, result slot and reset reservation.
template<class Backend>
class FeatureAccess {
public:
  FeatureAccess(Backend& backend, const Request& request, Receipt& receipt,
                FeatureAttestation grant, void* context) noexcept
      : _backend(backend), _request(request), _receipt(receipt), _grant(grant), _context(context) {}

  const OperationBudget& budget() const noexcept { return _request.budget; }
  bool attest(uint32_t requested) const {
    return _grant && _grant(_context, _request.feature, requested);
  }

  template<auto Method, class... Inputs>
  Status start(const Inputs&... inputs) {
    if constexpr (std::is_same_v<Backend, Core>) {
      return (_backend.*Method)(inputs..., budget(), _receipt.token);
    } else {
      return _backend.template startFeature<Method>(budget(), _receipt.token, inputs...);
    }
  }

  template<auto Method, class Permit, class... Inputs>
  Status call(const Permit& permit, Inputs&... inputs) {
    if constexpr (std::is_same_v<Backend, Core>) {
      return (_backend.*Method)(inputs..., permit);
    } else {
      return _backend.template callFeature<Method>(permit, inputs...);
    }
  }

  template<auto Method, class Output>
  Status read(Output& output) { return call<Method>(budget().io, output); }

  template<auto Method, class Output, class Permit>
  Status readWith(Output& output, const Permit& permit) { return call<Method>(permit, output); }

  template<auto Method, class Options, class Output>
  Status direct(const Options& options, Output& output) { return call<Method>(budget().io, options, output); }

  template<auto Method, class Output>
  Status cached(Output& output) {
    if constexpr (std::is_same_v<Backend, Core>) {
      output = (static_cast<const Core&>(_backend).*Method)();
      return Status::Ok();
    } else {
      return _backend.template featureSnapshot<Method>(output);
    }
  }

private:
  Backend& _backend;
  const Request& _request;
  Receipt& _receipt;
  FeatureAttestation _grant;
  void* _context;
};

template<class Backend>
Status executeFeature(Backend& backend, const Request& request, Receipt& receipt,
                      FeatureAttestation grant, void* context) {
  FeatureAccess<Backend> access(backend, request, receipt, grant, context);
  const auto& args = request.featureArgs;
  auto& fields = receipt.featureFields;
  switch (request.feature) {
#define FEATURE(id, name, minimum, maximum, mode, usage, summary) \
    case Feature::id: return diagnosticFeature(access, Feature::id, args, fields);
#include "Dp8382xCliDiagnosticCommands.inc"
#undef FEATURE
#define FEATURE(id, name, minimum, maximum, mode, usage, summary) \
    case Feature::id: return controlFeature(access, Feature::id, args, fields);
#include "Dp8382xCliControlCommands.inc"
#undef FEATURE
#define FEATURE(id, name, minimum, maximum, mode, usage, summary) \
    case Feature::id: return pinFeature(access, Feature::id, args, fields);
#include "Dp8382xCliPinCommands.inc"
#undef FEATURE
    case Feature::NONE: break;
  }
  return featureInvalid();
}

inline void describeFeatureResult(Feature feature, const OperationResult& result, FeatureFields& fields) {
  switch (feature) {
#define FEATURE(id, name, minimum, maximum, mode, usage, summary) \
    case Feature::id: diagnosticResult(Feature::id, result, fields); return;
#include "Dp8382xCliDiagnosticCommands.inc"
#undef FEATURE
#define FEATURE(id, name, minimum, maximum, mode, usage, summary) \
    case Feature::id: controlResult(Feature::id, result, fields); return;
#include "Dp8382xCliControlCommands.inc"
#undef FEATURE
#define FEATURE(id, name, minimum, maximum, mode, usage, summary) \
    case Feature::id: pinResult(Feature::id, result, fields); return;
#include "Dp8382xCliPinCommands.inc"
#undef FEATURE
    case Feature::NONE: break;
  }
}
/// @endcond
} // namespace DP8382x::Cli::owner_detail
