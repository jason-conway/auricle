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
	AudioStream::initialize_memory(allocatedAudioMemory, 16);
	init();
}

/**
 * @brief Clear out buffers
 * 
 */
void ConvolvIR::init(void)
{
	clearAllArrays();
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
	audioPassthrough = true;
	clearAllArrays();
	partitionIndex = 0;
	
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
	audioPassthrough = false;
}

/**
 * @brief 
 * 
 */
// __attribute__ ((optimize("-O3")))
void ConvolvIR::convolve(channel_t channel, float32_t *hrtf, float32_t *channelOutput)
{
	arm_fill_f32(0.0f, multAccum, 512); // Clear out previous data in accumulator
	
	int16_t shiftIndex = partitionIndex; // Set new starting point for sliding convolutionPartitions over the FFT of the impulse response
		
	for (size_t i = 0; i < PartitionCount; i++)
	{
		arm_cmplx_mult_cmplx_f32(&frequencyDelayLine[512 * shiftIndex], &hrtf[512 * i], cmplxProduct, 256);
		arm_add_f32(multAccum, cmplxProduct, multAccum, 512);

		// Decrease counter by 1 until we've reached zero, then restart
		shiftIndex = (shiftIndex + (PartitionCount - 1)) % PartitionCount;
	}

	arm_cfft_f32(&arm_cfft_sR_f32_len256, multAccum, InverseFFT, 1);

	for (size_t i = 0; i < PartitionSize; i++)
	{
		channelOutput[i] = multAccum[2 * i + channel];
	}
}

bool ConvolvIR::togglePassthrough(void)
{
	audioPassthrough = !audioPassthrough;
	return audioPassthrough;
}

void ConvolvIR::clearAllArrays(void)
{
	memset(frequencyDelayLine, 0, sizeof(frequencyDelayLine));

	memset(hrtf.letf, 0, sizeof(hrtf.letf));
	memset(hrtf.retf, 0, sizeof(hrtf.retf));
	
	memset(cmplxProduct, 0, sizeof(cmplxProduct));

	memset(overlappedAudio, 0, sizeof(overlappedAudio));

	memset(leftAudioData, 0, sizeof(leftAudioData));
	memset(rightAudioData, 0, sizeof(rightAudioData));
	memset(previousAudioData, 0, sizeof(previousAudioData));
	memset(inputQueueArray, 0, sizeof(inputQueueArray));
}

/**
 * @brief Updates every 128 samples / 2.9 ms
 * 
 */
void ConvolvIR::update(void)
{
	audio_block_t *leftAudio = receiveWritable(LEFT);
	audio_block_t *rightAudio = receiveWritable(RIGHT);

	if (leftAudio && rightAudio) // Data available on both the left and right channels
	{
		if (audioPassthrough) // Not messing with the data, just sending it through the pipe
		{
			AudioStream::transmit(leftAudio, LEFT);
			AudioStream::transmit(rightAudio, RIGHT);
			AudioStream::release(leftAudio);
			AudioStream::release(rightAudio);
			return;
		}

		// Disable interrupts while computing the convolution
		__disable_irq();
		// Use float32 for higher precision intermediate calculations
		arm_q15_to_float(leftAudio->data, leftAudioData, 128);
		arm_q15_to_float(rightAudio->data, rightAudioData, 128);

		for (size_t i = 0; i < PartitionSize; i++)
		{
			// Fill the first half of audioConvolutionBuffer with audio data from the previous sample
			overlappedAudio[2 * i] = previousAudioData[2 * i];		 // [0] [2] [4] ... [254]
			overlappedAudio[2 * i + 1] = previousAudioData[2 * i + 1]; // [1] [3] [5] ... [255]

			// Fill the last half of audioConvolutionBuffer with the current audio data
			overlappedAudio[2 * i + 256] = leftAudioData[i];	 // [256] [258] [260] ... [510]
			overlappedAudio[2 * i + 257] = rightAudioData[i]; // [257] [259] [261] ... [511]
	
			previousAudioData[2 * i] = leftAudioData[i];
			previousAudioData[2 * i + 1] = rightAudioData[i];
		}

		arm_cfft_f32(&arm_cfft_sR_f32_len256, overlappedAudio, ForwardFFT, 1);
		arm_copy_f32(overlappedAudio, &frequencyDelayLine[partitionIndex * 512], 512);

		convolve(LEFT, hrtf.letf, leftAudioData);
		convolve(RIGHT, hrtf.retf, rightAudioData);

		// Increase counter by 1 until we've reached the number of partitions, then reset counter to 0
		partitionIndex = (partitionIndex + 1) % PartitionCount;

		arm_float_to_q15(leftAudioData, leftAudio->data, 128);
		arm_float_to_q15(rightAudioData, rightAudio->data, 128);

		// Transmit left and right audio to the output
		AudioStream::transmit(leftAudio, LEFT);
		AudioStream::transmit(rightAudio, RIGHT);

		// Re-enable interrupts
		__enable_irq();

		AudioStream::release(leftAudio);
		AudioStream::release(rightAudio);
	}
}
