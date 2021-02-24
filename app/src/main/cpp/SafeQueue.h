//
// Created by Administrator on 2018/9/3.
//

#include <queue>
#include <pthread.h>
#include <android/log.h>

#define LOG(...) __android_log_print(ANDROID_LOG_ERROR,"zsq",__VA_ARGS__);

using namespace std;
template <typename T>
class SafeQueue {
    typedef void (*ReleaseCallback)(T*);
    typedef void (*SyncHandle)(queue<T> &);
public:
    SafeQueue(){
        pthread_mutex_init(&mutex,0);
        pthread_cond_init(&cond,0);
    }
    ~SafeQueue(){
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
    }

    int push(T value){
        pthread_mutex_lock(&mutex);
        if (isWord!=1){
            LOG("队列存储被暂停了,未能存储的元素被即时释放了");
            releaseCallback(&value);
            return 1;
        }
        q.push(value);
        //通知 有了新数据到达
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
        return 0;
    }

    int pop(T& value){
        pthread_mutex_lock(&mutex);
        if (isWord!=1){
            return 0;
        }
        int ret = 0;
        while (q.size()==0){
            //如果没数据就等待
            pthread_cond_wait(&cond,&mutex);
        }
        if (!q.empty()){
            value = q.front();
            q.pop();
            ret = 1;
        }
        pthread_mutex_unlock(&mutex);
        return ret;
    }

    void clear(){
        pthread_mutex_lock(&mutex);
        uint32_t  size = q.size();
        for (int i = 0; i < size; ++i) {
            //取出队首的数据
            T value = q.front();
            //释放value
            //releaseCallback != NULL
            if (releaseCallback)
                releaseCallback(&value);
            q.pop();
        }
        pthread_mutex_unlock(&mutex);
    }

    void setReleaseCallback(ReleaseCallback releaseCallback){
        this->releaseCallback = releaseCallback;
    }

    void setSyncHandle(SyncHandle s) {
        syncHandle = s;
    }

    void working(){
        isWord=1;
    };

    void worked(){
        isWord=0;
    }

    int getQueueSize(){
        return q.size();
    }

    bool empty(){
        return q.empty();
    }

    void sync() {
        pthread_mutex_lock(&mutex);
        //同步代码块 当我们调用sync方法的时候，能够保证是在同步块中操作queue 队列
        syncHandle(q);
        pthread_mutex_unlock(&mutex);
    }

private:
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    queue<T> q;
    ReleaseCallback releaseCallback;
    //是否工作的标记 1：工作  0：不接受数据
    int isWord;
    SyncHandle syncHandle;
};
