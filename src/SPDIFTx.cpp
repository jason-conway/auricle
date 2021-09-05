/**
 * @file SPDIFTx.cpp
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief IMXRT1060 HW S/PDIF
 * @version 0.1
 * @date 2021-06-22
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#include "SPDIFTx.h"

// S/PDIF transmit buffer
// TCD0_SADDR -> TCD31_SADDR
static int32_t __attribute__((section(".dmabuffers"), used)) __attribute__((aligned(32))) txBuffer[512];
audio_block_t __attribute__((section(".dmabuffers"), used)) __attribute__((aligned(32))) SPDIFTx::silentAudio;

audio_block_t *SPDIFTx::leftAudioBuffer[];
audio_block_t *SPDIFTx::rightAudioBuffer[];

DMAChannel SPDIFTx::eDMA(false);

/**
 * @brief Construct a new SPDIFTx::SPDIFTx object
 * 
 */
SPDIFTx::SPDIFTx(void) : AudioStream(2, inputQueueArray)
{
	init();
}

/**
 * @brief 
 * 
 */
void __attribute__((section(".flashmem"))) SPDIFTx::init(void)
{
	dmaChannel = configureDMA();
	configureSpdifRegisters();

	eDMA.triggerAtHardwareEvent(DMAMUX_SOURCE_SPDIF_TX);

	// Set Enable Request Register (pg 134)
	DMA_SERQ = dmaChannel;

	update_setup();

	eDMA.attachInterrupt(dmaISR);

	SPDIF_SCR |= SPDIF_SCR_DMA_TX_EN;     // DMA Transmit Request Enable
	SPDIF_STC |= SPDIF_STC_TX_ALL_CLK_EN; // SPDIF Transfer Clock Enable

	memset(&silentAudio, 0, sizeof(silentAudio));
	memset(inputQueueArray, 0, sizeof(inputQueueArray));

	for (size_t i = 0; i < 2; i++)
	{
		leftAudioBuffer[i] = nullptr;
		rightAudioBuffer[i] = nullptr;
	}
}

/**
 * @brief This ISR is triggered twice per major loop since the TCD Control and Status
 * register is set with DMA_TCD_CSR_INTMAJOR and DMA_TCD_CSR_INTHALF
 * 
 */
void SPDIFTx::dmaISR(void)
{
	uint32_t txOffset = getTxOffset((uint32_t)&txBuffer[0], 2048);

	// Clear Interrupt Request Register (pg 138)
	DMA_CINT = eDMA.channel; // Disable interrupt request for this DMA channel

	int32_t *txBaseAddress = &txBuffer[0] + txOffset;
	const int32_t *txStopAddress = &txBuffer[0] + txOffset + 256;

	audio_block_t *leftAudio = (leftAudioBuffer[0]) ? leftAudioBuffer[0] : &silentAudio;
	audio_block_t *rightAudio = (rightAudioBuffer[0]) ? rightAudioBuffer[0] : &silentAudio;

	dmaCopyAudio(txBaseAddress, txStopAddress, (const int16_t *)(leftAudio->data), (const int16_t *)(rightAudio->data));

	if (leftAudio != &silentAudio && rightAudio != &silentAudio)
	{
		release(leftAudio);
		release(rightAudio);

		leftAudioBuffer[0] = leftAudioBuffer[1];
		leftAudioBuffer[1] = nullptr;
		rightAudioBuffer[0] = rightAudioBuffer[1];
		rightAudioBuffer[1] = nullptr;
	}

	update_all();
}

/**
 * @brief 
 * 
 */
void SPDIFTx::update(void)
{
	audio_block_t *leftAudio = receiveReadOnly(leftChannel);
	audio_block_t *rightAudio = receiveReadOnly(rightChannel);

	__asm__ volatile("CPSID i" ::: "memory");

	if (leftAudio && rightAudio)
	{
		// Doing buffer packing in a loop is too slow, has to be done like this 
		// Left channel switcheroo
		if (leftAudioBuffer[0] == nullptr)
		{
			leftAudioBuffer[0] = leftAudio;
			leftAudio = nullptr;
		}
		else if (leftAudioBuffer[1] == nullptr)
		{
			leftAudioBuffer[1] = leftAudio;
			leftAudio = nullptr;
		}
		else
		{
			audio_block_t *shiftTemp = leftAudioBuffer[0];
			leftAudioBuffer[0] = leftAudioBuffer[1];
			leftAudioBuffer[1] = leftAudio;
			leftAudio = shiftTemp;
		}

		// Right channel switcheroo
		if (rightAudioBuffer[0] == nullptr)
		{
			rightAudioBuffer[0] = rightAudio;
			rightAudio = nullptr;
		}
		else if (rightAudioBuffer[1] == nullptr)
		{
			rightAudioBuffer[1] = rightAudio;
			rightAudio = nullptr;
		}
		else
		{
			audio_block_t *shiftTemp = rightAudioBuffer[0];
			rightAudioBuffer[0] = rightAudioBuffer[1];
			rightAudioBuffer[1] = rightAudio;
			rightAudio = shiftTemp;
		}
	}

	__asm__ volatile("CPSIE i" ::: "memory");

	if (leftAudio && rightAudio)
	{
		release(leftAudio);
		release(rightAudio);
	}
}

