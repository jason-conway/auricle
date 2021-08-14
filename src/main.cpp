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

#include "SPDIFTx.h"
#include "ash.h"

AudioInputUSB stereoIn;
SPDIFTx stereoOut;

CONVOLVIR convolvIR;
ASH ash;

// AudioConnection leftPassThrough(stereoIn, 0, stereoOut, 0);
// AudioConnection rightPassThrough(stereoIn, 1, stereoOut, 1);

AudioConnection leftInConv(stereoIn, leftChannel, convolvIR, leftChannel);
AudioConnection rightInConv(stereoIn, rightChannel, convolvIR, rightChannel);
AudioConnection leftOutConv(convolvIR, leftChannel, stereoOut, leftChannel);
AudioConnection rightOutConv(convolvIR, rightChannel, stereoOut, rightChannel);


int main(void)
{
	Serial.begin(115200);
	while (!(Serial))
	{
		//yield();
		delay(100);
	}

	SerialUSB.printf("Serial connected\n");
	ash.init();

	AudioMemory(20);
	
	while (1)
	{
		// float32_t usage = AudioProcessorUsage();
		// Serial.printf("%.2f%%\n", usage);

		ash.execLoop();
		yield();
		delay(100);
	}

	return EXIT_SUCCESS;
}
