
extern "C" {
#include "wavlib.h"
}
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <android/log.h>

#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))

//创建音频播放对象
void CreateBufferQueueAudioPlayer(
		WAV wav,
		SLEngineItf engineEngine,
		SLObjectItf outputMixObject,
		SLObjectItf &audioPlayerObject){

	// Android针对数据源的简单缓冲区队列定位器
	SLDataLocator_AndroidSimpleBufferQueue dataSourceLocator = {
		SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, // 定位器类型
		1                                        // 缓冲区数
	};

	// PCM数据源格式
	SLDataFormat_PCM dataSourceFormat = {
		SL_DATAFORMAT_PCM,        // 格式类型
		wav_get_channels(wav),    // 通道数
		wav_get_rate(wav) * 1000, // 毫赫兹/秒的样本数
		wav_get_bits(wav),        // 每个样本的位数
		wav_get_bits(wav),        // 容器大小
		SL_SPEAKER_FRONT_CENTER,  // 通道屏蔽
		SL_BYTEORDER_LITTLEENDIAN // 字节顺序
	};

	// 数据源是含有PCM格式的简单缓冲区队列
	SLDataSource dataSource = {
		&dataSourceLocator, // 数据定位器
		&dataSourceFormat   // 数据格式
	};

	// 针对数据接收器的输出混合定位器
	SLDataLocator_OutputMix dataSinkLocator = {
		SL_DATALOCATOR_OUTPUTMIX, // 定位器类型
		outputMixObject           // 输出混合
	};

	// 数据定位器是一个输出混合
	SLDataSink dataSink = {
		&dataSinkLocator, // 定位器
		0                 // 格式
	};

	// 需要的接口
	SLInterfaceID interfaceIds[] = {
		SL_IID_BUFFERQUEUE
	};

	// 需要的接口，如果所需要的接口不要用，请求将失败
	SLboolean requiredInterfaces[] = {
		SL_BOOLEAN_TRUE // for SL_IID_BUFFERQUEUE
	};

	// 创建音频播放器对象
	SLresult result = (*engineEngine)->CreateAudioPlayer(
			engineEngine,
			&audioPlayerObject,
			&dataSource,
			&dataSink,
			ARRAY_LEN(interfaceIds),
			interfaceIds,
			requiredInterfaces);
}
