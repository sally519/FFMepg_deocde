//
// Created by Administrator on 2021/2/2.
//

#include <cstring>
#include "DNFFmpeg.h"
#include "macro.h"
#include <pthread.h>
extern "C"{
#include <libavformat/avformat.h>
}

//线程要执行的方法
void * task_prepare(void * args){
    DNFFmpeg * dnfFmpeg= static_cast<DNFFmpeg *>(args);
    dnfFmpeg->_prepare();
    return 0;
}

DNFFmpeg::DNFFmpeg(JavaCallHelper* javaCallHelper,const char *dataSource){
    //防止dataSource指向的内存被释放
    this->javaCallHelper = javaCallHelper;
    this->dataSource=new char[strlen(dataSource)];
    strcpy(this->dataSource,dataSource);
}

DNFFmpeg::~DNFFmpeg() {
    DELETE(javaCallHelper);
    DELETE(dataSource);
}

void DNFFmpeg::prepare() {
    //创建一个线程，在线程里进行视频的解析
    pthread_create(&pid,0,task_prepare, this);
}

void DNFFmpeg::_prepare() {
    //初始化网络
    avformat_network_init();
    //1.打开媒体文件地址（文件地址、直播地址）
    //AVFormatContext 包含了视频的信息  （宽  高...）
    avFormatContext=0;
    //文件路径不对、手机没网，说明失败
    int ret = avformat_open_input(&avFormatContext,dataSource,0,0);
    //ret 不为0表示打开失败
    if(ret!=0){
        LOGE("FFMPEG_CAN_NOT_OPEN_URL");
        this->javaCallHelper->onError(THREAD_CHILD,FFMPEG_CAN_NOT_OPEN_URL);
        return;
    }
    //2.查找文件中的 音视频流
    ret=avformat_find_stream_info(avFormatContext,0);
    if(ret<0){
        LOGE("FFMPEG_CAN_NOT_FIND_STREAMS");
        this->javaCallHelper->onError(THREAD_CHILD,FFMPEG_CAN_NOT_FIND_STREAMS);
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
             this->javaCallHelper->onError(THREAD_CHILD,FFMPEG_FIND_DECODER_FAIL);
             return;
         }

         /**
          * 2.获得解码器上下文
          * */
        AVCodecContext* context=avcodec_alloc_context3(dec);
        if(NULL==context){
            LOGE("FFMPEG_ALLOC_CODEC_CONTEXT_FAIL");
            this->javaCallHelper->onError(THREAD_CHILD,FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            return;
        }
        /**
          * 3.设置上下文内的一些参数
          * */
         ret= avcodec_parameters_to_context(context,codecParameters);
         if(ret<0){
             LOGE("FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL");
             this->javaCallHelper->onError(THREAD_CHILD,FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
             return;
         }
        /**
         * 4.打开编码器
         * */
         ret=avcodec_open2(context,dec,0);
         if(ret!=0){
             LOGE("FFMPEG_OPEN_DECODER_FAIL");
             this->javaCallHelper->onError(THREAD_CHILD,FFMPEG_OPEN_DECODER_FAIL);
             return;
         }

        //音频
        if(codecParameters->codec_type==AVMEDIA_TYPE_VIDEO){
            videoChannel=new VideoChannel;
        } else if(codecParameters->codec_type==AVMEDIA_TYPE_AUDIO){
            audioChannel=new AudioChannel;
        }
    }

    //文件里没有音视频
    if(!audioChannel&&!videoChannel){
        LOGE("FFMPEG_NOMEDIA");
        this->javaCallHelper->onError(THREAD_CHILD,FFMPEG_NOMEDIA);
        return;
    }

    //准备完了  通知java 你可以开始播放了
    this->javaCallHelper->onPrepare(THREAD_CHILD);
    LOGE("初始化成功");
}