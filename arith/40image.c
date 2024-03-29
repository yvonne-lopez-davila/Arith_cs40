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

// todo: when converting from float-> int, for better precision use math.h function that rounds based on decimal value (vs typecasting which will always round down)

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "assert.h"
#include "compress40.h"
#include "pnm.h"
#include "a2methods.h"
#include "a2plain.h"
#include "a2blocked.h"
#include "arith40.h"

const int BLOCKSIZE = 2;
const float FL_QUANT_RANGE = 0.3;
const float D_QUANT_RANGE = 15;

// hold floating pt representations of a pixel
        // rgb floats
        // cvs floats
struct pixel_float
{
        float r_Y;
        float g_Pb;
        float b_Pr;
};

struct cvs_block
{
        unsigned Pb_index;
        unsigned Pr_index;
        unsigned a;
        signed b;
        signed c;
        signed d;

};

struct copy_info
{
        A2Methods_UArray2 prev_arr;
        A2Methods_T prev_methods;
        void (*transFun)();
        unsigned pnm_maxval;
};

struct block_info
{
        A2Methods_UArray2 trans_cvs; // transformed CVS coords
        A2Methods_T arr_methods;
        int counter;
        float Pb_avg;
        float Pr_avg; 
};

static void (*compress_or_decompress)(FILE *input) = compress40;

unsigned Arith40_index_of_chroma(float chroma); // todo delete

/* Compression functions */
Pnm_ppm initialize_ppm(FILE *input);
void compress40 (FILE *input);
A2Methods_UArray2 create_trimmed_float_arr(Pnm_ppm image, int width, int height);
bool is_even(int x);
void get_packed_cvs_block(int col, int row, A2Methods_UArray2 cvs_arr2b, void *elem, void *block_info);

/* Mapping one array to another (with transformation) */
void fill_transformed_from_rgb_flt(int col, int row, A2Methods_UArray2 curr_arr, void *elem, void *trans_info);
void fill_transformed_from_rgb_ints(int col, int row, A2Methods_UArray2 trim_arr, void *elem, void *trans_info); // todo maybe merge
void fill_cvs_float_from_block(int col, int row, A2Methods_UArray2 cvs_arr2b, void *elem, void *codeword_info_arr2p);
void fill_int_arr_from_rgb_flt(int col, int row, A2Methods_UArray2 curr_arr, void *elem, void *trans_info);

/* Transformation (helpers) */
void transform_rgb_floats_to_cvs(void *elem, struct pixel_float *rgb_pix);
void transform_rgb_ints_to_floats(void *elem, Pnm_rgb pix_val, int pnm_maxval);
void transform_rgb_floats_to_ints(void *elem, struct pixel_float *float_rgb, unsigned maxval);
void transform_cvs_to_rgb_float(void *elem, struct pixel_float *cvs_pix);
A2Methods_UArray2 create_blocked_arr(int width, int height, int size, int blocksize, A2Methods_T methods);
void transform_luminance_to_coeffs(A2Methods_UArray2 cvs_arr, A2Methods_T methods, int col, int row, struct cvs_block *curr_iWord);
void transform_coeffs_to_luminance(struct cvs_block *curr_iWord, struct pixel_float *b1_cvs, struct pixel_float *b2_cvs, struct pixel_float *b3_cvs, struct pixel_float *b4_cvs);
unsigned transform_a_to_unsigned(float a);
void set_unquantized_chroma_for_block(struct cvs_block *curr_iWord, struct pixel_float *b1_cvs, struct pixel_float *b2_cvs, struct pixel_float *b3_cvs, struct pixel_float *b4_cvs);

int make_even(int dim);
signed convert_coeff_to_signed(float coeff);

struct copy_info *initialize_copy_info(A2Methods_UArray2 prev_arr, A2Methods_T prev_methods, void (*transFun)());

/* Decompression functions */
void decompress40 (FILE *input);
//void pop_with_rgb_ints(int col, int row, A2Methods_UArray2 rgb_int_arr, void *elem, void *rgb_float_arr); // deleted

