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


// DMA buffer
float32_t __attribute__ ((section(".dmabuffers"), used)) convolutionPartitions[partitionCount][512]; // TODO: Initialize pointer here


/**
 * @brief Initialize RTUPCR
 * 
 * @param[in] impulseResponse Input pointer to the impulse response array
 * @return true 
 * @return false 
 */
int8_t RTUPCR::begin(float32_t *impulseResponse)
{
	if (!(partitionImpulseResponse(impulseResponse)))
	{
		Serial.printf("Error: sub-filter partitioning\n");
		return PARTITION_FAILURE;
	}

	arm_fill_f32(0.0, audioConvolutionBuffer, 512);

	for (size_t i = 0; i < partitionCount; i++)
	{
		arm_fill_f32(0.0, convolutionPartitions[i], 512);
	}

	audioReady = true;

	return INIT_SUCCESS;
}

/**
 * @brief Partition the original impulse response into sub-filters in order to perform convolution in real time.
 * The linearity of the FFT allows previously computed FFTs to be reused in the suceeding filters- decreasing the 
 * number of forward FFTs that need to be computed. Since multiple convolutions are summed together, the overall 
 * number of inverse FFTs is also cut down.
 * 
 * @param[in] impulseResponse Pointer to the IR coefficients to process
 * @return true 
 * @return false 
 */
bool RTUPCR::partitionImpulseResponse(float32_t *impulseResponse)
{
	for (size_t i = 0; i < partitionCount; i++)
	{
		// Clear array each time
		arm_fill_f32(0.0, impulsePartitionBuffer, 512);

		for (size_t j = 0; j < partitionSize; j++)
		{
			impulsePartitionBuffer[2 * j + 256] = impulseResponse[128 * i + j];
		}

		arm_cfft_f32(&arm_cfft_sR_f32_len256, impulsePartitionBuffer, forwardTransform, 1);

		for (size_t j = 0; j < 512; j++)
		{
			impulseResponseFFT[i][j] = impulsePartitionBuffer[j];
		}
	}

	return true;
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
		arm_q15_to_float(leftAudio->data, leftAudioData, audioBlockSize);
		arm_q15_to_float(rightAudio->data, rightAudioData, audioBlockSize);

		for (size_t i = 0; i < partitionSize; i++)
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

		arm_cfft_f32(&arm_cfft_sR_f32_len256, audioConvolutionBuffer, forwardTransform, 1); // FFT of input audio buffer

		convolutionPartition = &convolutionPartitions[0][0] + (512 * partitionIndex); // Address of the current partition

		arm_copy_f32(audioConvolutionBuffer, convolutionPartition, 512); // Copy result from FFT into current partition

		arm_fill_f32(0.0, multAccum, 512); // Clear out previous data in accumulator

		reversedPartitionIndex = partitionIndex; // Set new starting point

		for (size_t i = 0; i < partitionCount; i++)
		{
			convolutionPartition = &convolutionPartitions[0][0] + (512 * reversedPartitionIndex); // Going through partition addresses [partitionIndex => 0]
			impulsePartition = &impulseResponseFFT[0][0] + (512 * i);							  // [0 => partitionCount - 1]

			// Complex-by-complex multiplication of impulse response sub-filter and audio input samples
			arm_cmplx_mult_cmplx_f32(convolutionPartition, impulsePartition, cmplxProduct, 256);

			// Add complex product to buffer
			arm_add_f32(multAccum, cmplxProduct, multAccum, 512);

			// Decrease counter by 1 until we've reached zero, then restart
			reversedPartitionIndex = ((reversedPartitionIndex - 1) < 0) ? (partitionCount - 1) : (reversedPartitionIndex - 1);
		}

		// Increase counter by 1 until we've reached the number of partitions, then reset counter to 0
		partitionIndex = ((partitionIndex + 1) >= partitionCount) ? 0 : (partitionIndex + 1);

		// Take the inverse FFT of the MAC product
		arm_cfft_f32(&arm_cfft_sR_f32_len256, multAccum, inverseTransform, 1);

		// Move the resultant convolution product into left and right audio buffers
		for (size_t i = 0; i < partitionSize; i++)
		{
			leftAudioData[i] = multAccum[2 * i] * 0.03; 
			rightAudioData[i] = multAccum[2 * i + 1] * 0.03;
		}

		// Convert back to integer for transmit and release
		arm_float_to_q15(leftAudioData, leftAudio->data, audioBlockSize);
		arm_float_to_q15(rightAudioData, rightAudio->data, audioBlockSize);

		// Transmit left and right audio to the output
		transmit(leftAudio, STEREO_LEFT);
		transmit(rightAudio, STEREO_RIGHT);

		// Adios
		release(leftAudio);
		release(rightAudio);
	}
}