/**
 * @brief Copy audio data for left and right channels into txBuffer for 
 * 
 * @param pTx 
 * @param pTxStop 
 * @param leftAudioData 
 * @param rightAudioData 
 * @note With for loop: 16.90 uS
 * 
 */
inline void __attribute__((optimize("-O1"))) dmaCopyAudio(int32_t *pTx, const int32_t *pTxStop, const int16_t *leftAudioData, const int16_t *rightAudioData)
{
	do
	{
		SCB_CACHE_DCCIMVAC = (uintptr_t)pTx; // D-cache clean and invalidate by MVA to PoC
		__asm__ volatile("dsb");             // Sync execution stream
		*pTx++ = (*leftAudioData++) << 8;
		*pTx++ = (*rightAudioData++) << 8;
		*pTx++ = (*leftAudioData++) << 8;
		*pTx++ = (*rightAudioData++) << 8;
		*pTx++ = (*leftAudioData++) << 8;
		*pTx++ = (*rightAudioData++) << 8;
		*pTx++ = (*leftAudioData++) << 8;
		*pTx++ = (*rightAudioData++) << 8;
	} while (pTx < pTxStop);
}

/**
 * @brief 
 * 
 * @param txSourceAddress 
 * @param sourceBufferSize 
 * @return uint16_t 
 */
inline uint32_t SPDIFTx::getTxOffset(uint32_t txSourceAddress, uint32_t sourceBufferSize)
{
	// Set an offset when SADDR is in the second half of the major loop
	uint32_t txOffset = 0;
	uint32_t SADDR = (uint32_t)(eDMA.TCD->SADDR);
	if (SADDR < (txSourceAddress + sourceBufferSize / 2))
	{
		txOffset = 0x0100; // Apply 256 byte pointer offset
	}

	return txOffset;
}

/**
 * @brief Initialize eDMA and configure the Transfer Control Descriptor (TCD)
 * 
 * @return Returns the eDMA channel number
 */
uint8_t SPDIFTx::configureDMA(void)
{
	eDMA.begin(true);
	// TCD Source Address (pg 156)
	eDMA.TCD->SADDR = &txBuffer[0]; // DMA channel source address starts at the beginning of the transmission buffer

	// TCD Signed Source Address Offset (pg 157)
	eDMA.TCD->SOFF = 4; // int32_t => 4 bytes

	// TCD Transfer Attributes (pg 157)
	eDMA.TCD->ATTR =
		DMA_TCD_ATTR_SSIZE(0b10) | // Source data transfer size is 32-bit
		DMA_TCD_ATTR_DSIZE(0b10);  // Destination data transfer size is 32-bit

	// TCD Signed Minor Loop Offset (pg 161)
	eDMA.TCD->NBYTES_MLNO =
		DMA_TCD_NBYTES_DMLOE |              // Apply minor loop offset to the destination address
		DMA_TCD_NBYTES_MLOFFYES_MLOFF(-8) | // Minor loop offset size
		DMA_TCD_NBYTES_MLOFFYES_NBYTES(8);  // Transfer 8 bytes for each service request

	// TCD Last Source Address Adjustment (pg 163)
	eDMA.TCD->SLAST = -2048; // 2048 bytes to move SADDR back to &txBuffer[0]

	// TCD Destination Address (pg 164)
	eDMA.TCD->DADDR = &SPDIF_STL; // DMA channel destination address is audio data transmission register for the left SPDIF channel

	// TCD Signed Destination Address Offset (pg 165)
	eDMA.TCD->DOFF = 4; // int32_t => 4 bytes

	// TCD Current Minor Loop Link, Major Loop Count (pg 165)
	eDMA.TCD->CITER_ELINKNO = 256; // Must be equal to BITER

	// TCD Last Destination Address Adjustment/Scatter Gather Address (pg 168)
	eDMA.TCD->DLASTSGA = -8; // 8 bytes to move DADDR back to &SPDIF_STL

	// TCD Beginning Minor Loop Link, Major Loop Count (pg 171)
	eDMA.TCD->BITER_ELINKNO = 256; //

	// TCD Control and Status (pg 169)
	eDMA.TCD->CSR =
		DMA_TCD_CSR_INTHALF | // Enable an interrupt when major counter is half complete
		DMA_TCD_CSR_INTMAJOR; // Enable an interrupt when major iteration count completes

	return eDMA.channel;
}

