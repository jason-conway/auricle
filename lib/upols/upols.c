/**
 * @file upols.c
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief 
 * @version 0.1
 * @date 2021-11-11
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#include "upols.h"

/**
 * @brief 
 * 
 */
void convolve(const int16_t partitionIndex, float32_t *frequencyDelayLine, float32_t *hrtf, float32_t *channelOutput, const uint8_t channelID)
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
        channelOutput[i] = cmplxAccum[2 * i + channelID];
    }
}

void upols(float32_t *leftAudioData, float32_t *rightAudioData, hrtf_t *hrtf)
{
    __disable_irq();

    static float32_t overlappedAudioData[512];
    static float32_t frequencyDelayLine[512 * PartitionCount];
    static int16_t partitionIndex;

    for (size_t i = 0; i < PartitionSize; i++)
    {
        static float32_t overlapSave[256];

        // Fill the first half with the previous sample
        overlappedAudioData[2 * i] = overlapSave[2 * i];         // [0] [2] [4] ... [254]
        overlappedAudioData[2 * i + 1] = overlapSave[2 * i + 1]; // [1] [3] [5] ... [255]

        // Fill the last half with the current sample
        overlappedAudioData[2 * i + 256] = leftAudioData[i];  // [256] [258] [260] ... [510]
        overlappedAudioData[2 * i + 257] = rightAudioData[i]; // [257] [259] [261] ... [511]

        overlapSave[2 * i] = leftAudioData[i];
        overlapSave[2 * i + 1] = rightAudioData[i];
    }

    arm_cfft_f32(&arm_cfft_sR_f32_len256, overlappedAudioData, ForwardFFT, 1);
    arm_copy_f32(overlappedAudioData, &frequencyDelayLine[partitionIndex * 512], 512);

    convolve(partitionIndex, frequencyDelayLine, hrtf->letf, leftAudioData, LeftChannel);
    convolve(partitionIndex, frequencyDelayLine, hrtf->retf, rightAudioData, RightChannel);

    // Increment with wraparound
    partitionIndex = (partitionIndex + 1) % PartitionCount;

    // Re-enable interrupts
    __enable_irq();
}
