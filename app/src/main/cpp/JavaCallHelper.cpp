//
// Created by Administrator on 2021/2/2.
//

#include "JavaCallHelper.h"
#include "macro.h"

JavaCallHelper::JavaCallHelper(JavaVM *vm, JNIEnv *evn, jobject instance) {
    this->vm=vm;
    //如果在主线程回调
    this->evn=evn;
    //一旦涉及到jobject跨方法 跨线程 就需要创建全局引用
    this->instance=evn->NewGlobalRef(instance);

    jclass clazz=evn->GetObjectClass(instance);
    onErrorId=evn->GetMethodID(clazz,"onError","(I)V");
    onPrepareId=evn->GetMethodID(clazz,"onPrepare","()V");
}

JavaCallHelper::~JavaCallHelper() {
    this->evn->DeleteGlobalRef(instance);
}

void JavaCallHelper::onError(int thread,int errorCode) {
    LOGE("报错");
    //定义0为主线程、1为子线程
    if(thread==0){
        //主线程
        evn->CallVoidMethod(instance,onErrorId,errorCode);
    } else{
        //子线程
        JNIEnv* mEnv;
        //获得属于我这个线程的jniEnv
        vm->AttachCurrentThread(&mEnv,0);
        mEnv->CallVoidMethod(instance,onErrorId,errorCode);
        vm->DetachCurrentThread();
    }
}

void JavaCallHelper::onPrepare(int thread) {
    LOGE("准备好了");
    //定义0为主线程、1为子线程
    if(thread==0){
        //主线程
        evn->CallVoidMethod(instance,onPrepareId);
    } else{
        //子线程
        JNIEnv *mEnv;
        //获得属于我这个线程的jniEnv
        vm->AttachCurrentThread(&mEnv,0);
        mEnv->CallVoidMethod(instance,onPrepareId);
        vm->DetachCurrentThread();
    }
}
