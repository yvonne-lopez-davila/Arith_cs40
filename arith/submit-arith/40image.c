/*
 *     40image.c
 *     Authors: Yvonne Lopez Davila and Caroline Shaw
 *     ylopez02 and cshaw03
 *     HW4 / Arith
 *
 *     40image program supports conversion between full-color portable pixmap 
 *     images and compressed binary image files in either direction, as 
 *     specified by client. 
 *     The program achieves its purpose by transforming rgb pixels to component 
 *     video space, and subsequently packing those pixels in memory, and 
 *     applying the inverse process for decompression.
 *
 *      Usage: 
 *          1) 40image -d [filename]
 *            (Decompress filename)
 *
 *          2) 40image -c [filename]
 *            (Compress filename)
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


const int BLOCKSIZE = 2;  /* Dim of RGB blocks --> codeword */
const float FL_QUANT_RANGE = 0.3; /* Upper coeff float val */
const float D_QUANT_RANGE = 15;   /* Upper coeff unsigned val */
const unsigned CHAR_BITS = 8;     /* Num bits in a char */
const unsigned WORD_BITS = 32;    /* Num bits in packed word */
const float BITS9_MAX = 63;  /* Max int within 9-bit rep */

const int64_t Pr_LSB = 0;
const int64_t Pb_LSB = 4;
const int64_t d_LSB = 8;
const int64_t c_LSB = 14;
const int64_t b_LSB = 20;
const int64_t a_LSB = 26;

const int64_t Pr_WIDTH = 4;
const int64_t Pb_WIDTH = 4;
const int64_t d_WIDTH = 6;
const int64_t c_WIDTH = 6;
const int64_t b_WIDTH = 6;
const int64_t a_WIDTH = 6;


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
 *      Information needed to transform from one pixel representation to 
 *                                                              another
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

static void (*compress_or_decompress)(FILE *input) = compress40;

/* Compression functions */
Pnm_ppm initialize_ppm(FILE *input);
void compress40 (FILE *input);
bool is_even(int x);
A2Methods_UArray2 create_trimmed_float_arr(
        Pnm_ppm image, int width, int height);

/* Mapping one array to another (with transformation) */
void get_packed_cvs_block(int col, int row, A2Methods_UArray2 cvs_arr2b,
void *elem, void *block_info);
void get_packed_cvs_block(int col, int row, A2Methods_UArray2 cvs_arr2b,
void *elem, void *block_info);
void fill_transformed_from_rgb_flt(int col, int row, 
        A2Methods_UArray2 curr_arr, void *elem, void *trans_info);
void fill_transformed_from_rgb_ints(int col, int row,
        A2Methods_UArray2 trim_arr, void *elem, void *trans_info);
void fill_rgb_ints_from_rgb_flt(int col, int row, A2Methods_UArray2 curr_arr, 
        void *elem, void *trans_info);
void fill_cvs_float_from_block(int col, int row,
        A2Methods_UArray2 codeword_info_arr2p, void *elem, void *cvs_arr2b);
void pack_codewords(int col, int row, A2Methods_UArray2 word_info_arr,
        void *elem, void *codewords_arr);
A2Methods_UArray2 create_blocked_arr(int width, int height, int size,
        int blocksize, A2Methods_T methods);

/* Transformation (helpers) */
void transform_rgb_floats_to_cvs(void *elem, struct pixel_float *rgb_pix);
void init_floats_from_rgb_ints(void *elem, Pnm_rgb pix_val, unsigned maxval);
void init_ints_from_rgb_floats(void *elem, struct pixel_float *float_rgb,
       unsigned maxval);
void transform_cvs_to_rgb_float(void *elem, struct pixel_float *cvs_pix);
void transform_luminance_to_coeffs(A2Methods_UArray2 cvs_arr,
        A2Methods_T methods, int col, int row, struct cvs_block *curr_iWord);
// unsigned transform_a_to_unsigned(float a);
void transform_coeffs_to_luminance(struct cvs_block *curr_iWord, 
struct pixel_float *b1_cvs, struct pixel_float *b2_cvs,
struct pixel_float *b3_cvs, struct pixel_float *b4_cvs);
signed convert_coeff_to_signed(float coeff);
float convert_signed_to_coeff(signed coeff);


void unpack_codewords(int col, int row, A2Methods_UArray2 codewords,
        void *elem, void *codeword_info);

/* Other functions */
int make_even(int dim);
struct copy_info *initialize_copy_info(A2Methods_UArray2 prev_arr,
        A2Methods_T prev_methods, void (*transFun)());
void set_unquantized_chroma_for_block(struct cvs_block *curr_iWord,
        struct pixel_float *b1_cvs, struct pixel_float *b2_cvs,
        struct pixel_float *b3_cvs, struct pixel_float *b4_cvs);

/* Decompression functions */
void decompress40 (FILE *input);
A2Methods_UArray2 create_pixels_from_header(FILE *in, unsigned *width,
        unsigned *height);

/* Print Functions */
void print_codewords(A2Methods_UArray2 codewords, int width, int height);
void print_word(int col, int row, A2Methods_UArray2 codewords, void *elem,
        void* cl);
void populate_codewords(int col, int row, A2Methods_UArray2 codewords,
        void *elem, void *input_file);

