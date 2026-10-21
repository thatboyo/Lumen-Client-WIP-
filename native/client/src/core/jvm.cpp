#include "jvm.hpp"

#include "log.hpp"

#include <cstdarg>

namespace lumin {

bool Jvm::acquire() {
    if (vm()) return true;

    // The VM lives in jvm.dll inside the host process. Never load a different
    // copy from disk: it would be a second VM, which the spec does not allow.
    HMODULE jvmModule = GetModuleHandleA("jvm.dll");
    if (!jvmModule) return false;

    using GetCreatedJavaVMsFn = jint(JNICALL*)(JavaVM**, jsize, jsize*);
    auto getVMs = reinterpret_cast<GetCreatedJavaVMsFn>(
        GetProcAddress(jvmModule, "JNI_GetCreatedJavaVMs"));
    if (!getVMs) {
        VLOG_ERR("jvm.dll has no JNI_GetCreatedJavaVMs export");
        return false;
    }

    JavaVM* vms[1] = {nullptr};
    jsize count = 0;
    if (getVMs(vms, 1, &count) != JNI_OK || count < 1) return false;

    vmPtr() = vms[0];
    VLOG_INFO("attached to existing JVM at %p", static_cast<void*>(vmPtr()));
    return true;
}

JNIEnv* Jvm::env() {
    if (!vm()) return nullptr;

    JNIEnv* env = nullptr;
    jint result = vm()->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_8);
    if (result == JNI_OK) return env;

    if (result == JNI_EDETACHED) {
        JavaVMAttachArgs args{};
        args.version = JNI_VERSION_1_8;
        args.name = const_cast<char*>("Lumin-Thread");
        args.group = nullptr;
        if (vm()->AttachCurrentThread(reinterpret_cast<void**>(&env), &args) != JNI_OK) {
            VLOG_ERR("AttachCurrentThread failed");
            return nullptr;
        }
        return env;
    }

    return nullptr;
}

namespace jni {

std::string toStd(JNIEnv* env, jstring value) {
    if (!env || !value) return {};
    const char* chars = env->GetStringUTFChars(value, nullptr);
    if (!chars) return {};
    std::string out(chars);
    env->ReleaseStringUTFChars(value, chars);
    return out;
}

bool checkException(JNIEnv* env, const char* where) {
    if (!env->ExceptionCheck()) return false;

    env->ExceptionDescribe();
    env->ExceptionClear();
    VLOG_WARN("java exception cleared at %s", where);
    return true;
}

jobject callObjectQuiet(JNIEnv* env, jclass cls, jmethodID method, ...) {
    va_list args;
    va_start(args, method);
    jobject result = env->CallStaticObjectMethodV(cls, method, args);
    va_end(args);

    if (checkException(env, "callObjectQuiet")) return nullptr;
    return result;
}

} // namespace jni
} // namespace lumin
