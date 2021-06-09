/**
 * @file rtupcr.cpp
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief Real-Time Uniformly-Partitioned Convolution Reverb
 * @version 0.1
 * @date 2021-05-04
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#include "rtupcr.h"

float32_t __attribute__((section(".dmabuffers"), used)) audioConvolutionBuffer[512];

float32_t __attribute__((section(".dmabuffers"), used)) leftAudioData[STREAM_BLOCK_SIZE];		 // Left channel audio data as floating point vector
float32_t __attribute__((section(".dmabuffers"), used)) leftAudioPrevSample[STREAM_BLOCK_SIZE];	 // Left channel N-1
float32_t __attribute__((section(".dmabuffers"), used)) rightAudioData[STREAM_BLOCK_SIZE];		 // Right channel audio data as floating point vector
float32_t __attribute__((section(".dmabuffers"), used)) rightAudioPrevSample[STREAM_BLOCK_SIZE]; // Right channel N-1

/**
 * @brief 
 * 
 * @param leftImpulseResponse 
 * @param rightImpulseResponse 
 * @return int8_t 
 */
RTUPCR_STATUS RTUPCR::begin(float32_t *leftImpulseResponse, float32_t *rightImpulseResponse)
{
	for (size_t i = 0; i < PARTITION_COUNT; i++)
	{
		arm_fill_f32(0.0f, convolutionPartitions[i], 512);
		arm_fill_f32(0.0f, leftImpulseResponseFFT[i], 512);
		arm_fill_f32(0.0f, rightImpulseResponseFFT[i], 512);
	}

	partitionImpulseResponses(leftImpulseResponse, leftImpulseResponseFFT, rightImpulseResponse, rightImpulseResponseFFT);

	audioReady = true;

	Serial.printf(((const __FlashStringHelper *)("Impulse Response FFTs Completed\n")));

	return RTUPCR_SUCCESS;
}

/**
 * @brief Partition the original impulse response into sub-filters in order to perform convolution in real time.
 * The linearity of the FFT allows previously computed FFTs to be reused in the suceeding filters- decreasing the 
 * number of forward FFTs that need to be computed. Since multiple convolutions are summed together, the overall 
 * number of inverse FFTs is also cut down.
 * 
 * @param leftImpulseResponse 
 * @param leftImpulseResponseFFT 
 * @param rightImpulseResponse 
 * @param rightImpulseResponseFFT 
 * @return RTUPCR_STATUS 
 */
RTUPCR_STATUS RTUPCR::partitionImpulseResponses(float32_t *leftImpulseResponse, float32_t (*leftImpulseResponseFFT)[512], float32_t *rightImpulseResponse, float32_t (*rightImpulseResponseFFT)[512])
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
					impulsePartitionBuffer[2 * k + 256] = leftImpulseResponse[128 * j + k];
				}
				else
				{
					impulsePartitionBuffer[2 * k + 256] = rightImpulseResponse[128 * j + k];
				}
			}

			arm_cfft_f32(&arm_cfft_sR_f32_len256, impulsePartitionBuffer, FORWARD, 1);

			for (size_t k = 0; k < 512; k++)
			{
				if (!(i))
				{
					leftImpulseResponseFFT[j][k] = impulsePartitionBuffer[k];
				}
				else
				{
					rightImpulseResponseFFT[j][k] = impulsePartitionBuffer[k];
				}
			}
		}
	}

	return RTUPCR_SUCCESS;
}

/**
 * @brief 
 * 
 * @param leftImpulseResponseFFT 
 * @param rightImpulseResponseFFT 
 * @return RTUPCR_STATUS 
 */
RTUPCR_STATUS RTUPCR::convolve(float32_t (*leftImpulseResponseFFT)[512], float32_t (*rightImpulseResponseFFT)[512])
{
	float32_t multAccum[512];

	for (size_t i = 0; i < 2; i++)
	{
		arm_fill_f32(0.0f, multAccum, 512); // Clear out previous data in accumulator

		int16_t shiftIndex = partitionIndex; // Set new starting point

		if (!(i))
		{
			cmplxMultCmplx(multAccum, leftImpulseResponseFFT, shiftIndex);
		}
		else
		{
			cmplxMultCmplx(multAccum, rightImpulseResponseFFT, shiftIndex);
		}

		arm_cfft_f32(&arm_cfft_sR_f32_len256, multAccum, INVERSE, 1);

		// Move the resultant convolution product into left and right audio buffers
		for (size_t j = 0; j < PARTITION_SIZE; j++)
		{
			if (!(i))
			{
				leftAudioData[j] = multAccum[j * 2] * 0.03;
			}
			else
			{
				rightAudioData[j] = multAccum[j * 2 + 1] * 0.03;
			}
		}
	}

	return RTUPCR_SUCCESS;
}