int main(int argc, char *argv[])
{
        int i;

        for (i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-c") == 0) {
                        compress_or_decompress = compress40;
                } else if (strcmp(argv[i], "-d") == 0) {
                        compress_or_decompress = decompress40;
                } else if (*argv[i] == '-') {
                        fprintf(stderr, "%s: unknown option '%s'\n",
                                argv[0], argv[i]);
                        exit(1);
                } else if (argc - i > 2) {
                        fprintf(stderr, "Usage: %s -d [filename]\n"
                                "       %s -c [filename]\n",
                                argv[0], argv[0]);
                        exit(1);
                } else {
                        break;
                }
        }
        assert(argc - i <= 1);    /* at most one file on command line */
        if (i < argc) {
                FILE *fp = fopen(argv[i], "r");
                assert(fp != NULL);
                compress_or_decompress(fp);
                fclose(fp);
        } else {
                compress_or_decompress(stdin);
        }

        return EXIT_SUCCESS; 
}

/********** compress40 ********
 *
 * Description:
 *      Compresses the given PPM image file by packing pixel data into
 *      integer codewords. The compression process involves transforming the
 *      image data, creating packed codewords, and printing them to output.
 * Parameters:
 *      input: Pointer to input file containing the uncompressed PPM image data.
 * Return:
 *      None.
 * Expects:
 *      The input file pointer (input) is valid and opened in read mode.
 *      The input file contains valid uncompressed PPM image data.
 *      The compression process completes successfully.
 * Notes:
 *.
 ************************/ 
void compress40 (FILE *input)
{
        assert(input != NULL);
        /* C1 Initialize and populate pnm for uncompressed file */ 
        Pnm_ppm uncompressed = initialize_ppm(input);

        /* Initialize blocked methods */
        A2Methods_T methods_b = uarray2_methods_blocked;
        A2Methods_T methods_p = uarray2_methods_plain;
        assert(methods_b != NULL && methods_p != NULL);

        /* C2/C3 Create trimmed arr of rgb floats */
        A2Methods_UArray2 trimmed_rgb_flts = 
        create_trimmed_float_arr(uncompressed,
        make_even(uncompressed->width), 
        make_even(uncompressed->height));

        /* free RGB ints pixels array */
        Pnm_ppmfree(&uncompressed);
        uncompressed = NULL;

        int t_width = methods_p->width(trimmed_rgb_flts);
        int t_height = methods_p->height(trimmed_rgb_flts);

        /* Initialize blocked array with component video pixels */ 
        A2Methods_UArray2 comp_vid_arr = 
        create_blocked_arr(t_width,
        t_height,
        sizeof(struct pixel_float), BLOCKSIZE, methods_b);
         
        /* Transform RGB float --> CVS float */
        struct copy_info *info = initialize_copy_info(
        trimmed_rgb_flts, methods_p, transform_rgb_floats_to_cvs);
        methods_b->map_default(
            comp_vid_arr, fill_transformed_from_rgb_flt, info);

        /* Free RGB float array and transform info */
        methods_p->free(&trimmed_rgb_flts);
        trimmed_rgb_flts = NULL;
        free(info);

        /* Initialize array of cvs block coordinates with 1/4 dims and 
           transform info*/
        A2Methods_UArray2 word_info = methods_p->new(
        (t_width / BLOCKSIZE), (t_height / BLOCKSIZE), 
        sizeof(struct cvs_block));
        struct block_info *block_info = malloc(sizeof(struct block_info));
        assert(block_info != NULL);
        block_info->trans_cvs = word_info;
        block_info->arr_methods = methods_p;
        block_info->counter = 0;
        block_info->Pb_avg = 0;
        block_info->Pr_avg = 0;
        /* Convert CVS coords to unpacked codeword info ints */
        methods_b->map_default(comp_vid_arr, get_packed_cvs_block, block_info);

        /* Free CVS Array */
        methods_b->free(&comp_vid_arr);
        comp_vid_arr = NULL;
        free(block_info);

        /* Initalize array of packed codewords */
        A2Methods_UArray2 codewords = methods_p->new
        (t_width / BLOCKSIZE, t_height / BLOCKSIZE, sizeof(int64_t));

        /* Compress codeword info into 32 bit codewords */
        methods_p->map_row_major(word_info, pack_codewords, codewords);

        /* Free word_info array (CVS_block info structs) */
        methods_p->free(&word_info);
        word_info = NULL;

        /* Print codewords to output */
        print_codewords(codewords, t_width, t_height); 

        /* Free codewords array (int64_T) */
        methods_p->free(&codewords);
        codewords = NULL;
}

