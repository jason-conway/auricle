/**
 * @file ash.h
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief Auircle Shell
 * @version 0.1
 * @date 2021-08-08
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#ifndef ASH_H
#define ASH_H

#include "auricle.h"
#include "uscp.h"
#include "tpd3io.h"

#define PROGRESS_WIDTH 60
#define PROGRESS_BAR "============================================================"

class ASH
{
public:
    ASH(void);
    void execLoop(void);

private:
    void init(void);

    static void togglePower(void *);
    static void setAngle(void *);
    static void switchInput(void *);
    static void currentStatus(void *);
    static void testFunction(void *);
    static void audioPassthrough(void *);
    static void unknownCommand(void *);

    void progressBar(uint8_t progress);
};

#endif
