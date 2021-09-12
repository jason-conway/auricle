/**
 * @file auricle.h
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief Spatial Audio for Arm Cortex-M7
 * @version 0.1
 * @date 2021-05-04
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#pragma once

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <WProgram.h>

enum Stereo
{
	leftChannel,
	rightChannel
};

inline void msleep(uint32_t mS)
{
	uint32_t uS = 396000 * mS;
	uint32_t startingCycleCount = *reinterpret_cast<volatile uint32_t *>(0xE0001004);
	while (*reinterpret_cast<volatile uint32_t *>(0xE0001004) - startingCycleCount < uS);
}

