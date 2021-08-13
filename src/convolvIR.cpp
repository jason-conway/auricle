/**
 * @file CONVOLVIR.cpp
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief Spatial Audio for the Arm Cortex-M7
 * @version 0.1
 * @date 2021-05-04
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#include "convolvIR.h"

bool CONVOLVIR::audioPassthrough(false);

/**
 * @brief Construct a new CONVOLVIR::CONVOLVIR object
 * 
 */

CONVOLVIR::CONVOLVIR(void) : AudioStream(2, inputQueueArray)
{
	init();
}

/**
 * @brief 
 * 
 */
void __attribute__((section(".flashmem"))) CONVOLVIR::init(void)
{
	for (size_t i = 0; i < PARTITION_COUNT; i++)
	{
		arm_fill_f32(0.0f, convolutionPartitions[i], 512);
		arm_fill_f32(0.0f, hrtf.leftTF[i], 512);
		arm_fill_f32(0.0f, hrtf.rightTF[i], 512);
	}

	arm_fill_f32(0.0f, multAccum, 512);
	arm_fill_f32(0.0f, audioConvolutionBuffer, 512);
	arm_fill_f32(0.0f, leftAudioData, 128);
	arm_fill_f32(0.0f, leftAudioPrevSample, 128);
	arm_fill_f32(0.0f, rightAudioData, 128);
	arm_fill_f32(0.0f, rightAudioPrevSample, 128);
}

/**
 * @brief Partition the original impulse response into sub-filters in order to perform convolution in real time.
 * The linearity of the FFT allows previously computed FFTs to be reused in the suceeding filters- decreasing the 
 * number of forward FFTs that need to be computed. Since multiple convolutions are summed together, the overall 
 * number of inverse FFTs is also cut down.
 * 
 * @param hrir 
 * @return int8_t 
 */
void __attribute__((section(".flashmem"))) CONVOLVIR::convertIR(const HRIR *hrir) 
{
	audioReady = false;

	for (size_t i = 0; i < 2; i++)
	{
		float32_t impulsePartitionBuffer[512];

		for (size_t j = 0; j < PARTITION_COUNT; j++)
		{
			arm_fill_f32(0.0f, impulsePartitionBuffer, 512);

			for (size_t k = 0; k < PARTITION_SIZE; k++)
			{
				if (!(i))
				{
					impulsePartitionBuffer[2 * k + 256] = hrir->leftIR[128 * j + k];
				}
				else
				{
					impulsePartitionBuffer[2 * k + 256] = hrir->rightIR[128 * j + k];
				}
			}

			arm_cfft_f32(&arm_cfft_sR_f32_len256, impulsePartitionBuffer, FORWARD, 1);

			if (!(i))
			{
				arm_copy_f32(impulsePartitionBuffer, hrtf.leftTF[j], 512);
			}
			else
			{
				arm_copy_f32(impulsePartitionBuffer, hrtf.rightTF[j], 512);
			}
		}
	}
	audioReady = true;
}

/**
 * @brief 
 * 
 */
void CONVOLVIR::convolve(void)
{	
	for (size_t i = 0; i < 2; i++) // Two iterations - left and right
	{
		arm_fill_f32(0.0f, multAccum, 512); // Clear out previous data in accumulator

		int16_t shiftIndex = partitionIndex; // Set new starting point for sliding convolutionPartitions over the FFT of the impulse response

		if (!(i))
		{
			multiplyAccumulate(hrtf.leftTF, shiftIndex);
		}
		else
		{
			multiplyAccumulate(hrtf.rightTF, shiftIndex);
		}

		arm_cfft_f32(&arm_cfft_sR_f32_len256, multAccum, INVERSE, 1);

		// Move the resultant convolution product into left and right audio buffers
		for (size_t j = 0; j < PARTITION_SIZE; j++)
		{
			if (!(i))
			{
				leftAudioData[j] = multAccum[2 * j];
			}
			else
			{
				rightAudioData[j] = multAccum[2 * j + 1];
			}
		}
	}
}

/**
 * @brief Complex-by-complex multiplication of HRTF and audio input samples
 * 
 * @param hrtf 
 * @param shiftIndex 
 */
