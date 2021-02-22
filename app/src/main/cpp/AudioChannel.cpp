//
// Created by Administrator on 2021/2/2.
//

#include "AudioChannel.h"

AudioChannel::AudioChannel(int id,AVCodecContext* codecContext):BaseChannel(id,codecContext) {
    //44100*(双声道)*(16位)
    //根据布局获取声道数
    out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    out_samplesize = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);

    data= (uint8_t *)(malloc(44100 * out_channels * out_samplesize));
}

AudioChannel::~AudioChannel() {
    if(data){
        free(data);
    }
    frames.clear();
}

void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(context);
    //获得pcm 数据 多少个字节 data
    int dataSize = audioChannel->getPcm();
    if(dataSize > 0 ){
        // 接收16位数据
        (*bq)->Enqueue(bq,audioChannel->data,dataSize);
    }
}

void * audio_decode_task(void * args){
    AudioChannel * audioChannel= static_cast<AudioChannel *>(args);
    audioChannel->decode();
    return 0;
}

void * play_task(void * args){
    AudioChannel * audioChannel= static_cast<AudioChannel *>(args);
    audioChannel->_play();
    return 0;
}

void AudioChannel::play() {
    //设置为播放状态
    packets.working();
    frames.working();
    //0+输出声道+输出采样位+输出采样率+  输入的3个参数
    swrContext=swr_alloc_set_opts(0,/**声道数**/AV_CH_LAYOUT_STEREO,AV_SAMPLE_FMT_S16,44100,
            codecContext->channel_layout,codecContext->sample_fmt,codecContext->sample_rate,0,0);
    //初始化
    swr_init(swrContext);
    isPlaying=1;
    //1 、解码
    pthread_create(&pid_audio_decode, 0, audio_decode_task, this);
    //2、 播放
    pthread_create(&pid_audio_play, 0, play_task, this);
}

void AudioChannel::_play() {
    /**
     * 1、创建引擎并获取引擎接口
     */
    SLresult result;
    // 1.1 创建引擎 SLObjectItf engineObject
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 1.2 初始化引擎  init
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 1.3 获取引擎接口SLEngineItf engineInterface
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE,
                                           &engineInterface);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }

    /**
     * 2、设置混音器
     */
    // 2.1 创建混音器SLObjectItf outputMixObject
    result = (*engineInterface)->CreateOutputMix(engineInterface, &outputMixObject, 0,
                                                 0, 0);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 2.2 初始化混音器outputMixObject
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }

    /**
     * 3、创建播放器
     */
    //3.1 配置输入声音信息
    //创建buffer缓冲类型的队列 2个队列
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};
    //pcm数据格式
    //pcm+2(双声道)+44100(采样率:声音采集的频率)+ 16(采样位)+16(数据的大小)+LEFT|RIGHT(双声道)+小端数据
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1, SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN};

    //数据源 将上述配置信息放到这个数据源中
    SLDataSource slDataSource = {&android_queue, &pcm};

    //3.2  配置音轨(输出)
    //设置混音器
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&outputMix, NULL};
    //需要的接口  操作队列的接口
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    //3.3 创建播放器
    (*engineInterface)->CreateAudioPlayer(engineInterface, &bqPlayerObject, &slDataSource,
                                          &audioSnk, 1,
                                          ids, req);
    //初始化播放器
    (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);

    //得到接口后调用  获取Player接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerInterface);

    /**
     * 4、设置播放回调函数
     */
    //获取播放器队列接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                    &bqPlayerBufferQueueInterface);
    //设置回调
    (*bqPlayerBufferQueueInterface)->RegisterCallback(bqPlayerBufferQueueInterface,
                                                      bqPlayerCallback, this);
    /**
     * 5、设置播放状态
     */
    (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_PLAYING);
    /**
     * 6、手动激活一下这个回调
     */
    bqPlayerCallback(bqPlayerBufferQueueInterface, this);
}

//返回获取的pcm数据大小
int AudioChannel::getPcm() {
    int data_size=0;
    AVFrame * frame;
    int ret=frames.pop(frame);
    if(!isPlaying){
        if(ret){
            releaseAVFrame(&frame);
        }
        return data_size;
    }
    int64_t delay=swr_get_delay(swrContext,frame->sample_rate);
    //为了让数据匹配，我们需要对不匹配的数据重采样
    //将nb_sample个数据 由sample_rate采样率转换成44100后返回多少个数据
    //AV_ROUND_UP向上取整 如：1.1->2
    int64_t max_samples=av_rescale_rnd(delay + frame->nb_samples, 44100, frame->sample_rate, AV_ROUND_UP);
    //上下文+输出缓冲区+输出缓冲区的最大数据量+输入数据+输入数据个数
    //返回真正转换出多少个数据  单位是：44100*2
    int sample=swr_convert(swrContext, &data, max_samples, const_cast<const uint8_t **>(frame->data), frame->nb_samples);
    data_size=sample*out_samplesize*out_channels;
    return data_size;
}

void AudioChannel::decode() {
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