/********** decompress40 ******
 *
 * Description:
 *      Decompresses a compressed image file read from the provided input stream
 *      and writes the decompressed image to standard output.
 * Parameters:
 *      input: A pointer to the input file containing the compressed image.     
 * Return:
 *      void    
 * Expects:
 *      The input file pointer (input) must not be NULL.
 *      The compressed image format must adhere to the COMP40 Compressed image
 *      format version 2 specification.
 * Notes:
 *      The function assumes that the input file pointer (input) is valid and
 *      points to a properly formatted compressed image file. 
*************************/
void decompress40 (FILE *input)
{
        assert(input != NULL);
        /* Initialize plain and blocked methods */
        A2Methods_T methods_p = uarray2_methods_plain;        
        A2Methods_T methods_b = uarray2_methods_blocked;
        assert(methods_b != NULL && methods_p != NULL);

        unsigned width, height;

        /* Read header of input file and allocate array */
        A2Methods_UArray2 pixels_arr = create_pixels_from_header(input,
        &width, &height);

        /* Initialize local pixmap struct */
        struct Pnm_ppm pixmap = {
                .width = width,
                .height = height,
                .denominator = 255,
                .pixels = pixels_arr, 
                .methods = methods_p
        };

        /* Initalize and populate an array of packed codewords from ifile */
        A2Methods_UArray2 codewords = methods_p->new
        (pixmap.width / BLOCKSIZE, pixmap.height / BLOCKSIZE, sizeof(int64_t));
        methods_p->map_row_major(codewords, populate_codewords, input);

        /* Init/populate codeword info int array (Pb, Pr, a, b, c, d)
           (Packed codewords --> unpacked codewords) */
        A2Methods_UArray2 word_info_arr = methods_p->new(
            pixmap.width / BLOCKSIZE,
            pixmap.height / BLOCKSIZE,
            sizeof(struct cvs_block));
        methods_p->map_default(codewords, unpack_codewords, word_info_arr);

        /* Free codewords array */
        methods_p->free(&codewords);
        codewords = NULL;

        /* Init/Populate blocked array with component video pix floats:
           (CVS block ints --> CVS floats) */
        A2Methods_UArray2 comp_vid_arr = 
        create_blocked_arr(pixmap.width, pixmap.height,
         sizeof(struct pixel_float), BLOCKSIZE, methods_b);
        methods_p->map_row_major(word_info_arr, fill_cvs_float_from_block,
         comp_vid_arr);

        /* Free codeword info int array */
        methods_p->free(&word_info_arr);
        word_info_arr = NULL;

        /* Convert CVS floats to RGB floats array */
        struct copy_info *cvs_info = initialize_copy_info(comp_vid_arr,
        methods_b, transform_cvs_to_rgb_float);

        /* Initialize and populate RGB float array:
           (CVS floats --> RGB floats) */
        A2Methods_UArray2 rgb_float_arr = methods_p->new(
            pixmap.width,
            pixmap.height,
            sizeof(struct pixel_float)); 
        methods_p->map_default(rgb_float_arr, fill_transformed_from_rgb_flt, 
         cvs_info);

        /* Free CVS array and transformation info */
        methods_b->free(&comp_vid_arr);
        comp_vid_arr = NULL;
        free(cvs_info);

        /* Initialize rgb float to rgb int transfromation info */
        struct rgb_int_info *int_info = malloc(sizeof(struct rgb_int_info));
        assert(int_info != NULL);
        int_info->maxval = pixmap.denominator;
        int_info->rgb_floats = rgb_float_arr;
        int_info->rgb_flt_meths = methods_p;

        /* Populate RGB ints pixel array: (RGB floats --> RGB ints) */
        methods_p->map_default(pixmap.pixels, fill_rgb_ints_from_rgb_flt, 
        int_info);

        /* Free RGB float array and int info */
        methods_p->free(&rgb_float_arr);
        free(int_info);

        /* Print input PPM and free pixels array */
        Pnm_ppmwrite(stdout, &pixmap);
        methods_p->free(&pixmap.pixels);
}

/********** initialize_copy_info ********
 *
 * Description:
 *         Allocates memory for the copy_info structure
 *         and initializes fields with the provided parameters.
 * Parameters:
 *      prev_arr: A 2D array to be transformed.
 *      prev_methods: A structure containing methods for operating on the 
 *                                                          previous array.
 *      transFun: A function pointer to the transformation function.
 * Return:
 *      A pointer to the initialized copy_info structure.
 * Expects:
 *      -The prev_arr parameter is a valid 2D array.
 *      -The prev_methods parameter is a valid A2Methods_T structure.
 *      -The transFun parameter is a valid function pointer to the
 *       transformation function.
 *      -Memory allocation for the copy_info structure succeeds.
 * Notes: 
 *      - Client is responsible for freeing memory.
 ************************/
struct copy_info *initialize_copy_info(A2Methods_UArray2 prev_arr,
                         A2Methods_T prev_methods, void (*transFun)())
{
        /* ALlocate and intialize transformation info */
        struct copy_info *info = malloc(sizeof(struct copy_info));
        assert(info != NULL);

        info->prev_arr = prev_arr;
        info->prev_methods = prev_methods;
        info->transFun = transFun;

        return info;
}

/********** fill_transformed_from_rgb_flt ********
 *
 * Description:
 *      Fills a new array with transformed float values obtained from an RGB
        float array.
 * Parameters:
 *      col: The column index of the current element.
 *      row: The row index of the current element.
 *      curr_arr: The 2D array to be populated with transformed float values.
 *      elem: A pointer to the current element of the new array.
 *      trans_info: A pointer to the struct copy_info containing 
 *                                      transformation information.
 * Return:
 *      None.
 * Expects:
 *      -The col and row parameters are valid indices within the dimensions of
 *                                                                    curr_arr.
 *      -The curr_arr parameter is a valid 2D array.
 *      -The elem parameter is a valid pointer to the current element of 
 *                                                              curr_arr.
 *      -The trans_info parameter is a valid pointer to the struct copy_info 
 *                                                                 structure.
 * Notes:
 *      Retrieves the corresponding pixel value from the original image array,
 *      transforms it, and populates the current element of the new array with
 *      those values.
 ************************/
