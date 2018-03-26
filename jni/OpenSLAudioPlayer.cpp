#include <jni.h>
#include "com_example_openslaudio_AudioPlayer.h"
#include "CreateBufferQueueAudioPlayer.cpp"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <android/log.h>
extern "C" {
#include "wavlib.h"
}
#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,"renzhenming",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"renzhenming",FORMAT,##__VA_ARGS__);

#define ARRAR_LEN(a) (sizeof(a)/sizeof(a[0]))

//wav文件指针
WAV wav;

//引擎对象
SLObjectItf engineObject;

//引擎接口
SLEngineItf engineInterface;

//混音器
SLObjectItf outputMixObject;

//播放器
SLObjectItf audioPlayerObject;

//缓冲区队列接口
SLAndroidSimpleBufferQueueItf andioPlayerBufferQueueItf;

//播放接口
SLPlayItf audioPlayInterface;

//缓冲区
unsigned char *buffer;

//缓冲区大小
size_t bufferSize;


//打开文件
WAV OpenWaveFile(JNIEnv *env,jstring jinput){
	const char* cinput_file = env->GetStringUTFChars(jinput,NULL);

	/*typedef enum {
	    WAV_SUCCESS,         no error so far
	    WAV_NO_MEM,          can't acquire requested amount of memory
	    WAV_IO_ERROR,        unable to read/write (often write) data
	    WAV_BAD_FORMAT,      acquired data doesn't seem wav-compliant
	    WAV_BAD_PARAM,       bad parameter for requested operation
	    WAV_UNSUPPORTED,     feature not yet supported by wavlib
	} WAVError;*/
	WAVError err;

	//WAV wav_open(const char *filename, WAVMode mode, WAVError *err);
	/**
	 * typedef enum {
     *		WAV_READ  = 1,  open WAV file in read-only mode
     *		WAV_WRITE = 2,  open WAV file in write-only mode
     *		WAV_PIPE  = 4,  cut SEEEKs
	 *		} WAVMode;
	 */

	WAV wav = wav_open(cinput_file,WAV_READ,&err);

	//LOGI("%d",wav_get_bitrate(wav));
	env->ReleaseStringUTFChars(jinput,cinput_file);
	if(wav == 0){
		//get a human-readable short description of an error code
		//Return Value:a pointer to a C-string describing the given error code,
		//or NULL if given error code isn't known.
		//LOGE("%s",wav_strerror(err));
	}
	return wav;
}

//实例化对象
void RealizeObject(SLObjectItf object){
	//非异步（会阻塞线程）
	/**
	 * 	SLresult (*Realize) (
	 *		SLObjectItf self,
	 *		SLboolean async
	 *	);
	 */
	(*object)->Realize(object,SL_BOOLEAN_FALSE);
}

//上下文
struct PlayerContext{
	WAV wav;
	unsigned char *buffer;
	size_t bufferSize;

	PlayerContext(WAV wav,unsigned char *buffer,size_t bufferSize){
		this->wav = wav;
		this->buffer = buffer;
		this->bufferSize = bufferSize;
	}
};

//关闭文件
void CloseWaveFile(WAV wav){
	wav_close(wav);
}


//回调函数
/*
 * wav_read_data:
 *      read a buffer of pcm data from given wav file. Delivers data
 *      in wav-native-byte-order (little endian).
 *      This function doesn't mess with given data, it just reads
 *      data verbatim from wav file. so caller must take care to
 *      split/join channel data or do any needed operation.
 *
 * Parameters:
 *      handle: wav handle to write data in
 *      buffer: pointer to data to store the data readed
 *      bufsize: size of given buffer.
 * Return Value:
 *      return of bytes effectively readed from wav file.
 *      -1 means an error.
 * Side effects:
 *      N/A
 * Preconditions:
 *      given wav handle is a valid one obtained as return value of
 *      wav_open/wav_fdopen; wav handle was opened in WAV_READ mode.
 *      bufsize is a multiple of 2.
 * Postconditions:
 *      N/A
 *
 * ssize_t wav_read_data(WAV handle, uint8_t *buffer, size_t bufsize);
 */
void playerCallBack(SLAndroidSimpleBufferQueueItf andioPlayerBufferQueue,void *context){
	PlayerContext *ctx = (PlayerContext*)context;
	//读取数据
	ssize_t readSize = wav_read_data(ctx->wav,ctx->buffer,ctx->bufferSize);
	if(readSize > 0){
		/**
		 * 	SLresult (*Enqueue) (
				SLAndroidSimpleBufferQueueItf self,
				const void *pBuffer,
				SLuint32 size
			);
		 */
		(*andioPlayerBufferQueue)->Enqueue(andioPlayerBufferQueue,ctx->buffer,readSize);
	}else{
		//destory context中保存的东西

		//关闭文件
		CloseWaveFile(ctx->wav);

		//释放缓存
		delete ctx->buffer;
	}
}


