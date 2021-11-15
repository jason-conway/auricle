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
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <WProgram.h>

#define _section_flash __attribute__((section(".flashmem")))
#define _section_dma __attribute__((used, section(".dmabuffers")))
#define _section_dma_aligned __attribute__((used, section(".dmabuffers"), aligned(32)))

enum Stereo
{
	leftChannel,
	rightChannel
};

inline void msleep(uint32_t mS)
{
	uint32_t uS = 600000 * mS;
	uint32_t startingCycleCount = *(volatile uint32_t *)0xE0001004;
	while (*(volatile uint32_t *)0xE0001004 - startingCycleCount < uS);
}


