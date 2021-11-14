/**
 * @file upols.h
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief
 * @version 0.1
 * @date 2021-11-11
 *
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 *
 */

#pragma once

#include <stdlib.h>
#include <string.h>
#include <usb_serial.h>
#include <stdio.h>
#include <stdbool.h>
#include <arm_math.h>
#include <arm_const_structs.h>
#include <imxrt.h>

enum Lengths
{
	PartitionSize = 128, // Number of audio samples per partition
	PartitionCount = 64, // Number of partitions making up the filter
	ImpulseSamples = PartitionSize * PartitionCount,
};

enum FFT_Flags
{
	ForwardFFT,
	InverseFFT
};

enum ChannelID
{
	LeftChannel,
	RightChannel
};

#ifdef __cplusplus
extern "C"
{
#endif
	void convertImpulseResponse(uint16_t irIndex);
	void convolve(float32_t *leftAudioData, float32_t *rightAudioData);
#ifdef __cplusplus
}
#endif
