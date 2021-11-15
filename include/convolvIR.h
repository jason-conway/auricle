/**
 * @file convolvIR.h
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief Spatial Audio for Arm Cortex-M7
 * @version 0.1
 * @date 2021-05-04
 *
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 *
 */

#pragma once

#include <arm_math.h>
#include <arm_const_structs.h>
#include <wiring.h>
#include <AudioStream.h>
#include "auricle.h"
#include "upols.h"

class ConvolvIR : public AudioStream
{
public:
	ConvolvIR(void);
	virtual void update(void);
	bool togglePassthrough(void);
	void convertIR(uint16_t irIndex);

private:
	audio_block_t *inputQueueArray[2];

	bool audioPassthrough;
	bool audioMute;
};

extern ConvolvIR convolvIR;
