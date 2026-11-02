#include "NightfallCore.h"
#include "NightfallNet.h"

#include <android/log.h>
#include <jni.h>
#include <mutex>

namespace
{
    std::mutex g_Mutex;
    Nightfall::GameSim g_Sim;
    Nightfall::Ctl g_TouchCtl{};
}

extern "C" JNIEXPORT void JNICALL
Java_com_mikeanddenyse_nightfall_MainActivity_nativeInit(JNIEnv* /*env*/, jclass /*clazz*/)
{
    std::lock_guard<std::mutex> lock(g_Mutex);
    g_Sim.ReadCtl = [](int slot) {
        return slot == 0 ? g_TouchCtl : Nightfall::Ctl{};
    };
    g_Sim.Start(0, "mike", "denyse", true);
    __android_log_print(ANDROID_LOG_INFO, "NightfallNative", "Nightfall C++20 runtime initialized (16 worlds, 32 enemies, 16 bosses + Belial)");
}

extern "C" JNIEXPORT void JNICALL
Java_com_mikeanddenyse_nightfall_MainActivity_nativeStep(JNIEnv* /*env*/, jclass /*clazz*/, jfloat dt)
{
    std::lock_guard<std::mutex> lock(g_Mutex);
    g_Sim.Tick(dt);
    g_TouchCtl.Jp = false;
    g_TouchCtl.Ap = false;
}

extern "C" JNIEXPORT void JNICALL
Java_com_mikeanddenyse_nightfall_MainActivity_nativeTouch(JNIEnv* /*env*/, jclass /*clazz*/, jint action, jfloat normX, jfloat normY)
{
    std::lock_guard<std::mutex> lock(g_Mutex);
    bool down = (action == 0 || action == 2);
    if (!down)
    {
        g_TouchCtl = Nightfall::Ctl{};
        return;
    }
    if (normY > 0.65f)
    {
        g_TouchCtl.L = (normX < 0.15f);
        g_TouchCtl.R = (normX >= 0.15f && normX < 0.32f);
        if (normX >= 0.72f && normX < 0.86f) g_TouchCtl.Ap = true;
        if (normX >= 0.86f) { g_TouchCtl.Jp = true; g_TouchCtl.Jn = true; }
    }
}
