/**
 * @file rtupcr.h
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief Real-Time Uniformly-Partitioned Convolution Reverb
 * @version 0.1
 * @date 2021-05-04
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#ifndef RTUPCR_H
#define RTUPCR_H

#include <Audio.h>
#include <arm_math.h>
#include <arm_const_structs.h>

#define partitionSize 128
#define partitionCount 140
#define audioBlockSize 128

// IFFT Flags
#define forwardTransform 0
#define inverseTransform 1

// Return Values
#define PARTITION_SUCCESS 0
#define PARTITION_FAILURE 1
#define INIT_SUCCESS 0

class RTUPCR : public AudioStream
{
public:
	RTUPCR(void) : AudioStream(2, inputQueueArray)
	{
		// Constructor
	}

	int8_t begin(float32_t *impulseResponse);
	virtual void update(void);

	enum Channels
	{
		STEREO_LEFT,
		STEREO_RIGHT
	};

private:
	audio_block_t *inputQueueArray[2];
	int16_t partitionIndex = 0;
	int16_t reversedPartitionIndex = 0;

	bool audioReady = false;

	bool partitionImpulseResponse(float32_t *impulseResponse);

	float32_t impulseResponseFFT[partitionCount][512]; // Partitioned impulse response sub-filters
	float32_t audioConvolutionBuffer[512];			   // Convolution buffer

	// These keep track of which partition we're on
	float32_t *convolutionPartition; 
	float32_t *impulsePartition;

	float32_t leftAudioData[audioBlockSize];	   // Left channel audio data as floating point vector
	float32_t leftAudioPrevSample[audioBlockSize]; // Left channel N-1

	float32_t rightAudioData[audioBlockSize];		// Right channel audio data as floating point vector
	float32_t rightAudioPrevSample[audioBlockSize]; // Right channel N-1

	float32_t multAccum[512];	 // Multiply-and-accumulate (MAC) buffer
	float32_t cmplxProduct[512]; // Complex-by-complex multiplication buffer
};
#endif