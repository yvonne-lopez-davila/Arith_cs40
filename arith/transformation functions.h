/*
 *     transformation functions.h
 *     Authors: Yvonne Lopez Davila and Caroline Shaw
 *     ylopez02 and cshaw03
 *     HW4 / Arith
 *
 *     transformation functions for 
 *
 *     Notes:
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "assert.h"
#include "compress40.h"
#include "pnm.h"
#include "a2methods.h"
#include "a2plain.h"
#include "a2blocked.h"
#include "arith40.h"
#include "bitpack.h"
#include <math.h>

#ifndef BITPACK_INCLUDED
#define BITPACK_INCLUDED

void transform_rgb_floats_to_cvs(void *elem, struct pixel_float *rgb_pix);

void init_floats_from_rgb_ints(void *elem, Pnm_rgb pix_val, unsigned maxval);

void init_ints_from_rgb_floats(void *elem, struct pixel_float *float_rgb,
        unsigned maxval);

void transform_cvs_to_rgb_float(void *elem, struct pixel_float *cvs_pix);
A2Methods_UArray2 create_blocked_arr(int width, int height, int size,
        int blocksize, A2Methods_T methods);

void transform_luminance_to_coeffs(A2Methods_UArray2 cvs_arr,
        A2Methods_T methods, int col, int row, struct cvs_block *curr_iWord);

unsigned transform_a_to_unsigned(float a);

void transform_coeffs_to_luminance(struct cvs_block *curr_iWord, 

struct pixel_float *b1_cvs, struct pixel_float *b2_cvs,

struct pixel_float *b3_cvs, struct pixel_float *b4_cvs);
void unpack_codewords(int col, int row, A2Methods_UArray2 codewords,
        void *elem, void *codeword_info);

#endif