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

#define	partitionSize 128
#define	partitionCount 140
#define	audioBlockSize 128

class RTUPCR : public AudioStream
{
public:
	RTUPCR(void) : AudioStream(2, inputQueueArray)
	{
		//
	}

	bool begin(float32_t *impulseResponse);
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
	
	enum FFT_Direction
	{
		forwardTransform,
		inverseTransform
	};

	float32_t impulseResponseFFT[partitionCount][512];
	float32_t audioInFFT[512];

	float32_t *convolutionPartition;
	float32_t *impulsePartition;

};
#endif