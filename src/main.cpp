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

ConvolvIR convolvIR;
ASH ash;

AudioConnection leftInConv(stereoIn, leftChannel, convolvIR, leftChannel);
AudioConnection rightInConv(stereoIn, rightChannel, convolvIR, rightChannel);
AudioConnection leftOutConv(convolvIR, leftChannel, stereoOut, leftChannel);
AudioConnection rightOutConv(convolvIR, rightChannel, stereoOut, rightChannel);

int main(void)
{
	SerialUSB.begin(115200);
	
	while (!(SerialUSB))
	{
		uint32_t startingCycleCount = (*(volatile uint32_t *)0xE0001004);
		while ((*(volatile uint32_t *)0xE0001004) - startingCycleCount < (uint32_t)99000000);
	}
	
	ash.init();

	static __attribute__ ((section(".dmabuffers"), used)) audio_block_t allocatedAudioMemory[20]; 
	AudioStream::initialize_memory(allocatedAudioMemory, 20);
	
	while (1)
	{
		ash.execLoop();
	}

	return EXIT_SUCCESS;
}
