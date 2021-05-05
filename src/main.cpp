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
usb_serial_class usb;

AudioInputI2S stereoIn;
AudioOutputI2S stereoOut;

AudioConnection leftInConv(stereoIn, convolve.STEREO_LEFT, convolve, convolve.STEREO_LEFT);
AudioConnection rightInConv(stereoIn, convolve.STEREO_RIGHT, convolve, convolve.STEREO_RIGHT);
AudioConnection leftOutConv(convolve, convolve.STEREO_LEFT, stereoOut, convolve.STEREO_LEFT);
AudioConnection rightOutConv(convolve, convolve.STEREO_RIGHT, stereoOut, convolve.STEREO_RIGHT);

int main(void)
{
	float32_t impulseArray[IR_SAMPLES];
	convolve.begin(impulseArray);
	usb.begin(115200);

	while (!usb);

	usb.printf("Does it work?\n");

	// while(1)
	// {
	//     yield();
	// }

	return EXIT_SUCCESS;
}