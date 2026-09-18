#pragma once

#include "proxy.h"
#include "runtime_profile.h"

#include <cstddef>

namespace sakura {

enum class AdapterStatus {
    Experimental,
    ReadyForBindingValidation,
    ProductionValidated,
};

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
    AdapterStatus status;
    const ClassCandidate* minecraftClasses;
    size_t minecraftClassCount;
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
};

const RuntimeAdapter* FindRuntimeAdapter(MinecraftVersion version, ModLoader loader);
const RuntimeAdapter* SelectRuntimeAdapter(const RuntimeProfile& profile);

// Validates names and JVM descriptors without calling game methods.
bool ValidateRuntimeAdapter(JNIEnv* env, jobject classLoader, const RuntimeAdapter& adapter);
void LogRuntimeAdapter(const RuntimeAdapter& adapter);
const char* AdapterStatusName(AdapterStatus status);

// Shared lookup order: official -> SRG/MCP -> Yarn/intermediary -> descriptor.
jclass ResolveClass(JNIEnv* env, jobject classLoader, const ClassCandidate& candidate);
jmethodID ResolveMethod(JNIEnv* env, jclass klass, const MethodCandidate& candidate);

} // namespace sakura
