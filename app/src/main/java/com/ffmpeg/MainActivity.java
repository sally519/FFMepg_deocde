package com.ffmpeg;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity implements View.OnClickListener, DNPlayer.onFFmpegInitErrorBack, DNPlayer.onPrepareOk {

    private DNPlayer dnPlayer;
    private String dataSource="http://ivi.bupt.edu.cn/hls/cctv6hd.m3u8";
//    private String dataSource="http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear2/prog_index.m3u8";
    private SurfaceView video_surface;
    private Button start_btn;
    private Button stop_btn;
    private Button pause_btn;
    private Button continue_play_btn;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        video_surface=findViewById(R.id.video_surface);
        start_btn=findViewById(R.id.start_btn);
        stop_btn=findViewById(R.id.stop_btn);
        pause_btn=findViewById(R.id.pause_btn);
        continue_play_btn=findViewById(R.id.continue_play_btn);
        start_btn.setOnClickListener(this);
        stop_btn.setOnClickListener(this);
        pause_btn.setOnClickListener(this);
        continue_play_btn.setOnClickListener(this);

        dnPlayer=new DNPlayer();
        dnPlayer.setSurfaceView(video_surface);
        dnPlayer.setDataSource(dataSource);
        dnPlayer.setOnFFmpegInitErrorBack(this);
        dnPlayer.setOnPrepareOk(this);
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()){
            case R.id.start_btn:
                dnPlayer.prepare();
                break;
            case R.id.stop_btn:
                dnPlayer.stop();
                break;
            case R.id.pause_btn:
                dnPlayer.pause();
                break;
            case R.id.continue_play_btn:
                dnPlayer.continuePlay();
                break;
        }
    }

    @Override
    public void onFFmpegInitError(int ret) {
        runOnUiThread(() -> Toast.makeText(MainActivity.this, "初始化失败，对应错误码："+ ret, Toast.LENGTH_SHORT).show());
    }

    @Override
    public void onOk() {
        runOnUiThread(() -> {
            Toast.makeText(MainActivity.this, "FFmpeg解码器准备成功了，可以开始播放了", Toast.LENGTH_SHORT).show();
            dnPlayer.start();
        });
    }
}