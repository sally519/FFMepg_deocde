//
// Created by Administrator on 2021/2/2.
//

#include <cstring>
#include "DNFFmpeg.h"
#include "macro.h"
#include <pthread.h>
#include <android/log.h>
extern "C"{
#include <libavformat/avformat.h>
#include <libavutil/time.h>
}
#define LOG(...) __android_log_print(ANDROID_LOG_ERROR,"zsq",__VA_ARGS__);

//线程要执行的方法
void * task_prepare(void * args){
    DNFFmpeg * dnfFmpeg= static_cast<DNFFmpeg *>(args);
    dnfFmpeg->_prepare();
    return 0;
}

void * play(void * args){
    DNFFmpeg * dnfFmpeg= static_cast<DNFFmpeg *>(args);
    dnfFmpeg->_start();
    return 0;
}

DNFFmpeg::DNFFmpeg(JavaCallHelper* javaCallHelper,const char *dataSource){
    this->javaCallHelper = javaCallHelper;
    //防止dataSource指向的内存被释放
    this->dataSource=new char[strlen(dataSource)+1];
    strcpy(this->dataSource,dataSource);
    LOG("播放链接%s", this->dataSource);
}

DNFFmpeg::~DNFFmpeg() {
    DELETE(dataSource);
}

void DNFFmpeg::prepare() {
    //创建一个线程，在线程里进行视频的解析
    pthread_create(&pid_prepare, 0, task_prepare, this);
}

void DNFFmpeg::_prepare() {
    //初始化网络
    avformat_network_init();
    //1.打开媒体文件地址（文件地址、直播地址）
    //AVFormatContext 包含了视频的信息  （宽  高...）
    avFormatContext=0;
    //文件路径不对、手机没网，说明失败
    AVDictionary  *option=0;
    //设置超时时间5s  单位是微秒
    av_dict_set(&option,"timeout","5000000",0);
    int ret = avformat_open_input(&avFormatContext,
            dataSource,
    /**指示打开的媒体格式（传空，ffmpeg会自动推倒）**/0,
                                  &option);
    av_dict_free(&option);
    //ret 不为0表示打开失败
    if(ret!=0){
        LOGE("FFMPEG_CAN_NOT_OPEN_URL");
        if(isPlaying){
            this->javaCallHelper->onError(THREAD_CHILD,FFMPEG_CAN_NOT_OPEN_URL);
        }
        return;
    }
    //2.查找文件中的 音视频流
    ret=avformat_find_stream_info(avFormatContext,0);
    if(ret<0){
        LOGE("FFMPEG_CAN_NOT_FIND_STREAMS");
        if(isPlaying) {
            this->javaCallHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        }
        return;
    }
    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
        //可能代表式一个视频 也可能是一个音频
        AVStream *stream=avFormatContext->streams[i];
        AVCodecParameters *codecParameters=stream->codecpar;

        /**
         * 无论是音频还是视频都需要干的一些事情
         * 1.通过当前流使用的编码方式来查找 解码器 ：avcodec_find_decoder()
         * */
         AVCodec* dec=avcodec_find_decoder(codecParameters->codec_id);
         if(NULL==dec){
             LOGE("FFMPEG_FIND_DECODER_FAIL");
             if(isPlaying) {
                 this->javaCallHelper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
             }
             return;
         }

         /**
          * 2.获得解码器上下文
          * */
        AVCodecContext* codecContext=avcodec_alloc_context3(dec);
        if(NULL == codecContext){
            LOGE("FFMPEG_ALLOC_CODEC_CONTEXT_FAIL");
            if(isPlaying) {
                this->javaCallHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            }
            return;
        }

        /**
          * 3.设置上下文内的一些参数
          * */
         ret= avcodec_parameters_to_context(codecContext, codecParameters);
         if(ret<0){
             LOGE("FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL");
             if(isPlaying) {
                 this->javaCallHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
             }
             return;
         }

        /**
         * 4.打开编码器
         * */
         ret=avcodec_open2(codecContext, dec, 0);
         if(ret!=0){
             LOGE("FFMPEG_OPEN_DECODER_FAIL");
             if(isPlaying) {
                 this->javaCallHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
             }
             return;
         }
         //单位
         AVRational time_base=stream->time_base;
        //音频
        if(codecParameters->codec_type==AVMEDIA_TYPE_VIDEO){
            //帧率：单位时间内要显示多少个图像
            AVRational frame_rate=stream->avg_frame_rate;
            int fps = av_q2d(frame_rate);
            videoChannel=new VideoChannel(i,codecContext,time_base,fps);
            videoChannel->setRenderFrameCallback(renderFrameCallback);
        } else if(codecParameters->codec_type==AVMEDIA_TYPE_AUDIO){
            audioChannel=new AudioChannel(i,codecContext,time_base);
        }
    }

    //文件里没有音视频
    if(!audioChannel&&!videoChannel){
        LOGE("FFMPEG_NOMEDIA");
        if(isPlaying) {
            this->javaCallHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        }
        return;
    }

    //准备完了  通知java 你可以开始播放了
    if(isPlaying) {
        this->javaCallHelper->onPrepare(THREAD_CHILD);
    }
    LOGE("初始化成功");
}

