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
 * The filters_t struct holds two filters, one for each ear.
 *
 */

#include "upols.h"
#include "./../../include/tablIR.h"

// Filter impulse responses
typedef struct filters_t
{
	float32_t left[512 * PartitionCount];
	float32_t right[512 * PartitionCount];
} filters_t;

typedef struct upols_t
{
	int16_t currentIndex;					   // Current partition index
	float32_t slidingWindow[512];			   // Time-domain sliding window
	float32_t delayLine[512 * PartitionCount]; // Frequency-domain delay line
} upols_t;

filters_t filters;

void processFilters(const uint16_t irIndex)
{
	// Start by clearing any contents that may be in memory
	memset(&filters, 0, sizeof(filters));

	// Loop twice, left channel when i == 0, right channel when i == 1
	for (size_t i = 0; i < 2; i++)
	{
		float32_t subfilterSpectra[512]; // DFT spectra of an indiviual filter partition
		float32_t *filter = i ? filters.right : filters.left;

		for (size_t j = 0; j < PartitionCount; j++)
		{
			// Zero out impulsePartitionBuffer at the start of a new partition
			clear512(subfilterSpectra);

			for (size_t k = 0; k < PartitionSize; k++)
			{
				// impulsePartitionBuffer[2 * k + 256] = irTable[2 * ImpulseSamples * irIndex + ImpulseSamples * i + 128 * j + k];

				// Zero-padded on the left side
				subfilterSpectra[2 * k + 256] = irTable[ImpulseSamples * i + 128 * j + k];
			}

			// Compute the DFT of the partition and copy to hrtf
			arm_cfft_f32(&arm_cfft_sR_f32_len256, subfilterSpectra, ForwardFFT, 1);
			cp512(subfilterSpectra, &filter[512 * j]);
		}
	}
}

/**
 * @brief Perform frequency-domain convolution by point-wise multiplication of DFT spectra
 *
 * @param channelOutput Pointer to the time-domain output buffer
 * @param filterID ID of the channel being operated on [left -> 0] [right -> 1]
 */
void _convolve(upols_t *upols, float32_t *channelOutput, const uint8_t filterID)
{
	// Frequency-domain accumulation buffer
	float32_t cmplxAccum[512] = {0};
	float32_t *filter = filterID ? filters.right : filters.left;

	int16_t shiftIndex = upols->currentIndex; // New starting point
	
	for (size_t i = 0; i < PartitionCount; i++)
	{
		// Fast multiply-accumulate for complex numbers
		cmac512(&upols->delayLine[512 * shiftIndex], &filter[512 * i], cmplxAccum);

		// Decrement with wraparound
		shiftIndex = (shiftIndex + (PartitionCount - 1)) % PartitionCount;
	}

	arm_cfft_f32(&arm_cfft_sR_f32_len256, cmplxAccum, InverseFFT, 1);

#pragma GCC unroll 8
	for (size_t i = 0; i < PartitionSize; i++)
	{
		channelOutput[i] = cmplxAccum[2 * i + filterID]; // Time-aliased portion isn't copied
	}
}

/**
 * @brief Overlap and save input audio samples
 * 
 * @param upols upols_t instance
 * @param leftAudioData Pointer to left channel audio
 * @param rightAudioData Pointer to right channel audio
 */
void overlapSamples(upols_t *upols, const float32_t *leftAudioData, const float32_t *rightAudioData)
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
 * @brief 
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
	cp512(upols.slidingWindow, &upols.delayLine[upols.currentIndex * 512]);

	_convolve(&upols, leftAudioData, LeftFilter);
	_convolve(&upols, rightAudioData, RightFilter);

	// Increment with wraparound
	upols.currentIndex = (upols.currentIndex + 1) % PartitionCount;

	// Convert back to input type
	arm_float_to_q15(leftAudioData, leftAudio, 128);
	arm_float_to_q15(rightAudioData, rightAudio, 128);
}
