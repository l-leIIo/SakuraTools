#include "runtime_adapter.h"
#include "jni_lookup.h"
#include "proxy.h"

#include <cstring>

namespace sakura {
namespace {

// These are real runtime class names. Forge/NeoForge use Mojang names in modern
// development mappings; Fabric/Quilt additionally expose the same classes under
// intermediary/Yarn names. The descriptor is used as a final structural check.
static const ClassCandidate kModernConnection[] = {
    {"net.minecraft.network.Connection", "net.minecraft.class_2535", "Lnet/minecraft/network/Connection;"},
};
static const ClassCandidate kLegacyConnection[] = {
    {"net.minecraft.network.NetworkManager", "net.minecraft.network.NetworkManager", "Lnet/minecraft/network/NetworkManager;"},
};
static const ClassCandidate kModernListener[] = {
    {"net.minecraft.client.multiplayer.ClientPacketListener", "net.minecraft.class_634", "Lnet/minecraft/client/multiplayer/ClientPacketListener;"},
};
static const ClassCandidate kLegacyListener[] = {
    {"net.minecraft.client.network.NetHandlerPlayClient", "net.minecraft.client.network.NetHandlerPlayClient", "Lnet/minecraft/client/network/NetHandlerPlayClient;"},
};

static const MethodCandidate kModernMinecraftInstance[] = {
    {"getInstance", "m_91087_", "()Lnet/minecraft/client/Minecraft;", true},
};
static const MethodCandidate kLegacyMinecraftInstance[] = {
    {"getMinecraft", "func_71410_x", "()Lnet/minecraft/client/Minecraft;", true},
};
static const MethodCandidate kModernMinecraftConnection[] = {
    {"getConnection", "m_91403_", "()Lnet/minecraft/client/multiplayer/ClientPacketListener;", false},
};
static const MethodCandidate kLegacyMinecraftConnection[] = {
    {"getConnection", "func_147114_u", "()Lnet/minecraft/client/network/NetHandlerPlayClient;", false},
};
static const MethodCandidate kModernListenerConnection[] = {
    {"getConnection", "m_104910_", "()Lnet/minecraft/network/Connection;", false},
};
static const MethodCandidate kLegacyListenerConnection[] = {
    {"getNetworkManager", "func_147298_b", "()Lnet/minecraft/network/NetworkManager;", false},
};
static const MethodCandidate kModernHelloName[] = {
    {"getName", "", "()Ljava/lang/String;", false},
};
static const MethodCandidate kLegacyHelloName[] = {
    {"getName", "field_149306_a", "Ljava/lang/String;", false},
};

static const RuntimeAdapter kAdapters[] = {
    {MinecraftVersion::V1_8_9, ModLoader::Vanilla, "1.8.9 vanilla", kLegacyConnection, 1, kLegacyListener, 1, kLegacyMinecraftInstance, kLegacyMinecraftConnection, kLegacyListenerConnection, kLegacyHelloName},
    {MinecraftVersion::V1_8_9, ModLoader::Forge, "1.8.9 Forge", kLegacyConnection, 1, kLegacyListener, 1, kLegacyMinecraftInstance, kLegacyMinecraftConnection, kLegacyListenerConnection, kLegacyHelloName},
    {MinecraftVersion::V1_12_2, ModLoader::Vanilla, "1.12.2 vanilla", kLegacyConnection, 1, kLegacyListener, 1, kLegacyMinecraftInstance, kLegacyMinecraftConnection, kLegacyListenerConnection, kLegacyHelloName},
    {MinecraftVersion::V1_12_2, ModLoader::Forge, "1.12.2 Forge", kLegacyConnection, 1, kLegacyListener, 1, kLegacyMinecraftInstance, kLegacyMinecraftConnection, kLegacyListenerConnection, kLegacyHelloName},
    {MinecraftVersion::V1_16_5, ModLoader::Vanilla, "1.16.5 vanilla", kLegacyConnection, 1, kLegacyListener, 1, kLegacyMinecraftInstance, kLegacyMinecraftConnection, kLegacyListenerConnection, kLegacyHelloName},
    {MinecraftVersion::V1_16_5, ModLoader::Forge, "1.16.5 Forge", kLegacyConnection, 1, kLegacyListener, 1, kLegacyMinecraftInstance, kLegacyMinecraftConnection, kLegacyListenerConnection, kLegacyHelloName},
    {MinecraftVersion::V1_16_5, ModLoader::Fabric, "1.16.5 Fabric", kLegacyConnection, 1, kLegacyListener, 1, kLegacyMinecraftInstance, kLegacyMinecraftConnection, kLegacyListenerConnection, kLegacyHelloName},
    {MinecraftVersion::V1_18_2, ModLoader::Vanilla, "1.18.2 vanilla", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, kModernMinecraftConnection, kModernListenerConnection, kModernHelloName},
    {MinecraftVersion::V1_18_2, ModLoader::Forge, "1.18.2 Forge", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, kModernMinecraftConnection, kModernListenerConnection, kModernHelloName},
    {MinecraftVersion::V1_18_2, ModLoader::Fabric, "1.18.2 Fabric", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, kModernMinecraftConnection, kModernListenerConnection, kModernHelloName},
    {MinecraftVersion::V1_18_2, ModLoader::Quilt, "1.18.2 Quilt", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, kModernMinecraftConnection, kModernListenerConnection, kModernHelloName},
    {MinecraftVersion::V1_20_1, ModLoader::Vanilla, "1.20.1 vanilla", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, kModernMinecraftConnection, kModernListenerConnection, kModernHelloName},
    {MinecraftVersion::V1_20_1, ModLoader::Forge, "1.20.1 Forge", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, kModernMinecraftConnection, kModernListenerConnection, kModernHelloName},
    {MinecraftVersion::V1_20_1, ModLoader::NeoForge, "1.20.1 NeoForge", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, kModernMinecraftConnection, kModernListenerConnection, kModernHelloName},
    {MinecraftVersion::V1_20_1, ModLoader::Fabric, "1.20.1 Fabric", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, kModernMinecraftConnection, kModernListenerConnection, kModernHelloName},
    {MinecraftVersion::V1_20_1, ModLoader::Quilt, "1.20.1 Quilt", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, kModernMinecraftConnection, kModernListenerConnection, kModernHelloName},
    {MinecraftVersion::V1_20_4, ModLoader::Vanilla, "1.20.4 vanilla", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, kModernMinecraftConnection, kModernListenerConnection, kModernHelloName},
    {MinecraftVersion::V1_20_4, ModLoader::Forge, "1.20.4 Forge", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, kModernMinecraftConnection, kModernListenerConnection, kModernHelloName},
    {MinecraftVersion::V1_20_4, ModLoader::NeoForge, "1.20.4 NeoForge", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, kModernMinecraftConnection, kModernListenerConnection, kModernHelloName},
    {MinecraftVersion::V1_20_4, ModLoader::Fabric, "1.20.4 Fabric", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, kModernMinecraftConnection, kModernListenerConnection, kModernHelloName},
    {MinecraftVersion::V1_20_4, ModLoader::Quilt, "1.20.4 Quilt", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, kModernMinecraftConnection, kModernListenerConnection, kModernHelloName},
    {MinecraftVersion::V1_21_0, ModLoader::Vanilla, "1.21 vanilla", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, kModernMinecraftConnection, kModernListenerConnection, kModernHelloName},
    {MinecraftVersion::V1_21_0, ModLoader::Fabric, "1.21 Fabric", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, kModernMinecraftConnection, kModernListenerConnection, kModernHelloName},
    {MinecraftVersion::V1_21_0, ModLoader::NeoForge, "1.21 NeoForge", kModernConnection, 1, kModernListener, 1, kModernMinecraftInstance, kModernMinecraftConnection, kModernListenerConnection, kModernHelloName},
};

const RuntimeAdapter* FindExact(MinecraftVersion version, ModLoader loader) {
    for (const RuntimeAdapter& adapter : kAdapters)
        if (adapter.version == version && adapter.loader == loader)
            return &adapter;
    return nullptr;
}

jclass FindClass(JNIEnv* env, jobject loader, const ClassCandidate& candidate) {
    jclass klass = LoadClassInLoader(env, loader, candidate.officialName);
    if (!klass && candidate.legacyName && std::strcmp(candidate.officialName, candidate.legacyName) != 0)
        klass = LoadClassInLoader(env, loader, candidate.legacyName);
    if (env->ExceptionCheck()) env->ExceptionClear();
    return klass;
}

} // namespace

const RuntimeAdapter* FindRuntimeAdapter(MinecraftVersion version, ModLoader loader) {
    if (loader == ModLoader::Auto) loader = ModLoader::Vanilla;
    return FindExact(version, loader);
}

const RuntimeAdapter* SelectRuntimeAdapter(const RuntimeProfile& profile) {
    return FindRuntimeAdapter(profile.version, profile.loader);
}

bool ValidateRuntimeAdapter(JNIEnv* env, jobject classLoader, const RuntimeAdapter& adapter) {
    bool connection = false;
    bool listener = false;
    for (size_t i = 0; i < adapter.connectionClassCount; ++i) {
        jclass c = FindClass(env, classLoader, adapter.connectionClasses[i]);
        if (c) { connection = true; env->DeleteLocalRef(c); break; }
    }
    for (size_t i = 0; i < adapter.listenerClassCount; ++i) {
        jclass c = FindClass(env, classLoader, adapter.listenerClasses[i]);
        if (c) { listener = true; env->DeleteLocalRef(c); break; }
    }
    return connection && listener;
}

void LogRuntimeAdapter(const RuntimeAdapter& adapter) {
    LogTo("Adapter: %s; connection=%s; listener=%s", adapter.displayName,
          adapter.connectionClasses[0].officialName, adapter.listenerClasses[0].officialName);
}

} // namespace sakura
