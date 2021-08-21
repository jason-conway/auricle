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

#ifndef ConvolvIR_H
#define ConvolvIR_H

#include <Audio.h>
#include "auricle.h"
#include <arm_math.h>
#include <arm_const_structs.h>

// Return Values
#define HRTF_COMPUTATION_SUCCESS 0
#define HRTF_COMPUTATION_FAILURE -1

typedef struct HRIR
{
	float32_t *leftIR;
	float32_t *rightIR;
} HRIR;


class ConvolvIR : public AudioStream
{
public:	
	ConvolvIR(void);
	virtual void update(void);
	void setPassthrough(bool passthrough);
	void convertIR(const HRIR *hrir);

private:
	audio_block_t *inputQueueArray[2];

	enum Partition
	{
		partitionSize = 128,
		partitionCount = 64
	};

	enum FFT_Flags
	{
		ForwardFFT,
		InverseFFT
	};

	typedef struct HRTF
	{
		float32_t leftTF[partitionCount][512];
		float32_t rightTF[partitionCount][512];
	} HRTF;

	HRTF hrtf;

	void init(void);
	void convolve(void);
	void multiplyAccumulate(float32_t (*hrtf)[512], int16_t shiftIndex);

	volatile bool audioReady = false;
	volatile bool audioPassthrough = false;
	
	uint16_t partitionIndex = 0;
	float32_t convolutionPartitions[partitionCount][512];
	float32_t audioConvolutionBuffer[512];

	float32_t leftAudioData[128];		 // Left channel audio data as floating point vector
	float32_t leftAudioPrevSample[128];	 // Left channel N-1
	float32_t rightAudioData[128];		 // Right channel audio data as floating point vector
	float32_t rightAudioPrevSample[128]; // Right channel N-1
	float32_t multAccum[512];
	float32_t cmplxProduct[512];
};

extern ConvolvIR convolvIR;

#endif
