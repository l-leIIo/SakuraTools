#include "runtime_adapter.h"
#include "jni_lookup.h"
#include "proxy.h"

#include <cstdio>

namespace sakura {
namespace {

#define CLASS(O, S, Y, D) {O, S, Y, D}
#define METHOD(O, S, Y, D, ST) {O, S, Y, D, ST}

static const ClassCandidate kMinecraft[] = {
    CLASS("net.minecraft.client.Minecraft", "net.minecraft.client.Minecraft", "net.minecraft.class_310", "Lnet/minecraft/client/Minecraft;")
};
static const ClassCandidate kConnectionLegacy[] = {
    CLASS("net.minecraft.network.NetworkManager", "net.minecraft.network.NetworkManager", "net.minecraft.class_2535", "Lnet/minecraft/network/NetworkManager;")
};
static const ClassCandidate kConnectionModern[] = {
    CLASS("net.minecraft.network.Connection", "net.minecraft.network.Connection", "net.minecraft.class_2535", "Lnet/minecraft/network/Connection;")
};
static const ClassCandidate kListenerLegacy[] = {
    CLASS("net.minecraft.client.network.NetHandlerPlayClient", "net.minecraft.client.network.NetHandlerPlayClient", "net.minecraft.class_634", "Lnet/minecraft/client/network/NetHandlerPlayClient;")
};
static const ClassCandidate kListenerModern[] = {
    CLASS("net.minecraft.client.multiplayer.ClientPacketListener", "net.minecraft.client.multiplayer.ClientPacketListener", "net.minecraft.class_634", "Lnet/minecraft/client/multiplayer/ClientPacketListener;")
};

static const MethodCandidate kInstanceLegacy[] = {
    METHOD("getMinecraft", "func_71410_x", "getInstance", "()Lnet/minecraft/client/Minecraft;", true)
};
static const MethodCandidate kInstanceModern[] = {
    METHOD("getInstance", "m_91087_", "getInstance", "()Lnet/minecraft/client/Minecraft;", true)
};
static const MethodCandidate kMinecraftConnectionLegacy[] = {
    METHOD("getConnection", "func_147114_u", "getNetworkHandler", "()Lnet/minecraft/client/network/NetHandlerPlayClient;", false)
};
static const MethodCandidate kMinecraftConnectionModern[] = {
    METHOD("getConnection", "m_91403_", "getNetworkHandler", "()Lnet/minecraft/client/multiplayer/ClientPacketListener;", false)
};
static const MethodCandidate kListenerConnectionLegacy[] = {
    METHOD("getNetworkManager", "func_147298_b", "getConnection", "()Lnet/minecraft/network/NetworkManager;", false)
};
static const MethodCandidate kListenerConnectionModern[] = {
    METHOD("getConnection", "m_104910_", "getConnection", "()Lnet/minecraft/network/Connection;", false)
};

struct Family {
    ProtocolFamily protocol;
    const char* name;
    const ClassCandidate* connection;
    const ClassCandidate* listener;
    const MethodCandidate* instance;
    const MethodCandidate* minecraftConnection;
    const MethodCandidate* listenerConnection;
};

static const Family kLegacy{ProtocolFamily::Legacy18To112, "legacy-1.8-to-1.12", kConnectionLegacy, kListenerLegacy, kInstanceLegacy, kMinecraftConnectionLegacy, kListenerConnectionLegacy};
static const Family kTransitional{ProtocolFamily::Transitional113To116, "transitional-1.13-to-1.16", kConnectionModern, kListenerModern, kInstanceModern, kMinecraftConnectionModern, kListenerConnectionModern};
static const Family kModern{ProtocolFamily::Modern117To120, "modern-1.17-to-1.20", kConnectionModern, kListenerModern, kInstanceModern, kMinecraftConnectionModern, kListenerConnectionModern};
static const Family kModern121{ProtocolFamily::Modern121Plus, "modern-1.21-plus", kConnectionModern, kListenerModern, kInstanceModern, kMinecraftConnectionModern, kListenerConnectionModern};

const Family& FamilyFor(MinecraftVersion version) {
    switch (version) {
    case MinecraftVersion::V1_8_0:
    case MinecraftVersion::V1_8_8:
    case MinecraftVersion::V1_8_9:
    case MinecraftVersion::V1_9_4:
    case MinecraftVersion::V1_10_2:
    case MinecraftVersion::V1_11_2:
    case MinecraftVersion::V1_12_2:
        return kLegacy;
    case MinecraftVersion::V1_13_2:
    case MinecraftVersion::V1_14_4:
    case MinecraftVersion::V1_15_2:
    case MinecraftVersion::V1_16_5:
        return kTransitional;
    case MinecraftVersion::V1_17_1:
    case MinecraftVersion::V1_18_2:
    case MinecraftVersion::V1_18_3:
    case MinecraftVersion::V1_19_2:
    case MinecraftVersion::V1_19_4:
    case MinecraftVersion::V1_20_1:
    case MinecraftVersion::V1_20_2:
    case MinecraftVersion::V1_20_4:
    case MinecraftVersion::V1_20_6:
        return kModern;
    case MinecraftVersion::V1_21_0:
    case MinecraftVersion::V1_21_1:
    case MinecraftVersion::V1_21_3:
        return kModern121;
    default:
        return kModern;
    }
}

const char* LoaderName(ModLoader loader) {
    switch (loader) {
    case ModLoader::Vanilla: return "vanilla";
    case ModLoader::Forge: return "forge";
    case ModLoader::NeoForge: return "neoforge";
    case ModLoader::Fabric: return "fabric";
    case ModLoader::Quilt: return "quilt";
    default: return "auto";
    }
}

} // namespace

const char* AdapterStatusName(AdapterStatus status) {
    switch (status) {
    case AdapterStatus::Experimental: return "experimental";
    case AdapterStatus::ReadyForBindingValidation: return "binding-validation";
    case AdapterStatus::ProductionValidated: return "production-validated";
    default: return "unknown";
    }
}

const char* ProtocolFamilyName(ProtocolFamily family) {
    switch (family) {
    case ProtocolFamily::Legacy18To112: return "legacy-1.8-1.12";
    case ProtocolFamily::Transitional113To116: return "transitional-1.13-1.16";
    case ProtocolFamily::Modern117To120: return "modern-1.17-1.20";
    case ProtocolFamily::Modern121Plus: return "modern-1.21+";
    default: return "unknown";
    }
}

jclass ResolveClass(JNIEnv* env, jobject classLoader, const ClassCandidate& candidate) {
    const char* names[] = {candidate.officialName, candidate.srgName, candidate.yarnName};
    for (const char* name : names) {
        if (!name || !*name) continue;
        jclass klass = LoadClassInLoader(env, classLoader, name);
        if (klass) return klass;
        if (env->ExceptionCheck()) env->ExceptionClear();
    }
    // A descriptor is a JVM signature, not a binary class name. This final
    // fallback only searches classes that JVMTI reports as already loaded.
    return candidate.descriptor && *candidate.descriptor
        ? proxy_server::findLoadedBySig(env, candidate.descriptor) : nullptr;
}

jmethodID ResolveMethod(JNIEnv* env, jclass klass, const MethodCandidate& candidate) {
    if (!klass || !candidate.descriptor) return nullptr;
    const char* names[] = {candidate.officialName, candidate.srgName, candidate.yarnName};
    for (const char* name : names) {
        if (!name || !*name) continue;
        jmethodID id = candidate.isStatic
            ? env->GetStaticMethodID(klass, name, candidate.descriptor)
            : env->GetMethodID(klass, name, candidate.descriptor);
        if (id) return id;
        if (env->ExceptionCheck()) env->ExceptionClear();
    }
    return proxy_server::findMethodByDescriptor(klass, candidate.descriptor, candidate.isStatic);
}

const RuntimeAdapter* FindRuntimeAdapter(MinecraftVersion version, ModLoader loader) {
    static RuntimeAdapter adapter;
    if (version == MinecraftVersion::Auto) return nullptr;
    if (loader == ModLoader::Auto) loader = ModLoader::Vanilla;
    const Family& family = FamilyFor(version);
    static char display[128];
    std::snprintf(display, sizeof(display), "%s %s (%s)", ToString(version), LoaderName(loader), family.name);
    adapter = {version, loader, family.protocol, display, AdapterStatus::ReadyForBindingValidation,
               kMinecraft, 1, family.connection, 1, family.listener, 1,
               family.instance, 1, family.minecraftConnection, 1, family.listenerConnection, 1};
    return &adapter;
}

const RuntimeAdapter* SelectRuntimeAdapter(const RuntimeProfile& profile) {
    return FindRuntimeAdapter(profile.version, profile.loader);
}

bool ValidateRuntimeAdapter(JNIEnv* env, jobject classLoader, const RuntimeAdapter& adapter) {
    jclass minecraft = nullptr;
    jclass listener = nullptr;
    bool connectionFound = false;

    for (size_t i = 0; i < adapter.minecraftClassCount && !minecraft; ++i)
        minecraft = ResolveClass(env, classLoader, adapter.minecraftClasses[i]);
    for (size_t i = 0; i < adapter.listenerClassCount && !listener; ++i)
        listener = ResolveClass(env, classLoader, adapter.listenerClasses[i]);
    for (size_t i = 0; i < adapter.connectionClassCount && !connectionFound; ++i) {
        jclass connection = ResolveClass(env, classLoader, adapter.connectionClasses[i]);
        if (connection) {
            connectionFound = true;
            env->DeleteLocalRef(connection);
        }
    }

    bool methodsFound = false;
    if (minecraft && listener && adapter.minecraftInstanceCount && adapter.minecraftConnectionCount && adapter.listenerConnectionCount) {
        jmethodID instance = ResolveMethod(env, minecraft, adapter.minecraftInstance[0]);
        jmethodID mcConnection = ResolveMethod(env, minecraft, adapter.minecraftConnection[0]);
        jmethodID listenerConnection = ResolveMethod(env, listener, adapter.listenerConnection[0]);
        methodsFound = instance && mcConnection && listenerConnection;
    }

    if (minecraft) env->DeleteLocalRef(minecraft);
    if (listener) env->DeleteLocalRef(listener);
    return minecraft != nullptr && listener != nullptr && connectionFound && methodsFound;
}

void LogRuntimeAdapter(const RuntimeAdapter& adapter) {
    LogTo("Adapter: %s | status=%s | protocol=%s | connection=%s | listener=%s",
          adapter.displayName, AdapterStatusName(adapter.status), ProtocolFamilyName(adapter.protocol),
          adapter.connectionClasses[0].officialName, adapter.listenerClasses[0].officialName);
}

} // namespace sakura