void __attribute__((section(".flashmem"))) SPDIFTx::configureSpdifRegisters(void)
{
	uint32_t startingCount = ARM_DWT_CYCCNT;
	while (ARM_DWT_CYCCNT - startingCount < 396000);

	// Analog Audio PLL control Register (pg 1110)
	CCM_ANALOG_PLL_AUDIO =
		CCM_ANALOG_PLL_AUDIO_BYPASS |                    // Bypass the PLL
		CCM_ANALOG_PLL_AUDIO_ENABLE |                    // Enable PLL output
		CCM_ANALOG_PLL_AUDIO_POST_DIV_SELECT(0b10) |     // 0b10 — Divide by 1
		CCM_ANALOG_PLL_AUDIO_DIV_SELECT(SPDIF_LOOP_DIV); // PLL loop divider

	// Numerator and Denominator of Audio PLL Fractional Loop Divider Register (pg 1112)
	CCM_ANALOG_PLL_AUDIO_NUM = CCM_ANALOG_PLL_AUDIO_NUM_MASK & SPDIF_PLL_NUM;
	CCM_ANALOG_PLL_AUDIO_DENOM = CCM_ANALOG_PLL_AUDIO_DENOM_MASK & SPDIF_PLL_DENOM;

	CCM_ANALOG_PLL_AUDIO &= ~CCM_ANALOG_PLL_AUDIO_POWERDOWN; // ~Powers down the PLL.

	while (!(CCM_ANALOG_PLL_AUDIO & CCM_ANALOG_PLL_AUDIO_LOCK)); // Wait for PLL lock

	// Miscellaneous Register 2 (pg 1132)
	CCM_ANALOG_MISC2 &= ~(CCM_ANALOG_MISC2_AUDIO_DIV_MSB | CCM_ANALOG_MISC2_AUDIO_DIV_LSB);

	CCM_ANALOG_PLL_AUDIO &= ~CCM_ANALOG_PLL_AUDIO_BYPASS; //Disable Bypass

	// CCM Clock Gating Register 5 (pg 1090)
	CCM_CCGR5 &= ~CCM_CCGR5_SPDIF(CCM_CCGR_ON); // Gate clock before setting CCM_CDCDR

	// CCM D1 Clock Divider Register (pg 1065)
	CCM_CDCDR =
		(CCM_CDCDR & ~(CCM_CDCR_SPDIF0_CLK_MASK)) |               // CLK_SEL, CLK_PRED, and CLK_PODF masks
		CCM_CDCDR_SPDIF0_CLK_SEL(CCM_CDCR_SPDIF0_CLK_SEL_PLL4) |  // Derive clock from PLL4
		CCM_CDCDR_SPDIF0_CLK_PRED(CCM_CDCR_SPDIF0_CLK_PRED_DIV) | // Divider for spdif0 clock pred
		CCM_CDCDR_SPDIF0_CLK_PODF(CCM_CDCR_SPDIF0_CLK_PODF_DIV);  // Divider for spdif0 clock podf

	CCM_CCGR5 |= CCM_CCGR5_SPDIF(CCM_CCGR_ON); // Remove gate

	if (!(SPDIF_SCR & (SPDIF_SCR_DMA_RX_EN | SPDIF_SCR_DMA_TX_EN)))
	{
		SPDIF_SCR = SPDIF_SCR_SOFT_RESET; // SPDIF software reset
		while (SPDIF_SCR & SPDIF_SCR_SOFT_RESET); // Returns one during while resetting
	}
	else
	{
		return;
	}

	// SPDIF Configuration Register (pg 2037)
	SPDIF_SCR =
		SPDIF_SCR_RXFIFOFULL_SEL(0b00) |  // Full interrupt if at least 1 sample in Rx left and right FIFOs
		SPDIF_SCR_RXAUTOSYNC |            // Rx FIFO auto sync on
		SPDIF_SCR_TXAUTOSYNC |            // Tx FIFO auto sync on
		SPDIF_SCR_TXFIFOEMPTY_SEL(0b10) | // Empty interrupt if at most 8 samples in Tx left and right FIFOs
		SPDIF_SCR_TXFIFO_CTRL(0b01) |     // Tx Normal operation
		SPDIF_SCR_VALCTRL |               // Outgoing Validity always clear
		SPDIF_SCR_TXSEL(0b101) |          // Tx Normal operation
		SPDIF_SCR_USRC_SEL(0b11);         // U channel from on chip transmitter

	// PhaseConfig Register (pg 2040)
	SPDIF_SRPC =
		SPDIF_SRPC_CLKSRC_SEL(0b001) |       // Clock source selection if (DPLL Locked) SPDIF_RxClk else tx_clk (SPDIF0_CLK_ROOT)
		SPDIF_SRPC_GAINSEL(SPDIF_DPLL_GAIN); // Gain selection

	// SPDIFTxClk Register (pg 2052)
	SPDIF_STC =
		SPDIF_STC_TXCLK_SOURCE(0b001) |    // tx_clk input (from SPDIF0_CLK_ROOT)
		SPDIF_STC_TXCLK_DF(SPDIF_STC_DIV); // Divider factor (1-128)

	// SW_MUX_CTL_PAD_GPIO_AD_B1_02 SW MUX Control Register (pg 494)
	IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_02 = GPIO_AD_B1_02_MUX_MODE_SPDIF;
}
