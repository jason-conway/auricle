/**
 * @file SPDIFTx.h
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief IMXRT1060 HW S/PDIF
 * @version 0.1
 * @date 2021-06-22
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#pragma once

#include "auricle.h"
#include <AudioStream.h>
#include <DMAChannel.h>

#define SPDIF_DPLL_GAIN 0b11
#define SPDIF_LOOP_DIV 28
#define SPDIF_PLL_NUM 2240
#define SPDIF_PLL_DENOM 10000
#define SPDIF_STC_DIV 29

#define CCM_CDCR_SPDIF0_CLK_MASK (CCM_CDCDR_SPDIF0_CLK_SEL_MASK | CCM_CDCDR_SPDIF0_CLK_PRED_MASK | CCM_CDCDR_SPDIF0_CLK_PODF_MASK)
#define CCM_CDCR_SPDIF0_CLK_SEL_PLL4 0b00
#define CCM_CDCR_SPDIF0_CLK_PRED_DIV 7
#define CCM_CDCR_SPDIF0_CLK_PODF_DIV 0

#define GPIO_AD_B1_02_MUX_MODE_SPDIF 0b011

inline void dmaCopyAudio(int32_t *pTx, const int32_t *pTxStop, const int16_t *leftAudioData, const int16_t *rightAudioData);
inline void packAudioBuffer(audio_block_t *audioBlock, audio_block_t *audioBuffer[2]);

class SPDIFTx : public AudioStream
{
public:
	SPDIFTx(void);
	virtual void update(void);

private:
	audio_block_t *inputQueueArray[2];
	static audio_block_t *leftAudioBuffer[2];
	static audio_block_t *rightAudioBuffer[2];
	static audio_block_t silentAudio;

	static DMAChannel eDMA;
	uint8_t dmaChannel;

	void init(void);		
	static void configureSpdifRegisters(void);
	static uint8_t configureDMA(void);
	static void dmaISR(void);	
	static uint32_t getTxOffset(uint32_t txSourceAddress, uint32_t sourceBufferSize);

	enum Channels
	{
		STEREO_LEFT,
		STEREO_RIGHT
	};

};
