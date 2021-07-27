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

#include "auricle.h"
#include "convolvIR.h"
#include "SPDIFTx.h"
#include "IR.h"

AudioInputUSB stereoIn;
SPDIFTx stereoOut;
convolvIR convolve;
//AudioOutputSPDIF3 stereoOut;
//AudioConnection leftPassThrough(stereoIn, 0, stereoOut, 0);
//AudioConnection rightPassThrough(stereoIn, 1, stereoOut, 1);

AudioConnection leftInConv(stereoIn, convolve.STEREO_LEFT, convolve, convolve.STEREO_LEFT);
AudioConnection rightInConv(stereoIn, convolve.STEREO_RIGHT, convolve, convolve.STEREO_RIGHT);
AudioConnection leftOutConv(convolve, convolve.STEREO_LEFT, stereoOut, convolve.STEREO_LEFT);
AudioConnection rightOutConv(convolve, convolve.STEREO_RIGHT, stereoOut, convolve.STEREO_RIGHT);

int main(void)
{
	Serial.begin(115200);
	// while (!(Serial))
	// {
	// 	//yield();
	// 	delay(100);
	// }

	Serial.printf("Serial connected\n");
	
	static __attribute__ ((section(".dmabuffers"), used)) audio_block_t audioMemory[70];
	AudioStream::initialize_memory(audioMemory, 70);

	HRIR hrir;
	hrir.leftIR = &L0[0];
	hrir.rightIR = &R0[0];

	if (!(convolve.convertIR(&hrir)))
	{
		Serial.printf("IR Filter Created\n");
	}

	while (1)
	{
		delay(100);
		// float32_t usage = ((AudioStream::cpu_cycles_total) + (F_CPU_ACTUAL / 0.003715192666F)) / (F_CPU_ACTUAL / 0.001857596333F);
		// Serial.printf("%.2f%%\n", usage);
		// delay(500);

	}

	return EXIT_SUCCESS;
}
