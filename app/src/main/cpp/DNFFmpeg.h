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

private:
        char * dataSource;
        pthread_t pid;
        AVFormatContext * avFormatContext;
        JavaCallHelper* javaCallHelper;
        AudioChannel * audioChannel;
        VideoChannel * videoChannel;
};


#endif //FIEST_FFMPEG_DNFFMPEG_H
