//
// Created by Administrator on 2021/2/2.
//

#ifndef FIEST_FFMPEG_JAVACALLHELPER_H
#define FIEST_FFMPEG_JAVACALLHELPER_H

#include <jni.h>

class JavaCallHelper {
public:
    JavaCallHelper(JavaVM* vm,JNIEnv* evn,jobject instance);
    ~JavaCallHelper();
    void onError(int thread,int errorCode);
    void onPrepare(int thread);
private:
    JavaVM* vm;
    JNIEnv* evn;
    jobject instance;
    jmethodID onErrorId;
    jmethodID onPrepareId;
};


#endif //FIEST_FFMPEG_JAVACALLHELPER_H
