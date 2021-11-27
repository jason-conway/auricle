/**
 * @file math512.c
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief Optimized routines for block sizes of 512 (2048 bytes)
 * @version 0.1
 * @date 2021-11-27
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#include "math512.h"

/**
 * @brief Fast multiply-accumulate for complex numbers
 * 
 * @param cmplxA Pointer to first array of interleaved complex values
 * @param cmplxB Pointer to second array of interleaved complex values
 * @param cmplxAccum Pointer to accumulator buffer
 */
void cmac512(const float *cmplxA, const float *cmplxB, float *cmplxAccum)
{
	float cmplx[4];
	for (size_t i = 64; i > 0; i--)
	{
		cmplx[0] = *cmplxA++; // cmplx[0] => Real part of complxA
		cmplx[1] = *cmplxA++; // cmplx[1] => Imaginary part of complxA
		cmplx[2] = *cmplxB++; // cmplx[2] => Real part of complxB
		cmplx[3] = *cmplxB++; // cmplx[3] => Imaginary part of complxB

		*cmplxAccum++ += (cmplx[0] * cmplx[2]) - (cmplx[1] * cmplx[3]);
		*cmplxAccum++ += (cmplx[0] * cmplx[3]) + (cmplx[1] * cmplx[2]);

		cmplx[0] = *cmplxA++;
		cmplx[1] = *cmplxA++;
		cmplx[2] = *cmplxB++;
		cmplx[3] = *cmplxB++;

		*cmplxAccum++ += (cmplx[0] * cmplx[2]) - (cmplx[1] * cmplx[3]);
		*cmplxAccum++ += (cmplx[0] * cmplx[3]) + (cmplx[1] * cmplx[2]);

		cmplx[0] = *cmplxA++;
		cmplx[1] = *cmplxA++;
		cmplx[2] = *cmplxB++;
		cmplx[3] = *cmplxB++;

		*cmplxAccum++ += (cmplx[0] * cmplx[2]) - (cmplx[1] * cmplx[3]);
		*cmplxAccum++ += (cmplx[0] * cmplx[3]) + (cmplx[1] * cmplx[2]);

		cmplx[0] = *cmplxA++;
		cmplx[1] = *cmplxA++;
		cmplx[2] = *cmplxB++;
		cmplx[3] = *cmplxB++;

		*cmplxAccum++ += (cmplx[0] * cmplx[2]) - (cmplx[1] * cmplx[3]);
		*cmplxAccum++ += (cmplx[0] * cmplx[3]) + (cmplx[1] * cmplx[2]);
	}
}

/**
 * @brief Copy contents of src over to dest. Somehow faster than memcpy with gccarmnoneeabi and -O2
 * 
 * @param src Source buffer
 * @param dest Destination buffer
 */
void cp512(const float *src, float *dest)
{
	for (size_t i = 128; i > 0; i--)
	{
		*dest++ = *src++;
		*dest++ = *src++;
		*dest++ = *src++;
		*dest++ = *src++;
	}
}

/**
 * @brief Zero out destination array. Somehow faster than memset with gccarmnoneeabi and -O2
 * 
 * @param dest Destination buffer
 */
void clear512(float *dest)
{
	for (size_t i = 128; i > 0; i--)
	{
		*dest++ = 0;
		*dest++ = 0;
		*dest++ = 0;
		*dest++ = 0;
	}
}
