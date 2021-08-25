/**
 * @file hrir_144.h
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief HRIR coefficients for 144 degrees azimuth
 * @version 0.1
 * @date 2021-08-24
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 */

#pragma once

#include "auricle.h"
#include "convolvIR.h"
#include "stdint.h"

// float32_t __attribute__((section(".flashmem"))) HRIR_144[2][8192] = {
// 	{
