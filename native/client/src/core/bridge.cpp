#include "bridge.hpp"

#include "jvm.hpp"
#include "log.hpp"

#include <cmath>
#include <cstring>
#include <mutex>

namespace lumin {
namespace {

constexpr const char* kBridgeBinaryName = "dev.lumin.bridge.Bridge";
constexpr const char* kBridgeSlashName = "dev/lumin/bridge/Bridge";

std::mutex g_resolveMutex;
bool g_resolved = false;
jclass g_bridge = nullptr;

struct Methods {
    jmethodID isReady;
    jmethodID isInWorld;

    jmethodID getPosX, getPosY, getPosZ;
    jmethodID getEyeX, getEyeY, getEyeZ;
    jmethodID getMotionX, getMotionY, getMotionZ;
    jmethodID getYaw, getPitch;
    jmethodID isOnGround, isSprinting;

    jmethodID getHealth, getMaxHealth, getFood;

    jmethodID getCamX, getCamY, getCamZ;
    jmethodID getCamYaw, getCamPitch, getFov, getGamma;

    jmethodID getScreenWidth, getScreenHeight;
    jmethodID getTickCount, getEntityCount, getEntityVersion;
    jmethodID getDimension, getEntityData, getEntityNames;

    jmethodID setSprinting, setGamma, setStepHeight, setVelocity, setRotation;
    jmethodID setFly, setFallDistance, setItemUseCooldown, clickAttack, clickUse, sendChat;
};

Methods M{};

std::vector<EntitySnapshot> g_entities;
int g_entityVersion = -1;

// ---------------------------------------------------------------------
// Class resolution
// ---------------------------------------------------------------------

// Finds a class through every thread's context classloader. On Fabric the game
// and this bridge are loaded by Knot, which an attached native thread cannot
// reach with a bare FindClass.
jclass findClassThroughThreads(JNIEnv* env, const char* binaryName) {
    jclass threadCls = env->FindClass("java/lang/Thread");
    if (!threadCls) return nullptr;

    jmethodID getAllStackTraces =
        env->GetStaticMethodID(threadCls, "getAllStackTraces", "()Ljava/util/Map;");
    jmethodID getContextLoader = env->GetMethodID(
        threadCls, "getContextClassLoader", "()Ljava/lang/ClassLoader;");
    if (!getAllStackTraces || !getContextLoader) return nullptr;

    jobject map = env->CallStaticObjectMethod(threadCls, getAllStackTraces);
    if (jni::checkException(env, "getAllStackTraces") || !map) return nullptr;

    jclass mapCls = env->FindClass("java/util/Map");
    jmethodID entrySet = env->GetMethodID(mapCls, "entrySet", "()Ljava/util/Set;");
    jobject set = env->CallObjectMethod(map, entrySet);
    if (!set) return nullptr;

    jclass setCls = env->FindClass("java/util/Set");
    jmethodID iteratorMid = env->GetMethodID(setCls, "iterator", "()Ljava/util/Iterator;");
    jobject iterator = env->CallObjectMethod(set, iteratorMid);
    if (!iterator) return nullptr;

    jclass iterCls = env->FindClass("java/util/Iterator");
    jmethodID hasNext = env->GetMethodID(iterCls, "hasNext", "()Z");
    jmethodID next = env->GetMethodID(iterCls, "next", "()Ljava/lang/Object;");

    jclass entryCls = env->FindClass("java/util/Map$Entry");
    jmethodID getKey = env->GetMethodID(entryCls, "getKey", "()Ljava/lang/Object;");

    jclass classCls = env->FindClass("java/lang/Class");
    jmethodID forName = env->GetStaticMethodID(
        classCls, "forName", "(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;");

    jclass found = nullptr;

    while (env->CallBooleanMethod(iterator, hasNext)) {
        env->PushLocalFrame(8);

        jobject entry = env->CallObjectMethod(iterator, next);
        jobject thread = entry ? env->CallObjectMethod(entry, getKey) : nullptr;
        jobject loader = thread ? env->CallObjectMethod(thread, getContextLoader) : nullptr;

        if (loader) {
            jstring name = env->NewStringUTF(binaryName);
            auto cls = static_cast<jclass>(
                env->CallStaticObjectMethod(classCls, forName, name, JNI_TRUE, loader));
            if (env->ExceptionCheck()) {
                env->ExceptionClear();
                cls = nullptr;
            }
            if (cls) {
                found = static_cast<jclass>(env->PopLocalFrame(cls));
                break;
            }
        }

        env->PopLocalFrame(nullptr);
    }

    env->DeleteLocalRef(iterator);
    env->DeleteLocalRef(set);
    env->DeleteLocalRef(map);
    return found;
}

jclass resolveBridgeClass(JNIEnv* env) {
    jclass direct = env->FindClass(kBridgeSlashName);
    if (direct) {
        VLOG_INFO("bridge found on the system classloader");
        return direct;
    }
    env->ExceptionClear();

    jclass viaThreads = findClassThroughThreads(env, kBridgeBinaryName);
    if (viaThreads) {
        VLOG_INFO("bridge found through a thread context classloader");
        return viaThreads;
    }

    return nullptr;
}

// Env for a command call: null until the bridge is bound, so callers can bail out
// instead of dereferencing an unresolved class.
JNIEnv* commandEnv() {
    if (!Bridge::available()) return nullptr;
    return Jvm::env();
}

jmethodID mid(JNIEnv* env, const char* name, const char* sig, bool isStatic = true) {
    jmethodID id = isStatic ? env->GetStaticMethodID(g_bridge, name, sig)
                            : env->GetMethodID(g_bridge, name, sig);
    if (!id) {
        VLOG_ERR("bridge is missing %s%s", name, sig);
        env->ExceptionClear();
    }
    return id;
}

} // namespace

double GameState::horizontalSpeed() const {
    return std::sqrt(mx * mx + mz * mz);
}

bool Bridge::resolve() {
    std::lock_guard<std::mutex> lock(g_resolveMutex);

    if (g_resolved && g_bridge) return true;
    g_resolved = false;

    JNIEnv* env = Jvm::env();
    if (!env) return false;

    jclass cls = resolveBridgeClass(env);
    if (!cls) return false;

    g_bridge = static_cast<jclass>(env->NewGlobalRef(cls));
    env->DeleteLocalRef(cls);
    if (!g_bridge) return false;

    M.isReady = mid(env, "isReady", "()Z");
    M.isInWorld = mid(env, "isInWorld", "()Z");

    M.getPosX = mid(env, "getPosX", "()D");
    M.getPosY = mid(env, "getPosY", "()D");
    M.getPosZ = mid(env, "getPosZ", "()D");
    M.getEyeX = mid(env, "getEyeX", "()D");
    M.getEyeY = mid(env, "getEyeY", "()D");
    M.getEyeZ = mid(env, "getEyeZ", "()D");
    M.getMotionX = mid(env, "getMotionX", "()D");
    M.getMotionY = mid(env, "getMotionY", "()D");
    M.getMotionZ = mid(env, "getMotionZ", "()D");
    M.getYaw = mid(env, "getYaw", "()F");
    M.getPitch = mid(env, "getPitch", "()F");
    M.isOnGround = mid(env, "isOnGround", "()Z");
    M.isSprinting = mid(env, "isSprinting", "()Z");

    M.getHealth = mid(env, "getHealth", "()F");
    M.getMaxHealth = mid(env, "getMaxHealth", "()F");
    M.getFood = mid(env, "getFood", "()I");

    M.getCamX = mid(env, "getCamX", "()D");
    M.getCamY = mid(env, "getCamY", "()D");
    M.getCamZ = mid(env, "getCamZ", "()D");
    M.getCamYaw = mid(env, "getCamYaw", "()F");
    M.getCamPitch = mid(env, "getCamPitch", "()F");
    M.getFov = mid(env, "getFov", "()D");
    M.getGamma = mid(env, "getGamma", "()D");

    M.getScreenWidth = mid(env, "getScreenWidth", "()I");
    M.getScreenHeight = mid(env, "getScreenHeight", "()I");
    M.getTickCount = mid(env, "getTickCount", "()I");
    M.getEntityCount = mid(env, "getEntityCount", "()I");
    M.getEntityVersion = mid(env, "getEntityVersion", "()I");
    M.getDimension = mid(env, "getDimension", "()Ljava/lang/String;");
    M.getEntityData = mid(env, "getEntityData", "()[D");
    M.getEntityNames = mid(env, "getEntityNames", "()[Ljava/lang/String;");

    M.setSprinting = mid(env, "setSprinting", "(Z)V");
    M.setGamma = mid(env, "setGamma", "(D)V");
    M.setStepHeight = mid(env, "setStepHeight", "(D)V");
    M.setVelocity = mid(env, "setVelocity", "(DDD)V");
    M.setRotation = mid(env, "setRotation", "(FF)V");
    M.setFly = mid(env, "setFly", "(ZD)V");
    M.setFallDistance = mid(env, "setFallDistance", "(D)V");
    M.setItemUseCooldown = mid(env, "setItemUseCooldown", "(I)V");
    M.clickAttack = mid(env, "clickAttack", "()V");
    M.clickUse = mid(env, "clickUse", "()V");
    M.sendChat = mid(env, "sendChat", "(Ljava/lang/String;)V");

    if (!M.isReady || !M.getPosX || !M.setVelocity) {
        VLOG_ERR("bridge resolved but its API is incomplete");
        env->DeleteGlobalRef(g_bridge);
        g_bridge = nullptr;
        return false;
    }

    g_resolved = true;
    VLOG_INFO("bridge resolved and bound");
    return true;
}

bool Bridge::available() {
    return g_resolved && g_bridge != nullptr;
}

GameState Bridge::state() {
    GameState s;
    if (!available()) return s;

    JNIEnv* env = Jvm::env();
    if (!env) return s;

    s.ready = env->CallStaticBooleanMethod(g_bridge, M.isReady) == JNI_TRUE;
    s.inWorld = env->CallStaticBooleanMethod(g_bridge, M.isInWorld) == JNI_TRUE;

    s.px = env->CallStaticDoubleMethod(g_bridge, M.getPosX);
    s.py = env->CallStaticDoubleMethod(g_bridge, M.getPosY);
    s.pz = env->CallStaticDoubleMethod(g_bridge, M.getPosZ);
    s.eyeX = env->CallStaticDoubleMethod(g_bridge, M.getEyeX);
    s.eyeY = env->CallStaticDoubleMethod(g_bridge, M.getEyeY);
    s.eyeZ = env->CallStaticDoubleMethod(g_bridge, M.getEyeZ);
    s.mx = env->CallStaticDoubleMethod(g_bridge, M.getMotionX);
    s.my = env->CallStaticDoubleMethod(g_bridge, M.getMotionY);
    s.mz = env->CallStaticDoubleMethod(g_bridge, M.getMotionZ);
    s.yaw = env->CallStaticFloatMethod(g_bridge, M.getYaw);
    s.pitch = env->CallStaticFloatMethod(g_bridge, M.getPitch);
    s.onGround = env->CallStaticBooleanMethod(g_bridge, M.isOnGround) == JNI_TRUE;
    s.sprinting = env->CallStaticBooleanMethod(g_bridge, M.isSprinting) == JNI_TRUE;

    s.health = env->CallStaticFloatMethod(g_bridge, M.getHealth);
    s.maxHealth = env->CallStaticFloatMethod(g_bridge, M.getMaxHealth);
    s.food = env->CallStaticIntMethod(g_bridge, M.getFood);

    s.camX = env->CallStaticDoubleMethod(g_bridge, M.getCamX);
    s.camY = env->CallStaticDoubleMethod(g_bridge, M.getCamY);
    s.camZ = env->CallStaticDoubleMethod(g_bridge, M.getCamZ);
    s.camYaw = env->CallStaticFloatMethod(g_bridge, M.getCamYaw);
    s.camPitch = env->CallStaticFloatMethod(g_bridge, M.getCamPitch);
    s.fov = env->CallStaticDoubleMethod(g_bridge, M.getFov);
    s.gamma = env->CallStaticDoubleMethod(g_bridge, M.getGamma);

    s.screenW = env->CallStaticIntMethod(g_bridge, M.getScreenWidth);
    s.screenH = env->CallStaticIntMethod(g_bridge, M.getScreenHeight);
    s.tick = env->CallStaticIntMethod(g_bridge, M.getTickCount);

    auto dim = static_cast<jstring>(env->CallStaticObjectMethod(g_bridge, M.getDimension));
    if (dim) {
        s.dimension = jni::toStd(env, dim);
        env->DeleteLocalRef(dim);
    }

    if (jni::checkException(env, "Bridge::state")) return GameState{};
    return s;
}

const std::vector<EntitySnapshot>& Bridge::entities() {
    if (!available()) return g_entities;

    JNIEnv* env = Jvm::env();
    if (!env) return g_entities;

    int version = env->CallStaticIntMethod(g_bridge, M.getEntityVersion);
    if (version == g_entityVersion) return g_entities;
    g_entityVersion = version;

    g_entities.clear();

    auto data = static_cast<jdoubleArray>(env->CallStaticObjectMethod(g_bridge, M.getEntityData));
    auto names = static_cast<jobjectArray>(env->CallStaticObjectMethod(g_bridge, M.getEntityNames));
    if (!data) return g_entities;

    jsize len = env->GetArrayLength(data);
    constexpr int stride = 8;
    if (len >= stride) {
        std::vector<double> flat(static_cast<size_t>(len));
        env->GetDoubleArrayRegion(data, 0, len, flat.data());

        size_t count = static_cast<size_t>(len) / stride;
        g_entities.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            const double* row = flat.data() + i * stride;
            EntitySnapshot e;
            e.x = row[0];
            e.y = row[1];
            e.z = row[2];
            e.width = row[3];
            e.height = row[4];
            e.health = static_cast<float>(row[5]);
            e.maxHealth = static_cast<float>(row[6]);
            e.flags = static_cast<int>(row[7]);

            if (names && static_cast<jsize>(i) < env->GetArrayLength(names)) {
                auto name = static_cast<jstring>(env->GetObjectArrayElement(names, static_cast<jsize>(i)));
                if (name) {
                    e.name = jni::toStd(env, name);
                    env->DeleteLocalRef(name);
                }
            }
            g_entities.push_back(std::move(e));
        }
    }

