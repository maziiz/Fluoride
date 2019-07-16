#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <portaudio.h>
//#include <portaudiocpp/PortAudioCpp.hxx>

PaStream* Start_Recording(PaStream* stream,int FRAMES_PER_BUFFER,int Fs,int channels,int bit_depth);
PaStream* Start_Playing(PaStream* stream,int FRAMES_PER_BUFFER,int Fs,int channels,int bit_depth);
void Stop_Recording(PaStream* stream);
// FRAMES_PER_BUFFER is number of samples ber buffer (i.e. 80 for a 160 byte buffer)
//---------------------------------------------------------------------------------------------------------

PaStream* Start_Recording(PaStream* stream,int FRAMES_PER_BUFFER,int Fs,int channels,int bit_depth){

    const PaDeviceInfo* inputInfo;
	PaStreamParameters inputParameters;
	PaError err;
	printf("ok1\n\n");

	err = Pa_Initialize();
	if(err!=paNoError) LOG(FATAL)<<"CANNOT RECORD";



	inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
//	printf( "Input device # %d.\n", inputParameters.device );
	LOG(INFO) << "Input device #" << (int)inputParameters.device;
	inputInfo = Pa_GetDeviceInfo( inputParameters.device );
	printf( "    Name: %s\n", inputInfo->name );
	printf( "      LL: %g s\n", inputInfo->defaultLowInputLatency );
	printf( "      HL: %g s\n", inputInfo->defaultHighInputLatency );
	//       numChannels = inputInfo->maxInputChannels < outputInfo->maxOutputChannels
	//               ? inputInfo->maxInputChannels : outputInfo->maxOutputChannels;
	//       printf( "Num channels = %d.\n", numChannels );

    inputParameters.channelCount = channels;
    if(bit_depth==8){inputParameters.sampleFormat=paInt8;}
    else if(bit_depth==16){inputParameters.sampleFormat=paInt16;}
    else if(bit_depth==24){inputParameters.sampleFormat=paInt24;}

    inputParameters.suggestedLatency = inputInfo->defaultHighInputLatency ;
    inputParameters.hostApiSpecificStreamInfo = NULL;
       /* -- setup -- */
    err = Pa_OpenStream(
                    &stream,
                    &inputParameters,
                    NULL,
                    Fs,
                    FRAMES_PER_BUFFER,
                    paClipOff,      /* we won't output out of range samples so don't bother clipping them */
                    NULL, /* no callback, use blocking API */
                    NULL ); /* no callback, so no callback userData */
    err = Pa_StartStream( stream );
    return stream;
}


//---------------------------------------------------------------------------------------------------------

PaStream* Start_Playing(PaStream* stream,int FRAMES_PER_BUFFER,int Fs,int channels,int bit_depth){

    const PaDeviceInfo* outputInfo;
	PaStreamParameters outputParameters;
	PaError err;
	err = Pa_Initialize();
	if(err!=paNoError) printf("errro\n\n");
	outputParameters.device = Pa_GetDefaultOutputDevice(); /* default Output device */
	printf( "Output device # %d.\n", outputParameters.device );
	outputInfo = Pa_GetDeviceInfo( outputParameters.device );
	printf( "    Name: %s\n", outputInfo->name );
	printf( "      LL: %g s\n", outputInfo->defaultLowOutputLatency );
	printf( "      HL: %g s\n", outputInfo->defaultHighOutputLatency );
	//       numChannels = inputInfo->maxInputChannels < outputInfo->maxOutputChannels
	//               ? inputInfo->maxInputChannels : outputInfo->maxOutputChannels;
	//       printf( "Num channels = %d.\n", numChannels );

    outputParameters.channelCount = channels;
    if(bit_depth==8){outputParameters.sampleFormat=paInt8;}
    else if(bit_depth==16){outputParameters.sampleFormat=paInt16;}
    else if(bit_depth==24){outputParameters.sampleFormat=paInt24;}

    outputParameters.suggestedLatency = outputInfo->defaultHighOutputLatency ;
    outputParameters.hostApiSpecificStreamInfo = NULL;
       /* -- setup -- */
    err = Pa_OpenStream(
                    &stream,
                    NULL,
                    &outputParameters,
                    Fs,
                    FRAMES_PER_BUFFER,
                    paClipOff,      /* we won't output out of range samples so don't bother clipping them */
                    NULL, /* no callback, use blocking API */
                    NULL ); /* no callback, so no callback userData */
    err = Pa_StartStream( stream );
    return stream;
}
//---------------------------------------------------------------------------------------------------------
void Stop_Recording(PaStream* stream){
Pa_StopStream( stream );
 Pa_Terminate();
}
//---------------------------------------------------------------------------------------------------------