void decompressTest(A2Methods_UArray2 rgb_float_arr, Pnm_ppm uncompressed); // todo delete testing function

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

// Compress40 TODO (function contract)
//todo maybe no new line needs to be added
void compress40 (FILE *input)
{
        /* C1 Initialize and populate pnm for uncompressed file */ 
        Pnm_ppm uncompressed = initialize_ppm(input); /* rgb ints */
        // decompressTest(uncompressed); // passed C1 todo del E = 0.0000
     
        //fprintf(stderr, "Maxval: %u \n", uncompressed->denominator);

        /* Initialize blocked methods */
        A2Methods_T methods_b = uarray2_methods_blocked;
        A2Methods_T methods_p = uarray2_methods_plain;
        //A2Methods_mapfun *map_b = methods_b->map_default; // block maj
        //A2Methods_mapfun *map_p = methods_p->map_default; // row maj
        assert(methods_b != NULL && methods_p != NULL);

        /* C2/C3 Create trimmed arr of rgb floats */
        // could be loss from trimming and from converting int to float
        A2Methods_UArray2 trimmed_rgb_flts = 
        create_trimmed_float_arr(uncompressed,
        make_even(uncompressed->width), 
        make_even(uncompressed->height)); // C3 passed
        //decompressTest(trimmed_rgb_flts, uncompressed); // E = 0.0000

////////////////////////////////////////////////////////////////////////////
        /* Initialize blocked array with component video pixels */ 
         A2Methods_UArray2 comp_vid_arr = 
         create_blocked_arr(methods_p->width(trimmed_rgb_flts),
         methods_p->height(trimmed_rgb_flts),
         sizeof(struct pixel_float), BLOCKSIZE, methods_b);
         //decompressTest(comp_vid_arr, uncompressed); // E = .0023
/*
         fprintf(stderr, "WIDTH: %d \n", methods_b->width(comp_vid_arr));
         fprintf(stderr, "HEIGHT: %d \n", methods_b->height(comp_vid_arr));
*/

         /* C4 Updates CVS arr to hold values transformed from RGB floats */
         /* Initialize rgb to cvs transfromation info */
        // struct copy_info *info = initialize_copy_info(trimmed_rgb_flts, methods_p, transform_rgb_floats_to_cvs); // todo free

        struct copy_info *info = malloc(sizeof(struct copy_info));
        assert(info != NULL);

        info->prev_arr = trimmed_rgb_flts;
        info->prev_methods = methods_p;
        info->transFun = transform_rgb_floats_to_cvs;
        info->pnm_maxval = -1; // unused here

         /* Map rgb floats to cvs*/
         methods_b->map_default(comp_vid_arr, fill_transformed_from_rgb_flt, info);
  //       printf("COMPRESSION done \n");
        // decompressTest(comp_vid_arr, uncompressed); E = .0023
////////////////////////////////////////////////////////////////////////////

        /* C6 Convert chroma avgs to quantized, luminance to cos coeffs*/
        /* Initialize array of cvs block coordinates with 1/BLOCKSIZE dims*/
        A2Methods_UArray2 codeword_info_arr = methods_p->new((methods_p->width(trimmed_rgb_flts)) / BLOCKSIZE, (methods_p->height(trimmed_rgb_flts)) / BLOCKSIZE, sizeof(struct cvs_block));

        /* Initialize cl info for cvs transformation */
        struct block_info *block_info = malloc(sizeof(struct block_info)); 
        assert(block_info != NULL);
        block_info->trans_cvs = codeword_info_arr; // this becomes param of decompress
        block_info->arr_methods = methods_p;
        block_info->counter = 0;
        block_info->Pb_avg = 0;
        block_info->Pr_avg = 0;
        
        /* Populates codeword info array with coords for codeword packing */
        methods_b->map_default(comp_vid_arr, get_packed_cvs_block, block_info);
        decompressTest(codeword_info_arr, uncompressed); // E = .3553
        
/*  
        printf("W: %d \n ", uncompressed->width);
        printf("H: %d \n ", uncompressed->height);
*/
//todo comment back in after tests 
        // Pnm_ppmfree(&uncompressed);
        // uncompressed = NULL; //todo ptr make null after 
}

