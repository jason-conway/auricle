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

#define D3SR_POW_ON 1
#define D3SR_POW_OFF 0




class TPD3IO
{
public:



    
    
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

    struct InputStates
    {
        uint8_t boardPower;
    };

    struct D3Status
    {
        uint8_t power : 1;
        uint8_t inputMode : 2;
        uint8_t pllLock : 1;
    };

    
    
};


#endif