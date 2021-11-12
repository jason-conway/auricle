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
	PartitionSize = 128,
	PartitionCount = 64,
	TransformSize = 2 * PartitionSize,
	ComplexValues = 2 * TransformSize,
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

typedef struct hrtf_t
{
	float32_t letf[512 * PartitionCount];
	float32_t retf[512 * PartitionCount];
} hrtf_t;

#ifdef __cplusplus
extern "C"
{
#endif
	void upols(float32_t *leftAudio, float32_t *rightAudio, hrtf_t *hrtf);
#ifdef __cplusplus
}
#endif
