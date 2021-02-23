//
// Created by Administrator on 2021/2/2.
//

#ifndef FIEST_FFMPEG_VIDEOCHANNEL_H
#define FIEST_FFMPEG_VIDEOCHANNEL_H


#include "BaseChannel.h"
#include "AudioChannel.h"

extern "C"{
#include <libswscale/swscale.h>
};

typedef void (*RenderFrameCallback)(uint8_t *, int,int,int);
class VideoChannel : public BaseChannel{
public:
    VideoChannel(int id,AVCodecContext* codecContext,AVRational time_base,int fps);
    ~VideoChannel();
    void setAudioChannel(AudioChannel * audioChannel);
    void play();
    void render();
    void setRenderFrameCallback(RenderFrameCallback renderFrameCallback);
private:
    pthread_t pid_decode;
    pthread_t pid_render;
    SwsContext* swsContext=0;
    RenderFrameCallback renderFrameCallback;
    int fps;
    AudioChannel * audioChannel=0;
};


#endif //FIEST_FFMPEG_VIDEOCHANNEL_H
