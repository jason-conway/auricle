/**
 * @file convolvIR.h
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief Spatial Audio for Arm Cortex-M7
 * @version 0.1
 * @date 2021-05-04
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#ifndef CONVOLVIR_H
#define CONVOLVIR_H

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

typedef struct HRIR
{
	float32_t *leftIR;
	float32_t *rightIR;
} HRIR;

class convolvIR : public AudioStream
{
public:
	convolvIR(void);

	int8_t convertIR(const HRIR *hrir);

	virtual void update(void);

	enum Channels
	{
		STEREO_LEFT,
		STEREO_RIGHT
	};

private:
	audio_block_t *inputQueueArray[2];

	typedef struct HRTF
	{
		float32_t leftTF[PARTITION_COUNT][512];
		float32_t rightTF[PARTITION_COUNT][512];
	} HRTF;

	HRTF hrtf;

	void init(void);
	int8_t convolve(void);
	int8_t multiplyAccumulate(float32_t (*hrtf)[512], int16_t shiftIndex);

	volatile bool audioReady = false;
	
	uint16_t partitionIndex = 0;
	float32_t convolutionPartitions[PARTITION_COUNT][512];
	float32_t audioConvolutionBuffer[512];

	float32_t leftAudioData[128];		 // Left channel audio data as floating point vector
	float32_t leftAudioPrevSample[128];	 // Left channel N-1
	float32_t rightAudioData[128];		 // Right channel audio data as floating point vector
	float32_t rightAudioPrevSample[128]; // Right channel N-1
	float32_t multAccum[512];
	float32_t cmplxProduct[512];
};

#endif
