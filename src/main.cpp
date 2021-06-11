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

#include "convolvir.h"
#include "auricle.h"
#include "IR.h"

Auricle convolve;

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
		delay(500);
	}
	
	Serial.printf(((const __FlashStringHelper *)("Serial connected\n")));

	static DMAMEM audio_block_t audioMemory[70];
	AudioStream::initialize_memory(audioMemory, 70);

	codec.enable();
	codec.volume(0.5);

	HRIR hrir;
	hrir.leftIR = &leftIR[0];
	hrir.rightIR = &rightIR[0];
	
	if (!(convolve.begin(&hrir)))
	{
		Serial.printf(((const __FlashStringHelper *)("IR Filter Created\n")));
	}

	while (1)
	{
		float32_t masterVolume = stereoIn.volume();
		if (masterVolume > 0)
		{
			masterVolume = 0.5 * masterVolume + 0.3;
		}
		codec.volume(masterVolume);

		float32_t usage = (((AudioStream::cpu_cycles_total) + (F_CPU_ACTUAL / 128 / 44100.0f * 128 / 100)) / (F_CPU_ACTUAL / 64 / 44100.0f * 128 / 100));
		Serial.printf("%.1f%%\n", usage);

		delay(100);
	}

	return EXIT_SUCCESS;
}
