/**
 * @file d3io.c
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief Spatial Audio for Arm Cortex-M7
 * @version 0.1
 * @date 2021-07-11
 *
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 *
 */

#include "d3io.h"

typedef enum d3Input_t
{
	ModeNull,
	ModeUSB,
	ModeOPT,
	ModeRCA,
	ModeBNC
} d3Input_t;

typedef enum d3Power_t
{
	PowerNull,
	PowerOff,
	PowerOn
} d3Power_t;

enum D3_PLL_Return_Codes
{
	NotLocked,
	Locked
};

typedef struct d3State_t
{
	d3Power_t power;
	d3Input_t input;
} d3State_t;

d3State_t d3state;

/**
 * @brief Configure GPIO for interfacing with the D3
 *
 */

void d3initGPIO(void)
{
	volatile uint32_t *gpio_pow_reg = (volatile uint32_t *)(&GPIO_DR_SIG_OUT);
	volatile uint32_t *gpio_sel_reg = (volatile uint32_t *)(&GPIO_DR_SIG_OUT);
	volatile uint32_t *gpio_usb_reg = (volatile uint32_t *)(&GPIO_DR_SIG_IN);
	volatile uint32_t *gpio_opt_reg = (volatile uint32_t *)(&GPIO_DR_SIG_IN);
	volatile uint32_t *gpio_rca_reg = (volatile uint32_t *)(&GPIO_DR_SIG_IN);
	volatile uint32_t *gpio_bnc_reg = (volatile uint32_t *)(&GPIO_DR_SIG_IN);

	volatile uint32_t *gpio_pow_mux = (volatile uint32_t *)(&GPIO_IOMUX_SIG_POW);
	volatile uint32_t *gpio_sel_mux = (volatile uint32_t *)(&GPIO_IOMUX_SIG_SEL);
	volatile uint32_t *gpio_usb_mux = (volatile uint32_t *)(&GPIO_IOMUX_SIG_USB);
	volatile uint32_t *gpio_opt_mux = (volatile uint32_t *)(&GPIO_IOMUX_SIG_OPT);
	volatile uint32_t *gpio_rca_mux = (volatile uint32_t *)(&GPIO_IOMUX_SIG_RCA);
	volatile uint32_t *gpio_bnc_mux = (volatile uint32_t *)(&GPIO_IOMUX_SIG_BNC);

	volatile uint32_t *gpio_pow_pad = (volatile uint32_t *)(&GPIO_PAD_SIG_POW);
	volatile uint32_t *gpio_sel_pad = (volatile uint32_t *)(&GPIO_PAD_SIG_SEL);
	volatile uint32_t *gpio_usb_pad = (volatile uint32_t *)(&GPIO_PAD_SIG_USB);
	volatile uint32_t *gpio_opt_pad = (volatile uint32_t *)(&GPIO_PAD_SIG_OPT);
	volatile uint32_t *gpio_rca_pad = (volatile uint32_t *)(&GPIO_PAD_SIG_RCA);
	volatile uint32_t *gpio_bnc_pad = (volatile uint32_t *)(&GPIO_PAD_SIG_BNC);

	*(gpio_pow_reg + 1) |= GPIO_MASK_SIG_POW;
	*gpio_pow_pad = IOMUXC_PAD_DSE(7);
	*gpio_pow_mux = GPIO_MUX_MODE_ALT5;

	*(gpio_sel_reg + 1) |= GPIO_MASK_SIG_SEL;
	*gpio_sel_pad = IOMUXC_PAD_DSE(7);
	*gpio_sel_mux = GPIO_MUX_MODE_ALT5;

	*(gpio_usb_reg + 1) &= ~GPIO_MASK_SIG_USB;
	*gpio_usb_pad = IOMUXC_PAD_DSE(7);
	*gpio_usb_mux = GPIO_MUX_MODE_ALT5;

	*(gpio_opt_reg + 1) &= ~GPIO_MASK_SIG_OPT;
	*gpio_opt_pad = IOMUXC_PAD_DSE(7);
	*gpio_opt_mux = GPIO_MUX_MODE_ALT5;

	*(gpio_rca_reg + 1) &= ~GPIO_MASK_SIG_RCA;
	*gpio_rca_pad = IOMUXC_PAD_DSE(7);
	*gpio_rca_mux = GPIO_MUX_MODE_ALT5;

	*(gpio_bnc_reg + 1) &= ~GPIO_MASK_SIG_BNC;
	*gpio_bnc_pad = IOMUXC_PAD_DSE(7);
	*gpio_bnc_mux = GPIO_MUX_MODE_ALT5;

	d3state.input = ModeNull;
	d3state.power = PowerNull;
}

