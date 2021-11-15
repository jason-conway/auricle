/**
 * @file convolvIR.cpp
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief Spatial Audio for the Arm Cortex-M7
 * @version 0.1
 * @date 2021-05-04
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#include "convolvIR.h"

// #pragma GCC optimize ("O1")

/**
 * @brief Construct a new ConvolvIR::ConvolvIR object
 * 
 */
ConvolvIR::ConvolvIR(void) : AudioStream(2, inputQueueArray)
{
	_section_dma static audio_block_t allocatedAudioMemory[16];
	initialize_memory(allocatedAudioMemory, 16);
	audioPassthrough = true;
	pinMode(33, 1);
}


void ConvolvIR::convertIR(uint16_t irIndex)
{
	audioMute = true;
	digitalWriteFast(33, 1);
	convertImpulseResponse(irIndex);
	digitalWriteFast(33, 0);
	audioMute = false;
	audioPassthrough = false;

}

bool ConvolvIR::togglePassthrough(void)
{
	audioPassthrough = !audioPassthrough;
	return audioPassthrough;
}

/**
 * @brief Updates every 128 samples / 2.9 ms
 * 
 */
void ConvolvIR::update(void)
{
	if (audioMute)
	{
		return;
	}

	audio_block_t *leftAudio = receiveWritable(LeftChannel);
	audio_block_t *rightAudio = receiveWritable(RightChannel);

	if (leftAudio && rightAudio) // Data available on both the left and right channels
	{
		if (audioPassthrough) // Not messing with the data, just sending it through the pipe
		{
			transmit(leftAudio, LeftChannel);
			transmit(rightAudio, RightChannel);
			release(leftAudio);
			release(rightAudio);
			return;
		}
		else
		{
			__disable_irq();

			digitalWriteFast(33,1);
			convolve(leftAudio->data, rightAudio->data);
			digitalWriteFast(33,0);

			// Transmit left and right audio to the output
			transmit(leftAudio, LeftChannel);
			transmit(rightAudio, RightChannel);

			release(leftAudio);
			release(rightAudio);

			// Re-enable interrupts
			__enable_irq();
			
		}
	}
}
