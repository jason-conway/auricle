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

audio_block_t *SPDIFTx::leftAudioMSB = nullptr;
audio_block_t *SPDIFTx::rightAudioMSB = nullptr;
audio_block_t *SPDIFTx::leftAudioLSB = nullptr;
audio_block_t *SPDIFTx::rightAudioLSB = nullptr;

DMAChannel SPDIFTx::eDMA(false);

/**
 * @brief 
 * 
 */
void __attribute__((section(".flashmem"))) SPDIFTx::begin(void)
{
    configureDMA();
    configureSpdifRegisters();

    eDMA.triggerAtHardwareEvent(DMAMUX_SOURCE_SPDIF_TX);
    update_setup();

    eDMA.enable();
    eDMA.attachInterrupt(dmaISR);

    SPDIF_SCR |= SPDIF_SCR_DMA_TX_EN;     // DMA Transmit Request Enable
    SPDIF_STC |= SPDIF_STC_TX_ALL_CLK_EN; // SPDIF Transfer Clock Enable

    memset(&silentAudio, 0, sizeof(silentAudio));
}

/**
 * @brief 
 * 
 */
void SPDIFTx::dmaISR(void)
{
    uint32_t srcAddress = (uint32_t)(eDMA.TCD->SADDR);

    eDMA.clearInterrupt();

    int32_t *txBaseAddress = (srcAddress < ((uint32_t)txBuffer + 1024)) ? (txBuffer + 256) : (txBuffer);
    const int32_t *txEndAddress = (srcAddress < ((uint32_t)txBuffer + 1024)) ? (txBuffer + 512) : (txBuffer + 256);

    audio_block_t *leftAudio = (leftAudioMSB) ? leftAudioMSB : &silentAudio;
    audio_block_t *rightAudio = (rightAudioMSB) ? rightAudioMSB : &silentAudio;

    const int16_t *leftAudioData = (const int16_t *)(leftAudio->data);
    const int16_t *rightAudioData = (const int16_t *)(rightAudio->data);

    do
    {
        SCB_CACHE_DCCIMVAC = (uintptr_t)txBaseAddress; // D-cache clean and invalidate by MVA to PoC
        __asm__ volatile("dsb");                       // Sync execution stream

        *txBaseAddress++ = (*leftAudioData++) << 8;
        *txBaseAddress++ = (*rightAudioData++) << 8;

        *txBaseAddress++ = (*leftAudioData++) << 8;
        *txBaseAddress++ = (*rightAudioData++) << 8;

        *txBaseAddress++ = (*leftAudioData++) << 8;
        *txBaseAddress++ = (*rightAudioData++) << 8;

        *txBaseAddress++ = (*leftAudioData++) << 8;
        *txBaseAddress++ = (*rightAudioData++) << 8;

    } while (txBaseAddress < txEndAddress);

    if (leftAudio != &silentAudio && rightAudio != &silentAudio)
    {
        release(leftAudio);
        release(rightAudio);

        leftAudioMSB = leftAudioLSB;
        rightAudioMSB = rightAudioLSB;

        leftAudioLSB = nullptr;
        rightAudioLSB = nullptr;
    }

    update_all();
}

void SPDIFTx::update(void)
{
    audio_block_t *leftAudio = receiveReadOnly(STEREO_LEFT);
    audio_block_t *rightAudio = receiveReadOnly(STEREO_RIGHT);

    __asm__ volatile("CPSID i":::"memory");

    if (leftAudio && rightAudio)
    {
        // Left channel switcheroo
        if (leftAudioMSB == nullptr)
        {
            leftAudioMSB = leftAudio;
            leftAudio = nullptr;
        }
        else if (leftAudioLSB == nullptr)
        {
            leftAudioLSB = leftAudio;
            leftAudio = nullptr;
        }
        else
        {
            audio_block_t *shiftTemp = leftAudioMSB;
            leftAudioMSB = leftAudioLSB;
            leftAudioLSB = leftAudio;
            leftAudio = shiftTemp;
        }

        // Right channel switcheroo
        if (rightAudioMSB == nullptr)
        {
            rightAudioMSB = rightAudio;
            rightAudio = nullptr;
        }
        else if (rightAudioLSB == nullptr)
        {
            rightAudioLSB = rightAudio;
            rightAudio = nullptr;
        }
        else
        {
            audio_block_t *shiftTemp = rightAudioMSB;
            rightAudioMSB = rightAudioLSB;
            rightAudioLSB = rightAudio;
            rightAudio = shiftTemp;
        }
    }

    __asm__ volatile("CPSIE i":::"memory");

    if (leftAudio && rightAudio)
    {
        release(leftAudio);
        release(rightAudio);
    }
}

/**
 * @brief Initialize eDMA and configure the Transfer Control Descriptor (TCD)
 * 
 */
void SPDIFTx::configureDMA(void)
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
    eDMA.TCD->SLAST = -2048; // Size of the transmission buffer

    // TCD Destination Address (pg 164)
    eDMA.TCD->DADDR = &SPDIF_STL; // DMA channel destination address is audio data transmission register for the left SPDIF channel

    // TCD Signed Destination Address Offset (pg 165)
    eDMA.TCD->DOFF = 4; // int32_t => 4 bytes

    // TCD Current Minor Loop Link, Major Loop Count (pg 165)
    eDMA.TCD->CITER_ELINKNO = 256; // Must be equal to BITER

    // TCD Last Destination Address Adjustment/Scatter Gather Address (pg 168)
    eDMA.TCD->DLASTSGA = -8; //

    // TCD Beginning Minor Loop Link, Major Loop Count (pg 171)
    eDMA.TCD->BITER_ELINKNO = 256; //

    // TCD Control and Status (pg 169)
    eDMA.TCD->CSR =
        DMA_TCD_CSR_INTHALF | // Enable an interrupt when major counter is half complete
        DMA_TCD_CSR_INTMAJOR; // Enable an interrupt when major iteration count completes
}

void __attribute__((section(".flashmem"))) SPDIFTx::configureSpdifRegisters(void)
{
    uint32_t startingCount = ARM_DWT_CYCCNT;
    while (ARM_DWT_CYCCNT - startingCount < 396000)
        ;

    // Analog Audio PLL control Register (pg 1110)
    CCM_ANALOG_PLL_AUDIO =
        CCM_ANALOG_PLL_AUDIO_BYPASS | // Bypass the PLL
        CCM_ANALOG_PLL_AUDIO_ENABLE | // Enable PLL output
        CCM_ANALOG_PLL_AUDIO_POST_DIV_SELECT(0b10) |
        CCM_ANALOG_PLL_AUDIO_DIV_SELECT(SPDIF_LOOP_DIV); // PLL loop divider

    // Numerator and Denominator of Audio PLL Fractional Loop Divider Register (pg 1112)
    CCM_ANALOG_PLL_AUDIO_NUM = CCM_ANALOG_PLL_AUDIO_NUM_MASK & SPDIF_PLL_NUM;
    CCM_ANALOG_PLL_AUDIO_DENOM = CCM_ANALOG_PLL_AUDIO_DENOM_MASK & SPDIF_PLL_DENOM;

    CCM_ANALOG_PLL_AUDIO &= ~CCM_ANALOG_PLL_AUDIO_POWERDOWN; // ~Powers down the PLL.

    while (!(CCM_ANALOG_PLL_AUDIO & CCM_ANALOG_PLL_AUDIO_LOCK))
        ; // Wait for PLL lock

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
        while (SPDIF_SCR & SPDIF_SCR_SOFT_RESET)
            ; // Returns one during the reset
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
