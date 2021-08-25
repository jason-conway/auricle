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

#include <WProgram.h>
#include <pins_arduino.h>

enum Stereo
{
	leftChannel,
	rightChannel
};


#define IR_SAMPLES 8192