//function contract
// todo remove this function, just here for testing
// todo change input to correct input after compression has been fully implemented --> FILE *input
void decompressTest(A2Methods_UArray2 codeword_info_arr, Pnm_ppm uncompressed)
{

        /* Initialize plain and blocked methods */
        A2Methods_T methods_p = uarray2_methods_plain;        
        A2Methods_T methods_b = uarray2_methods_blocked;
        assert(methods_b != NULL && methods_p != NULL);

        /* Initialize blocked array with component video pixels */ 
        A2Methods_UArray2 comp_vid_arr = 
        create_blocked_arr((BLOCKSIZE * methods_p->width(codeword_info_arr)),
        (BLOCKSIZE * methods_p->height(codeword_info_arr)),
        sizeof(struct pixel_float), BLOCKSIZE, methods_b);

        /* D8 Convert cvs block ints to cvs floats */
        // iterate through comp_vid arr in block major order, and iterate 4 times more slowly through codeword_info_arr 
        methods_p->map_default(codeword_info_arr, fill_cvs_float_from_block, comp_vid_arr);

///////////////////////////////////////////////////////////////////
        /* D9 Convert CVS float to RGB floats */
        // initialize comp_vid_array info
        //struct copy_info *cvs_info = initialize_copy_info(comp_vid_arr, methods_b, transform_cvs_to_rgb_float); // todo free
        struct copy_info *cvs_info = malloc(sizeof(struct copy_info));
        assert(cvs_info != NULL);
        cvs_info->prev_arr = comp_vid_arr;
        cvs_info->prev_methods = methods_b;
        cvs_info->transFun = transform_cvs_to_rgb_float;
        cvs_info->pnm_maxval = 2;

        /* Create rgb float array */
        A2Methods_UArray2 rgb_float_arr = methods_p->new(methods_b->width(comp_vid_arr), methods_b->height(comp_vid_arr), sizeof(struct pixel_float)); 
/*
        fprintf(stderr, "w: %d, h: %d \n", methods_b->width(comp_vid_arr), methods_b->height(comp_vid_arr));
*/
        /* CVS float to RGB float */
        methods_p->map_default(rgb_float_arr, fill_transformed_from_rgb_flt, cvs_info);
        fprintf(stderr, "TRACE\n");
//////////////////////////////////////////////////////////////////////////

        /* D11 Populate 2D array of RGB int vals */
        /* Initialize a new 2D array with new dims */
        ////////////////////////////////////////////////////////////////////////

        // creates pixels arr of pnm rgb ints
        A2Methods_UArray2 pixel = methods_p->new(methods_p->width(rgb_float_arr), methods_p->height(rgb_float_arr), sizeof(struct Pnm_rgb));

        /* Initialize rgb float to rgb int transfromation info */
        /*
        struct copy_info *rgb_fl_info = initialize_copy_info(rgb_float_arr, methods_p, transform_rgb_floats_to_ints); // todo free
        */

        struct copy_info *rgb_fl_info = malloc(sizeof(struct copy_info));
        assert(rgb_fl_info != NULL);
        rgb_fl_info->prev_arr = rgb_float_arr;
        rgb_fl_info->prev_methods = methods_p;
        rgb_fl_info->transFun = transform_rgb_floats_to_ints;
        rgb_fl_info->pnm_maxval = uncompressed->denominator;

        fprintf(stderr, "MAXVAL BEFORE CALL: %u\n", rgb_fl_info->pnm_maxval);

        /* D10 Populate with RGB ints pixel array (RGB floats to RGB ints)*/
        // could lose a bit more data here because rounding floats to ints
        methods_p->map_default(pixel, fill_int_arr_from_rgb_flt, rgb_fl_info); // this is in C3 E test, which ppmdiffs to 0.0000

        ///////////////////////////////////////////////////////////////////////

        uncompressed->width = methods_p->width(pixel);
        uncompressed->height = methods_p->height(pixel);
        uncompressed->pixels = pixel;
        
        /* D12 Print uncompressed PPM and free PPM */
        Pnm_ppmwrite(stdout, uncompressed);
        Pnm_ppmfree(&uncompressed);


        // fclose(inputfile);  TODO put these in main after decompress
        // exit(EXIT_SUCCESS); TODO 
}