void DNFFmpeg::start() {
    //正在播放
    isPlaying= 1;
    if(audioChannel){
        audioChannel->packets.working();
        audioChannel->play();
    }
    if(videoChannel){
        videoChannel->packets.working();
        videoChannel->setAudioChannel(audioChannel);
        videoChannel->play();
    }
    pthread_create(&pid_play,0,play, this);
}

/**
 * 专门读取数据包
 * */
void DNFFmpeg::_start() {
    int ret;
    /**
     * 1.读取媒体数据包（音视频数据包）
     * */
     while (isPlaying){
         //通过pause变量控制读包，达到暂停的效果
         if(pause){
             continue;
         }
         //读取的时候读慢一点，防止OOM
         if(audioChannel&&audioChannel->packets.getQueueSize()>50){
             av_usleep(1000*10);
             continue;
         }
         if(videoChannel&&videoChannel->packets.getQueueSize()>50){
             av_usleep(1000*10);
             continue;
         }
         AVPacket * packet=av_packet_alloc();
         ret=av_read_frame(avFormatContext,packet);
         if (ret==0){//0表示成功 其他失败
             if(audioChannel&&packet->stream_index==audioChannel->id){
                 audioChannel->packets.push(packet);
             } else if (videoChannel&&packet->stream_index==videoChannel->id){
                 videoChannel->packets.push(packet);
             }
         } else if(ret==AVERROR_EOF){
             //读取成功 但是还没有播放完
             if(audioChannel->packets.empty()&&videoChannel->packets.empty()
             &&audioChannel->frames.empty()&&audioChannel->frames.empty()){
                 break;
             }
             //如果是做直播，可以sleep
             //如果支持点播（播放本地文件），seek后退要重新进入循环
         } else{
             break;
         }
     }
    /**
     * 2.解码
     * */
     audioChannel->stop();
     videoChannel->stop();
}

void DNFFmpeg::setRenderDataCallback(RenderFrameCallback renderFrameCallback) {
    this->renderFrameCallback=renderFrameCallback;
}

void * stop_play(void * args){
    DNFFmpeg *fmpeg= static_cast<DNFFmpeg *>(args);
    //等待prepare结束
    pthread_join(fmpeg->pid_prepare,0);
    //保证start线程也结束
    pthread_join(fmpeg->pid_play,0);
    DELETE(fmpeg->audioChannel);
    DELETE(fmpeg->videoChannel);
    if(fmpeg->avFormatContext){
        //先关闭输入输出流，再释放上下文
        avformat_close_input(&fmpeg->avFormatContext);
        avformat_free_context(fmpeg->avFormatContext);
        fmpeg->avFormatContext=0;
    }
    DELETE(fmpeg);
    return 0;
}

void DNFFmpeg::stop() {
    isPlaying=0;
    javaCallHelper=0;
    pthread_create(&pid_stop,0,stop_play, this);
}

void DNFFmpeg::pause_video() {
    this->pause= true;
    if(audioChannel){
        audioChannel->pause();
    }
    if(videoChannel){
        videoChannel->pause();
    }
}

void DNFFmpeg::continue_video() {
    this->pause= false;
    if(audioChannel){
        audioChannel->restart();
    }
    if(videoChannel){
        videoChannel->restart();
    }
}
