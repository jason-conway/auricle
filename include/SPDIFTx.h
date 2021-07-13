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

#ifndef SPDIFTx_H
#define SPDIFTx_H

#include <WProgram.h>
#include <AudioStream.h>
#include <DMAChannel.h>

#define SPDIF_DPLL_GAIN 0b11
#define SPDIF_CLK_PRED_DIV 7
#define SPDIF_CLK_PODF_DIV 0
#define SPDIF_LOOP_DIV 28
#define SPDIF_PLL_NUM 2240
#define SPDIF_PLL_DENOM 10000

#define SPDIF_STC_DIV 29

#define CCM_CDCR_SPDIF0_CLK_MASK (CCM_CDCDR_SPDIF0_CLK_SEL_MASK | CCM_CDCDR_SPDIF0_CLK_PRED_MASK | CCM_CDCDR_SPDIF0_CLK_PODF_MASK)
#define CCM_CDCR_SPDIF0_CLK_SEL_PLL4 0b00
#define CCM_CDCR_SPDIF0_CLK_PRED_DIV 7
#define CCM_CDCR_SPDIF0_CLK_PODF_DIV 0

#define GPIO_AD_B1_02_MUX_MODE_SPDIF 0b011


class SPDIFTx : public AudioStream
{
public:
	SPDIFTx(void) : AudioStream(2, inputQueueArray)
	{
		begin();
	}

	virtual void update(void);

protected:
	static void configureSpdifRegisters(void);
	static void configureDMA(void);
	static void dmaISR(void);
	static DMAChannel eDMA;

private:
	audio_block_t *inputQueueArray[2];

	void begin(void);

	enum Channels
	{
		STEREO_LEFT,
		STEREO_RIGHT
	};

	static audio_block_t *leftAudioMSB;
	static audio_block_t *rightAudioMSB;
	static audio_block_t *leftAudioLSB;
	static audio_block_t *rightAudioLSB;

	static audio_block_t silentAudio;
};

#endif
