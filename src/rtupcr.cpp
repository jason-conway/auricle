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

// DMA Buffers
float32_t __attribute__((section(".dmabuffers"), used)) convolutionFFT[partitionCount][512];
float32_t __attribute__((section(".dmabuffers"), used)) impulsePartitionBuffer[512];

/**
 * @brief Create partitioned filter mask from impulse response
 * 
 * @param[in] impulseResponse Impulse Response to be used for filter mask
 * @return bool
 */
bool RTUPCR::begin(float32_t *impulseResponse)
{
    for (size_t i = 0; i < partitionCount; i++)
	{
		// Clear arrays each time
		arm_fill_f32(0.0, convolutionFFT[i], 512);
		arm_fill_f32(0.0, impulsePartitionBuffer, 512);

		for (size_t j = 0; j < partitionSize; j++)
		{
			impulsePartitionBuffer[2 * j + 256] = impulseResponse[128 * i + j];
		}

		// CMSIS complex FFT of the impulse response partition. Output will be stuffed into the overall impulseResponseFFT
		arm_cfft_f32(&arm_cfft_sR_f32_len256, impulsePartitionBuffer, forwardTransform, 1);

		for (size_t j = 0; j < 512; j++) // Aforementioned stuffing lol
		{
			impulseResponseFFT[i][j] = impulsePartitionBuffer[j];
		}
	}

	arm_fill_f32(0.0, audioInFFT, 512); 

	audioReady = true;

	return true;
}

/**
 * @brief Where the magic *will* happen
 * 
 */
void RTUPCR::update(void)
{
	if (!(audioReady)) // Impulse response hasn't been processed yet
	{
		return;
	}

}
