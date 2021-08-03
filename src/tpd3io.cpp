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

TPD3IO::TPD3IO(void)
{
    init();
}

/**
 * @brief 
 * 
 */
void TPD3IO::init(void)
{
    // Outputs on GPIO6, inputs on GPIO9
    volatile uint32_t *gpio_pow_reg = &GPIO_REG_SIG_OUT;
    volatile uint32_t *gpio_sel_reg = &GPIO_REG_SIG_OUT;
    volatile uint32_t *gpio_usb_reg = &GPIO_REG_SIG_IN;
    volatile uint32_t *gpio_opt_reg = &GPIO_REG_SIG_IN;
    volatile uint32_t *gpio_rca_reg = &GPIO_REG_SIG_IN;
    volatile uint32_t *gpio_bnc_reg = &GPIO_REG_SIG_IN;

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
    *(gpio_pow_mux) = GPIO_MUX;

    *(gpio_sel_reg + 1) |= GPIO_MASK_SIG_SEL;
    *(gpio_sel_pad) = IOMUXC_PAD_DSE(7);
    *(gpio_sel_mux) = GPIO_MUX;

    *(gpio_usb_reg + 1) &= ~GPIO_MASK_SIG_USB;
    *(gpio_usb_pad) = IOMUXC_PAD_DSE(7);
    *(gpio_usb_mux) = GPIO_MUX;

    *(gpio_opt_reg + 1) &= ~GPIO_MASK_SIG_OPT;
    *(gpio_opt_pad) = IOMUXC_PAD_DSE(7);
    *(gpio_opt_mux) = GPIO_MUX;

    *(gpio_rca_reg + 1) &= ~GPIO_MASK_SIG_RCA;
    *(gpio_rca_pad) = IOMUXC_PAD_DSE(7);
    *(gpio_rca_mux) = GPIO_MUX;

    *(gpio_bnc_reg + 1) &= ~GPIO_MASK_SIG_BNC;
    *(gpio_bnc_pad) = IOMUXC_PAD_DSE(7);
    *(gpio_bnc_mux) = GPIO_MUX;

}

void TPD3IO::powerOn(void)
{
    GPIO6_DR_SET = GPIO_MASK_SIG_POW;
    delayMicroseconds(1250);
    GPIO6_DR_CLEAR = GPIO_MASK_SIG_POW;
}

void TPD3IO::switchInput(void)
{
    GPIO6_DR_SET = GPIO_MASK_SIG_SEL;
    delayMicroseconds(1250);
    GPIO6_DR_CLEAR = GPIO_MASK_SIG_SEL;
}