//function contract- copy_info
// create and return a struct
// note: client responsibility to free
struct copy_info *initialize_copy_info(A2Methods_UArray2 prev_arr, A2Methods_T prev_methods, void (*transFun)())
{
        struct copy_info *info = malloc(sizeof(struct copy_info));
        assert(info != NULL);

        info->prev_arr = prev_arr;
        info->prev_methods = prev_methods;
        info->transFun = transFun;

        return info;
}

// cl take in a struct with og_img and a function pointer,
// fill_transformed_from_rgb_flt TODO (function contract)
// curr array is comp_vid_arr
void fill_transformed_from_rgb_flt(int col, int row, A2Methods_UArray2 curr_arr, void *elem, void *trans_info)
{
        //fprintf(stderr, "(col, row) = %d, %d\n", col, row);
        struct copy_info *info = trans_info;

        /* Get element from orig img arr analagous position */
        struct pixel_float *pix_val = info->prev_methods->at(info->prev_arr, col, row); // casting a cvs float as an rgb float todo del

        /* Populate new arr with transformed float vals*/
        info->transFun(elem, pix_val); // 1st param is (current rgb float pixel), 2nd param holds rgb_float holding (cvs float) values todo del
/*
*/
        // fprintf(stderr, "r_val old = %.4f\n", pix_val->r_Y);
        //fprintf(stderr, "R new = %u\n", ((struct Pnm_rgb *)elem)->red);
        // fprintf(stderr, "G new = %u\n", ((struct Pnm_rgb *)elem)->green);
        // fprintf(stderr, "B new = %u\n", ((struct Pnm_rgb *)elem)->blue);

        (void) curr_arr;        
}

void fill_int_arr_from_rgb_flt(int col, int row, A2Methods_UArray2 curr_arr, void *elem, void *trans_info)
{
        //fprintf(stderr, "(col, row) = %d, %d\n", col, row);
        struct copy_info *info = trans_info;

        /* Get element from orig img arr analagous position */
        struct pixel_float *pix_val = info->prev_methods->at(info->prev_arr, col, row); // casting a cvs float as an rgb float todo del

        /* Populate new arr with transformed float vals*/
        transform_rgb_floats_to_ints(elem, pix_val, info->pnm_maxval); // 1st param is (current rgb float pixel), 2nd param holds rgb_float holding (cvs float) values todo del
/*
*/
        // fprintf(stderr, "r_val old = %.4f\n", pix_val->r_Y);
        //fprintf(stderr, "R new = %u\n", ((struct Pnm_rgb *)elem)->red);
        // fprintf(stderr, "G new = %u\n", ((struct Pnm_rgb *)elem)->green);
        // fprintf(stderr, "B new = %u\n", ((struct Pnm_rgb *)elem)->blue);

        (void) curr_arr;        
}

// todo contract- transform_rgb_floats_to_cvs
void transform_rgb_floats_to_cvs(void *elem, struct pixel_float *rgb_pix)
{
        struct pixel_float *cvs_pix = (struct pixel_float *)elem;
        
        cvs_pix->r_Y = .299 * rgb_pix->r_Y + 0.587 * rgb_pix->g_Pb + 0.114 * rgb_pix->b_Pr;
        cvs_pix->g_Pb = -0.168736 * rgb_pix->r_Y - 0.331264 *  rgb_pix->g_Pb + 0.5 * rgb_pix->b_Pr;
        cvs_pix->b_Pr = 0.5 * rgb_pix->r_Y - 0.418688 * rgb_pix->g_Pb - 0.081312 * rgb_pix->b_Pr;

        fprintf(stderr, "Y: %.4f\n", cvs_pix->r_Y);
        fprintf(stderr, "Pb: %.4f\n", cvs_pix->g_Pb);
        fprintf(stderr, "Pr: %.4f\n", cvs_pix->b_Pr);
}

