/*
 *     pix_transformations.c
 *     Authors: Yvonne Lopez Davila and Caroline Shaw
 *     ylopez02 and cshaw03
 *     HW4 / Arith
 *
 *     Transformation functions for mapping between pixel representations
 *
 *     Notes:
 *
 */

#include "pix_transformations.h"

const float BITS9_MAX = 511;  /* Max int within 9-bit rep */
const float FL_QUANT_RANGE = 0.3; /* Upper coeff float val */
const float D_QUANT_RANGE = 15;   /* Upper coeff unsigned val */

const int64_t Pr_LSB = 0;
const int64_t Pb_LSB = 4;
const int64_t d_LSB = 8;
const int64_t c_LSB = 13;
const int64_t b_LSB = 18;
const int64_t a_LSB = 23;

const int64_t Pr_WIDTH = 4;
const int64_t Pb_WIDTH = 4;
const int64_t d_WIDTH = 5;
const int64_t c_WIDTH = 5;
const int64_t b_WIDTH = 5;
const int64_t a_WIDTH = 9;



/********** transform_rgb_floats_to_cvs ********
 *
 * Description:
 *      Transforms an RGB float pixel into a YPbPr (CVS) float pixel.
 * Parameters:
 *      elem: A pointer to the destination pixel to store the 
 *                                              transformed values.
 *      rgb_pix: A pointer to the source RGB float pixel containing the 
 *                                                      original values.
 * Return:
 *      None.
 * Expects:
 *      -The elem parameter is a valid pointer to a struct pixel_float 
 *                                      representing the destination pixel.
 *      -The rgb_pix parameter is a valid pointer to a struct pixel_float
 *                                      representing the source RGB pixel.
 * Notes:
 *      Performs a color space transformation from RGB to YPbPr (CVS) space
 *      and stores the transformed values in the destination pixel.
 ************************/
void transform_rgb_floats_to_cvs(void *elem, struct pixel_float *rgb_pix)
{
        assert(elem != NULL);
        assert(rgb_pix != NULL);
        struct pixel_float *cvs_pix = (struct pixel_float *)elem;

        /* Initialize transformed Y, Pb, Pr CVS float vals for a cell */
        cvs_pix->r_Y = .299 * rgb_pix->r_Y + 0.587 * rgb_pix->g_Pb +
                0.114 * rgb_pix->b_Pr;
        cvs_pix->g_Pb = -0.168736 * rgb_pix->r_Y - 0.331264 *  rgb_pix->g_Pb
                + 0.5 * rgb_pix->b_Pr;
        cvs_pix->b_Pr = 0.5 * rgb_pix->r_Y - 0.418688 * rgb_pix->g_Pb -
                0.081312 * rgb_pix->b_Pr;
}

/********** init_floats_from_rgb_ints ********
 *
 * Description:
 *      Initializes a floating-point pixel structure from RGB integer values.
 * Parameters:
 *      elem: Pointer to the floating-point pixel structure to initialize.
 *      pix_val: Pointer to the RGB integer pixel values.
 *      maxval: Maximum value for RGB components.
 * Return:
 *      None
 * Expects:
 *      -The elem pointer is not NULL.
 *      -The pix_val pointer is not NULL and contains valid RGB integer values.
 *      -The maxval is set to 255 (assuming RGB component range).
 * Notes:
 *      Converts RGB integer pixel values to floating-point representation,
 *      scaling each component by the maximum value.
 ************************/
void init_floats_from_rgb_ints(void *elem, Pnm_rgb pix_val, unsigned maxval)
{
        struct pixel_float *curr_pix = (struct pixel_float *)elem;

        /* Scale rgb ints by maxval and convert to floats */
        curr_pix->r_Y = (float)pix_val->red / maxval;
        curr_pix->g_Pb = (float)pix_val->green / maxval;
        curr_pix->b_Pr = (float)pix_val->blue / maxval;        
}

/********** init_ints_from_rgb_floats ********
 *
 * Description:
 *      Initializes RGB integer pixel values from floating-point RGB pixel 
 *      values.
 * Parameters:
 *      elem: Pointer to the current RGB integer pixel to initialize.
 *      float_rgb: Pointer to the floating-point RGB pixel containing 
 *                                                              the values.
 *      maxval: Maximum value for RGB components (255).
 * Return:
 *      None
 * Expects:
 *      -The elem pointer is not NULL and points to a valid Pnm_rgb structure.
 *      -The float_rgb pointer is not NULL and points to a valid struct 
 *                                                              pixel_float.
 * Notes:
 *      scaled by the maximum value of RGB components.
 ************************/
