#include "runtime_adapter.h"
#include "jni_lookup.h"
#include "proxy.h"

#include <cstdio>
#include <cstring>

namespace sakura {
namespace {

#define CLASS(OFFICIAL, SRG, YARN, DESC) {OFFICIAL, SRG, YARN, DESC}
#define METHOD(OFFICIAL, SRG, YARN, DESC, STATIC) {OFFICIAL, SRG, YARN, DESC, STATIC}

static const ClassCandidate kMinecraft[] = {
    CLASS("net.minecraft.client.Minecraft", "net.minecraft.client.Minecraft", "net.minecraft.class_310", "Lnet/minecraft/client/Minecraft;")
};
static const ClassCandidate kLegacyConnection[] = {
    CLASS("net.minecraft.network.NetworkManager", "net.minecraft.network.NetworkManager", "net.minecraft.class_2535", "Lnet/minecraft/network/NetworkManager;")
};
static const ClassCandidate kModernConnection[] = {
    CLASS("net.minecraft.network.Connection", "net.minecraft.network.Connection", "net.minecraft.class_2535", "Lnet/minecraft/network/Connection;")
};
static const ClassCandidate kLegacyListener[] = {
    CLASS("net.minecraft.client.network.NetHandlerPlayClient", "net.minecraft.client.network.NetHandlerPlayClient", "net.minecraft.class_634", "Lnet/minecraft/client/network/NetHandlerPlayClient;")
};
static const ClassCandidate kModernListener[] = {
    CLASS("net.minecraft.client.multiplayer.ClientPacketListener", "net.minecraft.client.multiplayer.ClientPacketListener", "net.minecraft.class_634", "Lnet/minecraft/client/multiplayer/ClientPacketListener;")
};
static const MethodCandidate kLegacyMinecraftInstance[] = {
    METHOD("getMinecraft", "func_71410_x", "getInstance", "()Lnet/minecraft/client/Minecraft;", true)
};
static const MethodCandidate kModernMinecraftInstance[] = {
    METHOD("getInstance", "m_91087_", "getInstance", "()Lnet/minecraft/client/Minecraft;", true)
};
static const MethodCandidate kLegacyMinecraftConnection[] = {
    METHOD("getConnection", "func_147114_u", "getNetworkHandler", "()Lnet/minecraft/client/network/NetHandlerPlayClient;", false)
};
static const MethodCandidate kModernMinecraftConnection[] = {
    METHOD("getConnection", "m_91403_", "getNetworkHandler", "()Lnet/minecraft/client/multiplayer/ClientPacketListener;", false)
};
static const MethodCandidate kLegacyListenerConnection[] = {
    METHOD("getNetworkManager", "func_147298_b", "getConnection", "()Lnet/minecraft/network/NetworkManager;", false)
};
static const MethodCandidate kModernListenerConnection[] = {
    METHOD("getConnection", "m_104910_", "getConnection", "()Lnet/minecraft/network/Connection;", false)
};

struct Family {
    const char* name;
    const ClassCandidate* connection;
    const ClassCandidate* listener;
    const MethodCandidate* instance;
    const MethodCandidate* mcConnection;
    const MethodCandidate* listenerConnection;
};
static const Family kLegacy{"legacy", kLegacyConnection, kLegacyListener, kLegacyMinecraftInstance, kLegacyMinecraftConnection, kLegacyListenerConnection};
static const Family kModern{"modern", kModernConnection, kModernListener, kModernMinecraftInstance, kModernMinecraftConnection, kModernListenerConnection};

bool Legacy(MinecraftVersion v) {
    return v == MinecraftVersion::V1_8_0 || v == MinecraftVersion::V1_8_8 ||
           v == MinecraftVersion::V1_8_9 || v == MinecraftVersion::V1_9_4 ||
           v == MinecraftVersion::V1_10_2 || v == MinecraftVersion::V1_11_2 ||
           v == MinecraftVersion::V1_12_2;
}
const Family& FamilyFor(MinecraftVersion v) { return Legacy(v) ? kLegacy : kModern; }
const char* LoaderName(ModLoader l) {
    switch (l) {
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

jclass ResolveClass(JNIEnv* env, jobject classLoader, const ClassCandidate& candidate) {
    const char* names[] = {candidate.officialName, candidate.srgName, candidate.yarnName};
    for (const char* name : names) {
        if (!name || !*name) continue;
        jclass klass = LoadClassInLoader(env, classLoader, name);
        if (klass) return klass;
        if (env->ExceptionCheck()) env->ExceptionClear();
    }
    // A descriptor is a signature, not a Java binary name. This fallback is only
    // valid for an already-loaded class and never invents a class from a suffix.
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
    static char display[96];
    std::snprintf(display, sizeof(display), "%s %s (%s)", ToString(version), LoaderName(loader), family.name);
    adapter = {version, loader, display, AdapterStatus::Experimental,
               kMinecraft, 1, family.connection, 1, family.listener, 1,
               family.instance, 1, family.mcConnection, 1, family.listenerConnection, 1};
    return &adapter;
}

const RuntimeAdapter* SelectRuntimeAdapter(const RuntimeProfile& profile) {
    return FindRuntimeAdapter(profile.version, profile.loader);
}

bool ValidateRuntimeAdapter(JNIEnv* env, jobject classLoader, const RuntimeAdapter& adapter) {
    bool minecraft = false, connection = false, listener = false;
    jclass mc = nullptr;
    for (size_t i = 0; i < adapter.minecraftClassCount; ++i) {
        mc = ResolveClass(env, classLoader, adapter.minecraftClasses[i]);
        if (mc) { minecraft = true; break; }
    }
    for (size_t i = 0; i < adapter.connectionClassCount; ++i) {
        jclass c = ResolveClass(env, classLoader, adapter.connectionClasses[i]);
        if (c) { connection = true; env->DeleteLocalRef(c); break; }
    }
    jclass listener = nullptr;
    for (size_t i = 0; i < adapter.listenerClassCount; ++i) {
        listener = ResolveClass(env, classLoader, adapter.listenerClasses[i]);
        if (listener) { listener = listener; break; }
    }
    listener = listener;
    if (listener) listener = listener;
    listener = nullptr;
    // Resolve again without retaining a stale local reference from a failed path.
    for (size_t i = 0; i < adapter.listenerClassCount; ++i) {
        jclass c = ResolveClass(env, classLoader, adapter.listenerClasses[i]);
        if (c) { listener = c; break; }
    }
    if (listener) { listener = listener; }
    listener = listener;
    bool methods = false;
    if (mc && listener) {
        jmethodID instance = ResolveMethod(env, mc, adapter.minecraftInstance[0]);
        jmethodID mcConnection = ResolveMethod(env, mc, adapter.minecraftConnection[0]);
        jmethodID listenerConnection = ResolveMethod(env, listener, adapter.listenerConnection[0]);
        methods = instance && mcConnection && listenerConnection;
    }
    if (mc) env->DeleteLocalRef(mc);
    if (listener) env->DeleteLocalRef(listener);
    return minecraft && connection && listener && methods;
}

void LogRuntimeAdapter(const RuntimeAdapter& adapter) {
    LogTo("Adapter: %s | status=%s | connection=%s | listener=%s", adapter.displayName,
          AdapterStatusName(adapter.status), adapter.connectionClasses[0].officialName,
          adapter.listenerClasses[0].officialName);
}

} // namespace sakura
