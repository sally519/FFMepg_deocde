//
// Created by Administrator on 2021/2/2.
//

#ifndef FIEST_FFMPEG_AUDIOCHANNEL_H
#define FIEST_FFMPEG_AUDIOCHANNEL_H


#include "BaseChannel.h"

#include "BaseChannel.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
extern  "C"{
#include <libswresample/swresample.h>
};

class AudioChannel : public BaseChannel{
public:
    AudioChannel(int id,AVCodecContext* codecContext,AVRational time_base);
    ~AudioChannel();
    void play();
    void _play();
    int getPcm();

    void stop();
    void pause();

    void restart();

public:
    uint8_t *data;
private:
    pthread_t pid_audio_decode;
    pthread_t pid_audio_play;
    /**
     * OpenSL ES
     */
    // 引擎与引擎接口
    SLObjectItf engineObject = 0;
    SLEngineItf engineInterface = 0;
    //混音器
    SLObjectItf outputMixObject = 0;
    //播放器
    SLObjectItf bqPlayerObject = 0;
    //播放器接口
    SLPlayItf bqPlayerInterface = 0;

    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueueInterface =0;

    //重采样
    SwrContext *swrContext = 0;

    int out_channels;
    int out_samplesize;
};

#endif //FIEST_FFMPEG_AUDIOCHANNEL_H
