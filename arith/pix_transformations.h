/*
 *     pix_transformations.h
 *     Authors: Yvonne Lopez Davila and Caroline Shaw
 *     ylopez02 and cshaw03
 *     HW4 / Arith
 *
 *     Transformation functions for mapping between pixel representations.
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

#ifndef PIXEL_TRANSFORMATIONS_H
#define PIXEL_TRANSFORMATIONS_H



/********** pixel_float *************
 *      Struct contains variables hold 3-coordinate float
 *      representations of pixels. Can be used for both
 *      RGB floats (indicates by first value in var name) or
 *      Component video space floats (2nd val in var name)
 * 
 *      Contains:
 *      float r_Y: red RGB int or Y CVS luminance value
 *      float r_Pb: green RGB int or Pb CVS chroma value
 *      float b_Pr: blue RGB int or Y Pr CVS chroma value
 *
 **************************************/
struct pixel_float
{
        float r_Y;
        float g_Pb;
        float b_Pr;
};

/********** cvs_block *************
 *      Struct contains variables that represent pixel info from
 *      a block of component video space pixels. This contains info
 *      necessary to pack codewords into 32-bit representations.
 * 
 *      Contains:
 *      unsigned Pb_index: quantized index of pixel Pb chroma value  
 *      unsigned Pr_index: quantized index of pixel Pr chroma value
 *      unsigned a: average brightness of an image, in 9-bit representation
 *      signed b: degree of brightness increase when moving across
 *                img top to bottom, in signed quantized rep (-15 to 15)
 *      signed c: degree of brightness increase when moving across
 *                img left to right, in signed quantized rep (-15 to 15)
 *      signed d: degree to which pixels on one diagonal are brighter than
 *                pixels on other diagnoal, signed quantized rep (-15 to 15)
 *
 **************************************/
struct cvs_block
{
        unsigned Pb_index;
        unsigned Pr_index;
        unsigned a;
        signed b;
        signed c;
        signed d;
};

/********** copy_info *************
 *      Information needed to transform from one pixel representation to another
 * 
 *      Contains:
 *       A2Methods_UArray2 prev_arr: array from which transform info is coming 
 *                                 from
 *       A2Methods_T prev_methods: methods needed by prev_arr
 *       void (*transFun)():       transformation function needed 
 *
 **************************************/
struct copy_info
{
        A2Methods_UArray2 prev_arr;
        A2Methods_T prev_methods;
        void (*transFun)();
};

/********** block_info *************
 *      Info needed to transform from coordinate video space blocks 
 *      to smaller codeword info array (can be used cl)
 *      Contains:
 *      A2Methods_UArray2 trans_cvs: array2b of transformed CVS coords
 *      A2Methods_T arr_methods: methods needed for trans_cvs
 *      int counter: counter tracks when mapper moves to new block
 *      float Pb_avg: average Pb in a block, updated in block iteration
 *      float Pr_avg: average Pb in a block, updated in block iteration
 *
 **************************************/
struct block_info
{
        A2Methods_UArray2 trans_cvs;
        A2Methods_T arr_methods;
        int counter;
        float Pb_avg;
        float Pr_avg; 
};


/********** rgb_int_info *************
 *      Info needed to convert from rgb float to int
 *      Contains:
    A2Methods_UArray2 rgb_floats: array of rgb floats (transformer)
    A2Methods_T rgb_flt_meths: methods used by rgb float arr
    unsigned maxval: maxval (to scale rgb ints)
 *
 **************************************/
struct rgb_int_info 
{
    A2Methods_UArray2 rgb_floats;
    A2Methods_T rgb_flt_meths;
    unsigned maxval;
};


void transform_rgb_floats_to_cvs(void *elem, struct pixel_float *rgb_pix);

void init_floats_from_rgb_ints(void *elem, Pnm_rgb pix_val, unsigned maxval);

void init_ints_from_rgb_floats(void *elem, struct pixel_float *float_rgb,
        unsigned maxval);

void transform_cvs_to_rgb_float(void *elem, struct pixel_float *cvs_pix);

void transform_luminance_to_coeffs(A2Methods_UArray2 cvs_arr,
        A2Methods_T methods, int col, int row, struct cvs_block *curr_iWord);

unsigned transform_a_to_unsigned(float a);

void transform_coeffs_to_luminance(struct cvs_block *curr_iWord, 

struct pixel_float *b1_cvs, struct pixel_float *b2_cvs,

struct pixel_float *b3_cvs, struct pixel_float *b4_cvs);
void unpack_codewords(int col, int row, A2Methods_UArray2 codewords,
        void *elem, void *codeword_info);
signed convert_coeff_to_signed(float coeff);
float convert_signed_to_coeff(signed coeff);


#endif