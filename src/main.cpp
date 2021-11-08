/**
 * @file main.cpp
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief Spatial Audio for Arm Cortex-M7
 * @version 0.1
 * @date 2021-05-04
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#include "convolvIR.h"
#include "auricle.h"
#include "spdifTx.h"
#include "ash.h"

AudioInputUSB stereoIn;
SpdifTx stereoOut;
ConvolvIR convolvIR;

// AudioConnection passthoughLeft(stereoIn, leftChannel, stereoOut, leftChannel);
// AudioConnection passthroughRight(stereoIn, rightChannel, stereoOut, rightChannel);
AudioConnection leftInConv(stereoIn, leftChannel, convolvIR, leftChannel);
AudioConnection rightInConv(stereoIn, rightChannel, convolvIR, rightChannel);
AudioConnection leftOutConv(convolvIR, leftChannel, stereoOut, leftChannel);
AudioConnection rightOutConv(convolvIR, rightChannel, stereoOut, rightChannel);

Ash ash;

usb_serial_class *stdStream = &SerialUSB;

int main(void)
{
	stdStream->begin(115200);
	
	while (!(stdStream))
	{
		msleep(100);
	}
	
	ash.init();

	while (1)
	{
		ash.execLoop();
	}
	
	return EXIT_SUCCESS;
}