void fill_transformed_from_rgb_flt(int col, int row,
 A2Methods_UArray2 curr_arr, void *elem, void *trans_info)
{
        assert(elem != NULL);
        assert(curr_arr != NULL);
        struct copy_info *info = trans_info;

        /* Get element from transformer arr at analagous position */
        struct pixel_float *pix_val = info->prev_methods->at(
                                        info->prev_arr, col, row);

        /* Populate new arr with transformed float vals*/
        info->transFun(elem, pix_val);
        
        (void) curr_arr;        
}

/********** fill_rgb_ints_from_rgb_flt ********
 *
 * Description:
 *      Fills a new array with RGB integer values obtained from an RGB float 
        array.
 * Parameters:
 *      col: The column index of the current element.
 *      row: The row index of the current element.
 *      curr_arr: The 2D array to be populated with RGB integer values.
 *      elem: A pointer to the current element of the new array.
 *      trans_info: A pointer to the struct rgb_int_info containing 
 *                                       transformation information.
 * Return:
 *      None.
 * Expects:
 *      -The col and row parameters are valid indices within the dimensions of
 *                                                               curr_arr.
 *      -The curr_arr parameter is a valid 2D array.
 *      -The elem parameter is a valid pointer to the current element of 
 *                                                               curr_arr.
 *      -The trans_info parameter is a valid pointer to the struct rgb_int_info
 *                                                              structure.
 * Notes:
 *      -Retrieves corresponding pixel value from original RGB float array,
 *      transforms it into integer vals, and populates the current element of
 *      the new array with those RGB integer values.
 ************************/
void fill_rgb_ints_from_rgb_flt(int col, int row, A2Methods_UArray2 curr_arr,
                                                void *elem, void *trans_info)
{
        assert(elem != NULL);
        assert(curr_arr != NULL);
        
        struct rgb_int_info *info = trans_info; 

        /* Get element from orig img arr analagous position */
        struct pixel_float *pix_val = info->rgb_flt_meths->at(
                                        info->rgb_floats, col, row);

        /* Populate new arr with transformed float vals*/
        init_ints_from_rgb_floats(elem, pix_val, info->maxval);
        (void) curr_arr;        
}

/********** create_blocked_arr ********
 *
 * Description:
 *      Creates a new 2D array with blocked layout using the specified 
 *      dimensions, element size, block size, and A2Methods_T interface.
 * Parameters:
 *      width: The width of the 2D array.
 *      height: The height of the 2D array.
 *      size: The size of each element in bytes.
 *      blocksize: The size of each block in the blocked layout.
 *      methods: A pointer to the A2Methods_T interface for array manipulation.
 * Return:
 *      A new 2D array with blocked layout.
 * Expects:
 *      -The methods parameter is not NULL.
 *      -The specified dimensions (width and height) are non-negative.
 *      -The size parameter is positive.
 *      -The blocksize parameter is positive.
 * Notes:
 *      Initializes a new 2D array with blocked layout using the provided
 *      parameters and the specified array manipulation methods.
 ************************/
A2Methods_UArray2 create_blocked_arr(int width, int height, int size,
int blocksize, A2Methods_T methods)
{
        assert(methods != NULL);

        /* Initialize a blocked array with given dimensions*/
        A2Methods_UArray2 blocked_arr = methods->new_with_blocksize(
            width,
            height,
            size,
            blocksize);
        assert(blocked_arr != NULL);
         
        return blocked_arr;
}

/********** make_even ********
 *
 * Description:
 *      Adjusts the given dimension to be even if it is odd.
 * Parameters:
 *      dim: The dimension to be adjusted.
 * Return:
 *      The adjusted dimension, ensuring it is even.
 * Expects:
 *      None.
 * Notes:
 *      If the given dimension is odd, it is decremented to make it even.
 ************************/
int make_even(int dim)
{
        /* If dimension is even, return one less */
        if (!is_even(dim))
        {
                return --dim;
        }
        return dim;
}

/********** create_trimmed_float_arr ********
 *
 * Description:
 *      Creates a new 2D array of floating-point pixels with specified 
 *      dimensions, derived from the given PPM image.
 * Parameters:
 *      image: The input PPM image from which data is derived.
 *      width: The width of the new 2D array.
 *      height: The height of the new 2D array.
 * Return:
 *      A new 2D array of floating-point pixels.
 * Expects:
 *      The image pointer is not NULL and contains valid data.
 *      The specified dimensions (width and height) are non-negative.
 * Notes:
 *      Initializes a new 2D array of floating-point pixels using the provided 
 *      PPM image and its dimensions, populating it with data from the original
 *      PPM image.
 ************************/
