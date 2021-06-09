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

#define PARTITION_SIZE 128
#define PARTITION_COUNT 64
#define STREAM_BLOCK_SIZE 128

// IFFT Flags
#define FORWARD 0
#define INVERSE 1

// Return Values
typedef enum
{
	RTUPCR_SUCCESS = 0,	
	PARTITION_FAILURE = -1,	 
	CMPLX_MULT_FAILURE = -2,
	CONVOLVE_FAILURE = -3
} RTUPCR_STATUS;


class RTUPCR : public AudioStream
{
public:
	RTUPCR(void) : AudioStream(2, inputQueueArray)
	{
		// Constructor
	}

	RTUPCR_STATUS begin(float32_t *leftImpulseResponse, float32_t *rightImpulseResponse);
	virtual void update(void);

	enum Channels
	{
		STEREO_LEFT,
		STEREO_RIGHT
	};

private:
	audio_block_t *inputQueueArray[2];

	RTUPCR_STATUS partitionImpulseResponses(float32_t *leftImpulseResponse, float32_t (*leftImpulseResponseFFT)[512], float32_t *rightImpulseResponse, float32_t (*rightImpulseResponseFFT)[512]);
	RTUPCR_STATUS convolve(float32_t (*leftImpulseResponseFFT)[512], float32_t (*rightImpulseResponseFFT)[512]);
	RTUPCR_STATUS cmplxMultCmplx(float32_t *accumulator, float32_t (*impulseResponseFFT)[512], int16_t shiftIndex);

	bool audioReady = false;
	uint16_t partitionIndex = 0;

	float32_t leftImpulseResponseFFT[PARTITION_COUNT][512];
	float32_t rightImpulseResponseFFT[PARTITION_COUNT][512];

	float32_t convolutionPartitions[PARTITION_COUNT][512];
};
#endif