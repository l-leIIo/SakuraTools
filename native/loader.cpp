#include "proxy.h"
#include "runtime_adapter.h"
#include "runtime_profile.h"
#include "trampolines.h"
#include "relay_handler.h"
#include "connection_hook.h"
#include "b_server.h"
#include "runtime_gate.h"
#include "../injector/RuntimeControl.h"

#include <cstdio>

sakura::RuntimeProfile g_runtimeProfile = sakura::DefaultRuntimeProfile();

namespace {

bool deletePressed(DeleteKeyEdge& edge) {
    DWORD foregroundPid = 0;
    HWND window = GetForegroundWindow();
    if (window) GetWindowThreadProcessId(window, &foregroundPid);
    return edge.update((GetAsyncKeyState(VK_DELETE) & 0x8000) != 0,
                       foregroundPid == GetCurrentProcessId());
}

bool startSession(JNIEnv* env, DeleteKeyEdge& edge) {
    const sakura::RuntimeAdapter* adapter = sakura::SelectRuntimeAdapter(g_runtimeProfile);
    if (!adapter) {
        LogTo("START: no adapter for selected version/loader");
        return false;
    }
    jobject loader = GetMinecraftClassLoader(env, g_jvmti);
    if (!loader || !sakura::ValidateRuntimeAdapter(env, loader, *adapter)) {
        LogTo("START: selected adapter classes were not found; refusing unsafe injection");
        if (loader) env->DeleteLocalRef(loader);
        return false;
    }
    sakura::LogRuntimeAdapter(*adapter);
    env->DeleteLocalRef(loader);

    LogTo("START: enabling proxy; version=%s loader=%s source=%s",
          sakura::ToString(g_runtimeProfile.version), sakura::ToString(g_runtimeProfile.loader),
          g_runtimeProfile.source.c_str());
    bool installed = false;
    for (int i = 0; i < 240; ++i) {
        if (deletePressed(edge)) return false;
        if (InstallHookBridge(env)) { installed = true; break; }
        Sleep(100);
    }
    if (!installed || !InstallRelayHandler(env)) return false;
    g_runtimeGate.start();
    if (!InstallConnectionHook(env) || !InstallBServer(env)) return false;

    LiveConnectionState connection = LiveConnectionState::Failed;
    for (int attempt = 0; attempt < 10; ++attempt) {
        if (deletePressed(edge)) return false;
        if (env->PushLocalFrame(128) != JNI_OK) { env->ExceptionClear(); return false; }
        connection = BServer_TryCaptureLiveConnection(env);
        env->PopLocalFrame(nullptr);
        if (connection != LiveConnectionState::Failed) break;
        Sleep(100);
    }
    if (connection == LiveConnectionState::Failed) return false;
    if (connection == LiveConnectionState::NotConnected && !BServer_BlockAMainThreadUntilBConnected(env)) return false;
    LogTo("START: active on port 25565");
    return true;
}

bool stopSession(JNIEnv* env) {
    BServer_RequestStop();
    if (!g_runtimeGate.stopFor(std::chrono::milliseconds(1000))) return false;
    return StopBServer(env) && UninstallConnectionHook(env) && RelayHandler_DetachAll(env);
}

DWORD WINAPI RuntimeThread(LPVOID) {
    char eventName[96];
    std::snprintf(eventName, sizeof(eventName), SAKURA_RESUME_PREFIX "%lu", GetCurrentProcessId());
    HANDLE resume = CreateEventA(nullptr, FALSE, FALSE, eventName);
    if (!resume) return 0;
    if (GetLastError() == ERROR_ALREADY_EXISTS) { SetEvent(resume); CloseHandle(resume); return 0; }

    g_runtimeProfile = sakura::LoadRuntimeProfile();
    LogTo("Runtime: version=%s loader=%s source=%s", sakura::ToString(g_runtimeProfile.version),
          sakura::ToString(g_runtimeProfile.loader), g_runtimeProfile.source.c_str());

    HMODULE jvm = nullptr;
    for (int i = 0; i < 600 && !jvm; ++i) { jvm = GetModuleHandleA("jvm.dll"); if (!jvm) Sleep(100); }
    using GetVms = jint (JNICALL*)(JavaVM**, jsize, jsize*);
    auto getVms = jvm ? reinterpret_cast<GetVms>(GetProcAddress(jvm, "JNI_GetCreatedJavaVMs")) : nullptr;
    if (getVms) {
        for (int i = 0; i < 600; ++i) {
            jsize count = 0;
            if (getVms(&g_vm, 1, &count) == JNI_OK && count && g_vm) break;
            g_vm = nullptr;
            Sleep(100);
        }
    }
    JNIEnv* env = nullptr;
    JavaVMAttachArgs args{JNI_VERSION_1_8, const_cast<char*>("SakuraToolsRuntime"), nullptr};
    if (!g_vm || g_vm->AttachCurrentThreadAsDaemon(reinterpret_cast<void**>(&env), &args) != JNI_OK) { CloseHandle(resume); return 0; }
    if (g_vm->GetEnv(reinterpret_cast<void**>(&g_jvmti), JVMTI_VERSION_1_2) != JNI_OK) { g_vm->DetachCurrentThread(); CloseHandle(resume); return 0; }

    DeleteKeyEdge edge;
    bool active = false, clean = true, requested = true;
    ULONGLONG retryAt = 0;
    for (;;) {
        if (active) BServer_CheckLoginTimeout(env);
        if (!active && !clean && GetTickCount64() >= retryAt) {
            if (env->PushLocalFrame(512) == JNI_OK) { clean = stopSession(env); env->PopLocalFrame(nullptr); }
            retryAt = GetTickCount64() + 1000;
        }
        if (requested && !active && clean) {
            requested = false;
            if (env->PushLocalFrame(512) == JNI_OK) { active = startSession(env, edge); if (!active) clean = stopSession(env); env->PopLocalFrame(nullptr); }
        }
        if (deletePressed(edge) && (active || requested)) {
            ResetEvent(resume); requested = false;
            if (env->PushLocalFrame(512) == JNI_OK) { clean = stopSession(env); active = false; env->PopLocalFrame(nullptr); }
        }
        DWORD wait = WaitForSingleObject(resume, active ? 25 : 250);
        if (wait == WAIT_OBJECT_0 && !active) requested = true;
        if (wait == WAIT_FAILED) break;
    }
    g_vm->DetachCurrentThread();
    CloseHandle(resume);
    return 0;
}

} // namespace

void InitializeProxy(JNIEnv*) {
    HANDLE thread = CreateThread(nullptr, 0, RuntimeThread, nullptr, 0, nullptr);
    if (thread) CloseHandle(thread);
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        InitializeProxy(nullptr);
    }
    return TRUE;
}