/**
 * @brief Check all D3 inputs and set d3state.input
 *
 */
_section_flash
void checkAll(void)
{
	if (GPIO_PSR_SIG_IN & GPIO_MASK_SIG_USB)
	{
		d3state.input = ModeUSB;
		d3state.power = PowerOn;
	}
	else if (GPIO_PSR_SIG_IN & GPIO_MASK_SIG_OPT)
	{
		d3state.input = ModeOPT;
		d3state.power = PowerOn;
	}
	else if (GPIO_PSR_SIG_IN & GPIO_MASK_SIG_RCA)
	{
		d3state.input = ModeRCA;
		d3state.power = PowerOn;
	}
	else if (GPIO_PSR_SIG_IN & GPIO_MASK_SIG_BNC)
	{
		d3state.input = ModeBNC;
		d3state.power = PowerOn;
	}
	else
	{
		d3state.input = ModeNull;
		d3state.power = PowerNull;
	}
}

/**
 * @brief Write SIG_POW HIGH for 50ms to toggle power on the D3 board
 *
 */
_section_flash
void d3togglePower(void)
{
	GPIO6_DR_SET = GPIO_MASK_SIG_POW;
	msleep(50);
	GPIO6_DR_CLEAR = GPIO_MASK_SIG_POW;

	checkAll();

	d3state.power = (d3state.input) ? PowerOn : PowerOff;
}

/**
 * @brief Write SIG_SEL HIGH for 50ms switch inputs on the D3 board
 *
 */
_section_flash
void d3switchInput(void)
{
	checkAll();
	uint8_t currentInput = d3state.input;
	uint8_t targetInput = (currentInput == ModeOPT) ? ModeUSB : ModeOPT;

	printf("Switching to input: %s\r\n", (currentInput == ModeOPT) ? "USB" : "OPT");
	do
	{
		GPIO6_DR_SET = GPIO_MASK_SIG_SEL;
		msleep(50); 
		GPIO6_DR_CLEAR = GPIO_MASK_SIG_SEL;
		checkAll();
	} while (d3state.input != targetInput);

	printf("Done\r\n");
}

/**
 * @brief Check if the D3's PLL is locked
 *
 * @return Returns D3_PLL_LOCKED if locked, otherwise D3_PLL_NOT_LOCKED
 */
_section_flash
uint8_t pllStatus(void)
{
	for (size_t i = 0; i < 8; i++) // 2 seconds
	{
		msleep(250);

		if (!(GPIO_PSR_SIG_IN & GPIO_MASK_SIG_OPT)) // SIG_OPT remains HIGH >= 2 seconds
		{
			return NotLocked;
		}
	}
	return Locked;
}

/**
 * @brief Report current status of the D3
 *
 */
_section_flash
void d3currentStatus(void)
{
	checkAll();
	printf("Current Input: ");
	switch (d3state.input)
	{
	case ModeUSB:
		printf("USB\r\n");
		break;
	case ModeOPT:
		printf("Optical\r\n");
		printf("PLL is %s\r\n", (pllStatus() == Locked) ? "locked" : "not locked");
		break;
	default:
		printf("NULL\r\n");
		break;
	}
}