void init_ints_from_rgb_floats(void *elem, struct pixel_float *float_rgb,
        unsigned maxval)
{
        assert(elem);
        assert(float_rgb != NULL);

        /* Initialize unscaled integers from scaled RGB floats */
        Pnm_rgb int_pix = (struct Pnm_rgb *)elem;
        int_pix->red = (int)(float_rgb->r_Y * maxval);
        int_pix->green = (int)(float_rgb->g_Pb * maxval);
        int_pix->blue = (int)(float_rgb->b_Pr * maxval);
}

/********** transform_cvs_to_rgb_float ********
 *
 * Description:
 *      Transforms a YPbPr (CVS) float pixel into an RGB float pixel.
 * Parameters:
 *      elem: A pointer to the destination pixel to store the transformed 
 *                                                                     values.
 *      cvs_pix: A pointer to the source YPbPr (CVS) float pixel containing 
                                                        the original values.
 * Return:
 *      None.
 * Expects:
 *      -The elem parameter is a valid pointer to a struct pixel_float 
 *                                      representing the destination pixel.
 *      -The cvs_pix parameter is a valid pointer to a struct pixel_float 
 *                                   representing the source YPbPr (CVS) pixel.
 * Notes:
 *      Performs a color space transformation from YPbPr (CVS) to RGB space
 *      and stores the transformed values in the destination pixel.
 ************************/
void transform_cvs_to_rgb_float(void *elem, struct pixel_float *cvs_pix)
{
        assert(elem != NULL);
        assert(cvs_pix != NULL);
        struct pixel_float *rgb_pix = (struct pixel_float *)elem;
        
        /* Transform (pb, Pr, Y) to (r,g,b) floats for a cell */
        rgb_pix->r_Y = 1.0 * cvs_pix->r_Y + 0.0 * cvs_pix->g_Pb + 
                1.402 * cvs_pix->b_Pr;
        rgb_pix->g_Pb = 1.0 * cvs_pix->r_Y - 0.344136 * cvs_pix->g_Pb -
                0.714136 * cvs_pix->b_Pr;
        rgb_pix->b_Pr = 1.0 * cvs_pix->r_Y + 1.772 * cvs_pix->g_Pb + 
                0.0 * cvs_pix->b_Pr;
}

/********** transform_a_to_unsigned ********
 *
 * Description:
 *   Converts a floating-point value representing a cosine coefficient 'a' into
 *   an unsigned 9-bit integer within the range [0, 512]. The resulting integer
 *   is then represented in 2's complement format if the original value was
 *   negative.
 *      
 * Parameters:
 *   a:  The floating-point value of the cosine coefficient 'a' to be converted
 *       to an unsigned integer.
 *      
 * Return:
 *   unsigned: The unsigned 9-bit integer representation of the transformed
 *             coefficient 'a'.
 *      
 * Expects:
 *   -The parameter 'a' must be a valid floating-point number.
 *    
 * Notes:
 *     Function does not perform any memory allocation or deallocation.
 *     Client is responsible for memory.
*******/
unsigned transform_a_to_unsigned(float a)
{
        /* Represent unsigned float in 9 bit int range */
        unsigned ua = (int)round(a * BITS9_MAX);

        /* Take 2s complement to get positive representation */
        if (a < 0) {
                ua = (~(ua) + 1);
        }
        /* Handle error cases outside luminance range (-1 to 1)*/
        if (ua >= (BITS9_MAX)) {
                ua = BITS9_MAX;
        }
        return ua;
}

/********** convert_signed_to_coeff ********
 *
 * Description:
 *      Converts a signed integer coefficient value to a floating-point value 
 *      within a specified range and returns the result.
 * Parameters:
 *      coeff: The signed integer coefficient value to be converted to a 
 *      floating-point value.
 * Return:
 *      Returns the converted floating-point coefficient value.
 * Expects:
 *      -The coeff parameter must be a valid signed integer.
 * Notes:
 *      Dequantizes the input signed integer coefficient value to fit within a 
 *      specified floating-point range.
 ************************/
