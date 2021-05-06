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

RTUPCR convolve;

AudioInputI2S stereoIn;
AudioOutputI2S stereoOut;

AudioConnection leftInConv(stereoIn, convolve.STEREO_LEFT, convolve, convolve.STEREO_LEFT);
AudioConnection rightInConv(stereoIn, convolve.STEREO_RIGHT, convolve, convolve.STEREO_RIGHT);
AudioConnection leftOutConv(convolve, convolve.STEREO_LEFT, stereoOut, convolve.STEREO_LEFT);
AudioConnection rightOutConv(convolve, convolve.STEREO_RIGHT, stereoOut, convolve.STEREO_RIGHT);

int main(void)
{
	float32_t impulseArray[IR_SAMPLES]; // Internal options?? Rotary position switch?
	static DMAMEM audio_block_t audioMemory[50];
	AudioStream::initialize_memory(audioMemory, 50);

	convolve.begin(impulseArray);
	Serial.begin(115200);

	while (!Serial);

	Serial.printf("Does it work?\n");

	while (1)
	{
		Serial.printf("Yes...\n");
	    yield();
		delay(1000);
	}

	return EXIT_SUCCESS;
}