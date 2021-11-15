/**
 * @file spdifTx.h
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

class SpdifTx : public AudioStream
{
public:
	SpdifTx(void);
	virtual void update(void);

private:
	void init(void);
	static void configureSpdifRegisters(void);
	static void spdifInterleave(int32_t *pTx, const int16_t *leftAudioData, const int16_t *rightAudioData);
	static void dmaISR(void);

	static uint8_t configureDMA(void);
	static int32_t getTxOffset(uint32_t txSourceAddress, uint32_t sourceBufferSize);

	audio_block_t *inputQueueArray[2];
	static audio_block_t *leftAudioBuffer[2];
	static audio_block_t *rightAudioBuffer[2];

	static DMAChannel eDMA;
	uint8_t dmaChannel;

	enum PLL
	{
		SPDIF_DPLL_GAIN = 0b11,
		SPDIF_PLL_NUM = 2240,
		SPDIF_PLL_DENOM = 10000
	};

	enum CDCR_SPDIF0
	{
		CCM_CDCR_SPDIF0_CLK_SEL_PLL4 = 0b00,
		CCM_CDCR_SPDIF0_CLK_PRED_DIV = 7,
		CCM_CDCR_SPDIF0_CLK_PODF_DIV = 0,
		CCM_CDCR_SPDIF0_CLK_MASK = (CCM_CDCDR_SPDIF0_CLK_SEL_MASK | CCM_CDCDR_SPDIF0_CLK_PRED_MASK | CCM_CDCDR_SPDIF0_CLK_PODF_MASK),
	};

	enum SPDIF
	{
		SPDIF_LOOP_DIV = 28,
		SPDIF_STC_DIV = 29,
		GPIO_AD_B1_02_MUX_MODE_SPDIF = 0b011
	};
};
