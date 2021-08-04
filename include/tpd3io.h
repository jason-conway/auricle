/**
 * @file tpd3io.h
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief Spatial Audio for Arm Cortex-M7
 * @version 0.1
 * @date 2021-07-11
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#ifndef TPD3IO_H
#define TPD3IO_H

#include "auricle.h"
#include "imxrt.h"

// Outputs on GPIO Register 6
#define GPIO_DR_SIG_OUT (IMXRT_GPIO6.DR) // GPIO6 Data Register
#define GPIO_PSR_SIG_OUT (IMXRT_GPIO6.PSR) // GPIO6 Pad Sample Register

// Inputs on Data Register 9 
#define GPIO_DR_SIG_IN (IMXRT_GPIO9.DR) // GPIO9 Data Register
#define GPIO_PSR_SIG_IN (IMXRT_GPIO9.PSR) // GPIO9 Pad Sample Register

// SW_MUX_CTL Registers
#define GPIO_IOMUX_SIG_POW IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_03 // pg 476
#define GPIO_IOMUX_SIG_SEL IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_02 // pg 475
#define GPIO_IOMUX_SIG_USB IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_04   // pg 432
#define GPIO_IOMUX_SIG_OPT IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_05   // pg 433
#define GPIO_IOMUX_SIG_RCA IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_06   // pg 435
#define GPIO_IOMUX_SIG_BNC IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_08   // pg 437

// SW_PAD_CTL Registers
#define GPIO_PAD_SIG_POW IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_03 // pg 636
#define GPIO_PAD_SIG_SEL IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_02 // pg 634
#define GPIO_PAD_SIG_USB IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_04   // pg 566
#define GPIO_PAD_SIG_OPT IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_05   // pg 568
#define GPIO_PAD_SIG_RCA IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_06   // pg 569
#define GPIO_PAD_SIG_BNC IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_08   // pg 573

// Bitmasks
#define GPIO_MASK_SIG_POW (1 << 3) // 8
#define GPIO_MASK_SIG_SEL (1 << 2) // 4
#define GPIO_MASK_SIG_USB (1 << 4) // 16
#define GPIO_MASK_SIG_OPT (1 << 5) // 32
#define GPIO_MASK_SIG_RCA (1 << 6) // 64
#define GPIO_MASK_SIG_BNC (1 << 8) // 256

// MUX_MODE ALT5 ==> GPIO
#define GPIO_MUX_MODE_ALT5 (0b0101 | 0b00010000)

#define D3_PLL_NOT_LOCKED 0
#define D3_PLL_LOCKED 1
#define D3_INCORRECT_MODE 2

class TPD3IO
{
public:
    TPD3IO(void);

private:
    enum D3InputMode
    {
        MODE_NULL,
        USB,
        OPT,
        RCA,
        BNC
    };

    enum D3Power
    {
        POWER_OFF,
        POWER_ON
    };

    typedef struct D3Status
    {
        uint8_t power;
        volatile uint8_t mode;
    } D3Status;
    
    D3Status d3status;

    void init(void);

    uint8_t readGPIO(void);
    uint8_t pllStatus(void);
    uint8_t checkPower(void);
    uint8_t checkSigUSB(void);
    uint8_t checkSigOPT(void);
    uint8_t checkSigRCA(void);
    uint8_t checkSigBNC(void);
    void togglePower(void);
    void switchInput(void);

};

#endif