A2Methods_UArray2 create_trimmed_float_arr(Pnm_ppm image, int width, int height)
{
        /* Initialize a new 2D array with new dims */
        A2Methods_UArray2 trim_arr = image->methods->new(
            width,
            height,
            sizeof(struct pixel_float));

        /* Populate with data from old array */
        A2Methods_mapfun *map = image->methods->map_default;
        map(trim_arr, fill_transformed_from_rgb_ints, image);

        return trim_arr;
}

/********** fill_transformed_from_rgb_ints ********
 *
 * Description:
 *      Fills a trimmed array of floating-point pixels from an array of RGB 
 *      integer pixels.
 * Parameters:
 *      col: Column index of the current pixel.
 *      row: Row index of the current pixel.
 *      trim_arr: Trimmed array of floating-point pixels to fill.
 *      elem: Pointer to the current element in the trimmed array.
 *      og_img: Pointer to the original Pnm_ppm structure containing RGB 
 *                                                              integer pixels.
 * Return:
 *      None
 * Expects:
 *      -The trim_arr pointer is not NULL.
 *      -The elem pointer is not NULL.
 *      -The og_img pointer is not NULL and points to a valid Pnm_ppm 
 *                                                         structure.
 *      -The orig Pnm_ppm structure contains valid data.
 * Notes:
 *      Fills the trimmed array with floating-point pixel values
 *      transformed from RGB integer pixel values from the original image.
 ************************/
void fill_transformed_from_rgb_ints(int col, int row, 
        A2Methods_UArray2 trim_arr, void *elem, void *og_img)
{
        assert(elem != NULL);
        assert(trim_arr != NULL);

        /* Get element from orig img arr at analagous position */
        Pnm_ppm orig = (Pnm_ppm)og_img;
        Pnm_rgb pix_val = orig->methods->at(orig->pixels, col, row);

        /* Transform rgb ints to scaled floats and update trim array */
        init_floats_from_rgb_ints(elem, pix_val, orig->denominator);

        (void) trim_arr;        
}

/********** is_even ********
 *
 * Description:
 *      Checks if an integer is even.
 * Parameters:
 *      x: Integer value to check.
 * Return:
 *      Returns true if the integer is even, false otherwise.
 * Expects:
 *      None
 * Notes:
 *      None
 ************************/
bool is_even(int x)
{
        return (x % 2 == 0);
}

/********** initialize_ppm ********
 *
 * Description:
 *      Initializes a PPM image structure by reading from a file.
 * Parameters:
 *      input: A pointer to the input file.
 * Return:
 *      Returns a Pnm_ppm structure representing the image read from the file.
 * Expects:
 *      -The input file pointer must not be NULL.
 *      -The input file must contain a valid PPM image.
 * Notes:
 *       Assumes default to UArray2 methods.
 ************************/
Pnm_ppm initialize_ppm(FILE *input)
{       
        assert(input != NULL);
        
        /* Default to UArray2 methods */
        A2Methods_T methods = uarray2_methods_plain;
        assert(methods != NULL);
        /* Populate pnm ppm from input file */
        Pnm_ppm image = Pnm_ppmread(input, methods);

        return image;
}

/********** get_packed_cvs_block ********
 *
 * Description:
 *      Callback function used to process each element in a block of the chroma
 *      subsampled image array and pack the information into a structure.
 * Parameters:
 *      col: The column index of the current element in the chroma subsampled 
 *                                                                image array.
 *      row: The row index of the current element in the chroma subsampled 
 *                                                              image array.
 *      cvs_arr2b: The 2D array containing the chroma subsampled image.
 *      elem: A pointer to the current element in the chroma subsampled image 
 *      array.
 *      block_info: A pointer to a struct containing information about the 
 *                                                      block being processed.
 * Return:
 *      Does not return a value directly, but updates the block_info structure 
 *                                                     with packed information.
 * Expects:
 *      -The col and row indices must be within the bounds of the chroma 
 *                                                      subsampled image array.
 *      -The cvs_arr2b array must not be NULL.
 *      -The elem pointer must point to a valid element in the chroma 
 *                                              subsampled image array.
 *      -The block_info pointer must point to a valid struct containing
 *                               information about the block being processed.
 * Notes:
 *      -Callback function for mapping over a 2D array.
 *      -Calculates average chroma values for a block and packs the information
 *                                                               into a struct.
 *      -Transforms luminance values to cosine coefficients for the block.
 ************************/
