/**
 * @file auricle.cpp
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief Real-Time Uniformly-Partitioned Convolution 
 * @version 0.1
 * @date 2021-05-04
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#include "auricle.h"

float32_t __attribute__((section(".dmabuffers"), used)) audioConvolutionBuffer[512];

float32_t __attribute__((section(".dmabuffers"), used)) leftAudioData[128];		   // Left channel audio data as floating point vector
float32_t __attribute__((section(".dmabuffers"), used)) leftAudioPrevSample[128];  // Left channel N-1
float32_t __attribute__((section(".dmabuffers"), used)) rightAudioData[128];	   // Right channel audio data as floating point vector
float32_t __attribute__((section(".dmabuffers"), used)) rightAudioPrevSample[128]; // Right channel N-1

/**
 * @brief 
 * 
 * @param hrir 
 * @return int8_t 
 */
int8_t Auricle::begin(const HRIR *hrir)
{
	for (size_t i = 0; i < PARTITION_COUNT; i++)
	{
		arm_fill_f32(0.0f, convolutionPartitions[i], 512);
		arm_fill_f32(0.0f, hrtf.leftTF[i], 512);
		arm_fill_f32(0.0f, hrtf.rightTF[i], 512);
	}

	arm_fill_f32(0.0f, multAccum, 512);

	if (convertIR(hrir))
	{
		Serial.printf(((const __FlashStringHelper *)("Error Computing HRTFs\n")));
	}

	audioReady = true;

	Serial.printf(((const __FlashStringHelper *)("HRTFs Computed\n")));

	return 0;
}

/**
 * @brief Partition the original impulse response into sub-filters in order to perform convolution in real time.
 * The linearity of the FFT allows previously computed FFTs to be reused in the suceeding filters- decreasing the 
 * number of forward FFTs that need to be computed. Since multiple convolutions are summed together, the overall 
 * number of inverse FFTs is also cut down.
 * 
 * @param hrir 
 * @return int8_t 
 */
int8_t Auricle::convertIR(const HRIR *hrir)
{
	for (size_t i = 0; i < 2; i++)
	{
		float32_t impulsePartitionBuffer[512];

		for (size_t j = 0; j < PARTITION_COUNT; j++)
		{
			arm_fill_f32(0.0f, impulsePartitionBuffer, 512);

			for (size_t k = 0; k < PARTITION_SIZE; k++)
			{
				if (!(i))
				{
					impulsePartitionBuffer[2 * k + 256] = hrir->leftIR[128 * j + k];
				}
				else
				{
					impulsePartitionBuffer[2 * k + 256] = hrir->rightIR[128 * j + k];
				}
			}

			arm_cfft_f32(&arm_cfft_sR_f32_len256, impulsePartitionBuffer, FORWARD, 1);

			if (!(i))
			{
				arm_copy_f32(impulsePartitionBuffer, hrtf.leftTF[j], 512);
			}
			else
			{
				arm_copy_f32(impulsePartitionBuffer, hrtf.rightTF[j], 512);
			}
		}
	}

	return 0;
}

/**
 * @brief 
 * 
 * @return int8_t 
 */
int8_t Auricle::convolve(void)
{	
	for (size_t i = 0; i < 2; i++) // Two iterations - left and right
	{
		arm_fill_f32(0.0f, multAccum, 512); // Clear out previous data in accumulator

		int16_t shiftIndex = partitionIndex; // Set new starting point for sliding convolutionPartitions over the FFT of the impulse response

		if (!(i))
		{
			multiplyAccumulate(hrtf.leftTF, shiftIndex);
		}
		else
		{
			multiplyAccumulate(hrtf.rightTF, shiftIndex);
		}

		arm_cfft_f32(&arm_cfft_sR_f32_len256, multAccum, INVERSE, 1);

		// Move the resultant convolution product into left and right audio buffers
		for (size_t j = 0; j < PARTITION_SIZE; j++)
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

	return 0;
}

/**
 * @brief Complex-by-complex multiplication of HRTF and audio input samples
 * 
 * @param hrtf 
 * @param shiftIndex 
 * @return int8_t 
 */
int8_t Auricle::multiplyAccumulate(float32_t (*hrtf)[512], int16_t shiftIndex)
{
	for (size_t i = 0; i < PARTITION_COUNT; i++)
	{
		arm_cmplx_mult_cmplx_f32(convolutionPartitions[shiftIndex], hrtf[i], cmplxProduct, 256);

		arm_add_f32(multAccum, cmplxProduct, multAccum, 512);

		// Decrease counter by 1 until we've reached zero, then restart
		shiftIndex = ((shiftIndex - 1) < 0) ? (PARTITION_COUNT - 1) : (shiftIndex - 1);
	}
	return 0;
}

/**
 * @brief Updates every 128 samples / 2.9 ms
 * 
 */
void Auricle::update(void)
{
	if (!(audioReady)) // Impulse response hasn't been processed yet
	{
		return;
	}

	audio_block_t *leftAudio = receiveWritable(STEREO_LEFT);
	audio_block_t *rightAudio = receiveWritable(STEREO_RIGHT);

	//__disable_irq();

	if (leftAudio && rightAudio) // Data available on both the left and right channels
	{
		// Use float32 for higher precision intermediate calculations
		arm_q15_to_float(leftAudio->data, leftAudioData, 128);
		arm_q15_to_float(rightAudio->data, rightAudioData, 128);

		for (size_t i = 0; i < PARTITION_SIZE; i++)
		{
			// Fill the first half of audioConvolutionBuffer with audio data from the previous sample
			audioConvolutionBuffer[2 * i] = leftAudioPrevSample[i];		 // [0] [2] [4] ... [254]
			audioConvolutionBuffer[2 * i + 1] = rightAudioPrevSample[i]; // [1] [3] [5] ... [255]

			// Fill the last half of audioConvolutionBuffer with the current audio data
			audioConvolutionBuffer[2 * i + 256] = leftAudioData[i];	 // [256] [258] [260] ... [510]
			audioConvolutionBuffer[2 * i + 257] = rightAudioData[i]; // [257] [259] [261] ... [511]

			// Copy the current audio data into buffers for next sample overlap-and-save
			leftAudioPrevSample[i] = leftAudioData[i];
			rightAudioPrevSample[i] = rightAudioData[i];
		}

		arm_cfft_f32(&arm_cfft_sR_f32_len256, audioConvolutionBuffer, FORWARD, 1);

		arm_copy_f32(audioConvolutionBuffer, convolutionPartitions[partitionIndex], 512);
		
		convolve();

		// Increase counter by 1 until we've reached the number of partitions, then reset counter to 0
		partitionIndex = ((partitionIndex + 1) >= PARTITION_COUNT) ? 0 : (partitionIndex + 1);

		arm_float_to_q15(leftAudioData, leftAudio->data, 128);
		arm_float_to_q15(rightAudioData, rightAudio->data, 128);

		// Transmit left and right audio to the output
		transmit(leftAudio, STEREO_LEFT);
		transmit(rightAudio, STEREO_RIGHT);

		// Adios

		//__enable_irq();

		release(leftAudio);
		release(rightAudio);
	}
}
