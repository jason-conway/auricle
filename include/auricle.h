/**
 * @file auricle.h
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief Uniformly-Partitioned Convolution 
 * @version 0.1
 * @date 2021-05-04
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#ifndef AURICLE_H
#define AURICLE_H

#include <Audio.h>
#include <arm_math.h>
#include <arm_const_structs.h>

#define PARTITION_SIZE 128
#define PARTITION_COUNT 64

// IFFT Flags
#define FORWARD 0
#define INVERSE 1

// Return Values
#define HRTF_COMPUTATION_SUCCESS 0
#define HRTF_COMPUTATION_FAILURE -1

typedef struct {
	float32_t *leftIR;
	float32_t *rightIR;
} HRIR;

typedef struct {
	float32_t leftTF[PARTITION_COUNT][512];
	float32_t rightTF[PARTITION_COUNT][512];
} HRTF;

class Auricle : public AudioStream
{
public:
	Auricle(void) : AudioStream(2, inputQueueArray)
	{
		// Constructor
	}

	int8_t begin(const HRIR *hrir);

	virtual void update(void);

	enum Channels
	{
		STEREO_LEFT,
		STEREO_RIGHT
	};

private:
	audio_block_t *inputQueueArray[2];
	HRTF hrtf;

	int8_t convertHRIR(const HRIR *hrir);
	int8_t convolve(void);
	int8_t cmplxMultCmplx(float32_t *accumulator, float32_t (*impulseResponseFFT)[512], int16_t shiftIndex);

	bool audioReady = false;
	uint16_t partitionIndex = 0;

	float32_t convolutionPartitions[PARTITION_COUNT][512];
};
#endif