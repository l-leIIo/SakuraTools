#pragma once

#include "runtime_profile.h"
#include <cstddef>

namespace sakura {

struct MethodCandidate {
    const char* named;
    const char* mapped;
    const char* descriptor;
    bool isStatic;
};

struct ClassCandidate {
    const char* officialName;
    const char* legacyName;
    const char* descriptor;
};

struct RuntimeAdapter {
    MinecraftVersion version;
    ModLoader loader;
    const char* displayName;
    const ClassCandidate* connectionClasses;
    size_t connectionClassCount;
    const ClassCandidate* listenerClasses;
    size_t listenerClassCount;
    const MethodCandidate* minecraftInstance;
    const MethodCandidate* minecraftConnection;
    const MethodCandidate* listenerConnection;
    const MethodCandidate* serverboundHelloName;
};

const RuntimeAdapter* FindRuntimeAdapter(MinecraftVersion version, ModLoader loader);
const RuntimeAdapter* SelectRuntimeAdapter(const RuntimeProfile& profile);
bool ValidateRuntimeAdapter(JNIEnv* env, jobject classLoader, const RuntimeAdapter& adapter);
void LogRuntimeAdapter(const RuntimeAdapter& adapter);

} // namespace sakura
