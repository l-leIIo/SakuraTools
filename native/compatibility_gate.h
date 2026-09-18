#pragma once

#include "runtime_profile.h"

namespace sakura {

// The native forwarding implementation is version-specific. This gate prevents
// a binding-only profile from entering the 1.20.1 packet/snapshot code path.
struct CompatibilityDecision {
    bool allowed;
    const char* reason;
};

CompatibilityDecision CheckCompatibility(const RuntimeProfile& profile);

} // namespace sakura
