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
// #include "tablIR.h"

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
}


void ConvolvIR::convertIR(uint16_t irIndex)
{
	audioMute = true;
	convertImpulseResponse(irIndex);
	audioMute = false;
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

			float32_t leftAudioData[128];
			float32_t rightAudioData[128];

			arm_q15_to_float(leftAudio->data, leftAudioData, 128);
			arm_q15_to_float(rightAudio->data, rightAudioData, 128);

			convolve(leftAudioData, rightAudioData);

			arm_float_to_q15(leftAudioData, leftAudio->data, 128);
			arm_float_to_q15(rightAudioData, rightAudio->data, 128);

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