// todo contract- transform_cvs_to_rgb_float
//(inverse of prev)
// CVS_FLOAT -> RGB FLOAT
void transform_cvs_to_rgb_float(void *elem, struct pixel_float *cvs_pix)
{
        struct pixel_float *rgb_pix = (struct pixel_float *)elem;
        
        rgb_pix->r_Y = 1.0 * cvs_pix->r_Y + 0.0 * cvs_pix->g_Pb + 1.402 * cvs_pix->b_Pr;
        rgb_pix->g_Pb = 1.0 * cvs_pix->r_Y - 0.344136 * cvs_pix->g_Pb - 0.714136 * cvs_pix->b_Pr;
        rgb_pix->b_Pr = 1.0 * cvs_pix->r_Y + 1.772 * cvs_pix->g_Pb + 0.0 * cvs_pix->b_Pr;

        fprintf(stderr, "CVS Y: %.4f \n", cvs_pix->r_Y);
        fprintf(stderr, "CVS Pb: %.4f \n", cvs_pix->g_Pb);
        fprintf(stderr, "CVS Pr: %.4f \n", cvs_pix->b_Pr);

        fprintf(stderr, "RGB float r: %.4f \n", rgb_pix->r_Y);
        fprintf(stderr, "RGB float g: %.4f \n", rgb_pix->g_Pb);
        fprintf(stderr, "RGB float b: %.4f \n", rgb_pix->b_Pr);
}


// Decompress40 TODO (function contract)
// todo change input to correct input after compression has been fully implemented --> FILE *input
void decompress40 (FILE *input)
{
        // TODO works when called from commandline, but not when called from compress

        /* D11 Populate 2D array of RGB int vals */
        
        Pnm_ppm uncompressed = initialize_ppm(input);

        /* D12 Print uncompressed PPM and free PPM */
        Pnm_ppmwrite(stdout, uncompressed);
        Pnm_ppmfree(&uncompressed);

        // fclose(inputfile);  TODO put these in main after decompress
        // exit(EXIT_SUCCESS); TODO   
        (void) input;       

}

//fucntion contract- create_blocked_arr
A2Methods_UArray2 create_blocked_arr(int width, int height, int size, int blocksize, A2Methods_T methods)
{
        assert(methods != NULL);

        A2Methods_UArray2 blocked_arr = methods->new_with_blocksize(width, height, size, blocksize);
        assert(blocked_arr != NULL);
         
        return blocked_arr;
}

//todo contract- make_even
int make_even(int dim)
{
        if (!is_even(dim))
        {
                return --dim;
        }
        return dim;
}

//trim_odd_dims TODO (function contract)
// todo can probably make this and above a single function if add size parameter and some way of knowing whether to set block size (pass in methods)
A2Methods_UArray2 create_trimmed_float_arr(Pnm_ppm image, int width, int height)
{
        /* Initialize a new 2D array with new dims */
        A2Methods_UArray2 trim_arr = image->methods->new(width, height, sizeof(struct pixel_float));

        /* Initialize prev array info */
        // todo helper
        //struct copy_info *pixels_info = initialize_copy_info(image->pixels, uarray2_methods_plain, transform_rgb_ints_to_floats);
        /*
        struct copy_info *pixels_info = malloc(sizeof(struct copy_info));
        assert(pixels_info != NULL);
        pixels_info->prev_arr = image->pixels;
        pixels_info->prev_methods = uarray2_methods_plain;
        pixels_info->transFun = transform_rgb_ints_to_floats;
        */

        /* Populate with data from old array */
        A2Methods_mapfun *map = image->methods->map_default;
        map(trim_arr, fill_transformed_from_rgb_ints, image);

        //fprintf(stderr, "POST WIDTH AND HEIGHT: %d, %d \n", width, height);

        return trim_arr;
}

