#include "runtime_adapter.h"
#include "jni_lookup.h"
#include "proxy.h"

#include <cstring>

namespace sakura {
namespace {

static const ClassCandidate kLegacyConnection[] = {
    {"net.minecraft.network.NetworkManager", "net.minecraft.network.NetworkManager", "net.minecraft.class_2535", "Lnet/minecraft/network/NetworkManager;"},
};
static const ClassCandidate kModernConnection[] = {
    {"net.minecraft.network.Connection", "net.minecraft.network.NetworkManager", "net.minecraft.class_2535", "Lnet/minecraft/network/Connection;"},
};
static const ClassCandidate kLegacyListener[] = {
    {"net.minecraft.client.network.NetHandlerPlayClient", "net.minecraft.client.network.NetHandlerPlayClient", "net.minecraft.class_634", "Lnet/minecraft/client/network/NetHandlerPlayClient;"},
};
static const ClassCandidate kModernListener[] = {
    {"net.minecraft.client.multiplayer.ClientPacketListener", "net.minecraft.client.multiplayer.ClientPacketListener", "net.minecraft.class_634", "Lnet/minecraft/client/multiplayer/ClientPacketListener;"},
};

static const MethodCandidate kLegacyMinecraftInstance[] = {
    {"getMinecraft", "func_71410_x", "", "()Lnet/minecraft/client/Minecraft;", true},
};
static const MethodCandidate kModernMinecraftInstance[] = {
    {"getInstance", "m_91087_", "", "()Lnet/minecraft/client/Minecraft;", true},
};
static const MethodCandidate kLegacyMinecraftConnection[] = {
    {"getConnection", "func_147114_u", "", "()Lnet/minecraft/client/network/NetHandlerPlayClient;", false},
};
static const MethodCandidate kModernMinecraftConnection[] = {
    {"getConnection", "m_91403_", "", "()Lnet/minecraft/client/multiplayer/ClientPacketListener;", false},
};
static const MethodCandidate kLegacyListenerConnection[] = {
    {"getNetworkManager", "func_147298_b", "", "()Lnet/minecraft/network/NetworkManager;", false},
};
static const MethodCandidate kModernListenerConnection[] = {
    {"getConnection", "m_104910_", "", "()Lnet/minecraft/network/Connection;", false},
};

struct Family {
    bool legacy;
    const char* name;
    const ClassCandidate* connection;
    const ClassCandidate* listener;
    const MethodCandidate* mcInstance;
    const MethodCandidate* mcConnection;
    const MethodCandidate* listenerConnection;
};

static const Family kLegacyFamily{true, "legacy", kLegacyConnection, kLegacyListener, kLegacyMinecraftInstance, kLegacyMinecraftConnection, kLegacyListenerConnection};
static const Family kModernFamily{false, "modern", kModernConnection, kModernListener, kModernMinecraftInstance, kModernMinecraftConnection, kModernListenerConnection};

bool IsLegacy(MinecraftVersion version) {
    switch (version) {
    case MinecraftVersion::V1_8_0:
    case MinecraftVersion::V1_8_8:
    case MinecraftVersion::V1_8_9:
    case MinecraftVersion::V1_9_4:
    case MinecraftVersion::V1_10_2:
    case MinecraftVersion::V1_11_2:
    case MinecraftVersion::V1_12_2:
        return true;
    default:
        return false;
    }
}

const Family* FamilyFor(MinecraftVersion version) {
    return IsLegacy(version) ? &kLegacyFamily : &kModernFamily;
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

jclass FindClassCandidate(JNIEnv* env, jobject loader, const ClassCandidate& candidate) {
    const char* names[] = {candidate.officialName, candidate.srgName, candidate.yarnName};
    for (const char* name : names) {
        if (!name || !*name)
            continue;
        jclass klass = LoadClassInLoader(env, loader, name);
        if (klass)
            return klass;
        if (env->ExceptionCheck())
            env->ExceptionClear();
    }
    // A JVM signature is not a class name. FindLoadedBySig is therefore only a
    // last-resort check for an already loaded class, never a fabricated lookup.
    if (candidate.descriptor && *candidate.descriptor)
        return proxy_server::findLoadedBySig(env, candidate.descriptor);
    return nullptr;
}

} // namespace

const RuntimeAdapter* FindRuntimeAdapter(MinecraftVersion version, ModLoader loader) {
    static RuntimeAdapter adapter;
    if (version == MinecraftVersion::Auto)
        return nullptr;
    if (loader == ModLoader::Auto)
        loader = ModLoader::Vanilla;
    const Family* family = FamilyFor(version);
    adapter = RuntimeAdapter{
        version, loader, nullptr,
        family->connection, 1,
        family->listener, 1,
        family->mcInstance, 1,
        family->mcConnection, 1,
        family->listenerConnection, 1,
        nullptr, 0,
    };
    static char display[96];
    std::snprintf(display, sizeof(display), "%s %s (%s)", ToString(version), LoaderName(loader), family->name);
    adapter.displayName = display;
    return &adapter;
}

const RuntimeAdapter* SelectRuntimeAdapter(const RuntimeProfile& profile) {
    return FindRuntimeAdapter(profile.version, profile.loader);
}

bool ValidateRuntimeAdapter(JNIEnv* env, jobject classLoader, const RuntimeAdapter& adapter) {
    bool connection = false;
    bool listener = false;
    for (size_t i = 0; i < adapter.connectionClassCount; ++i) {
        jclass klass = FindClassCandidate(env, classLoader, adapter.connectionClasses[i]);
        if (klass) { connection = true; env->DeleteLocalRef(klass); break; }
    }
    for (size_t i = 0; i < adapter.listenerClassCount; ++i) {
        jclass klass = FindClassCandidate(env, classLoader, adapter.listenerClasses[i]);
        if (klass) { listener = true; env->DeleteLocalRef(klass); break; }
    }
    return connection && listener;
}

void LogRuntimeAdapter(const RuntimeAdapter& adapter) {
    LogTo("Adapter: %s | connection=%s | listener=%s", adapter.displayName,
          adapter.connectionClasses[0].officialName, adapter.listenerClasses[0].officialName);
}

} // namespace sakura
