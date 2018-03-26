package com.example.openslaudio;

import java.io.File;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;

public class MainActivity extends Activity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
	}

	/**
	 * 播放音频文件
	 * @param btn
	 */
	public void play(View btn){
		new Thread(){
			public void run() {
				File file = new File(Environment.getExternalStorageDirectory(),"music.mp3");
				AudioPlayer.play(file.getAbsolutePath());
			}
		}.start();
	}
}