//function contracts- transform_rgb_ints_to_floats
// Copy RGB values to pix in trimmed array
void transform_rgb_ints_to_floats(void *elem, Pnm_rgb pix_val, int pnm_maxval)
{
        struct pixel_float *curr_pix = (struct pixel_float *)elem;

        curr_pix->r_Y = ((float)pix_val->red) / pnm_maxval;
        curr_pix->g_Pb = ((float)pix_val->green) / pnm_maxval;
        curr_pix->b_Pr = ((float)pix_val->blue) / pnm_maxval;

        fprintf(stderr, "Red in transform: %u\n", pix_val->red);
        fprintf(stderr, "Green in transform: %u\n", pix_val->green);
        fprintf(stderr, "Blue in transform: %u\n", pix_val->blue);
}

//function contract- fill_transformed_from_rgb_ints
// todo cleanup merge copying functions: cl take in a struct with og_img and a function pointer,
// TODO MERGE
// apply TODO (function contract)
// rgb ints -> floats
void fill_transformed_from_rgb_ints(int col, int row, A2Methods_UArray2 trim_arr, void *elem, void *orig_ppm)
{
        //struct copy_info *info = trans_info;
        Pnm_ppm orig = (Pnm_ppm)orig_ppm;

        /* Get element from orig img arr analagous position */
        // todo: if we initialize this as a void ptr, can we reuse other func?
        //Pnm_rgb pix_val = info->prev_methods->at(info->prev_arr, col, row);
        Pnm_rgb pix_val = orig->methods->at(orig->pixels, col, row);
        
        // make this a function ptr instead
        transform_rgb_ints_to_floats(elem, pix_val, orig->denominator);
        //info->transFun(elem, pix_val);
/*
*/
        fprintf(stderr, "comp r_val int = %u\n", pix_val->red);
        //fprintf(stderr, "r_val new = %.4f\n", ((struct pixel_float *)elem)->r_Y);
        (void) trim_arr;        
}

// todo contract- transform_rgb_floats_to_ints
// updates int pixel to hold float values (rounded)
// RGB float to RGB int
void transform_rgb_floats_to_ints(void *elem, struct pixel_float *float_rgb, unsigned maxval)
{
        Pnm_rgb int_pix = (struct Pnm_rgb *)elem;
        fprintf(stderr, "MAXVAL: %u\n", maxval);
/*
        fprintf(stderr, "FLOAT VAL r: %.4f\n", float_rgb->r_Y);
        fprintf(stderr, "FLOAT VAL g: %.4f\n", float_rgb->g_Pb);                fprintf(stderr, "FLOAT VAL b: %.4f\n", float_rgb->b_Pr);
        // /fprintf(stderr, "FLOAT VAL: %.4f\n", float_rgb->r_Y)
*/

        int_pix->red = round(float_rgb->r_Y * maxval);
        int_pix->green = (round)(float_rgb->g_Pb * maxval);
        int_pix->blue = (round)(float_rgb->b_Pr *maxval);
/*
        fprintf(stderr, "INT VAL r: %u\n", int_pix->red);
        fprintf(stderr, "INT VAL g: %u\n", int_pix->green);
        fprintf(stderr, "INT VAL b: %u\n", int_pix->blue);
*/
}

bool is_even(int x)
{
        return (x % 2 == 0);
}

//initialize_ppm TODO (function contract)
Pnm_ppm initialize_ppm(FILE *input)
{
        
        assert(input != NULL);
        
        /* Default to UArray2 methods */
        A2Methods_T methods = uarray2_methods_plain;
        assert(methods != NULL);

        Pnm_ppm image = Pnm_ppmread(input, methods);
        //printf("trace d\n");

        return image;
}

