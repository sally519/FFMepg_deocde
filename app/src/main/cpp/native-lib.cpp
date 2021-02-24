#include <jni.h>
#include <string>
#include <queue>
#include <pthread.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include "DNFFmpeg.h"
#include "VideoChannel.h"
#include "macro.h"

//__VA_ARGS__代表...这个可变参数
#define LOG(...) __android_log_print(ANDROID_LOG_ERROR,"zsq",__VA_ARGS__);

using namespace std;

DNFFmpeg * ffmpeg =0;
JavaVM* javaVm=0;
ANativeWindow* window;
pthread_mutex_t  mutex;
JavaCallHelper * helper=0;

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

//用来画画的
static void renderCallback(uint8_t* data, int linesize, int width, int high){
    pthread_mutex_lock(&mutex);
    if(!window){
        pthread_mutex_unlock(&mutex);
        return;
    }
    //设置窗口属性
    ANativeWindow_setBuffersGeometry(window, width,high,WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        ANativeWindow_release(window);
        window = 0;
        pthread_mutex_unlock(&mutex);
        return;
    }
    //填充rgb数据给dst_data
    uint8_t * m_dst_data = static_cast<uint8_t *>(window_buffer.bits);
    int dst_line_size=window_buffer.stride*4;
    uint8_t *src_data = data;
    int src_linesize = linesize;
    //一行一行的拷贝
    for (int i = 0; i < window_buffer.height; ++i) {
        memcpy(m_dst_data+i*dst_line_size, src_data + i * src_linesize, dst_line_size);
    }
    ANativeWindow_unlockAndPost(window);
    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_DNPlayer_native_1prepare(JNIEnv *env, jobject thiz, jstring data_source) {
    const char* dataSource=env->GetStringUTFChars(data_source,0);
    //创建播放器
    pthread_mutex_init(&mutex,0);
    helper=new JavaCallHelper(javaVm,env,thiz);
    ffmpeg=new DNFFmpeg(helper,dataSource);
    ffmpeg->setRenderDataCallback(renderCallback);
    ffmpeg->prepare();
    env->ReleaseStringChars(data_source, reinterpret_cast<const jchar *>(dataSource));
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_DNPlayer_native_1start(JNIEnv *env, jobject thiz) {
    ffmpeg->start();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_DNPlayer_native_1setSurface(JNIEnv *env, jobject thiz, jobject surface) {
    // TODO: implement native_setSurface()
    pthread_mutex_lock(&mutex);
    if(window){
        ANativeWindow_release(window);
        window=0;
    }
    window=ANativeWindow_fromSurface(env,surface);
    pthread_mutex_unlock(&mutex);
    LOG("设置了窗口");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_DNPlayer_native_1stop(JNIEnv *env, jobject thiz) {
    if(ffmpeg){
        ffmpeg->stop();
    }
    DELETE(helper);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ffmpeg_DNPlayer_native_1release(JNIEnv *env, jobject thiz) {
    pthread_mutex_lock(&mutex);
    if(window){
        ANativeWindow_release(window);
        window=0;
    }
    pthread_mutex_unlock(&mutex);
    LOG("释放了窗口");
}