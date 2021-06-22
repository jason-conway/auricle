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
//AudioOutputSPDIF stereoOut;
AudioOutputI2S stereoOut;
// AudioConnection leftInConv(stereoIn, convolve.STEREO_LEFT, stereoOut, convolve.STEREO_LEFT);
// AudioConnection rightInConv(stereoIn, convolve.STEREO_RIGHT, stereoOut, convolve.STEREO_RIGHT);

AudioControlSGTL5000 codec;

AudioConnection leftInConv(stereoIn, convolve.STEREO_LEFT, convolve, convolve.STEREO_LEFT);
AudioConnection rightInConv(stereoIn, convolve.STEREO_RIGHT, convolve, convolve.STEREO_RIGHT);
AudioConnection leftOutConv(convolve, convolve.STEREO_LEFT, stereoOut, convolve.STEREO_LEFT);
AudioConnection rightOutConv(convolve, convolve.STEREO_RIGHT, stereoOut, convolve.STEREO_RIGHT);

int main(void)
{
	Serial.begin(115200);
	while (!(Serial))
	{
		yield();
		delay(100);
	}

	codec.enable();
	codec.volume(0.5);
	
	Serial.printf(((const __FlashStringHelper *)("Serial connected\n")));

	static DMAMEM audio_block_t audioMemory[70];
	AudioStream::initialize_memory(audioMemory, 70);

	HRIR hrir;
	hrir.leftIR = &L306[0];
	hrir.rightIR = &R306[0];
	
	if (!(convolve.begin(&hrir)))
	{
		Serial.printf(((const __FlashStringHelper *)("IR Filter Created\n")));
	}

	while (1)
	{
		// float32_t usage = (((AudioStream::cpu_cycles_total) + (F_CPU_ACTUAL / 128 / 44100.0f * 128 / 100)) / (F_CPU_ACTUAL / 64 / 44100.0f * 128 / 100));
		// Serial.printf("%.2f%%\n", usage);
		// delay(500);
		float32_t masterVolume = stereoIn.volume();
		if (masterVolume > 0)
		{
			masterVolume = 0.5 * masterVolume + 0.3;
		}
		codec.volume(masterVolume);


		hrir.leftIR = &L18[0];
		hrir.rightIR = &R18[0];
		convolve.convertIR(&hrir);		
		Serial.printf("18\n");
		delay(500);

		hrir.leftIR = &L36[0];
		hrir.rightIR = &R36[0];
		convolve.convertIR(&hrir);
		Serial.printf("36\n");
		delay(500);

		hrir.leftIR = &L54[0];
		hrir.rightIR = &R54[0];
		convolve.convertIR(&hrir);
		Serial.printf("54\n");
		delay(500);

		hrir.leftIR = &L72[0];
		hrir.rightIR = &R72[0];
		convolve.convertIR(&hrir);
		Serial.printf("72\n");
		delay(500);

		hrir.leftIR = &L90[0];
		hrir.rightIR = &R90[0];
		convolve.convertIR(&hrir);
		Serial.printf("90\n");
		delay(500);

		hrir.leftIR = &L108[0];
		hrir.rightIR = &R108[0];
		convolve.convertIR(&hrir);
		Serial.printf("108\n");
		delay(500);

		hrir.leftIR = &L126[0];
		hrir.rightIR = &R126[0];
		convolve.convertIR(&hrir);
		Serial.printf("126\n");
		delay(500);

		hrir.leftIR = &L144[0];
		hrir.rightIR = &R144[0];
		convolve.convertIR(&hrir);
		Serial.printf("144\n");
		delay(500);

		hrir.leftIR = &L162[0];
		hrir.rightIR = &R162[0];
		convolve.convertIR(&hrir);
		Serial.printf("162\n");
		delay(500);

		hrir.leftIR = &L180[0];
		hrir.rightIR = &R180[0];
		convolve.convertIR(&hrir);
		Serial.printf("180\n");
		delay(500);

		hrir.leftIR = &L198[0];
		hrir.rightIR = &R198[0];
		convolve.convertIR(&hrir);
		Serial.printf("198\n");
		delay(500);

		hrir.leftIR = &L216[0];
		hrir.rightIR = &R216[0];
		convolve.convertIR(&hrir);
		Serial.printf("216\n");
		delay(500);

		hrir.leftIR = &L234[0];
		hrir.rightIR = &R234[0];
		convolve.convertIR(&hrir);
		Serial.printf("234\n");
		delay(500);

		hrir.leftIR = &L252[0];
		hrir.rightIR = &R252[0];
		convolve.convertIR(&hrir);
		Serial.printf("252\n");
		delay(500);

		hrir.leftIR = &L270[0];
		hrir.rightIR = &R270[0];
		convolve.convertIR(&hrir);
		Serial.printf("270\n");
		delay(500);

		hrir.leftIR = &L288[0];
		hrir.rightIR = &R288[0];
		convolve.convertIR(&hrir);
		Serial.printf("288\n");
		delay(500);

		hrir.leftIR = &L306[0];
		hrir.rightIR = &R306[0];
		convolve.convertIR(&hrir);
		Serial.printf("306\n");
		delay(500);

		hrir.leftIR = &L324[0];
		hrir.rightIR = &R324[0];
		convolve.convertIR(&hrir);
		Serial.printf("324\n");
		delay(500);

		hrir.leftIR = &L342[0];
		hrir.rightIR = &R342[0];
		convolve.convertIR(&hrir);
		Serial.printf("342\n");
		delay(500);

		hrir.leftIR = &L0[0];
		hrir.rightIR = &R0[0];
		convolve.convertIR(&hrir);
		Serial.printf("0\n");
		delay(500);

	}

	return EXIT_SUCCESS;
}