/**
 * @brief Complex-by-complex multiplication of impulse response sub-filter and audio input samples
 * 
 * @param accumulator 
 * @param impulseResponseFFT 
 * @param shiftIndex 
 * @return RTUPCR_STATUS 
 */
RTUPCR_STATUS RTUPCR::cmplxMultCmplx(float32_t *accumulator, float32_t (*impulseResponseFFT)[512], int16_t shiftIndex)
{
	for (size_t i = 0; i < PARTITION_COUNT; i++)
	{
		for (size_t j = 0; j < 256; j = j + 4)
		{
			accumulator[2 * j + 0x0] += impulseResponseFFT[i][2 * j + 0x0] * convolutionPartitions[shiftIndex][2 * j + 0x0] - impulseResponseFFT[i][2 * j + 0x1] * convolutionPartitions[shiftIndex][2 * j + 0x1];
			accumulator[2 * j + 0x1] += impulseResponseFFT[i][2 * j + 0x1] * convolutionPartitions[shiftIndex][2 * j + 0x0] + impulseResponseFFT[i][2 * j + 0x0] * convolutionPartitions[shiftIndex][2 * j + 0x1];
			accumulator[2 * j + 0x2] += impulseResponseFFT[i][2 * j + 0x2] * convolutionPartitions[shiftIndex][2 * j + 0x2] - impulseResponseFFT[i][2 * j + 0x3] * convolutionPartitions[shiftIndex][2 * j + 0x3];
			accumulator[2 * j + 0x3] += impulseResponseFFT[i][2 * j + 0x3] * convolutionPartitions[shiftIndex][2 * j + 0x2] + impulseResponseFFT[i][2 * j + 0x2] * convolutionPartitions[shiftIndex][2 * j + 0x3];
			accumulator[2 * j + 0x4] += impulseResponseFFT[i][2 * j + 0x4] * convolutionPartitions[shiftIndex][2 * j + 0x4] - impulseResponseFFT[i][2 * j + 0x5] * convolutionPartitions[shiftIndex][2 * j + 0x5];
			accumulator[2 * j + 0x5] += impulseResponseFFT[i][2 * j + 0x5] * convolutionPartitions[shiftIndex][2 * j + 0x4] + impulseResponseFFT[i][2 * j + 0x4] * convolutionPartitions[shiftIndex][2 * j + 0x5];
			accumulator[2 * j + 0x6] += impulseResponseFFT[i][2 * j + 0x6] * convolutionPartitions[shiftIndex][2 * j + 0x6] - impulseResponseFFT[i][2 * j + 0x7] * convolutionPartitions[shiftIndex][2 * j + 0x7];
			accumulator[2 * j + 0x7] += impulseResponseFFT[i][2 * j + 0x7] * convolutionPartitions[shiftIndex][2 * j + 0x6] + impulseResponseFFT[i][2 * j + 0x6] * convolutionPartitions[shiftIndex][2 * j + 0x7];
		}

		// Decrease counter by 1 until we've reached zero, then restart
		shiftIndex = ((shiftIndex - 1) < 0) ? (PARTITION_COUNT - 1) : (shiftIndex - 1);
	}
	return RTUPCR_SUCCESS;
}

/**
 * @brief Updates every 128 samples / 2.9 ms
 * 
 */
void RTUPCR::update(void)
{
	if (!(audioReady)) // Impulse response hasn't been processed yet
	{
		return;
	}

	audio_block_t *leftAudio = receiveWritable(STEREO_LEFT);
	audio_block_t *rightAudio = receiveWritable(STEREO_RIGHT);

	if (leftAudio && rightAudio) // Data available on both the left and right channels
	{
		// Use float32 for higher precision intermediate calculations
		arm_q15_to_float(leftAudio->data, leftAudioData, STREAM_BLOCK_SIZE);
		arm_q15_to_float(rightAudio->data, rightAudioData, STREAM_BLOCK_SIZE);

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

		for (size_t i = 0; i < 512; i++)
		{
			convolutionPartitions[partitionIndex][i] = audioConvolutionBuffer[i];
		}

		convolve(leftImpulseResponseFFT, rightImpulseResponseFFT);

		// Increase counter by 1 until we've reached the number of partitions, then reset counter to 0
		partitionIndex = ((partitionIndex + 1) >= PARTITION_COUNT) ? 0 : (partitionIndex + 1);

		arm_float_to_q15(leftAudioData, leftAudio->data, STREAM_BLOCK_SIZE);
		arm_float_to_q15(rightAudioData, rightAudio->data, STREAM_BLOCK_SIZE);

		// Transmit left and right audio to the output
		transmit(leftAudio, STEREO_LEFT);
		transmit(rightAudio, STEREO_RIGHT);

		// Adios
		release(leftAudio);
		release(rightAudio);
	}
}