void get_packed_cvs_block(int col, int row, A2Methods_UArray2 cvs_arr2b,
void *elem, void *block_info)
{
        assert(cvs_arr2b != NULL);
        assert(elem != NULL);
        
        /* Cast void pointers */
        struct block_info *info = block_info;
        struct pixel_float *cvs_curr = (struct pixel_float *)elem;

        /* If in a block, update numerator for Pb, Pr averages */        
        if (info->counter < (BLOCKSIZE * BLOCKSIZE)) {
                /* Increase numerators of Pb and Pr avgs*/
                info->Pr_avg += cvs_curr->b_Pr;
                info->Pb_avg += cvs_curr->g_Pb;
                info->counter++;
        }

        /* At the last cell of the block, calculate averages and cos coeffs */
        if (info->counter == (BLOCKSIZE * BLOCKSIZE)) {
                /* Get element at transformed index of cvs arr2p info array */
                struct cvs_block *curr_iWord = 
                        info->arr_methods->at(info->trans_cvs,
                        col / BLOCKSIZE,
                        row / BLOCKSIZE);

                /* Calculate avg chroma float vals */
                info->Pr_avg = info->Pr_avg / (BLOCKSIZE * BLOCKSIZE);
                info->Pb_avg = info->Pb_avg / (BLOCKSIZE * BLOCKSIZE);

                /* Populate Pb and Pr indices for codeword packing */
                curr_iWord->Pb_index = Arith40_index_of_chroma(info->Pb_avg);
                curr_iWord->Pr_index = Arith40_index_of_chroma(info->Pr_avg);

                /* Transform luminance values to cosine coefficients */
                transform_luminance_to_coeffs(
                    cvs_arr2b, uarray2_methods_blocked, col, row, curr_iWord);

                /* Reset vars restart for next block */
                info->counter = 0;
                info->Pb_avg = 0;
                info->Pr_avg = 0;
        }

        (void)cvs_arr2b;
}

/********** fill_cvs_float_from_block ********
 *
 * Description:
 *      Fills the chroma vector space (CVS) array of floats with values derived 
 *      from the cosine coefficients and chroma indices stored in a block 
 *      structure.
 * Parameters:
 *      col: The column index of the current block in the codeword information 
 *                                                                      array.
 *      row: The row index of the current block in the codeword information 
 *                                                                      array.
 *      codeword_info_arr2p: Unused parameter (pointer to the codeword 
 *                                                      information array).
 *      elem: Pointer to the current block structure containing cosine 
 *                                      coefficients and chroma indices.
 *      cvs_arr2b: Pointer to the CVS array represented as a blocked 2D array 
 *                                                                    of floats.
 * Return:
 *      This function does not return a value.
 * Expects:
 *      -The col and row parameters must represent valid indices within the 
 *                                                      array dimensions.
 *      -The elem parameter must point to a valid block structure containing
 *                                      cosine coefficients and chroma indices.
 *      -The cvs_arr2b parameter must point to a valid blocked 2D array of 
 *      floats 
 *                                         representing the chroma vector space.
 * Notes:
 *      Fills the CVS array with float values derived from the cosine 
 *      coefficients and chroma indices stored in the block structure.
 ************************/
void fill_cvs_float_from_block(int col, int row,
A2Methods_UArray2 codeword_info_arr2p, void *elem, void *cvs_arr2b)
{
        assert(codeword_info_arr2p != NULL);

        /* Initialize codewords_info plain array methods */
        A2Methods_T meth_b = uarray2_methods_blocked;
        A2Methods_UArray2 cvs_arr = (A2Methods_UArray2 *)cvs_arr2b;

        /* Get word info for info corresponding to codeword at curr index */
        struct cvs_block *curr_iWord = (struct cvs_block *)elem;   

        /* Get indices of cells in block that maps to curr word index */
        struct pixel_float *b1_cvs = meth_b->at(
            cvs_arr, (col * 2), (row * 2));
        struct pixel_float *b2_cvs = meth_b->at(
            cvs_arr, (col * 2) + 1, (row * 2));
        struct pixel_float *b3_cvs = meth_b->at(
            cvs_arr, (col * 2), ((row * 2) + 1));
        struct pixel_float *b4_cvs = meth_b->at(
            cvs_arr, (col * 2) + 1, ((row * 2) + 1));

        /* Set float avg Pb and Pr values for each block from US indices */
        set_unquantized_chroma_for_block(
            curr_iWord, b1_cvs, b2_cvs, b3_cvs, b4_cvs);

        /* Transform cosine coefficients to luminance values */
        transform_coeffs_to_luminance(
            curr_iWord, b1_cvs, b2_cvs, b3_cvs, b4_cvs);

        (void)codeword_info_arr2p;
}

/********** set_unquantized_chroma_for_block ********
 *
 * Description:
 *      Sets the unquantized chroma (Pb and Pr) values for the four blocks in 
 *      the chroma vector space (CVS) array corresponding to the current block 
 *                                                                      index.
 * Parameters:
 *      curr_iWord: Pointer to the current block structure containing chroma 
 *                                                                    indices.
 *      b1_cvs: Pointer to the top-left block in the CVS array.
 *      b2_cvs: Pointer to the top-right block in the CVS array.
 *      b3_cvs: Pointer to the bottom-left block in the CVS array.
 *      b4_cvs: Pointer to the bottom-right block in the CVS array.
 * Return:
 *      This function does not return a value.
 * Expects:
 *      - The curr_iWord parameter must point to a valid block structure 
 *        containing chroma indices.
 *      - The b1_cvs, b2_cvs, b3_cvs, and b4_cvs parameters must point to 
 *        valid blocks in the chroma vector space array.
 * Notes:
 *    Sets the unquantized chroma (Pb and Pr) values for each block in the CVS 
 *    array based on the chroma indices stored in the current block structure.
 *    Sets the same chroma values for all four blocks within the same block 
 *    group.
 ************************/
