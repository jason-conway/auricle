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

convolvIR convolve;

AudioInputUSB stereoIn;
SPDIFTx stereoOut;

AudioConnection leftPassThrough(stereoIn, convolve.STEREO_LEFT, stereoOut, convolve.STEREO_LEFT);
AudioConnection rightPassThrough(stereoIn, convolve.STEREO_RIGHT, stereoOut, convolve.STEREO_RIGHT);

// AudioConnection leftInConv(stereoIn, convolve.STEREO_LEFT, convolve, convolve.STEREO_LEFT);
// AudioConnection rightInConv(stereoIn, convolve.STEREO_RIGHT, convolve, convolve.STEREO_RIGHT);
// AudioConnection leftOutConv(convolve, convolve.STEREO_LEFT, stereoOut, convolve.STEREO_LEFT);
// AudioConnection rightOutConv(convolve, convolve.STEREO_RIGHT, stereoOut, convolve.STEREO_RIGHT);

int main(void)
{
	Serial.begin(115200);
	while (!(Serial))
	{
		yield();
		delay(100);
	}

	Serial.printf("Serial connected\n");
	
	static DMAMEM audio_block_t audioMemory[70];
	AudioStream::initialize_memory(audioMemory, 70);

	HRIR hrir;
	hrir.leftIR = &L198[0];
	hrir.rightIR = &R198[0];

	if (!(convolve.begin(&hrir)))
	{
		Serial.printf("IR Filter Created\n");
	}

	while (1)
	{
		float32_t usage = (((AudioStream::cpu_cycles_total) + (F_CPU_ACTUAL / 128 / 44100.0f * 128 / 100)) / (F_CPU_ACTUAL / 64 / 44100.0f * 128 / 100));
		Serial.printf("%.2f%%\n", usage);
		delay(500);

		float32_t masterVolume = stereoIn.volume();
		if (!(masterVolume))
		{
	
		}
	}

	return EXIT_SUCCESS;
}