    env->DeleteLocalRef(data);
    if (names) env->DeleteLocalRef(names);
    return g_entities;
}

// ---------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------

void Bridge::setSprinting(bool value) {
    if (JNIEnv* env = commandEnv()) env->CallStaticVoidMethod(g_bridge, M.setSprinting, value);
}

void Bridge::setGamma(double value) {
    if (JNIEnv* env = commandEnv()) env->CallStaticVoidMethod(g_bridge, M.setGamma, value);
}

void Bridge::setStepHeight(double value) {
    if (JNIEnv* env = commandEnv()) env->CallStaticVoidMethod(g_bridge, M.setStepHeight, value);
}

void Bridge::setVelocity(double x, double y, double z) {
    if (JNIEnv* env = commandEnv()) env->CallStaticVoidMethod(g_bridge, M.setVelocity, x, y, z);
}

void Bridge::setRotation(float yaw, float pitch) {
    if (JNIEnv* env = commandEnv()) env->CallStaticVoidMethod(g_bridge, M.setRotation, yaw, pitch);
}

void Bridge::setFly(bool value, double speed) {
    if (JNIEnv* env = commandEnv()) env->CallStaticVoidMethod(g_bridge, M.setFly, value, speed);
}

void Bridge::setFallDistance(double value) {
    if (JNIEnv* env = commandEnv()) env->CallStaticVoidMethod(g_bridge, M.setFallDistance, value);
}

void Bridge::setItemUseCooldown(int value) {
    if (JNIEnv* env = commandEnv()) env->CallStaticVoidMethod(g_bridge, M.setItemUseCooldown, value);
}

void Bridge::clickAttack() {
    if (JNIEnv* env = commandEnv()) env->CallStaticVoidMethod(g_bridge, M.clickAttack);
}

void Bridge::clickUse() {
    if (JNIEnv* env = commandEnv()) env->CallStaticVoidMethod(g_bridge, M.clickUse);
}

void Bridge::sendChat(const std::string& message) {
    JNIEnv* env = commandEnv();
    if (!env) return;
    jstring str = env->NewStringUTF(message.c_str());
    if (!str) return;
    env->CallStaticVoidMethod(g_bridge, M.sendChat, str);
    env->DeleteLocalRef(str);
}

} // namespace lumin
