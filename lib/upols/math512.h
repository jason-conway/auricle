/**
 * @file math512.h
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief Optimized routines for block sizes of 512 (2048 bytes)
 * @version 0.1
 * @date 2021-11-26
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#pragma once

#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif
	void cmac512(const float *cmplxA, const float *cmplxB, float *cmplxAccum);
	void cp512(const float *src, float *dest);
	void clear512(float *dest);
#ifdef __cplusplus
}
#endif
