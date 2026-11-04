#pragma once
#include <atomic>
#if defined(PLATFORM_ANDROID)
#include <android_native_app_glue.h>
#include <jni.h>
extern "C" struct android_app* GetAndroidApp(void);
namespace SLTMobile {
inline std::atomic<bool> Background{false};
inline void SetBackground(bool paused){Background=paused;if(paused&&IsAudioDeviceReady())SetMasterVolume(0);}

inline void Call(const char* method) {
    auto app=GetAndroidApp();if(!app||!app->activity)return;
    JNIEnv* env=nullptr;JavaVM* vm=app->activity->vm;bool attached=vm->GetEnv(reinterpret_cast<void**>(&env),JNI_VERSION_1_6)==JNI_EDETACHED;
    if(attached&&vm->AttachCurrentThread(&env,nullptr)!=JNI_OK)return;
    auto type=env->GetObjectClass(app->activity->clazz);auto id=env->GetMethodID(type,method,"()V");
    if(id)env->CallVoidMethod(app->activity->clazz,id);
    if(env->ExceptionCheck())env->ExceptionClear();
    env->DeleteLocalRef(type);if(attached)vm->DetachCurrentThread();
}
}
#else
namespace SLTMobile {inline std::atomic<bool> Background{false};inline void Call(const char*){}}
#endif
