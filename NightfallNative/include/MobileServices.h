#pragma once
#include <atomic>
#if defined(PLATFORM_ANDROID)
#include <android_native_app_glue.h>
#include <jni.h>
extern "C" struct android_app* GetAndroidApp(void);
namespace SLTMobile {
inline std::atomic<bool> Background{false};

inline Vector2 InitialSurfaceSize() {
    auto app=GetAndroidApp();if(!app||!app->activity)return {0,0};
    JNIEnv* env=nullptr;JavaVM* vm=app->activity->vm;bool attached=vm->GetEnv(reinterpret_cast<void**>(&env),JNI_VERSION_1_6)==JNI_EDETACHED;
    if(attached&&vm->AttachCurrentThread(&env,nullptr)!=JNI_OK)return {0,0};
    auto type=env->GetObjectClass(app->activity->clazz);auto id=env->GetMethodID(type,"physicalSurfaceSize","()[I");
    jint sizes[2]={0,0};
    if(id){auto result=static_cast<jintArray>(env->CallObjectMethod(app->activity->clazz,id));if(result){if(env->GetArrayLength(result)>=2)env->GetIntArrayRegion(result,0,2,sizes);env->DeleteLocalRef(result);}}
    if(env->ExceptionCheck()){env->ExceptionClear();sizes[0]=sizes[1]=0;}
    env->DeleteLocalRef(type);if(attached)vm->DetachCurrentThread();
    TraceLog(LOG_INFO,"SLT_ANDROID_SURFACE: %dx%d",sizes[0],sizes[1]);
    return {static_cast<float>(sizes[0]),static_cast<float>(sizes[1])};
}
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
