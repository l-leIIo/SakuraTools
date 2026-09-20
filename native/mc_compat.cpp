#include "mc_compat.h"

#include <cstring>

namespace {

jclass LoadOptional(JNIEnv* env, jobject loader, const char* name) {
    if (!env || !loader) return nullptr;
    jclass loaderClass = env->GetObjectClass(loader);
    if (!loaderClass) return nullptr;
    jmethodID loadClass = env->GetMethodID(
        loaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    if (!loadClass) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        env->DeleteLocalRef(loaderClass);
        return nullptr;
    }
    jstring className = env->NewStringUTF(name);
    jobject result = env->CallObjectMethod(loader, loadClass, className);
    env->DeleteLocalRef(className);
    env->DeleteLocalRef(loaderClass);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        if (result) env->DeleteLocalRef(result);
        return nullptr;
    }
    return static_cast<jclass>(result);
}

bool HasClass(JNIEnv* env, jobject loader, const char* name) {
    jclass klass = LoadOptional(env, loader, name);
    if (!klass) return false;
    env->DeleteLocalRef(klass);
    return true;
}

} // namespace

jmethodID FindConstructorByDescriptor(JNIEnv* env, jclass klass,
                                      std::string_view descriptor) {
    if (!env || !klass || !g_jvmti) return nullptr;

    jint count = 0;
    jmethodID* methods = nullptr;
    if (g_jvmti->GetClassMethods(klass, &count, &methods) != JVMTI_ERROR_NONE)
        return nullptr;

    jmethodID result = nullptr;
    for (jint i = 0; i < count; ++i) {
        char* name = nullptr;
        char* signature = nullptr;
        char* generic = nullptr;
        if (g_jvmti->GetMethodName(methods[i], &name, &signature, &generic)
                == JVMTI_ERROR_NONE) {
            if (name && signature && std::strcmp(name, "<init>") == 0 &&
                descriptor == signature) {
                result = methods[i];
            }
        }
        if (name) g_jvmti->Deallocate(reinterpret_cast<unsigned char*>(name));
        if (signature) g_jvmti->Deallocate(reinterpret_cast<unsigned char*>(signature));
        if (generic) g_jvmti->Deallocate(reinterpret_cast<unsigned char*>(generic));
        if (result) break;
    }
    if (methods) g_jvmti->Deallocate(reinterpret_cast<unsigned char*>(methods));
    return result;
}

jmethodID FindFirstConstructorByDescriptor(JNIEnv* env, jclass klass,
                                           const char* const* descriptors,
                                           int count) {
    if (!descriptors || count <= 0) return nullptr;
    for (int i = 0; i < count; ++i) {
        if (jmethodID method = FindConstructorByDescriptor(env, klass, descriptors[i]))
            return method;
    }
    return nullptr;
}

MinecraftRuntime DetectMinecraftRuntime(JNIEnv* env, jobject classLoader) {
    if (!env || !classLoader) return MinecraftRuntime::Unknown;

    // The new login packet dependencies are absent from 1.20.1 and provide a
    // stable discriminator even when NeoForge has transformed the game classes.
    const bool modern =
        HasClass(env, classLoader, "net.minecraft.network.CommonListenerCookie") &&
        HasClass(env, classLoader, "net.minecraft.world.level.WorldDataConfiguration");
    if (modern) return MinecraftRuntime::Modern_1_21_8;

    if (HasClass(env, classLoader, "net.minecraft.network.Connection") &&
        HasClass(env, classLoader, "net.minecraft.network.protocol.game.ClientboundLoginPacket"))
        return MinecraftRuntime::Legacy_1_20_1;

    return MinecraftRuntime::Unknown;
}

const char* MinecraftRuntimeName(MinecraftRuntime runtime) {
    switch (runtime) {
        case MinecraftRuntime::Legacy_1_20_1: return "1.20.1";
        case MinecraftRuntime::Modern_1_21_8: return "1.21.8/NeoForge";
        default: return "unknown";
    }
}
