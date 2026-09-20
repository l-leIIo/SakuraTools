#pragma once

#include "proxy.h"

#include <string_view>

// Runtime compatibility helpers. The injected library must not rely on the
// Minecraft launcher version string: NeoForge can remap and transform classes
// before they are visible to JVMTI.
enum class MinecraftRuntime {
    Unknown,
    Legacy_1_20_1,
    Modern_1_21_8,
};

MinecraftRuntime DetectMinecraftRuntime(JNIEnv* env, jobject classLoader);

jmethodID FindConstructorByDescriptor(JNIEnv* env, jclass klass,
                                      std::string_view descriptor);

jmethodID FindFirstConstructorByDescriptor(JNIEnv* env, jclass klass,
                                           const char* const* descriptors,
                                           int count);

const char* MinecraftRuntimeName(MinecraftRuntime runtime);
