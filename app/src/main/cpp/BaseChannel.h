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
    BaseChannel(int id,AVCodecContext* codecContext,AVRational time_base):id(id),codecContext(codecContext),time_base(time_base){

    }
    virtual ~BaseChannel(){
        packets.setReleaseCallback(releaseCallback);
        packets.clear();
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

    /**
     * 释放avPacket
     * */
    static void releaseCallback(AVPacket** avPacket){
        if(avPacket){
            //如果不属于I帧
            if((*avPacket)->flags!=AV_PKT_FLAG_KEY){
                av_packet_free(avPacket);
                *avPacket=0;
            }
        }
    }

    /**
     * 解码
     * */
    void decode() {
        AVPacket *packet = 0;
        while (isPlaying) {
            //取出一个数据包
            int ret = packets.pop(packet);
            if (!isPlaying) {
                break;
            }
            //取出失败
            if (!ret) {
                continue;
            }
            //把包丢给解码器
            ret = avcodec_send_packet(codecContext, packet);
            releaseCallback(&packet);
            //重试
            if (ret != 0) {
                break;
            }
            //代表了一个图像 (将这个图像先输出来)
            AVFrame *frame = av_frame_alloc();
            //从解码器中读取 解码后的数据包 AVFrame
            ret = avcodec_receive_frame(codecContext, frame);
            //需要更多的数据才能够进行解码
            if (ret == AVERROR(EAGAIN)) {
                continue;
                break;
            }
            //再开一个        } else if (ret != 0) {线程 来播放 (流畅度)
            frames.push(frame);
        }
        releaseCallback(&packet);
    }

    virtual void play()=0;

    int id;
    SafeQueue<AVPacket*> packets;
    SafeQueue<AVFrame*> frames;
    bool isPlaying;
    AVCodecContext* codecContext;
    AVRational time_base;
    double clock;
};

#endif //FIEST_FFMPEG_BASECHANNEL_H