JNIEXPORT void JNICALL Java_com_example_openslaudio_AudioPlayer_play
  (JNIEnv *env, jclass jclazz, jstring input){

	//打开文件
	WAV wav = OpenWaveFile(env,input);

	//创建OpenSL ES引擎，OpenSL ES在Android平台下默认是线程安全的，这样设置是为了兼容其它平台
	/**
	 * 	SL_API SLresult SLAPIENTRY slCreateEngine(
			SLObjectItf             *pEngine,
			SLuint32                numOptions,
			const SLEngineOption    *pEngineOptions,
			SLuint32                numInterfaces,
			const SLInterfaceID     *pInterfaceIds,
			const SLboolean         * pInterfaceRequired
		);
	 */
	SLEngineOption engineOptions[] ={
			{(SLuint32)SL_ENGINEOPTION_THREADSAFE,(SLuint32)SL_BOOLEAN_TRUE}
	};
	slCreateEngine(&engineObject,ARRAR_LEN(engineObject),engineOptions,0,0,0);//没有接口

	//实例化对象，对象创建之后，处于未实例化状态，对象虽然存在但未分配任何资源，使用前先实例化（使用完之后destroy）
	RealizeObject(engineObject);

	//获取引擎接口
	/**
	 * 	SLresult (*GetInterface) (
			SLObjectItf self,
			const SLInterfaceID iid,
			void * pInterface
		);

		typedef const struct SLInterfaceID_ {
    		SLuint32 time_low;
    		SLuint16 time_mid;
    		SLuint16 time_hi_and_version;
    		SLuint16 clock_seq;
    		SLuint8  node[6];
		} * SLInterfaceID;
		extern SL_API const SLInterfaceID SL_IID_ENGINE;
	 */
	(*engineObject)->GetInterface(engineObject,SL_IID_ENGINE,&engineInterface);

	//创建输出混音器
	/**
	 * 	SLresult (*CreateOutputMix) (
			SLEngineItf self,
			SLObjectItf * pMix,
			SLuint32 numInterfaces,
			const SLInterfaceID * pInterfaceIds,
			const SLboolean * pInterfaceRequired
		);
	 */
	(*engineInterface)->CreateOutputMix(engineInterface,&outputMixObject,0,0,0);//没有接口

	//实例化混音器
	RealizeObject(outputMixObject);

	//创建缓冲区保存读取到的音频数据库
	//缓冲区大小       uint8_t wav_get_channels(WAV handle);
	bufferSize = wav_get_channels(wav)*wav_get_rate(wav)*wav_get_bits(wav);
	buffer = new unsigned char[bufferSize];

	//创建带有缓冲区队列的音频播放器
	CreateBufferQueueAudioPlayer(wav,engineInterface,outputMixObject,audioPlayerObject);

	//实例化音频播放器
	RealizeObject(audioPlayerObject);

	//获取缓冲区队列接口Buffer Queue Interface
	//通过缓冲区接口对缓冲区进行排序播放
	/**
	 * 	SLresult (*GetInterface) (
			SLObjectItf self,
			const SLInterfaceID iid,
			void * pInterface
		);

		typedef const struct SLInterfaceID_ {
    		SLuint32 time_low;
    		SLuint16 time_mid;
    		SLuint16 time_hi_and_version;
    		SLuint16 clock_seq;
    		SLuint8  node[6];
		} * SLInterfaceID;
		extern SL_API const SLInterfaceID SL_IID_BUFFERQUEUE;
	 */
	(*audioPlayerObject)->GetInterface(audioPlayerObject,SL_IID_BUFFERQUEUE,&andioPlayerBufferQueueItf);

	//注册音频播放器回调函数
	//当播放器完成对前一个缓冲区队列的播放时，回调函数会被调用，然后我们又继续读取音频数据，直到结束
	//上下文，包裹参数方便再回调函数中使用
	/**
	 * 	SLresult (*RegisterCallback) (
			SLAndroidSimpleBufferQueueItf self,
			slAndroidSimpleBufferQueueCallback callback,
			void* pContext
		);
	 */
	PlayerContext *ctx = new PlayerContext(wav,buffer,bufferSize);
	(*andioPlayerBufferQueueItf)->RegisterCallback(andioPlayerBufferQueueItf,playerCallBack,ctx);

	//获取Play Interface 通过对SetPlayState函数来启动播放音乐
	//一旦播放器被置为播放状态。该音频播放器开始等待缓冲区排队就绪
	/**
	 * 	SLresult (*GetInterface) (
			SLObjectItf self,
			const SLInterfaceID iid,
			void * pInterface
		);
	 */
	(*audioPlayerObject)->GetInterface(audioPlayerObject,SL_IID_PLAY,&audioPlayInterface);

	//设置播放状态
	/**
	 * 	SLresult (*SetPlayState) (
			SLPlayItf self,
			SLuint32 state
		);
	 */
	(*audioPlayInterface)->SetPlayState(audioPlayInterface,SL_PLAYSTATE_PLAYING);

	//开始，让第一个缓冲区入队
	playerCallBack(andioPlayerBufferQueueItf,ctx);
}














