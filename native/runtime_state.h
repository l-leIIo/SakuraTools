#pragma once

#include "runtime_profile.h"

namespace sakura {
struct RuntimeAdapter;
const RuntimeAdapter* SelectRuntimeAdapter(const RuntimeProfile& profile);
void LogRuntimeAdapter(const RuntimeAdapter& adapter);
}

extern sakura::RuntimeProfile g_runtimeProfile;
