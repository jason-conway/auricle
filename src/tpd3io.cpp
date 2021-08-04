/**
 * @file tpd3io.cpp
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief Spatial Audio for Arm Cortex-M7
 * @version 0.1
 * @date 2021-07-11
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#include "tpd3io.h"

/**
 * @brief Construct a new TPD3IO::TPD3IO object
 * 
 */
TPD3IO::TPD3IO(void)
{
    init();
}

/**
 * @brief Configure GPIO for interfacing with the D3
 * 
 */
void __attribute__((section(".flashmem"))) TPD3IO::init(void)
{
    // Outputs on GPIO6, inputs on GPIO9
    volatile uint32_t *gpio_pow_reg = &GPIO_DR_SIG_OUT;
    volatile uint32_t *gpio_sel_reg = &GPIO_DR_SIG_OUT;
    volatile uint32_t *gpio_usb_reg = &GPIO_DR_SIG_IN;
    volatile uint32_t *gpio_opt_reg = &GPIO_DR_SIG_IN;
    volatile uint32_t *gpio_rca_reg = &GPIO_DR_SIG_IN;
    volatile uint32_t *gpio_bnc_reg = &GPIO_DR_SIG_IN;

    volatile uint32_t *gpio_pow_mux = &GPIO_IOMUX_SIG_POW;
    volatile uint32_t *gpio_sel_mux = &GPIO_IOMUX_SIG_SEL;
    volatile uint32_t *gpio_usb_mux = &GPIO_IOMUX_SIG_USB;
    volatile uint32_t *gpio_opt_mux = &GPIO_IOMUX_SIG_OPT;
    volatile uint32_t *gpio_rca_mux = &GPIO_IOMUX_SIG_RCA;
    volatile uint32_t *gpio_bnc_mux = &GPIO_IOMUX_SIG_BNC;

    volatile uint32_t *gpio_pow_pad = &GPIO_PAD_SIG_POW;
    volatile uint32_t *gpio_sel_pad = &GPIO_PAD_SIG_SEL;
    volatile uint32_t *gpio_usb_pad = &GPIO_PAD_SIG_USB;
    volatile uint32_t *gpio_opt_pad = &GPIO_PAD_SIG_OPT;
    volatile uint32_t *gpio_rca_pad = &GPIO_PAD_SIG_RCA;
    volatile uint32_t *gpio_bnc_pad = &GPIO_PAD_SIG_BNC;

    *(gpio_pow_reg + 1) |= GPIO_MASK_SIG_POW;
    *(gpio_pow_pad) = IOMUXC_PAD_DSE(7);
    *(gpio_pow_mux) = GPIO_MUX_MODE_ALT5;

    *(gpio_sel_reg + 1) |= GPIO_MASK_SIG_SEL;
    *(gpio_sel_pad) = IOMUXC_PAD_DSE(7);
    *(gpio_sel_mux) = GPIO_MUX_MODE_ALT5;

    *(gpio_usb_reg + 1) &= ~GPIO_MASK_SIG_USB;
    *(gpio_usb_pad) = IOMUXC_PAD_DSE(7);
    *(gpio_usb_mux) = GPIO_MUX_MODE_ALT5;

    *(gpio_opt_reg + 1) &= ~GPIO_MASK_SIG_OPT;
    *(gpio_opt_pad) = IOMUXC_PAD_DSE(7);
    *(gpio_opt_mux) = GPIO_MUX_MODE_ALT5;

    *(gpio_rca_reg + 1) &= ~GPIO_MASK_SIG_RCA;
    *(gpio_rca_pad) = IOMUXC_PAD_DSE(7);
    *(gpio_rca_mux) = GPIO_MUX_MODE_ALT5;

    *(gpio_bnc_reg + 1) &= ~GPIO_MASK_SIG_BNC;
    *(gpio_bnc_pad) = IOMUXC_PAD_DSE(7);
    *(gpio_bnc_mux) = GPIO_MUX_MODE_ALT5;

    d3status.mode = MODE_NULL;
    d3status.power = POWER_OFF;

}

/**
 * @brief Write SIG_POW HIGH for 50ms to toggle power on the D3 board
 * 
 */
void __attribute__((section(".flashmem"))) TPD3IO::togglePower(void)
{
    GPIO6_DR_SET = GPIO_MASK_SIG_POW;
    delayMicroseconds(50000);
    GPIO6_DR_CLEAR = GPIO_MASK_SIG_POW;
    d3status.power = (d3status.power) ? POWER_OFF : POWER_ON;
}

/**
 * @brief Write SIG_SEL HIGH for 50ms switch inputs on the D3 board
 * 
 */
void __attribute__((section(".flashmem"))) TPD3IO::switchInput(void)
{
    GPIO6_DR_SET = GPIO_MASK_SIG_SEL;
    delayMicroseconds(50000);
    GPIO6_DR_CLEAR = GPIO_MASK_SIG_SEL;
}

/**
 * @brief Check if SIG_USB is HIGH
 * 
 * @return uint8_t 
 */
uint8_t __attribute__((section(".flashmem"))) TPD3IO::checkSigUSB(void)
{
    if (GPIO_PSR_SIG_IN & GPIO_MASK_SIG_USB)
    {
        d3status.mode = USB;
        return 1;
    }
    return 0;
}

/**
 * @brief Check if SIG_OPT is HIGH
 * 
 * @return uint8_t 
 */
uint8_t __attribute__((section(".flashmem"))) TPD3IO::checkSigOPT(void)
{
    if (GPIO_PSR_SIG_IN & GPIO_MASK_SIG_OPT)
    {
        d3status.mode = OPT;
        return 1;
    }
    return 0;
}

/**
 * @brief Check if SIG_RCA is HIGH
 * 
 * @return uint8_t 
 */
uint8_t __attribute__((section(".flashmem"))) TPD3IO::checkSigRCA(void)
{
    if (GPIO_PSR_SIG_IN & GPIO_MASK_SIG_RCA)
    {
        d3status.mode = RCA;
        return 1;
    }
    return 0;
}

/**
 * @brief Check if SIG_BNC is HIGH
 * 
 * @return uint8_t 
 */
uint8_t __attribute__((section(".flashmem"))) TPD3IO::checkSigBNC(void)
{
    if (GPIO_PSR_SIG_IN & GPIO_MASK_SIG_BNC)
    {
        d3status.mode = BNC;
        return 1;
    }
    return 0;
}

/**
 * @brief Check if the D3's PLL has locked in
 * 
 * @return uint8_t 
 */
uint8_t __attribute__((section(".flashmem"))) TPD3IO::pllStatus(void)
{
    if (d3status.mode == OPT)
    {
        for (size_t i = 0; i < 8; i++) // 2 seconds
        {
            uint32_t startingCycleCount = ARM_DWT_CYCCNT;

            if (!(checkSigOPT())) // SIG_OPT remains HIGH >= 2 seconds
            {
                return D3_PLL_NOT_LOCKED;
            }
            while (ARM_DWT_CYCCNT - startingCycleCount < (uint32_t)99000000); // 250ms

        } 
        return D3_PLL_LOCKED;
    }
    return D3_INCORRECT_MODE;
}