void set_unquantized_chroma_for_block(struct cvs_block *curr_iWord, 
struct pixel_float *b1_cvs, struct pixel_float *b2_cvs,
struct pixel_float *b3_cvs, struct pixel_float *b4_cvs)
{
        assert(b1_cvs != NULL && b2_cvs != NULL &&
                b3_cvs != NULL && b4_cvs != NULL);
        assert(curr_iWord);
        
        /* Unquantize chroma and set all block cells to avg chroma vals */
        b1_cvs->g_Pb = Arith40_chroma_of_index(curr_iWord->Pb_index);
        b1_cvs->b_Pr = Arith40_chroma_of_index(curr_iWord->Pr_index);

        b2_cvs->g_Pb = Arith40_chroma_of_index(curr_iWord->Pb_index);
        b2_cvs->b_Pr = Arith40_chroma_of_index(curr_iWord->Pr_index);

        b3_cvs->g_Pb = Arith40_chroma_of_index(curr_iWord->Pb_index);
        b3_cvs->b_Pr = Arith40_chroma_of_index(curr_iWord->Pr_index);

        b4_cvs->g_Pb = Arith40_chroma_of_index(curr_iWord->Pb_index);
        b4_cvs->b_Pr = Arith40_chroma_of_index(curr_iWord->Pr_index);
}

/********** pack_codewords ********
 *
 * Description:
 *      Packs the information from the codeword information array into a new 2D 
 *      array of 64-bit codewords.
 * Parameters:
 *      col: The column index of the current element in the codeword   
 *      information array.
 *      row: The row index of the current element in the codeword information 
 *           array.
 *      word_info_arr: The 2D array containing information about packed 
 *      codewords.
 *      elem: Pointer to the current element in the codeword information array.
 *      codewords_arr: Pointer to the new 2D array of 64-bit codewords to be 
 *      populated.
 * Return:
 *      This function does not return a value.
 * Expects:
 *      -The col and row indices must be within the bounds of the codeword 
 *      information array.
 *      -The word_info_arr array must not be NULL.
 *      -The elem pointer must point to a valid element in the codeword 
 *       information array.
 *      -The codewords_arr pointer must point to a valid 2D array of 64-bit 
 *       codewords.
 * Notes:
 *      Packs the information from the codeword information array into a new
 *      2D array of 64-bit codewords. Updates the codewords array with the 
 *      packed 
 *      information from the codeword information array.
 ************************/
void pack_codewords(int col, int row, A2Methods_UArray2 word_info_arr,
void *elem, void *codewords_arr)
{
        assert(word_info_arr != NULL);
        assert(elem != NULL);

        A2Methods_T methods = uarray2_methods_plain;
        A2Methods_UArray2 codewords = (A2Methods_UArray2 *)codewords_arr;
        struct cvs_block *word_info = (struct cvs_block *)elem;
        int64_t *curr_word = methods->at(codewords, col, row);

        /* Pack bits in Big-Endian Order (MSB first) */
        *curr_word = Bitpack_newu(
            *curr_word, Pr_WIDTH, Pr_LSB, word_info->Pr_index);
        *curr_word = Bitpack_newu(
            *curr_word, Pb_WIDTH, Pb_LSB, word_info->Pb_index);
        *curr_word = Bitpack_news(*curr_word, d_WIDTH, d_LSB, word_info->d);
        *curr_word = Bitpack_news(*curr_word, c_WIDTH, c_LSB, word_info->c);
        *curr_word = Bitpack_news(*curr_word, b_WIDTH, b_LSB, word_info->b);
        *curr_word = Bitpack_newu(*curr_word, a_WIDTH, a_LSB, word_info->a);

        (void)word_info_arr;
}

/********** print_codewords ********
 *
 * Description:
 *      Prints the information from the 2D array of 64-bit codewords to 
 *      standard output.
 * Parameters:
 *      codewords: The 2D array containing 64-bit codewords.
 *      width: The width of the codewords array.
 *      height: The height of the codewords array.
 * Return:
 *      This function does not return a value.
 * Expects:
 *      -The codewords array must not be NULL.
 *      -The width and height parameters must be non-negative.
 * Notes:
 *      Prints the information from the 2D array of 64-bit codewords to 
 *                                                      standard output.
 ************************/
void print_codewords(A2Methods_UArray2 codewords, int width, int height)
{
        assert(codewords != NULL);
        A2Methods_T methods = uarray2_methods_plain;
        
        /* Print header */
        printf("COMP40 Compressed image format 2\n%u %u\n", width, height);

        /* Iterate through the codewords and print in Big-Endian */
        methods->map_row_major(codewords, print_word, NULL);

        (void) codewords;
}

/********** print_word ********
 *
 * Description:
 *      Prints the characters encoded in a 64-bit codeword to standard output.
 * Parameters:
 *      col: The column index of the current codeword.
 *      row: The row index of the current codeword.
 *      codewords: A2Methods_UArray2 containing the codewords.
 *      elem: Pointer to the current codeword.
 *      cl: Pointer to client data (unused).
 * Return:
 *      This function does not return a value.
 * Expects:
 *      -The elem parameter must point to a valid 64-bit integer representing
 *                                                                 a codeword.
 *      -The codewords parameter must point to a valid A2Methods_UArray2 
 *                                              containing the codewords.
 * Notes:
 *      This function extracts and prints characters encoded in a 64-bit 
 *      codeword by splitting it into four 8-bit chunks.
 ************************/
