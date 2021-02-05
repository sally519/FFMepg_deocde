package com.ffmpeg;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity implements View.OnClickListener, DNPlayer.onFFmpegInitErrorBack, DNPlayer.onPrepareOk {

    private DNPlayer dnPlayer;
    private String dataSource="http://ivi.bupt.edu.cn/hls/cctv6hd.m3u8";
    private SurfaceView video_surface;
    private Button start_btn;
    private Button stop_btn;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        video_surface=findViewById(R.id.video_surface);
        start_btn=findViewById(R.id.start_btn);
        stop_btn=findViewById(R.id.stop_btn);
        start_btn.setOnClickListener(this);
        stop_btn.setOnClickListener(this);

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