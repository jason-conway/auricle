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
	enum Lengths
	{
		PartitionSize = 128,
		PartitionCount = 64,
		TransformSize = 2 * PartitionSize,
		ComplexValues = 2 * TransformSize,
		ImpulseSamples = PartitionSize * PartitionCount,
	};

	enum FFT_Flags
	{
		ForwardFFT,
		InverseFFT
	};

	typedef enum channel_t
	{
		LEFT,
		RIGHT
	} channel_t;

	typedef struct hrtf_t
	{
		float32_t letf[512 * PartitionCount];
		float32_t retf[512 * PartitionCount];
	} hrtf_t;

	hrtf_t hrtf;

	void init(void);
	void clearAllArrays(void);
	void convolve(channel_t channel, float32_t *hrtf, float32_t *channelOutput);

	bool audioPassthrough;
	uint16_t partitionIndex;

	audio_block_t *inputQueueArray[2];

	float32_t frequencyDelayLine[512 * PartitionCount];
	float32_t overlappedAudio[512];
	float32_t multAccum[512];
	float32_t cmplxProduct[512];
	float32_t previousAudioData[256];
	float32_t leftAudioData[128];
	float32_t rightAudioData[128];
};

extern ConvolvIR convolvIR;