void print_word(int col, int row, A2Methods_UArray2 codewords,
        void *elem, void* cl)
{
        assert(codewords != NULL);
        int64_t *curr_word = (int64_t *)elem;

        /* Fill chars by 8 bits starting at codeword MSB*/
        char one = Bitpack_getu(
            *curr_word, CHAR_BITS, WORD_BITS - (CHAR_BITS * 1));
        char two = Bitpack_getu(
            *curr_word, CHAR_BITS, WORD_BITS - (CHAR_BITS * 2));
        char three = Bitpack_getu(
            *curr_word, CHAR_BITS, WORD_BITS - (CHAR_BITS * 3));
        char four = Bitpack_getu(
            *curr_word, CHAR_BITS, WORD_BITS - (CHAR_BITS * 4));

        putchar(one);
        putchar(two);
        putchar(three);
        putchar(four);

        (void)col;
        (void)row;
        (void)cl;
        (void)codewords;
}

/********** create_pixels_from_header ********
 *
 * Description:
 *      Creates a 2D array of Pnm_rgb pixels based on the width and height 
 *      obtained from the header of a compressed image file.
 * Parameters:
 *      in: Pointer to the input file.
 *      width: Pointer to store the width of the image.
 *      height: Pointer to store the height of the image.
 * Return:
 *      A2Methods_UArray2: A 2D array of Pnm_rgb pixels.
 * Expects:
 *      -The in parameter must point to a valid FILE stream.
 *      -The width and height parameters must point to valid unsigned integers.
 *      -The header of the input file must match the expected format.
 *      -Memory allocation for the 2D array must be successful.
 *      -The input file must contain valid width and height values.
 * Notes:
 *      Reads the width and height from the header of the input file and 
 *      creates a new 2D array of Pnm_rgb pixels using the obtained dimensions.
 *      It is assumed that the input file follows the COMP40 compressed image 
 *      format 2.
 ************************/
A2Methods_UArray2 create_pixels_from_header(FILE *in, unsigned *width, 
        unsigned *height) 
{
        assert(in != NULL);
        assert(width != NULL);
        assert(height != NULL);

        /* Set the width and height of pixels array and read up to newline */
        int read = fscanf(in, "COMP40 Compressed image format 2\n%u %u",
                width, height);
        assert(read == 2);
        int c = getc(in);
        assert(c == '\n');

        /* Initialize pixels array */
        A2Methods_T meths = uarray2_methods_plain;
        A2Methods_UArray2 pixels_arr = meths->new(
            *width, *height, sizeof(struct Pnm_rgb));

        return pixels_arr;
}

/********** populate_codewords ********
 *
 * Description:
 *   Reads characters from the input file and populates the elements of the
 *   codewords array with 64-bit codewords, each constructed from four 
 *   consecutive
 *   characters read from the input file. The characters are packed into the
 *   codewords starting from the most significant bits (MSBs) in 8-bit 
 *   increments.
 *      
 * Parameters:
 *   col:          The column index of the current element being processed.
 *   row:          The row index of the current element being processed.
 *   codewords:    A 2D array storing the codewords to be populated.
 *   elem:         A pointer to the current element in the codewords array.
 *   input_file:   A pointer to the input file from which characters are read.
 *      
 * Return:
 *   void
 *      
 * Expects:
 *   -The parameters 'codewords', 'elem', and 'input_file' must not be NULL.
 *   -The 'codewords' array must be initialized and have sufficient dimensions
 *     to accommodate the specified row and column indices.
 *   -The input file must be open and readable.
 *   -The input file must contain sufficient characters to populate the 
 *     codewords array elements.
 *    
 * Notes:
 *      Modifies the elements of the codewords array based on the characters 
 *      read from the input file. Memory management for the codewords array and 
 *      input file is the responsibility of the caller.
*******/
void populate_codewords(int col, int row, A2Methods_UArray2 codewords,
        void *elem, void *input_file)
{
        assert(codewords != NULL);
        assert(elem != NULL);
        assert(input_file != NULL);

        FILE *input = (FILE *)input_file;
        int64_t *curr_word = (int64_t *)elem;

        /* Read in character by character and fill codeword*/
        char c = getc(input);
        /* Fill chars by 8 bits starting at codeword MSB*/
        int64_t mod_one = Bitpack_news(
            *curr_word, CHAR_BITS, WORD_BITS - (CHAR_BITS * 1), c);
        c = getc(input);
        int64_t mod_two = Bitpack_news(
            mod_one, CHAR_BITS, WORD_BITS - (CHAR_BITS * 2), c);
        c = getc(input);
        int64_t mod_three = Bitpack_news(
            mod_two, CHAR_BITS, WORD_BITS - (CHAR_BITS * 3), c);
        c = getc(input);
        int64_t encoded_word = Bitpack_news(
            mod_three, CHAR_BITS, WORD_BITS - (CHAR_BITS * 4), c);

        /* Set populated codeword once all chars have been read in */
        *curr_word = encoded_word;

        (void)codewords;
        (void)col;
        (void)row;
}



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
************************/
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
************************/
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
