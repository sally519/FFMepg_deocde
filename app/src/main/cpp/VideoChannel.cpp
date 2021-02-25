//
// Created by Administrator on 2021/2/2.
//

#include "VideoChannel.h"
#include <android/log.h>
extern "C"{
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
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

/**
 * 丢已经解码的图片
 * @param q
 */
void dropAvFrame(queue<AVFrame *> &q) {
    if (!q.empty()) {
        AVFrame *frame = q.front();
        BaseChannel::releaseAVFrame(&frame);
        q.pop();
    }
}

VideoChannel::VideoChannel(int id,AVCodecContext* codecContext,AVRational time_base,int fps) :BaseChannel(id,codecContext,time_base){
    frames.setSyncHandle(dropAvFrame);
    this->fps=fps;
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

//开始渲染
void VideoChannel::render() {
    swsContext=sws_getContext(
            codecContext->width,codecContext->height,codecContext->pix_fmt,
            codecContext->width,codecContext->height,AV_PIX_FMT_RGBA,
            SWS_BILINEAR,0,0,0);
    //每个画面刷新的间隔
    double frame_delays=1.0/fps*AV_TIME_BASE;
    AVFrame * frame=0;
    uint8_t * dst_data[4];//指针数组
    int dst_lines_ize[4];
    av_image_alloc(dst_data,dst_lines_ize,codecContext->width,codecContext->height,AV_PIX_FMT_RGBA,1);
    while (isPlaying){
        if(onPause){
            continue;
        }
        int ret=frames.pop(frame);
        if(ret!=1){
            continue;
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
        //获得当前画面播放的相对时间,视频的话一般用best_effort_timestamp，和音频有点区别
        clock=frame->best_effort_timestamp*av_q2d(time_base);
        //额外的画面刷新间隔时间
        double extra_delay=frame->repeat_pict/(2*fps)*AV_TIME_BASE;
        //真实需要的间隔时间
        double delays=extra_delay+frame_delays;
        //通过休眠来设置帧率
        if(!audioChannel){
            av_usleep(delays);
        }else{
            if(clock==0){
                av_usleep(delays);
            } else{
                double audioClock=audioChannel->clock;
                if(audioClock==0){//第一次audioClock可能为0，音频解码还没有开始，要防止线程陷入长时间沉睡
                    continue;
                }
                //音频相对播放时间-视频相对播放时间
                double diff_time= AV_TIME_BASE*(clock-audioClock);
                if(diff_time>0){
                    av_usleep(delays + diff_time);
                } else{
                    if(fabs(diff_time)>=0.05){
                        //丢帧
                        frames.sync();
                        releaseAVFrame(&frame);
                        continue;
                    }
                }
            }
        }
        renderFrameCallback(dst_data[0],dst_lines_ize[0],codecContext->width,codecContext->height);
        releaseAVFrame(&frame);
    }
    av_free(&dst_data[0]);
    releaseAVFrame(&frame);

    isPlaying=0;
    sws_freeContext(swsContext);
    swsContext=0;
}

void VideoChannel::setRenderFrameCallback(RenderFrameCallback renderFrameCallback) {
    this->renderFrameCallback=renderFrameCallback;
}

void VideoChannel::setAudioChannel(AudioChannel *audioChannel) {
    this->audioChannel=audioChannel;
}

void VideoChannel::stop() {
    LOG("视频停止播放");
    isPlaying=0;
    packets.worked();
    frames.worked();
    pthread_join(pid_decode,0);
    pthread_join(pid_render,0);
}

void VideoChannel::pause() {
    onPause=true;
}

void VideoChannel::restart() {
    onPause=false;
}
