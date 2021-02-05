//
// Created by Administrator on 2021/2/2.
//

#ifndef FIEST_FFMPEG_AUDIOCHANNEL_H
#define FIEST_FFMPEG_AUDIOCHANNEL_H


#include "BaseChannel.h"

class AudioChannel : public BaseChannel{
public:
    AudioChannel(int id,AVCodecContext* codecContext);
    void play();
};


#endif //FIEST_FFMPEG_AUDIOCHANNEL_H
