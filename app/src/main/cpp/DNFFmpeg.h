//
// Created by Administrator on 2021/2/2.
//

#ifndef FIEST_FFMPEG_DNFFMPEG_H
#define FIEST_FFMPEG_DNFFMPEG_H

#include "JavaCallHelper.h"
#include "AudioChannel.h"
#include "VideoChannel.h"

extern "C"{
#include <libavformat/avformat.h>
}

class DNFFmpeg {
    public:
        DNFFmpeg(JavaCallHelper* javaCallHelper,const char* dataSource);
        ~DNFFmpeg();

    void prepare();
    void _prepare();
    void start();
    void _start();
    void setRenderDataCallback(RenderFrameCallback renderFrameCallback);
    void stop();
    void pause_video();

    void continue_video();

public:
    pthread_t pid_prepare;
    pthread_t pid_play;
    pthread_t pid_stop;
    AudioChannel * audioChannel=0;
    VideoChannel * videoChannel=0;
    AVFormatContext * avFormatContext;
private:
        char * dataSource;
        JavaCallHelper* javaCallHelper;
        RenderFrameCallback renderFrameCallback;
        bool isPlaying=1;
        bool pause= false;
};

#endif //FIEST_FFMPEG_DNFFMPEG_H