// C6
// "apply function" as we iterate through cvs_arr in block major order
// expects counter to begin at 0
//function contract- get_packed_cvs_block
void get_packed_cvs_block(int col, int row, A2Methods_UArray2 cvs_arr2b, void *elem, void *block_info)
{
        struct block_info *info = block_info; //typecast cl struct
        struct pixel_float *cvs_curr = (struct pixel_float *)elem; // typecase curr cell block

        // 1) Get average chroma values for a block
        /* If in a block, update averages */        
        if (info->counter < (BLOCKSIZE * BLOCKSIZE)) {
                info->Pr_avg += cvs_curr->b_Pr;

                /* Increase numerator of Pb avg equat'n*/
                info->Pb_avg += cvs_curr->g_Pb;

                info->counter++;
                
        } 
        if (info->counter == (BLOCKSIZE * BLOCKSIZE)) {
                fprintf(stderr, "(Col, row) = %d, %d\n", col, row);
        
                //fprintf(stderr, "TRACE, counter: %d \n", info->counter);
                /* Get element at transformed index of cvs arr2p info array*/
                struct cvs_block *curr_iWord =  info->arr_methods->at(info->trans_cvs, col / BLOCKSIZE, row / BLOCKSIZE);
                
                /* Calculate avg chroma float vals */
                info->Pr_avg = info->Pr_avg / (BLOCKSIZE * BLOCKSIZE);
                //fprintf(stderr, "avg pr: %.4f\n", info->Pr_avg);                
                info->Pb_avg = info->Pb_avg / (BLOCKSIZE * BLOCKSIZE);
                fprintf(stderr, "avg pb: %.4f\n", info->Pb_avg);

                /* Quantize chroma to int and populate field at mapped index*/
                // todo undefined reference troubleshoot
                curr_iWord->Pb_index = Arith40_index_of_chroma(info->Pb_avg);
                curr_iWord->Pr_index = Arith40_index_of_chroma(info->Pr_avg);

                fprintf(stderr, "quantized chroma Pb: %u\n", curr_iWord->Pb_index);
                fprintf(stderr, "quantized chroma Pr: %u\n", curr_iWord->Pr_index);

                /* Transform luminance values to cosine coefficients */
                transform_luminance_to_coeffs(cvs_arr2b, uarray2_methods_blocked, col, row, curr_iWord);
                /* Reset vars restart for next block */
                info->counter = 0;
                info->Pb_avg = 0;
                info->Pr_avg = 0;
        }

        // fprintf(stderr, "end of function \n");

        (void)col;
        (void)row;
        (void)cvs_arr2b;
}


void transform_luminance_to_coeffs(A2Methods_UArray2 cvs_arr, A2Methods_T methods, int col, int row, struct cvs_block *curr_iWord)
{
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

        // since a is between 0 and 1 all will map to 0... what is this supposed to be?
        fprintf(stderr, "float a: %.4f\n", a); 
/*
        fprintf(stderr, "float b: %.4f\n", b);
        fprintf(stderr, "float c: %.4f\n", c);
        fprintf(stderr, "float d: %.4f\n", d);
*/

        curr_iWord->a = transform_a_to_unsigned(a);
        //curr_iWord->a = ~((int)round(a)) + 1;
//        curr_iWord->a = (unsigned)round(a); //todo: this doesn't work since bits are preserved (need 2s complement representation)

        curr_iWord->b = convert_coeff_to_signed(b);
        curr_iWord->c = convert_coeff_to_signed(c);
        curr_iWord->d = convert_coeff_to_signed(d);

        fprintf(stderr, "US a: %u\n", curr_iWord->a);
        /*
        fprintf(stderr, "int b: %d\n", curr_iWord->b);
        */
        fprintf(stderr, "int c: %d\n", curr_iWord->c);
        fprintf(stderr, "int d: %d\n", curr_iWord->d);
}

unsigned transform_a_to_unsigned(float a)
{
        unsigned ua = (int)round(a * 511); // todo put 512 in a constant

        if (a < 0) {
                /* Take 2s complement to get positive representation */
                ua = (~(ua) + 1);
        }
        /* Handle error cases outside luminance range (-1 to 1)*/
        if (ua > 512) {
                ua = 512;
        }
        return ua;
}


