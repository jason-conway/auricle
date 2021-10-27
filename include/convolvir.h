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

#pragma once

#include <Audio.h>
#include "auricle.h"
#include <arm_math.h>
#include <arm_const_structs.h>

class ConvolvIR : public AudioStream
{
public:
	ConvolvIR(void);
	virtual void update(void);
	bool togglePassthrough(void);
	void convertIR(uint8_t irIndex);

private:
	audio_block_t *inputQueueArray[2];

	enum Lengths
	{
		ImpulseSamples = 8192,
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
	void clearAllArrays(void);

	bool audioPassthrough;

	uint16_t partitionIndex;
	float32_t frequencyDelayLine[partitionCount][512];
	float32_t overlappedAudio[512];
	float32_t multAccum[512];
	float32_t cmplxProduct[512];

	float32_t leftAudioData[128];		 // Left channel audio data as floating point vector
	float32_t leftAudioPrevSample[128];	 // Left channel N-1
	float32_t rightAudioData[128];		 // Right channel audio data as floating point vector
	float32_t rightAudioPrevSample[128]; // Right channel N-1
};

extern ConvolvIR convolvIR;
