//
// Created by Administrator on 2018/9/3.
//

#include <queue>
#include <pthread.h>

using namespace std;
template <typename T>
class SafeQueue {
    typedef void (*ReleaseCallback)(T*);
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
        if (isWord!=1){
            return 404;
        }
        pthread_mutex_lock(&mutex);
        q.push(value);
        //通知 有了新数据到达
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
        return 0;
    }

    int pop(T& value){
        if (isWord!=1){
            return 404;
        }
        int ret = 0;
        pthread_mutex_lock(&mutex);
        while (q.empty()){
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

    void startWork(){
        isWord=1;
    };

    void stopWork(){
        isWord=0;
    }
private:
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    queue<T> q;
    ReleaseCallback releaseCallback;
    //是否工作的标记 1：工作  0：不接受数据
    int isWord;
};
