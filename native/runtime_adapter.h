#pragma once

#include "proxy.h"
#include "runtime_profile.h"

#include <cstddef>

namespace sakura {

struct MethodCandidate {
    const char* officialName;
    const char* srgName;
    const char* yarnName;
    const char* descriptor;
    bool isStatic;
};

struct ClassCandidate {
    const char* officialName;
    const char* srgName;
    const char* yarnName;
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
    size_t minecraftInstanceCount;
    const MethodCandidate* minecraftConnection;
    size_t minecraftConnectionCount;
    const MethodCandidate* listenerConnection;
    size_t listenerConnectionCount;
    const MethodCandidate* helloName;
    size_t helloNameCount;
};

const RuntimeAdapter* FindRuntimeAdapter(MinecraftVersion version, ModLoader loader);
const RuntimeAdapter* SelectRuntimeAdapter(const RuntimeProfile& profile);
bool ValidateRuntimeAdapter(JNIEnv* env, jobject classLoader, const RuntimeAdapter& adapter);
void LogRuntimeAdapter(const RuntimeAdapter& adapter);

} // namespace sakura
