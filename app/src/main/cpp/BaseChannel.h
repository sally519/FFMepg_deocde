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
    BaseChannel(int id):id(id){

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
            avPacket=0;
        }
    }

    int id;
    SafeQueue<AVPacket*> packets;
};

#endif //FIEST_FFMPEG_BASECHANNEL_H
