#include <jni.h>
#include <string>
#include <queue>
#include <pthread.h>
#include <android/log.h>
#include "SafeQueue.h"
#include "DNFFmpeg.h"

//__VA_ARGS__代表...这个可变参数
#define LOG(...) __android_log_print(ANDROID_LOG_ERROR,"zsq",__VA_ARGS__);

using namespace std;

DNFFmpeg * ffmpeg =0;
JavaVM* javaVm=0;
int JNI_OnLoad(JavaVM* vm,void *r){
    javaVm=vm;
    return JNI_VERSION_1_6;
}

extern "C" {
    #include "libavutil/avutil.h"
}

//typedef struct people{
//    int age;
//};
//
//void callback(people &p){
//    LOG("被抛出来的这个人的年龄%d", p.age);
//}

extern "C" JNIEXPORT jstring JNICALL
Java_com_ffmpeg_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {

//    void (*p) (people &p);
//    p=callback;
//    SafeQueue<people> safeQueue;
//    safeQueue.setReleaseCallback(p);
//    people pp;
//    pp.age=29;
//    safeQueue.push(pp);
//    safeQueue.clear();

    return env->NewStringUTF(av_version_info());
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_DNPlayer_native_1prepare(JNIEnv *env, jobject thiz, jstring data_source) {
    const char* dataSource=env->GetStringUTFChars(data_source,0);

    //创建播放器
    ffmpeg=new DNFFmpeg(new JavaCallHelper(javaVm,env,thiz),dataSource);
    ffmpeg->prepare();

    env->ReleaseStringChars(data_source, reinterpret_cast<const jchar *>(dataSource));
}extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_DNPlayer_native_1start(JNIEnv *env, jobject thiz) {
    ffmpeg->start();
}