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
	init();
}

/**
 * @brief 
 * 
 */
void ConvolvIR::init(void)
{
	partitionIndex = 0;
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
	partitionIndex = 0;
}

/**
 * @brief 
 * 
 */
void ConvolvIR::convolve(channel_t channel, float32_t *hrtf, float32_t *channelOutput)
{
	static float32_t cmplxAccum[512];
	static float32_t cmplxProduct[512];

	arm_fill_f32(0.0f, cmplxAccum, 512); // Reset

	int16_t shiftIndex = partitionIndex; // Set new starting point for sliding convolutionPartitions over the FFT of the impulse response

	for (size_t i = 0; i < PartitionCount; i++)
	{
		arm_cmplx_mult_cmplx_f32(&frequencyDelayLine[512 * shiftIndex], &hrtf[512 * i], cmplxProduct, 256);
		arm_add_f32(cmplxAccum, cmplxProduct, cmplxAccum, 512);

		// Decrement with wraparound
		shiftIndex = (shiftIndex + (PartitionCount - 1)) % PartitionCount;
	}

	arm_cfft_f32(&arm_cfft_sR_f32_len256, cmplxAccum, InverseFFT, 1);

	for (size_t i = 0; i < PartitionSize; i++)
	{
		channelOutput[i] = cmplxAccum[2 * i + channel];
	}
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

	audio_block_t *leftAudio = receiveWritable(LEFT);
	audio_block_t *rightAudio = receiveWritable(RIGHT);

	if (leftAudio && rightAudio) // Data available on both the left and right channels
	{
		if (audioPassthrough) // Not messing with the data, just sending it through the pipe
		{
			transmit(leftAudio, LEFT);
			transmit(rightAudio, RIGHT);
			release(leftAudio);
			release(rightAudio);
			return;
		}
		else
		{
			__disable_irq();

			static float32_t overlappedAudioData[512];
			static float32_t leftAudioData[128];
			static float32_t rightAudioData[128];

			// Use float32 for higher precision intermediate calculations
			arm_q15_to_float(leftAudio->data, leftAudioData, 128);
			arm_q15_to_float(rightAudio->data, rightAudioData, 128);

			for (size_t i = 0; i < PartitionSize; i++)
			{
				static float32_t overlapSave[256];

				// Fill the first half with the previous sample
				overlappedAudioData[2 * i] = overlapSave[2 * i];		   // [0] [2] [4] ... [254]
				overlappedAudioData[2 * i + 1] = overlapSave[2 * i + 1]; // [1] [3] [5] ... [255]

				// Fill the last half with the current sample
				overlappedAudioData[2 * i + 256] = leftAudioData[i];  // [256] [258] [260] ... [510]
				overlappedAudioData[2 * i + 257] = rightAudioData[i]; // [257] [259] [261] ... [511]

				overlapSave[2 * i] = leftAudioData[i];
				overlapSave[2 * i + 1] = rightAudioData[i];
			}

			arm_cfft_f32(&arm_cfft_sR_f32_len256, overlappedAudioData, ForwardFFT, 1);
			arm_copy_f32(overlappedAudioData, &frequencyDelayLine[partitionIndex * 512], 512);

			convolve(LEFT, hrtf.letf, leftAudioData);
			convolve(RIGHT, hrtf.retf, rightAudioData);

			// Increase counter by 1 until we've reached the number of partitions, then reset counter to 0
			partitionIndex = (partitionIndex + 1) % PartitionCount;

			arm_float_to_q15(leftAudioData, leftAudio->data, 128);
			arm_float_to_q15(rightAudioData, rightAudio->data, 128);

			// Transmit left and right audio to the output
			transmit(leftAudio, LEFT);
			transmit(rightAudio, RIGHT);

			// Re-enable interrupts
			__enable_irq();

			release(leftAudio);
			release(rightAudio);
		}
	}
}
