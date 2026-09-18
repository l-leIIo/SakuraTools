#include "runtime_adapter.h"
#include "jni_lookup.h"
#include "proxy.h"

#include <cstring>

namespace sakura {
namespace {

static const ClassCandidate kLegacyConnection[] = {
    {"net.minecraft.network.NetworkManager", "func_150729_e", "net.minecraft.class_2535", "Lnet/minecraft/network/NetworkManager;"},
};
static const ClassCandidate kModernConnection[] = {
    {"net.minecraft.network.Connection", "NetworkManager", "net.minecraft.class_2535", "Lnet/minecraft/network/Connection;"},
};
static const ClassCandidate kLegacyListener[] = {
    {"net.minecraft.client.network.NetHandlerPlayClient", "field_147125_b", "net.minecraft.class_634", "Lnet/minecraft/client/network/NetHandlerPlayClient;"},
};
static const ClassCandidate kModernListener[] = {
    {"net.minecraft.client.multiplayer.ClientPacketListener", "f_104699_", "net.minecraft.class_634", "Lnet/minecraft/client/multiplayer/ClientPacketListener;"},
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
static const MethodCandidate kLegacyHelloName[] = {
    {"getName", "field_149306_a", "", "()Ljava/lang/String;", false},
};
static const MethodCandidate kModernHelloName[] = {
    {"getName", "m_123097_", "", "()Ljava/lang/String;", false},
};

static const RuntimeAdapter kAdapters[] = {
    {MinecraftVersion::V1_8_0, ModLoader::Vanilla, "1.8.x vanilla legacy", kLegacyConnection, 1, kLegacyListener, 1, kLegacyMinecraftInstance, 1, kLegacyMinecraftConnection, 1, kLegacyListenerConnection, 1, kLegacyHelloName, 1},
    {MinecraftVersion::V1_8_0, ModLoader::Forge, "1.8.x Forge legacy", kLegacyConnection, 1, kLegacyListener, 1, kLegacyMinecraftInstance, 1, kLegacyMinecraftConnection, 1, kLegacyListenerConnection, 1, kLegacyHelloName, 1},
    {MinecraftVersion::V1_8_8, ModLoader::Vanilla, "1.8.8 vanilla legacy", kLegacyConnection, 1, kLegacyListener, 1, kLegacyMinecraftInstance, 1, kLegacyMinecraftConnection, 1, kLegacyListenerConnection, 1, kLegacyHelloName, 1},
    {MinecraftVersion::V1_8_8, ModLoader::Forge, "1.8.8 Forge legacy", kLegacyConnection, 1, kLegacyListener, 1, kLegacyMinecraftInstance, 1, kLegacyMinecraftConnection, 1, kLegacyListenerConnection, 1, kLegacyHelloName, 1},
    {MinecraftVersion::V1_8_9, ModLoader::Vanilla, "1.8.9 vanilla legacy", kLegacyConnection, 1, kLegacyListener, 1, kLegacyMinecraftInstance, 1, kLegacyMinecraftConnection, 1, kLegacyListenerConnection, 1, kLegacyHelloName, 1},
    {MinecraftVersion::V1_8_9, ModLoader::Forge, "1.8.9 Forge legacy", kLegacyConnection, 1, kLegacyListener, 1, kLegacyMinecraftInstance, 1, kLegacyMinecraftConnection, 1, kLegacyListenerConnection, 1, kLegacyHelloName, 1},
    {MinecraftVersion::V1_9_4, ModLoader::Vanilla, "1.9.4 vanilla legacy", kLegacyConnection, 1, kLegacyListener, 1, kLegacyMinecraftInstance, 1, kLegacyMinecraftConnection, 1, kLegacyListenerConnection, 1, kLegacyHelloName, 1},
    {MinecraftVersion::V1_9_4, ModLoader::Forge, "1.9.4 Forge legacy", kLegacyConnection, 1, kLegacyListener, 1, kLegacyMinecraftInstance, 1, kLegacyMinecraftConnection, 1, kLegacyListenerConnection, 1, kLegacyHelloName, 1},
    {MinecraftVersion::V1_10_2, ModLoader::Vanilla, "1.10.2 vanilla legacy", kLegacyConnection, 1, kLegacyListener, 1, kLegacyMinecraftInstance, 1, kLegacyMinecraftConnection, 1, kLegacyListenerConnection, 1, kLegacyHelloName, 1},
    {MinecraftVersion::V1_10_2, ModLoader::Forge, "1.10.2 Forge legacy", kLegacyConnection, 1, kLegacyListener, 1, kLegacyMinecraftInstance, 1, kLegacyMinecraftConnection, 1, kLegacyListenerConnection, 1, kLegacyHelloName, 1},
    {MinecraftVersion::V1_11_2, ModLoader::Vanilla, "1.11.2 vanilla legacy", kLegacyConnection, 1, kLegacyListener, 1, kLegacyMinecraftInstance, 1, kLegacyMinecraftConnection, 1, kLegacyListenerConnection, 1, kLegacyHelloName, 1},
    {MinecraftVersion::V1_11_2, ModLoader::Forge, "1.11.2 Forge legacy", kLegacyConnection, 1, kLegacyListener, 1, kLegacyMinecraftInstance, 1, kLegacyMinecraftConnection, 1, kLegacyListenerConnection, 1, kLegacyHelloName, 1},
    {MinecraftVersion::V1_12_2, ModLoader::Vanilla, "1.12.2 vanilla legacy", kLegacyConnection, 1, kLegacyListener, 1, kLegacyMinecraftInstance, 1, kLegacyMinecraftConnection, 1, kLegacyListenerConnection, 1, kLegacyHelloName, 1},
    {MinecraftVersion::V1_12_2, ModLoader::Forge, "1.12.2 Forge legacy", kLegacyConnection, 1, kLegacyListener, 1, kLegacyMinecraftInstance, 1, kLegacyMinecraftConnection, 1, kLegacyListenerConnection, 1, kLegacyHelloName, 1},
    {MinecraftVersion::V1_13_2, ModLoader::Vanilla, "1.13.2 vanilla transitional", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_13_2, ModLoader::Forge, "1.13.2 Forge transitional", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_14_4, ModLoader::Vanilla, "1.14.4 vanilla transitional", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_14_4, ModLoader::Forge, "1.14.4 Forge transitional", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_15_2, ModLoader::Vanilla, "1.15.2 vanilla transitional", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_15_2, ModLoader::Forge, "1.15.2 Forge transitional", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_16_5, ModLoader::Vanilla, "1.16.5 vanilla transitional", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_16_5, ModLoader::Forge, "1.16.5 Forge transitional", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_16_5, ModLoader::Fabric, "1.16.5 Fabric transitional", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_17_1, ModLoader::Vanilla, "1.17.1 vanilla modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_17_1, ModLoader::Forge, "1.17.1 Forge modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_18_2, ModLoader::Vanilla, "1.18.2 vanilla modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_18_2, ModLoader::Forge, "1.18.2 Forge modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_18_2, ModLoader::Fabric, "1.18.2 Fabric modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_18_2, ModLoader::Quilt, "1.18.2 Quilt modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_18_3, ModLoader::Vanilla, "1.18.3 vanilla modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_18_3, ModLoader::Forge, "1.18.3 Forge modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_19_2, ModLoader::Vanilla, "1.19.2 vanilla modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_19_2, ModLoader::Forge, "1.19.2 Forge modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_19_2, ModLoader::Fabric, "1.19.2 Fabric modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_19_2, ModLoader::Quilt, "1.19.2 Quilt modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_19_4, ModLoader::Vanilla, "1.19.4 vanilla modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_19_4, ModLoader::Forge, "1.19.4 Forge modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_19_4, ModLoader::NeoForge, "1.19.4 NeoForge modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_19_4, ModLoader::Fabric, "1.19.4 Fabric modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_19_4, ModLoader::Quilt, "1.19.4 Quilt modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_20_1, ModLoader::Vanilla, "1.20.1 vanilla modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_20_1, ModLoader::Forge, "1.20.1 Forge modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_20_1, ModLoader::NeoForge, "1.20.1 NeoForge modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_20_1, ModLoader::Fabric, "1.20.1 Fabric modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_20_1, ModLoader::Quilt, "1.20.1 Quilt modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_20_2, ModLoader::Vanilla, "1.20.2 vanilla modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_20_2, ModLoader::Forge, "1.20.2 Forge modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_20_2, ModLoader::NeoForge, "1.20.2 NeoForge modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_20_4, ModLoader::Vanilla, "1.20.4 vanilla modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_20_4, ModLoader::Forge, "1.20.4 Forge modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_20_4, ModLoader::NeoForge, "1.20.4 NeoForge modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_20_4, ModLoader::Fabric, "1.20.4 Fabric modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_20_4, ModLoader::Quilt, "1.20.4 Quilt modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_20_6, ModLoader::Vanilla, "1.20.6 vanilla modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_20_6, ModLoader::NeoForge, "1.20.6 NeoForge modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_21_0, ModLoader::Vanilla, "1.21.0 vanilla modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_21_0, ModLoader::Fabric, "1.21.0 Fabric modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_21_0, ModLoader::NeoForge, "1.21.0 NeoForge modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_21_1, ModLoader::Vanilla, "1.21.1 vanilla modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_21_1, ModLoader::Fabric, "1.21.1 Fabric modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_21_1, ModLoader::NeoForge, "1.21.1 NeoForge modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_21_3, ModLoader::Vanilla, "1.21.3 vanilla modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_21_3, ModLoader::Fabric, "1.21.3 Fabric modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
    {MinecraftVersion::V1_21_3, ModLoader::NeoForge, "1.21.3 NeoForge modern", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, 1, kModernMinecraftConnection, 1, kModernListenerConnection, 1, kModernHelloName, 1},
};

jclass FindClassCandidate(JNIEnv* env, jobject loader, const ClassCandidate& candidate) {
    if (candidate.officialName && *candidate.officialName) {
        jclass klass = LoadClassInLoader(env, loader, candidate.officialName);
        if (klass) return klass;
    }
    if (candidate.srgName && *candidate.srgName) {
        jclass klass = LoadClassInLoader(env, loader, candidate.srgName);
        if (klass) return klass;
    }
    if (candidate.yarnName && *candidate.yarnName) {
        jclass klass = LoadClassInLoader(env, loader, candidate.yarnName);
        if (klass) return klass;
    }
    if (candidate.descriptor && *candidate.descriptor) {
        // Descriptor fallback: if already loaded by the JVM, resolve by signature.
        jclass klass = proxy_server::findLoadedBySig(env, candidate.descriptor);
        if (klass) return klass;
    }
    return nullptr;
}

jmethodID FindMethodCandidate(JNIEnv* env, jclass klass, const MethodCandidate& candidate) {
    if (candidate.officialName && *candidate.officialName) {
        jmethodID id = env->GetMethodID(klass, candidate.officialName, candidate.descriptor);
        if (id || !env->ExceptionCheck()) return id;
        env->ExceptionClear();
    }
    if (candidate.srgName && *candidate.srgName) {
        jmethodID id = env->GetMethodID(klass, candidate.srgName, candidate.descriptor);
        if (id || !env->ExceptionCheck()) return id;
        env->ExceptionClear();
    }
    if (candidate.yarnName && *candidate.yarnName) {
        jmethodID id = env->GetMethodID(klass, candidate.yarnName, candidate.descriptor);
        if (id || !env->ExceptionCheck()) return id;
        env->ExceptionClear();
    }
    if (candidate.descriptor && *candidate.descriptor) {
        return proxy_server::findMethodByDescriptor(klass, candidate.descriptor, candidate.isStatic);
    }
    return nullptr;
}

const RuntimeAdapter* FindExact(MinecraftVersion version, ModLoader loader) {
    for (const RuntimeAdapter& adapter : kAdapters)
        if (adapter.version == version && adapter.loader == loader)
            return &adapter;
    return nullptr;
}

const RuntimeAdapter* FindNearestVersion(MinecraftVersion version) {
    int currentBest = INT_MAX;
    const RuntimeAdapter* best = nullptr;
    for (const RuntimeAdapter& adapter : kAdapters) {
        if (adapter.loader != ModLoader::Vanilla && adapter.loader != ModLoader::Forge && adapter.loader != ModLoader::NeoForge && adapter.loader != ModLoader::Fabric && adapter.loader != ModLoader::Quilt)
            continue;
        int delta = std::abs(static_cast<int>(adapter.version) - static_cast<int>(version));
        if (delta < currentBest) {
            currentBest = delta;
            best = &adapter;
        }
    }
    return best;
}

} // namespace

const RuntimeAdapter* FindRuntimeAdapter(MinecraftVersion version, ModLoader loader) {
    if (loader == ModLoader::Auto) loader = ModLoader::Vanilla;
    if (const RuntimeAdapter* exact = FindExact(version, loader))
        return exact;
    if (const RuntimeAdapter* fallback = FindExact(version, ModLoader::Forge))
        return fallback;
    if (const RuntimeAdapter* fallback = FindExact(version, ModLoader::Vanilla))
        return fallback;
    if (const RuntimeAdapter* nearest = FindNearestVersion(version))
        return nearest;
    return nullptr;
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
