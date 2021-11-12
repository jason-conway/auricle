/**
 * @file convolvIR.cpp
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief Spatial Audio for the Arm Cortex-M7
 * @version 0.1
 * @date 2021-05-04
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#include "convolvIR.h"
#include "tablIR.h"

// #pragma GCC optimize ("O1")

/**
 * @brief Construct a new ConvolvIR::ConvolvIR object
 * 
 */
ConvolvIR::ConvolvIR(void) : AudioStream(2, inputQueueArray)
{
	_section_dma static audio_block_t allocatedAudioMemory[16];
	initialize_memory(allocatedAudioMemory, 16);
	audioPassthrough = true;
}

/**
 * @brief Partition the original impulse response into sub-filters in order to perform convolution in real time.
 * The linearity of the FFT allows previously computed FFTs to be reused in the suceeding filters- decreasing the 
 * number of forward FFTs that need to be computed. Since multiple convolutions are summed together, the overall 
 * number of inverse FFTs is also cut down.
 * 
 * @param irIndex 
 */
void ConvolvIR::convertIR(uint8_t irIndex)
{
	audioMute = true;
	memset(&hrtf, 0, sizeof(hrtf));

	for (size_t i = 0; i < 2; i++)
	{
		float32_t impulsePartitionBuffer[512];
		for (size_t j = 0; j < PartitionCount; j++)
		{
			arm_fill_f32(0.0f, impulsePartitionBuffer, 512);

			for (size_t k = 0; k < PartitionSize; k++)
			{
				// impulsePartitionBuffer[2 * k + 256] = irTable[2 * ImpulseSamples * irIndex + ImpulseSamples * i + 128 * j + k];
				impulsePartitionBuffer[2 * k + 256] = irTable[ImpulseSamples * i + 128 * j + k];
			}

			arm_cfft_f32(&arm_cfft_sR_f32_len256, impulsePartitionBuffer, ForwardFFT, 1);
			arm_copy_f32(impulsePartitionBuffer, i ? &hrtf.retf[512 * j] : &hrtf.letf[512 * j], 512);
		}
	}
	audioMute = false;
}

bool ConvolvIR::togglePassthrough(void)
{
	audioPassthrough = !audioPassthrough;
	return audioPassthrough;
}

/**
 * @brief Updates every 128 samples / 2.9 ms
 * 
 */
void ConvolvIR::update(void)
{
	if (audioMute)
	{
		return;
	}

	audio_block_t *leftAudio = receiveWritable(LeftChannel);
	audio_block_t *rightAudio = receiveWritable(RightChannel);

	if (leftAudio && rightAudio) // Data available on both the left and right channels
	{
		if (audioPassthrough) // Not messing with the data, just sending it through the pipe
		{
			transmit(leftAudio, LeftChannel);
			transmit(rightAudio, RightChannel);
			release(leftAudio);
			release(rightAudio);
			return;
		}
		else
		{
			float32_t leftAudioData[128];
			float32_t rightAudioData[128];

			arm_q15_to_float(leftAudio->data, leftAudioData, 128);
			arm_q15_to_float(rightAudio->data, rightAudioData, 128);

			upols(leftAudioData, rightAudioData, &hrtf);

			arm_float_to_q15(leftAudioData, leftAudio->data, 128);
    		arm_float_to_q15(rightAudioData, rightAudio->data, 128);

			// Transmit left and right audio to the output
			transmit(leftAudio, LeftChannel);
			transmit(rightAudio, RightChannel);

			release(leftAudio);
			release(rightAudio);
		}
	}
}