float convert_signed_to_coeff(signed coeff)
{
        /* Map int coeff to proportionate float coeff */
        float dequant_mapped = (FL_QUANT_RANGE * (coeff / D_QUANT_RANGE));

        /* If mapped value exceeds max in range, set to max */
        if (dequant_mapped > FL_QUANT_RANGE) {
                dequant_mapped = FL_QUANT_RANGE;
        }

        return dequant_mapped;
}

/********** convert_coeff_to_signed ********
 *
 * Description:
 *      Converts a floating-point coefficient value to a signed integer within
 *      a specified range and returns the result.
 * Parameters:
 *      coeff: The floating-point coefficient value to be converted to a signed 
 *      integer.
 * Return:
 *      Returns the converted signed integer coefficient value.
 * Expects:
 *      -The coeff parameter must be a valid floating-point number.
 * Notes:
 *      Quantizes the input coefficient value to fit within a specified signed
 *                                                               integer range.
 ************************/
signed convert_coeff_to_signed(float coeff)
{
        /* Map float coeff to proportionate int coeff */
        signed quant_mapped = round(D_QUANT_RANGE * (coeff / FL_QUANT_RANGE));
        
        /* If mapped value exceeds max in range, set to max */
        if (quant_mapped > D_QUANT_RANGE) {
                quant_mapped = D_QUANT_RANGE;
        }

        return quant_mapped;
}

/********** transform_luminance_to_coeffs ********
 *
 * Description:
 *   Applies the discrete cosine transformation to the luminance values of a
 *   block of pixels in a Component Video (CVS) array and stores the resulting
 *   cosine coefficients in the provided CVS block structure.
 *      
 * Parameters:
 *   cvs_arr:   A2Methods_UArray2 representing the Component Video (CVS) array
 *              containing the luminance values of the block of pixels.
 *   methods:   A2Methods_T representing the methods used to access elements in
 *              the CVS array.
 *   col:       The column index of the top-left corner of the block of pixels
 *              within the CVS array.
 *   row:       The row index of the top-left corner of the block of pixels
 *              within the CVS array.
 *   curr_iWord: Pointer to the CVS block structure where the resulting cosine
 *              coefficients will be stored.
 *      
 * Return:
 *   void
 *      
 * Expects:
 *   -The CVS array (cvs_arr) and its methods (methods) must not be NULL.
 *   -The provided column (col) and row (row) indices must be within the
 *     bounds of the CVS array.
 *   -The CVS block structure (curr_iWord) must be initialized and allocated
 *     by the caller.
 * Notes:
 *   -This function does not perform any memory allocation or deallocation.
 *     Memory management is the responsibility of the caller.
*******/
void transform_luminance_to_coeffs(A2Methods_UArray2 cvs_arr,
        A2Methods_T methods, int col, int row, struct cvs_block *curr_iWord)
{
        assert(cvs_arr);
        assert(curr_iWord);

        /* Get positions of previous cells in current cvs block */
        struct pixel_float *Y1_pos = methods->at(cvs_arr, col - 1, row - 1);
        struct pixel_float *Y2_pos = methods->at(cvs_arr, col, row - 1);
        struct pixel_float *Y3_pos = methods->at(cvs_arr, col - 1, row);
        struct pixel_float *Y4_pos = methods->at(cvs_arr, col, row);

        /* Initialize luminance float values for a block */
        float Y1 = Y1_pos->r_Y;
        float Y2 = Y2_pos->r_Y;
        float Y3 = Y3_pos->r_Y;
        float Y4 = Y4_pos->r_Y;

        /* Apply discrete cosine transformation to cvs luminance vals */
        float a = (Y4 + Y3 + Y2 + Y1) / 4.0;
        float b = (Y4 + Y3 - Y2 - Y1) / 4.0;
        float c = (Y4 - Y3 + Y2 - Y1) / 4.0;
        float d = (Y4 - Y3 - Y2 + Y1) / 4.0;

        /* Initialize a, b, c, d values */
        curr_iWord->a = transform_a_to_unsigned(a);
        curr_iWord->b = convert_coeff_to_signed(b);
        curr_iWord->c = convert_coeff_to_signed(c);
        curr_iWord->d = convert_coeff_to_signed(d);
}

