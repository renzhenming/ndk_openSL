package com.example.openslaudio;

public class AudioPlayer {

	public native static void play(String filePath);
	
	static{
		System.loadLibrary("OpenSLAudioPlayer");
	}
}