// returns unisgned in b/w -15/15
signed convert_coeff_to_signed(float coeff)
{
        //fprintf(stderr, "b input float: %.4f\n", coeff);
        signed quant_mapped = round(D_QUANT_RANGE * (coeff / FL_QUANT_RANGE));
        if (quant_mapped > D_QUANT_RANGE) {
                //fprintf(stderr, "EXCEEDS RANGE\n");
                quant_mapped = D_QUANT_RANGE;
        }
        //fprintf(stderr, "quantized: %d\n", quant_mapped);

        return quant_mapped;
}
//
void fill_cvs_float_from_block(int col, int row, A2Methods_UArray2 codeword_info_arr2p, void *elem, void *cvs_arr2b)
{
        /* Initialize codewords_info plain array methods */
        A2Methods_T meth_b = uarray2_methods_blocked;
        A2Methods_UArray2 cvs_arr = (A2Methods_UArray2 *)cvs_arr2b;

        /* Get word info for info corresponding to codeword at curr index */
        struct cvs_block *curr_iWord = (struct cvs_block *)elem;

        // Get indices of cells in block that maps to curr word index
        struct pixel_float *b1_cvs = meth_b->at(cvs_arr, (col * 2), (row * 2));
        struct pixel_float *b2_cvs = meth_b->at(cvs_arr, (col * 2) + 1, (row * 2));
        struct pixel_float *b3_cvs = meth_b->at(cvs_arr, (col * 2), ((row * 2) + 1));
        struct pixel_float *b4_cvs = meth_b->at(cvs_arr, (col * 2) + 1, ((row * 2) + 1));


// todo put in a helper func --> take in a pixel float position and set both
        /* Set float avg Pb and Pr values for each block from US indices */
        set_unquantized_chroma_for_block(curr_iWord, b1_cvs, b2_cvs, b3_cvs, b4_cvs);
       
        /* Transform cosine coefficients to luminance values */
        transform_coeffs_to_luminance(curr_iWord, b1_cvs, b2_cvs, b3_cvs, b4_cvs);

        (void)codeword_info_arr2p;
}

void set_unquantized_chroma_for_block(struct cvs_block *curr_iWord, struct pixel_float *b1_cvs, struct pixel_float *b2_cvs, struct pixel_float *b3_cvs, struct pixel_float *b4_cvs)
{
        b1_cvs->g_Pb = Arith40_chroma_of_index(curr_iWord->Pb_index);
        b1_cvs->b_Pr = Arith40_chroma_of_index(curr_iWord->Pr_index);

        b2_cvs->g_Pb = Arith40_chroma_of_index(curr_iWord->Pb_index);
        b2_cvs->b_Pr = Arith40_chroma_of_index(curr_iWord->Pr_index);

        b3_cvs->g_Pb = Arith40_chroma_of_index(curr_iWord->Pb_index);
        b3_cvs->b_Pr = Arith40_chroma_of_index(curr_iWord->Pr_index);

        b4_cvs->g_Pb = Arith40_chroma_of_index(curr_iWord->Pb_index);
        b4_cvs->b_Pr = Arith40_chroma_of_index(curr_iWord->Pr_index);
}

//if we take in positions instead, then we only need to calculate positions once
// don't need array anymore since we have a pointers to the cells we need
// dont need methods, don't need col or row 
void transform_coeffs_to_luminance(struct cvs_block *curr_iWord, struct pixel_float *b1_cvs, struct pixel_float *b2_cvs, struct pixel_float *b3_cvs, struct pixel_float *b4_cvs)
{
        /* Initialize luminance cells of a block in row major order */
        b1_cvs->r_Y = (float)(curr_iWord->a - curr_iWord->b - curr_iWord->c + curr_iWord->d);
        b2_cvs->r_Y = (float)(curr_iWord->a - curr_iWord->b + curr_iWord->c - curr_iWord->d);
        b3_cvs->r_Y = (float)(curr_iWord->a + curr_iWord->b - curr_iWord->c - curr_iWord->d);
        b4_cvs->r_Y = (float)(curr_iWord->a + curr_iWord->b + curr_iWord->c + curr_iWord->d);

        //fprintf(stdout, "Luminance: %.4f \n", b1_cvs->r_Y);        
}