//#define __USE_GNU
//#define _GNU_SOURCE

//#include <opus/opus.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
//#include "CircularBuffer.h"
#include "BlockingRecord.h"
//#include "Parameters.h"

//--------------------------------------------------------------------------------------------------------------------
//circular_buf_t cbuf;		//global
//unsigned char* wptr;
//unsigned char* rptr;
//sem_t EmptySem;
//sem_t FullSem;

extern int OpenOutputStream(
         const char* address, bool mono, uint32_t sampleRate, audio_stream_out_t** streamOut);

void *Record_thread_func(void *param);

int SamplingRate=16000;
int FrameDuration=40 ;	//ms // frame duration must be 40 sec to record 640 bytes
int bit_Depth=16;
int Compress_ratio=8;
int Complexity=0;
int Channels=1;

void *Record_thread_func(void *param)
{


	int SampleBytes	= ((int)bit_Depth/(int)8); // 2
	int Frame_Size	= ((int)SampleBytes*(int)Channels); // 2
	int NumFrames 	=(((int)SamplingRate*(int)FrameDuration)/(int)1000); // 16000*10/1000= 160/480
	int BufferSize	 =((int)NumFrames*(int)Frame_Size);// 160,000 * 2

	int CodecBitRate =(((int)SamplingRate/(int)Compress_ratio)*(int)bit_Depth);
	int EncodedBytes =((int)BufferSize/(int)Compress_ratio);

    			// put the thread on a separate core
//    OpusEncoder *enc;
    char* buf;
    PaStream*  st=NULL;
    int len=0;
	int error=-1;
	int i;

//	int mask=3;
//	cpu_set_t cpuset;
//    CPU_ZERO(&cpuset);
//    CPU_SET(mask,&cpuset);
//
//    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t),&cpuset) <0) {
//        perror("pthread_setaffinity_np");
//    }
	  const char* address_ = "1";
	  int rc;
	  audio_stream_out_t* streamOut;


    st=Start_Recording(st,NumFrames,SamplingRate,Channels,bit_Depth);
    if(st==NULL) printf("\n\n st=null  :/ \n\n");

//    enc=opus_encoder_create(SamplingRate, Channels, OPUS_APPLICATION_AUDIO, &error);
//    if(error!=OPUS_OK) printf("Encoder creation Error\n");
//    opus_encoder_ctl(enc, OPUS_SET_BITRATE(CodecBitRate));      // !!! default is determined based on #channels,Fs
//    opus_encoder_ctl(enc,  OPUS_SET_VBR(0));				// use CBR
//    opus_encoder_ctl(enc, OPUS_SET_COMPLEXITY(Complexity));
//    opus_encoder_ctl(enc, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));


    buf=(char*)malloc(BufferSize);    // recording buffer
    memset(buf,0,BufferSize);					// remark
	rc = OpenOutputStream(address_, true /*mono*/, 16000, &streamOut);

    for(;;){


		for(i=0;i<(EncodedBytes/20);i++)
//			sem_wait(&EmptySem);

		Pa_ReadStream(st,buf,NumFrames);

//		wptr=buf_get_WritePtr(&cbuf);
//		len = opus_encode(enc,(opus_int16*)buf,NumFrames,wptr,BufferSize);
		for(i=0;i<(EncodedBytes/20);i++){
//			buf_mov_writePtr(&cbuf);
//			sem_post(&FullSem);
		}


		if(len<0) printf("an error occured during encoding\n");

	}

	Stop_Recording(st);
//	opus_encoder_destroy(enc);
	return NULL;
}
//--------------------------------------------------------------------------------------------------------------------
