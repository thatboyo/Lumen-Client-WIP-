#pragma once

#include <jni.h>
#include <string>

namespace lumin {

// Owns the JavaVM of the host process and hands out a JNIEnv for *this* thread.
//
// Nothing here creates a VM: the game already has one. We only borrow it.
class Jvm {
public:
    // Looks for the already-running VM inside the host process.
    // Returns false (and is safe to call again later) until the VM exists.
    static bool acquire();

    static bool acquired() { return vm() != nullptr; }

    // JNIEnv for the calling thread, attaching it to the VM on first use.
    static JNIEnv* env();

    static JavaVM* vm() { return vmPtr(); }

private:
    static JavaVM*& vmPtr() {
        static JavaVM* vm = nullptr;
        return vm;
    }
};

// Small helpers so call sites read like normal code and never leak local refs.
namespace jni {

std::string toStd(JNIEnv* env, jstring value);

// Calls a static Object-returning method and clears any pending exception,
// returning nullptr instead. Used during class/loader discovery, where a failed
// lookup is expected and must not abort anything.
jobject callObjectQuiet(JNIEnv* env, jclass cls, jmethodID method, ...);

bool checkException(JNIEnv* env, const char* where);

} // namespace jni
} // namespace lumin
