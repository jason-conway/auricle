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

#define D3SR_POW_ON 1
#define D3SR_POW_OFF 0

#define GPIO_REG_SIG_OUT (IMXRT_GPIO6.DR)

#define GPIO_IOMUX_SIG_POW IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_03
#define GPIO_PAD_SIG_POW IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_03
#define GPIO_MASK_SIG_POW ( 1<< 3)

#define GPIO_IOMUX_SIG_SEL IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_02
#define GPIO_PAD_SIG_SEL IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_02
#define GPIO_MASK_SIG_SEL (1 << 2)

#define GPIO_REG_SIG_IN (IMXRT_GPIO9.DR)

#define GPIO_IOMUX_SIG_USB IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_04
#define GPIO_PAD_SIG_USB IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_04
#define GPIO_MASK_SIG_USB (1 << 4) 

#define GPIO_IOMUX_SIG_OPT IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_05
#define GPIO_PAD_SIG_OPT IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_05
#define GPIO_MASK_SIG_OPT (1 << 5) 

#define GPIO_IOMUX_SIG_RCA IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_06
#define GPIO_PAD_SIG_RCA IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_06
#define GPIO_MASK_SIG_RCA (1 << 6)

#define GPIO_IOMUX_SIG_BNC IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_08
#define GPIO_PAD_SIG_BNC IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_08
#define GPIO_MASK_SIG_BNC (1 << 8)

#define GPIO_MUX 0b10101

class TPD3IO
{
public:
    TPD3IO(void);


    
    
private:
    enum D3_IO
    {
        SIG_POW,
        SIG_SEL,
        SIG_USB,
        SIG_OPT,
        SIG_RCA,
        SIG_BNC
    };

    void init(void);
    void powerOn(void);
    void switchInput(void);
    
};


#endif