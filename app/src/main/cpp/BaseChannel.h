//
// Created by Administrator on 2021/2/3.
//

#ifndef FIEST_FFMPEG_BASECHANNEL_H
#define FIEST_FFMPEG_BASECHANNEL_H

#include "SafeQueue.h"
extern "C"{
    #include <libavcodec/avcodec.h>
};
class BaseChannel{
public:
    BaseChannel(int id,AVCodecContext* codecContext):id(id),codecContext(codecContext){

    }
    virtual ~BaseChannel(){
        packets.setReleaseCallback(releaseCallback);
        packets.clear();
    }

    /**
     * 释放avPacket
     * */
    static void releaseCallback(AVPacket** avPacket){
        if(avPacket){
            av_packet_free(avPacket);
            *avPacket=0;
        }
    }

    /**
     * 释放帧
     * */
    static void releaseAVFrame(AVFrame** pAvFrame){
        if(pAvFrame){
            av_frame_free(pAvFrame);
            *pAvFrame=0;
        }
    }

    virtual void play()=0;

    int id;
    SafeQueue<AVPacket*> packets;
    bool isPlaying;
    AVCodecContext* codecContext;
};

#endif //FIEST_FFMPEG_BASECHANNEL_H