void __attribute__ ((optimize("-O1"))) CONVOLVIR::multiplyAccumulate(float32_t (*hrtf)[512], int16_t shiftIndex)
{
	for (size_t i = 0; i < PARTITION_COUNT; i++)
	{
		arm_cmplx_mult_cmplx_f32(convolutionPartitions[shiftIndex], hrtf[i], cmplxProduct, 256);

		arm_add_f32(multAccum, cmplxProduct, multAccum, 512);

		// Decrease counter by 1 until we've reached zero, then restart
		shiftIndex = ((shiftIndex - 1) < 0) ? (PARTITION_COUNT - 1) : (shiftIndex - 1);
	}
}

void CONVOLVIR::setPassthrough(bool passthrough)
{
	CONVOLVIR::audioPassthrough = passthrough;
}

void CONVOLVIR::setAngle(uint16_t sourceAngle)
{
	uint16_t index = floor(sourceAngle / 45); // Change later
	HRIR hrir;
	hrir.leftIR = IR_LUT[index][0];
	hrir.rightIR = IR_LUT[index][1];
}
/**
 * @brief Updates every 128 samples / 2.9 ms
 * 
 */
void CONVOLVIR::update(void)
{
	if (!(audioReady)) // Impulse response hasn't been processed yet
	{
		return;
	}
	
	audio_block_t *leftAudio = receiveWritable(leftChannel);
	audio_block_t *rightAudio = receiveWritable(rightChannel);

	if (leftAudio && rightAudio) // Data available on both the left and right channels
	{
		if (audioPassthrough) // Not messing with the data, just sending it through the pipe
		{
			transmit(leftAudio, leftChannel);
			transmit(rightAudio, rightChannel);
			release(leftAudio);
			release(rightAudio);
			return;
		}

		// Disable interrupts while computing the convolution 
		__asm__ volatile("CPSID i":::"memory");

		// Use float32 for higher precision intermediate calculations
		arm_q15_to_float(leftAudio->data, leftAudioData, 128);
		arm_q15_to_float(rightAudio->data, rightAudioData, 128);
		

		for (size_t i = 0; i < PARTITION_SIZE; i++)
		{
			// Fill the first half of audioConvolutionBuffer with audio data from the previous sample
			audioConvolutionBuffer[2 * i] = leftAudioPrevSample[i];		 // [0] [2] [4] ... [254]
			audioConvolutionBuffer[2 * i + 1] = rightAudioPrevSample[i]; // [1] [3] [5] ... [255]

			// Fill the last half of audioConvolutionBuffer with the current audio data
			audioConvolutionBuffer[2 * i + 256] = leftAudioData[i];	 // [256] [258] [260] ... [510]
			audioConvolutionBuffer[2 * i + 257] = rightAudioData[i]; // [257] [259] [261] ... [511]

			// Copy the current audio data into buffers for next sample overlap-and-save
			//leftAudioPrevSample[i] = leftAudioData[i];
			//rightAudioPrevSample[i] = rightAudioData[i];
		}
#if 0

		for (size_t i = 0; i < PARTITION_SIZE; i++)
		{
			leftAudioPrevSample[i] = leftAudioData[i];
			rightAudioPrevSample[i] = rightAudioData[i];
		}
#endif

#if 1
		arm_copy_f32(leftAudioData, leftAudioPrevSample, 128);
		arm_copy_f32(rightAudioData, rightAudioPrevSample, 128);
#endif

		arm_cfft_f32(&arm_cfft_sR_f32_len256, audioConvolutionBuffer, FORWARD, 1);

		arm_copy_f32(audioConvolutionBuffer, convolutionPartitions[partitionIndex], 512);
		
		convolve();

		// Increase counter by 1 until we've reached the number of partitions, then reset counter to 0
		partitionIndex = ((partitionIndex + 1) >= PARTITION_COUNT) ? 0 : (partitionIndex + 1);

		arm_float_to_q15(leftAudioData, leftAudio->data, 128);
		arm_float_to_q15(rightAudioData, rightAudio->data, 128);

		// Transmit left and right audio to the output
		transmit(leftAudio, leftChannel);
		transmit(rightAudio, rightChannel);
		
		// Re-enable interrupts
		__asm__ volatile("CPSIE i":::"memory");

		release(leftAudio);
		release(rightAudio);
	}
}