/********** transform_coeffs_to_luminance ********
 *
 * Description:
 *      Transforms the cosine coefficients stored in the current block 
 *      structure to luminance values and sets them for the four blocks in the
 *      chroma vector space (CVS) array.
 * Parameters:
 *      curr_iWord: Pointer to the current block structure containing cosine
 *                                                               coefficients.
 *      b1_cvs: Pointer to the top-left block in the CVS array.
 *      b2_cvs: Pointer to the top-right block in the CVS array.
 *      b3_cvs: Pointer to the bottom-left block in the CVS array.
 *      b4_cvs: Pointer to the bottom-right block in the CVS array.
 * Return:
 *      This function does not return a value.
 * Expects:
 *      -The curr_iWord parameter must point to a valid block structure 
 *                                      containing cosine coefficients.
 *      -The b1_cvs, b2_cvs, b3_cvs, and b4_cvs parameters must point to valid
 *                                      blocks in the chroma vector space array.
 * Notes:
 *      Transforms the cosine coefficients stored in the current block 
 *      structure to luminance values using predefined equations and sets them
 *      for each block in the CVS array.It modifies the luminance values of the 
 *      four blocks in the CVS array based on the cosine coefficients.
 ************************/
void transform_coeffs_to_luminance(struct cvs_block *curr_iWord, 
struct pixel_float *b1_cvs, struct pixel_float *b2_cvs, 
struct pixel_float *b3_cvs, struct pixel_float *b4_cvs)
{
        assert(b1_cvs != NULL && b2_cvs != NULL &&
               b3_cvs != NULL && b4_cvs != NULL);
        assert(curr_iWord != NULL);

        /* Unpack a, b, c, d int values to floats */
        float a = ((float)curr_iWord->a) / BITS9_MAX;
        float b = convert_signed_to_coeff(curr_iWord->b);
        float c = convert_signed_to_coeff(curr_iWord->c);
        float d = convert_signed_to_coeff(curr_iWord->d);

        /* Initialize luminance cells of a block in row major order */
        b1_cvs->r_Y = (float)(a - b - c + d);
        b2_cvs->r_Y = (float)(a - b + c - d);
        b3_cvs->r_Y = (float)(a + b - c - d);
        b4_cvs->r_Y = (float)(a + b + c + d);        
}


/********** unpack_codewords ********
 *
 * Description:
 *      Unpacks the information from the 2D array of 64-bit codewords into a 
 *      new 2D array of codeword information.
 * Parameters:
 *      col: The column index of the current element in the codewords array.
 *      row: The row index of the current element in the codewords array.
 *      codewords: The 2D array containing 64-bit codewords.
 *      elem: Pointer to the current element in the codewords array.
 *      codeword_info: Pointer to the new 2D array of codeword information to 
 *                                                              be populated.
 * Return:
 *      This function does not return a value.
 * Expects:
 *      -The col and row indices must be within the bounds of the codewords 
 *      array.
 *      -The codewords array must not be NULL.
 *      -The elem pointer must point to a valid element in the codewords array.
 *      -The codeword_info pointer must point to a valid 2D array of codeword 
 * information.
 * Notes:
 *      Unpacks the information from the 2D array of 64-bit codewords into a 
 *      new 2D array of codeword information. Updates the codeword information
 *      array with the unpacked information from the codewords array.
 ************************/
void unpack_codewords(int col, int row, A2Methods_UArray2 codewords,
void *elem, void *codeword_info)
{
        assert(codewords != NULL);
        assert(elem != NULL);

        /* Set types for codeword and codeword info arrays */
        A2Methods_UArray2 word_info_arr = (A2Methods_UArray2 *)codeword_info;
        int64_t *curr_word = (int64_t *)elem;

        /* Get pointer to corresponding unpacked codeword index */
        A2Methods_T methods = uarray2_methods_plain;
        struct cvs_block *word_info = methods->at(word_info_arr, col, row);

        /* Extract fields from codeword and populate info array */
        word_info->Pr_index = Bitpack_getu(*curr_word, Pr_WIDTH, Pr_LSB);
        word_info->Pb_index = Bitpack_getu(*curr_word, Pb_WIDTH, Pb_LSB);
        word_info->d = Bitpack_gets(*curr_word, d_WIDTH, d_LSB);
        word_info->c = Bitpack_gets(*curr_word, c_WIDTH, c_LSB);
        word_info->b = Bitpack_gets(*curr_word, b_WIDTH, b_LSB);
        word_info->a = Bitpack_getu(*curr_word, a_WIDTH, a_LSB);

       (void)codewords;
}
