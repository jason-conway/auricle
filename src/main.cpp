/**
 * @file main.cpp
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief Impulse Response Convolution Reverb for Arm Cortex-M7
 * @version 0.1
 * @date 2021-05-04
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#include "convolvir.h"
#include "rtupcr.h"
#include "IR.h"

RTUPCR convolve;

AudioInputUSB stereoIn;
AudioOutputI2S stereoOut;

AudioConnection leftInConv(stereoIn, convolve.STEREO_LEFT, convolve, convolve.STEREO_LEFT);
AudioConnection rightInConv(stereoIn, convolve.STEREO_RIGHT, convolve, convolve.STEREO_RIGHT);
AudioConnection leftOutConv(convolve, convolve.STEREO_LEFT, stereoOut, convolve.STEREO_LEFT);
AudioConnection rightOutConv(convolve, convolve.STEREO_RIGHT, stereoOut, convolve.STEREO_RIGHT);

AudioControlSGTL5000 codec;

int main(void)
{
	Serial.begin(115200);
	while (!(Serial))
	{
		delay(50);
	}

	Serial.printf("Serial connected\n");

	static DMAMEM audio_block_t audioMemory[70];
	AudioStream::initialize_memory(audioMemory, 70);

	codec.enable();
	codec.volume(0.5);

	if (!(convolve.begin(IR)))
	{
		Serial.printf("IR Filter Created\n");
	}
	
	while (1)
	{
		float32_t masterVolume = stereoIn.volume();

		if (masterVolume > 0)
		{
			masterVolume = 0.5 * masterVolume + 0.3;
		}
		codec.volume(masterVolume);
		delay(100);
	}

	return EXIT_SUCCESS;
}