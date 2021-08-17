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

	d3status.mode = ModeNull;
	d3status.power = PowerNull;
}

/**
 * @brief Write SIG_POW HIGH for 50ms to toggle power on the D3 board
 * 
 */
void __attribute__((section(".flashmem"))) TPD3IO::togglePower(void)
{
	GPIO6_DR_SET = GPIO_MASK_SIG_POW;
	this->constDelay();
	GPIO6_DR_CLEAR = GPIO_MASK_SIG_POW;

	this->checkAll();

	d3status.power = (d3status.mode) ? PowerOn : PowerOff;
}

/**
 * @brief Write SIG_SEL HIGH for 50ms switch inputs on the D3 board
 * 
 */
void __attribute__((section(".flashmem"))) TPD3IO::switchInput(void)
{
	SerialUSB.printf("Current Input: %s\r\n", d3status.mode);
	GPIO6_DR_SET = GPIO_MASK_SIG_SEL;
	this->constDelay();
	GPIO6_DR_CLEAR = GPIO_MASK_SIG_SEL;
	SerialUSB.printf("New Input: %s\r\n", d3status.mode);
}

/**
 * @brief Report current status of the D3
 * 
 */
void __attribute__((section(".flashmem"))) TPD3IO::currentStatus(void)
{
	if (d3status.power == PowerOn)
	{
		this->checkAll();
		SerialUSB.printf("Current Input: ");
		switch (d3status.mode)
		{
		case ModeUSB:
			SerialUSB.printf("USB\r\n");
			break;
		case ModeOPT:
			SerialUSB.printf("Optical\r\n");
			SerialUSB.printf("PLL is %s\r\n", (this->pllStatus() == Locked) ? "Locked" : "Not Locked");
			break;
		case ModeRCA:
			SerialUSB.printf("RCA\r\n");
			break;
		case ModeBNC:
			SerialUSB.printf("BNC\r\n");
			break;
		default:
			SerialUSB.printf("Mode Unknown\r\n");
			break;
		}
	}
	else
	{
		SerialUSB.printf("D3 is not powered on\r\n");
	}
}

/**
 * @brief Check all D3 inputs and set d3status.mode
 * 
 */
void __attribute__((section(".flashmem"))) TPD3IO::checkAll(void)
{
	if (GPIO_PSR_SIG_IN & GPIO_MASK_SIG_USB)
	{
		d3status.mode = ModeUSB;
	}
	else if (GPIO_PSR_SIG_IN & GPIO_MASK_SIG_OPT)
	{
		d3status.mode = ModeOPT;
	}
	else if (GPIO_PSR_SIG_IN & GPIO_MASK_SIG_RCA)
	{
		d3status.mode = ModeRCA;
	}
	else if (GPIO_PSR_SIG_IN & GPIO_MASK_SIG_BNC)
	{
		d3status.mode = ModeBNC;
	}
	else
	{
		d3status.mode = ModeNull;
	}
}

/**
 * @brief Check if the D3's PLL is locked
 * 
 * @return Returns D3_PLL_LOCKED if locked, otherwise D3_PLL_NOT_LOCKED
 */
uint8_t __attribute__((section(".flashmem"))) TPD3IO::pllStatus(void)
{
	for (size_t i = 0; i < 8; i++) // 2 seconds
	{
		this->constDelay();

		if (!(GPIO_PSR_SIG_IN & GPIO_MASK_SIG_OPT)) // SIG_OPT remains HIGH >= 2 seconds
		{
			return NotLocked;
		}
	}
	return Locked;
}

/**
 * @brief Simple 250ms delay. Waits for 99,000,000 clock cycles
 * 
 */
void __attribute__((section(".flashmem"))) TPD3IO::constDelay(void)
{
	uint32_t startingCycleCount = (*(volatile uint32_t *)0xE0001004);
	while ((*(volatile uint32_t *)0xE0001004) - startingCycleCount < (uint32_t)99000000); // 250ms
}