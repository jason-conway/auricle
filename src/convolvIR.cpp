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

// #pragma GCC optimize ("O1") // Out-performs Ofast

/**
 * @brief Construct a new ConvolvIR::ConvolvIR object
 * 
 */
ConvolvIR::ConvolvIR(void) : AudioStream(2, inputQueueArray)
{
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
		for (size_t j = 0; j < partitionCount; j++)
		{
			arm_fill_f32(0.0f, impulsePartitionBuffer, 512);

			for (size_t k = 0; k < partitionSize; k++)
			{				
				impulsePartitionBuffer[2 * k + 256] = irTable[2 * ImpulseSamples * irIndex + ImpulseSamples * i + 128 * j + k];
			}

			arm_cfft_f32(&arm_cfft_sR_f32_len256, impulsePartitionBuffer, ForwardFFT, 1);
			arm_copy_f32(impulsePartitionBuffer, !(i) ? hrtf.leftTF[j] : hrtf.rightTF[j], 512);
		}
	}
	audioPassthrough = false;
}

/**
 * @brief 
 * 
 */
void ConvolvIR::convolve(void)
{
	arm_cfft_f32(&arm_cfft_sR_f32_len256, overlappedAudio, ForwardFFT, 1);
	arm_copy_f32(overlappedAudio, frequencyDelayLine[partitionIndex], 512);
	for (size_t i = 0; i < 2; i++) // Two iterations - left and right
	{
		arm_fill_f32(0.0f, multAccum, 512); // Clear out previous data in accumulator

		int16_t shiftIndex = partitionIndex; // Set new starting point for sliding convolutionPartitions over the FFT of the impulse response
		
		for (size_t j = 0; j < partitionCount; j++)
		{
			arm_cmplx_mult_cmplx_f32(frequencyDelayLine[shiftIndex], !(i) ? hrtf.leftTF[j] : hrtf.rightTF[j], cmplxProduct, 256);
			
			arm_add_f32(multAccum, cmplxProduct, multAccum, 512);

			// Decrease counter by 1 until we've reached zero, then restart
			shiftIndex = ((shiftIndex - 1) < 0) ? (partitionCount - 1) : (shiftIndex - 1);
		}

		arm_cfft_f32(&arm_cfft_sR_f32_len256, multAccum, InverseFFT, 1);

		// Move the resultant convolution product into left and right audio buffers
		for (size_t j = 0; j < partitionSize; j++)
		{
			if (!(i))
			{
				leftAudioData[j] = multAccum[2 * j];
			}
			else
			{
				rightAudioData[j] = multAccum[2 * j + 1];
			}
		}
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

	memset(hrtf.leftTF, 0, sizeof(hrtf.leftTF));
	memset(hrtf.rightTF, 0, sizeof(hrtf.rightTF));

	memset(multAccum, 0, sizeof(multAccum));
	memset(cmplxProduct, 0, sizeof(cmplxProduct));

	memset(overlappedAudio, 0, sizeof(overlappedAudio));

	memset(leftAudioData, 0, sizeof(leftAudioData));
	memset(leftAudioPrevSample, 0, sizeof(leftAudioPrevSample));
	memset(rightAudioData, 0, sizeof(rightAudioData));
	memset(rightAudioPrevSample, 0, sizeof(rightAudioPrevSample));
	
	memset(inputQueueArray, 0, sizeof(inputQueueArray));
}

/**
 * @brief Updates every 128 samples / 2.9 ms
 * 
 */
void ConvolvIR::update(void)
{
	audio_block_t *leftAudio = receiveWritable(leftChannel);
	audio_block_t *rightAudio = receiveWritable(rightChannel);

	if (leftAudio && rightAudio) // Data available on both the left and right channels
	{
		if (audioPassthrough) // Not messing with the data, just sending it through the pipe
		{
			transmit(leftAudio, leftChannel);
			transmit(rightAudio, rightChannel);
			release(leftAudio);
			release(rightAudio);
			return;
		}

		// Disable interrupts while computing the convolution
		__disable_irq();

		// Use float32 for higher precision intermediate calculations
		arm_q15_to_float(leftAudio->data, leftAudioData, 128);
		arm_q15_to_float(rightAudio->data, rightAudioData, 128);

		for (size_t i = 0; i < partitionSize; i++)
		{
			// Fill the first half of audioConvolutionBuffer with audio data from the previous sample
			overlappedAudio[2 * i] = leftAudioPrevSample[i];		 // [0] [2] [4] ... [254]
			overlappedAudio[2 * i + 1] = rightAudioPrevSample[i]; // [1] [3] [5] ... [255]

			// Fill the last half of audioConvolutionBuffer with the current audio data
			overlappedAudio[2 * i + 256] = leftAudioData[i];	 // [256] [258] [260] ... [510]
			overlappedAudio[2 * i + 257] = rightAudioData[i]; // [257] [259] [261] ... [511]
			
			leftAudioPrevSample[i] = leftAudioData[i];
			rightAudioPrevSample[i] = rightAudioData[i];
		}

		convolve();

		// Increase counter by 1 until we've reached the number of partitions, then reset counter to 0
		partitionIndex = ((partitionIndex + 1) >= partitionCount) ? 0 : (partitionIndex + 1);
		
		arm_float_to_q15(leftAudioData, leftAudio->data, 128);
		arm_float_to_q15(rightAudioData, rightAudio->data, 128);

		// Transmit left and right audio to the output
		transmit(leftAudio, leftChannel);
		transmit(rightAudio, rightChannel);

		// Re-enable interrupts
		__enable_irq();

		release(leftAudio);
		release(rightAudio);
	}
}
