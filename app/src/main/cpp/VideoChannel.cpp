//
// Created by Administrator on 2021/2/2.
//

#include "VideoChannel.h"
#include <android/log.h>
extern "C"{
#include <libavutil/imgutils.h>
}
#define LOG(...) __android_log_print(ANDROID_LOG_ERROR,"zsq",__VA_ARGS__);

void * decode_task(void * args){
    VideoChannel * videoChannel= static_cast<VideoChannel *>(args);
    videoChannel->decode();
    return 0;
}

void * render_task(void * args){
    VideoChannel * videoChannel= static_cast<VideoChannel *>(args);
    videoChannel->render();
    return 0;
}

VideoChannel::VideoChannel(int id,AVCodecContext* codecContext) :BaseChannel(id,codecContext){
    frames.setReleaseCallback(releaseAVFrame);
}

VideoChannel::~VideoChannel() {
    frames.clear();
}

void VideoChannel::play() {
    isPlaying=1;
    frames.working();
    //1.解码
    pthread_create(&pid_decode,0,decode_task,this);
    //再开一个线程来播放
    pthread_create(&pid_render,0,render_task,this);
}

void VideoChannel::decode() {
    AVPacket * packet=0;
    int i=0;
    while (isPlaying){
        i++;
        int i=0;
        //取出一个数据包
        int ret=packets.pop(packet);
        if(!isPlaying){
            break;
        }
        if(ret!=1){
            continue;
        }
        ret=avcodec_send_packet(codecContext,packet);
        releaseCallback(&packet);
        if(ret!=0){
            break;
        }
        //代表了一个图像
        AVFrame * avFrame=av_frame_alloc();
        //从解码器里读取解码后的数据包
        ret=avcodec_receive_frame(codecContext,avFrame);
        if(ret==AVERROR(EAGAIN)){
            continue;
        } else if(ret!=0){
            break;
        } else{
            frames.push(avFrame);
            //再开一个线程播放  保证流畅度  方便影视片同步
        }
    }
    releaseCallback(&packet);
}

//开始渲染
void VideoChannel::render() {
    swsContext=sws_getContext(
            codecContext->width,codecContext->height,codecContext->pix_fmt,
            codecContext->width,codecContext->height,AV_PIX_FMT_RGBA,
            SWS_BILINEAR,0,0,0);
    AVFrame * frame=0;
    uint8_t * dst_data[4];//指针数组
    int dst_lines_ize[4];
    av_image_alloc(dst_data,dst_lines_ize,codecContext->width,codecContext->height,AV_PIX_FMT_RGBA,1);
    while (isPlaying){
        int ret=frames.pop(frame);
        if(ret!=1){

        }
        if(!isPlaying){
            break;
        }
        sws_scale(swsContext,
                frame->data,
                /**表示每一行存放的字节长度**/frame->linesize,
                0,
                codecContext->height,
                dst_data,
                dst_lines_ize);
        renderFrameCallback(dst_data[0],dst_lines_ize[0],codecContext->width,codecContext->height);
        releaseAVFrame(&frame);
    }
    av_free(&dst_data[0]);
    releaseAVFrame(&frame);
}

void VideoChannel::setRenderFrameCallback(RenderFrameCallback renderFrameCallback) {
    this->renderFrameCallback=renderFrameCallback;
}