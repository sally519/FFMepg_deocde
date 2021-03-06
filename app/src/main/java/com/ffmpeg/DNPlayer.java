package com.ffmpeg;

import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class DNPlayer implements SurfaceHolder.Callback {

    private String dataSource;
    private SurfaceView surfaceView;
    private onFFmpegInitErrorBack onFFmpegInitErrorBack;
    private onPrepareOk onPrepareOk;

    static {
        System.loadLibrary("native-lib");
    }

    public native void native_prepare(String dataSource);
    public native void native_start();
    public native void native_stop();
    public native void native_pause();
    public native void continue_play();
    public native void native_release();
    public native void native_setSurface(Surface surface);

    public void continuePlay() {
        continue_play();
    }

    public interface onFFmpegInitErrorBack{
        void onFFmpegInitError(int ret);
    }

    public interface onPrepareOk{
        void onOk();
    }

    private SurfaceHolder holder;

    /**
     * 让使用者 设置播放的文件和地址
     * */
    public void setDataSource(String dataSource){
        this.dataSource=dataSource;
    }

    /**
     * 开始播放
     * */
    public void start(){
        native_start();
    }

    /**
     * 准备好要播放的视频
     * */
    public void prepare(){
        native_prepare(dataSource);
    }

    /**
     * 停止播放
     * */
    public void stop(){
        native_stop();
    }

    /**
     * 暂停播放
     * */
    public void pause(){
        native_pause();
    }

    /**
     * 让使用者 释放播放链接
     * */
    public void release(){
        holder.removeCallback(this);
        native_release();
    }

    /**
     * 设置播放显示的画布
     * */
    public void setSurfaceView(SurfaceView surfaceView) {
        if(null!=holder){
            holder.removeCallback(this);
        }
        this.surfaceView = surfaceView;
        holder = surfaceView.getHolder();
        holder.addCallback(this);
    }

    /**
     * FFmpeg已经准备完成，可以播放时的回调
     * */
    public void onPrepare(){
        if(null!=onPrepareOk){
            onPrepareOk.onOk();
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        native_setSurface(holder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    public void onError(int errorCode){
        if(null!=onFFmpegInitErrorBack) {
            this.onFFmpegInitErrorBack.onFFmpegInitError(errorCode);
        }
    }

    public void setOnFFmpegInitErrorBack(DNPlayer.onFFmpegInitErrorBack onFFmpegInitErrorBack) {
        this.onFFmpegInitErrorBack = onFFmpegInitErrorBack;
    }

    public void setOnPrepareOk(DNPlayer.onPrepareOk onPrepareOk) {
        this.onPrepareOk = onPrepareOk;
    }
}
