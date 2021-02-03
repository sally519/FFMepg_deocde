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

private:
        char * dataSource;
        pthread_t pid_prepare;
        pthread_t pid_play;
        AVFormatContext * avFormatContext;
        JavaCallHelper* javaCallHelper;
        AudioChannel * audioChannel=0;
        VideoChannel * videoChannel=0;
        bool isPlaying;
};

#endif //FIEST_FFMPEG_DNFFMPEG_H
