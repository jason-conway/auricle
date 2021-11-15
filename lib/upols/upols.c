/**
 * @file upols.c
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief Fast Stereo Convolution using a Uniformly-Paritioned Overlap-Save Scheme
 * @version 0.95
 * @date 2021-05-04
 *
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 * @details 
 * HRIR - Head-Related Impulse Response
 * HRTF - Head-Related Transfer Function
 * The HRTF is the Discrete Fourier Transform of the HRIR
 * The hrtf_t struct holds two HRTFs, one for each ear.
 * 		letf - Left Ear Transfer Function
 * 		retf - Right Ear Transfer Function
 * 	
 */

#include "upols.h"
#include "./../../include/tablIR.h"

typedef struct hrtf_t
{
	float32_t letf[512 * PartitionCount];
	float32_t retf[512 * PartitionCount];
} hrtf_t;

typedef struct upols_t
{
	int16_t currentIndex;					   // Current partition index
	float32_t slidingWindow[512];			   // Time-domain sliding window
	float32_t delayLine[512 * PartitionCount]; // Frequency-domain delay line
} upols_t;

hrtf_t hrtf;

/**
 * @brief Convert set of left and right channel HRIRs into a set of uniformly partitioned frequency-domain HRTFs
 * 
 * The linearity of the FFT allows previously computed FFTs to be reused in the suceeding filters- decreasing the 
 * number of forward FFTs that need to be computed. Since multiple convolutions are summed together, the overall 
 * number of inverse FFTs is also cut down.
 * 
 * @param irIndex 
 */
void convertImpulseResponse(const uint16_t irIndex)
{
	// Start by clearing any contents that may be in memory
	memset(&hrtf, 0, sizeof(hrtf));

	// Loop twice, left channel when i == 0, right channel when i == 1
	for (size_t i = 0; i < 2; i++)
	{
		float32_t impulsePartitionBuffer[512]; // DFT spectra of an indiviual filter partition
		float32_t *channelTF = i ? hrtf.retf : hrtf.retf;

		for (size_t j = 0; j < PartitionCount; j++)
		{
			// Zero out impulsePartitionBuffer at the start of a new partition
			arm_fill_f32(0.0f, impulsePartitionBuffer, 512);

			for (size_t k = 0; k < PartitionSize; k++)
			{
				// impulsePartitionBuffer[2 * k + 256] = irTable[2 * ImpulseSamples * irIndex + ImpulseSamples * i + 128 * j + k];

				// Zero-padded on the left side
				impulsePartitionBuffer[2 * k + 256] = irTable[ImpulseSamples * i + 128 * j + k];
			}

			// Compute the DFT of the partition and copy to hrtf
			arm_cfft_f32(&arm_cfft_sR_f32_len256, impulsePartitionBuffer, ForwardFFT, 1);
			arm_copy_f32(impulsePartitionBuffer, &channelTF[512 * j], 512);
		}
	}
}

/**
 * @brief Perform frequency-domain convolution by point-wise multiplication of DFT spectra
 * 
 * @param channelOutput Pointer to the time-domain output buffer
 * @param channelID ID of the channel being operated on [left -> 0] [right -> 1]
 */
void _convolve(upols_t *upols, float32_t *channelOutput, const uint8_t channelID)
{
	// Frequency-domain accumulation buffer
	static float32_t cmplxAccum[512];
	float32_t cmplxProduct[512];

	arm_fill_f32(0.0f, cmplxAccum, 512); // Zero buffer between calls

	int16_t shiftIndex = upols->currentIndex; // Set new starting point for sliding convolutionPartitions over the FFT of the impulse response

	float32_t *channelTF = channelID ? hrtf.retf : hrtf.retf;

	for (size_t i = 0; i < PartitionCount; i++)
	{
		// Point-wise multiplication of the DFT spectra
		arm_cmplx_mult_cmplx_f32(&upols->delayLine[512 * shiftIndex], &channelTF[512 * i], cmplxProduct, 256);
		arm_add_f32(cmplxAccum, cmplxProduct, cmplxAccum, 512);

		// Decrement with wraparound
		shiftIndex = (shiftIndex + (PartitionCount - 1)) % PartitionCount;
	}

	arm_cfft_f32(&arm_cfft_sR_f32_len256, cmplxAccum, InverseFFT, 1);

	// Discarding the time-aliased block
	for (size_t i = 0; i < PartitionSize; i++)
	{
		channelOutput[i] = cmplxAccum[2 * i + channelID];
	}
}

/**
 * @brief Overlap and save input audio samples
 * 
 * @param leftAudioData 
 * @param rightAudioData 
 */
inline static void overlapSamples(upols_t *upols, const float32_t *leftAudioData, const float32_t *rightAudioData)
{
	for (size_t i = 0; i < PartitionSize; i++)
	{
		// Previous left channel samples in the even indexes, right channel samples in the odd indexes
		static float32_t previousAudioData[256];

		// Fill the first half with the previous sample
		upols->slidingWindow[2 * i] = previousAudioData[2 * i];			// [0] [2] [4] ... [254]
		upols->slidingWindow[2 * i + 1] = previousAudioData[2 * i + 1]; // [1] [3] [5] ... [255]

		// Fill the last half with the current sample
		upols->slidingWindow[2 * i + 256] = leftAudioData[i];  // [256] [258] [260] ... [510]
		upols->slidingWindow[2 * i + 257] = rightAudioData[i]; // [257] [259] [261] ... [511]

		// Save a copy of current sample for the next audio block
		previousAudioData[2 * i] = leftAudioData[i];
		previousAudioData[2 * i + 1] = rightAudioData[i];
	}
}

/**
 * @brief Only one FFT/IFFT transform is required per input block.
 * 
 * @param leftAudioData 
 * @param rightAudioData 
 */
void convolve(int16_t *leftAudio, int16_t *rightAudio)
{
	static upols_t upols;

	float32_t leftAudioData[128];
	float32_t rightAudioData[128];

	arm_q15_to_float(leftAudio, leftAudioData, 128);
	arm_q15_to_float(rightAudio, rightAudioData, 128);

	overlapSamples(&upols, leftAudioData, rightAudioData);

	// Take FFT of time-domain input buffer and copy to the FDL
	arm_cfft_f32(&arm_cfft_sR_f32_len256, upols.slidingWindow, ForwardFFT, 1);
	arm_copy_f32(upols.slidingWindow, &upols.delayLine[upols.currentIndex * 512], 512);

	_convolve(&upols, leftAudioData, LeftChannel);
	_convolve(&upols, rightAudioData, RightChannel);

	// Increment with wraparound
	upols.currentIndex = (upols.currentIndex + 1) % PartitionCount;

	arm_float_to_q15(leftAudioData, leftAudio, 128);
	arm_float_to_q15(rightAudioData, rightAudio, 128);
}
