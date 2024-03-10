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
const float BITS9_MAX = 511;      /* Max int within 9-bit rep */
const unsigned CHAR_BITS = 8;
const unsigned WORD_BITS = 32;

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
};

struct block_info
{
        A2Methods_UArray2 trans_cvs; // transformed CVS coords
        A2Methods_T arr_methods;
        int counter;
        float Pb_avg;
        float Pr_avg; 
};

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
A2Methods_UArray2 create_trimmed_float_arr(Pnm_ppm image, int width, int height);
bool is_even(int x);
void get_packed_cvs_block(int col, int row, A2Methods_UArray2 cvs_arr2b, void *elem, void *block_info);

/* Mapping one array to another (with transformation) */
void fill_transformed_from_rgb_flt(int col, int row, A2Methods_UArray2 curr_arr, void *elem, void *trans_info);
void fill_transformed_from_rgb_ints(int col, int row, A2Methods_UArray2 trim_arr, void *elem, void *trans_info); // todo maybe merge
void fill_rgb_ints_from_rgb_flt(int col, int row, A2Methods_UArray2 curr_arr, void *elem, void *trans_info);
void fill_cvs_float_from_block(int col, int row, A2Methods_UArray2 codeword_info_arr2p, void *elem, void *cvs_arr2b);
void pack_codewords(int col, int row, A2Methods_UArray2 word_info_arr, void *elem, void *codewords_arr);

/* Transformation (helpers) */
void transform_rgb_floats_to_cvs(void *elem, struct pixel_float *rgb_pix);
void init_floats_from_rgb_ints(void *elem, Pnm_rgb pix_val, unsigned maxval);
//void init_floats_from_rgb_ints(void *elem, Pnm_rgb pix_val, int pnm_maxval);
void init_ints_from_rgb_floats(void *elem, struct pixel_float *float_rgb, unsigned maxval);
void transform_cvs_to_rgb_float(void *elem, struct pixel_float *cvs_pix);
A2Methods_UArray2 create_blocked_arr(int width, int height, int size, int blocksize, A2Methods_T methods);
void transform_luminance_to_coeffs(A2Methods_UArray2 cvs_arr, A2Methods_T methods, int col, int row, struct cvs_block *curr_iWord);
unsigned transform_a_to_unsigned(float a);
void transform_coeffs_to_luminance(struct cvs_block *curr_iWord, struct pixel_float *b1_cvs, struct pixel_float *b2_cvs, struct pixel_float *b3_cvs, struct pixel_float *b4_cvs);
void unpack_codewords(int col, int row, A2Methods_UArray2 codewords, void *elem, void *codeword_info);

int make_even(int dim);
struct copy_info *initialize_copy_info(A2Methods_UArray2 prev_arr, A2Methods_T prev_methods, void (*transFun)());
signed convert_coeff_to_signed(float coeff);
float convert_signed_to_coeff(signed coeff);
void set_unquantized_chroma_for_block(struct cvs_block *curr_iWord, struct pixel_float *b1_cvs, struct pixel_float *b2_cvs, struct pixel_float *b3_cvs, struct pixel_float *b4_cvs);


/* Decompression functions */
void decompress40 (FILE *input);
//void pop_with_rgb_ints(int col, int row, A2Methods_UArray2 rgb_int_arr, void *elem, void *rgb_float_arr); // deleted
A2Methods_UArray2 create_pixels_from_header(FILE *in, unsigned *width, unsigned *height);

/* Test functions */
void decompressTest(FILE *uncompressed);  // todo delete testing function
void test_print_word_info(int col, int row, A2Methods_UArray2 word_info, void *elem, void *cl);

/* Print Functions */
void print_codewords(A2Methods_UArray2 codewords, int width, int height);
void print_word(int col, int row, A2Methods_UArray2 codewords, void *elem, void* cl);
void populate_codewords(int col, int row, A2Methods_UArray2 codewords, void *elem, void *input_file);

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
void compress40 (FILE *input)
{
        /* C1 Initialize and populate pnm for uncompressed file */ 
        Pnm_ppm uncompressed = initialize_ppm(input); /* rgb ints */
        // decompressTest(uncompressed); // passed C1 todo del E = 0.0000

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

        int t_width = methods_p->width(trimmed_rgb_flts);
        int t_height = methods_p->height(trimmed_rgb_flts);

////////////////////////////////////////////////////////////////////////////
        /* Initialize blocked array with component video pixels */ 
         A2Methods_UArray2 comp_vid_arr = 
         create_blocked_arr(t_width,
         t_height,
         sizeof(struct pixel_float), BLOCKSIZE, methods_b);
         //decompressTest(comp_vid_arr, uncompressed); // E = .0023

         /* C4 Updates CVS arr to hold values transformed from RGB floats */
         /* Initialize rgb to cvs transfromation info */
         struct copy_info *info = initialize_copy_info(trimmed_rgb_flts, methods_p, transform_rgb_floats_to_cvs); // todo free
         /* Map rgb floats to cvs*/
         methods_b->map_default(comp_vid_arr, fill_transformed_from_rgb_flt, info);
        //decompressTest(comp_vid_arr, uncompressed); // E = .0023
////////////////////////////////////////////////////////////////////////////

        methods_p->free(&trimmed_rgb_flts);

        /* C6 a Convert chroma averages to quantized*/
        /* Initialize array of cvs block coordinates with 1/4 dims */
        A2Methods_UArray2 word_info = methods_p->new( (t_width / BLOCKSIZE), (t_height / BLOCKSIZE), sizeof(struct cvs_block));
        
        /* Initialize cl info for cvs to word_info transformation */
        struct block_info *block_info = malloc(sizeof(struct block_info)); 
        assert(block_info != NULL);
        block_info->trans_cvs = word_info;
        block_info->arr_methods = methods_p;
        block_info->counter = 0;
        block_info->Pb_avg = 0;
        block_info->Pr_avg = 0;
        
        methods_b->map_default(comp_vid_arr, get_packed_cvs_block, block_info);
       //decompressTest(word_info, uncompressed); // E = .0754

        /* Initalize array of packed codewords */
        A2Methods_UArray2 codewords = methods_p->new(t_width / BLOCKSIZE, t_height / BLOCKSIZE, sizeof(int64_t));

        /* Compress codeword info into 32 bit codewords */
        methods_p->map_row_major(word_info, pack_codewords, codewords);
        // decompressTest(codewords, uncompressed); // E = .0754

        /* Print codewords to output */
        print_codewords(codewords, t_width, t_height);

        (void) word_info;
        (void) trimmed_rgb_flts; // array of rgb floats

//todo comment back in after tests 
        Pnm_ppmfree(&uncompressed);
        uncompressed = NULL; //todo ptr make null after 
}
// todo move this

// todo remove this function, just here for testing
// todo change input to correct input after compression has been fully implemented --> FILE *input
void decompressTest(FILE *uncompressed)
{
        /* Initialize plain and blocked methods */
        A2Methods_T methods_p = uarray2_methods_plain;        
        A2Methods_T methods_b = uarray2_methods_blocked;
        assert(methods_b != NULL && methods_p != NULL);

        unsigned width, height;

        /* Read header of uncompressed file and allocate array */
        A2Methods_UArray2 pixels_arr = create_pixels_from_header(uncompressed, &width, &height);
        // fprintf(stdout, "width, height: %u, %u\n", width, height);

        /* Initialize local pixmap struct */
        struct Pnm_ppm pixmap = {
                .width = width,
                .height = height,
                .denominator = 255,
                .pixels = pixels_arr, 
                .methods = methods_p
        };

        /* Initalize array of packed codewords */
        A2Methods_UArray2 codewords = methods_p->new(pixmap.width / BLOCKSIZE, pixmap.height / BLOCKSIZE, sizeof(int64_t));

        // iterate through, filling with file data
        methods_p->map_row_major(codewords, populate_codewords, uncompressed);

        // methods_p->map_default(word_info_arr, test_print_word_info, NULL); 

        // for (int i = 0; i < 50; i++) {
        //         struct cvs_block *word = methods_p->at(word_info_arr, i, 0);
        //         fprintf(stdout, "word info from arr (D_input): \n");
        //         fprintf(stdout, "Pr: %u \n", word->Pr_index);
        //         fprintf(stdout, "Pb %u \n", word->Pb_index);
        //         fprintf(stdout, "d %d \n", word->d);
        //         fprintf(stdout, "c %d \n", word->c);
        //         fprintf(stdout, "b %d \n", word->b);
        //         fprintf(stdout, "a %u \n", word->a);

        // }


        /* Initialize codeword info array */
        A2Methods_UArray2 word_info_arr = methods_p->new(methods_p->width(codewords), methods_p->height(codewords), sizeof(struct cvs_block));

        /* Unpack codewords to codeword info array */
        methods_p->map_default(codewords, unpack_codewords, word_info_arr);

        // methods_p->map_default(word_info_arr, test_print_word_info, NULL); 

        /* Initialize blocked array with component video pixels */ 
        A2Methods_UArray2 comp_vid_arr = 
        create_blocked_arr((BLOCKSIZE * methods_p->width(word_info_arr)),
        (BLOCKSIZE * methods_p->height(word_info_arr)),
        sizeof(struct pixel_float), BLOCKSIZE, methods_b);

        /* D8 Convert cvs block ints to cvs floats */
        // iterate through comp_vid arr in block major order, and iterate 4 times more slowly through codeword_info_arr 
        methods_p->map_row_major(word_info_arr, fill_cvs_float_from_block, comp_vid_arr);


///////////////////////////////////////////////////////////////////
        /* D9 Convert CVS float to RGB floats */
        // initialize comp_vid_array info
        //struct copy_info *cvs_info = initialize_copy_info(comp_vid_arr, methods_b, transform_cvs_to_rgb_float); // todo free

        struct copy_info *cvs_info = malloc(sizeof(struct copy_info));
        assert(cvs_info != NULL);
        cvs_info->prev_arr = comp_vid_arr;
        cvs_info->prev_methods = methods_b;
        cvs_info->transFun = transform_cvs_to_rgb_float;

        // create rgb float array
        A2Methods_UArray2 rgb_float_arr = methods_p->new(methods_b->width(comp_vid_arr), methods_b->height(comp_vid_arr), sizeof(struct pixel_float)); 

        methods_p->map_default(rgb_float_arr, fill_transformed_from_rgb_flt, cvs_info);
//////////////////////////////////////////////////////////////////////////

        /* D11 Populate 2D array of RGB int vals */
        /* Initialize a new 2D array with new dims */
        ////////////////////////////////////////////////////////////////////////

        // creates pixels arr of pnm rgb ints
        A2Methods_UArray2 pixel = methods_p->new(methods_p->width(rgb_float_arr), methods_p->height(rgb_float_arr), sizeof(struct Pnm_rgb));

        /* Initialize rgb float to rgb int transfromation info */
//        struct copy_info *rgb_fl_info = initialize_copy_info(rgb_float_arr, methods_p, init_ints_from_rgb_floats); // todo free


        struct rgb_int_info *int_info = malloc(sizeof(struct rgb_int_info));
        int_info->maxval = pixmap.denominator;
        int_info->rgb_floats = rgb_float_arr;
        int_info->rgb_flt_meths = methods_p;


        // A2Methods_mapfun *map = methods_p->map_default; // row maj
        // methods_p->map_default(pixel, pop_with_rgb_ints, rgb_float_arr);

        /* D10 Populate RGB ints pixel array with RGB floats */
        methods_p->map_default(pixel, fill_rgb_ints_from_rgb_flt, int_info); // this is in C3 E test, which ppmdiffs to 0.0000

        ///////////////////////////////////////////////////////////////////////

        pixmap.width = methods_p->width(pixel);
        pixmap.height = methods_p->height(pixel);
        pixmap.pixels = pixel;
        
        /* D12 Print uncompressed PPM and free PPM */
        Pnm_ppmwrite(stdout, &pixmap);
//        Pnm_ppmfree(&pixmap);// todo review: no freeing bc local var


        // fclose(inputfile);  TODO put these in main after decompress
        // exit(EXIT_SUCCESS); TODO 
}

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
// apply TODO (function contract)
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

void fill_rgb_ints_from_rgb_flt(int col, int row, A2Methods_UArray2 curr_arr, void *elem, void *trans_info)
{
        //fprintf(stderr, "(col, row) = %d, %d\n", col, row);
        //struct copy_info *info = trans_info;

        struct rgb_int_info *info = trans_info; 

        /* Get element from orig img arr analagous position */
        struct pixel_float *pix_val = info->rgb_flt_meths->at(info->rgb_floats, col, row); // casting a cvs float as an rgb float todo del

        /* Populate new arr with transformed float vals*/
        init_ints_from_rgb_floats(elem, pix_val, info->maxval); // 1st param is (current rgb float pixel), 2nd param holds rgb_float holding (cvs float) values todo del
/*
*/
        // fprintf(stderr, "r_val old = %.4f\n", pix_val->r_Y);
//        fprintf(stderr, "R new = %u\n", ((struct Pnm_rgb *)elem)->red);
        // fprintf(stderr, "G new = %u\n", ((struct Pnm_rgb *)elem)->green);
        // fprintf(stderr, "B new = %u\n", ((struct Pnm_rgb *)elem)->blue);

        (void) curr_arr;        
}

// todo contract
void transform_rgb_floats_to_cvs(void *elem, struct pixel_float *rgb_pix)
{
        struct pixel_float *cvs_pix = (struct pixel_float *)elem;
        
        cvs_pix->r_Y = .299 * rgb_pix->r_Y + 0.587 * rgb_pix->g_Pb + 0.114 * rgb_pix->b_Pr;
        cvs_pix->g_Pb = -0.168736 * rgb_pix->r_Y - 0.331264 *  rgb_pix->g_Pb + 0.5 * rgb_pix->b_Pr;
        cvs_pix->b_Pr = 0.5 * rgb_pix->r_Y - 0.418688 * rgb_pix->g_Pb - 0.081312 * rgb_pix->b_Pr;
}

// todo contract 
//(inverse of prev)
// CVS_FLOAT -> RGB FLOAT
void transform_cvs_to_rgb_float(void *elem, struct pixel_float *cvs_pix)
{
        struct pixel_float *rgb_pix = (struct pixel_float *)elem;
        
        rgb_pix->r_Y = 1.0 * cvs_pix->r_Y + 0.0 * cvs_pix->g_Pb + 1.402 * cvs_pix->b_Pr;
        rgb_pix->g_Pb = 1.0 * cvs_pix->r_Y - 0.344136 * cvs_pix->g_Pb - 0.714136 * cvs_pix->b_Pr;
        rgb_pix->b_Pr = 1.0 * cvs_pix->r_Y + 1.772 * cvs_pix->g_Pb + 0.0 * cvs_pix->b_Pr;

        // fprintf(stderr, "CVS Y: %.4f \n", cvs_pix->r_Y);
        // fprintf(stderr, "RGB float r: %.4f \n", rgb_pix->r_Y);
}


// Decompress40 TODO (function contract)
// todo change input to correct input after compression has been fully implemented --> FILE *input
void decompress40 (FILE *uncompressed)
{
        /* Initialize plain and blocked methods */
        A2Methods_T methods_p = uarray2_methods_plain;        
        A2Methods_T methods_b = uarray2_methods_blocked;
        assert(methods_b != NULL && methods_p != NULL);

        unsigned width, height;

        /* Read header of uncompressed file and allocate array */
        A2Methods_UArray2 pixels_arr = create_pixels_from_header(uncompressed, &width, &height);
        // fprintf(stdout, "width, height: %u, %u\n", width, height);

        /* Initialize local pixmap struct */
        struct Pnm_ppm pixmap = {
                .width = width,
                .height = height,
                .denominator = 255,
                .pixels = pixels_arr, 
                .methods = methods_p
        };

        /* Initalize array of packed codewords */
        A2Methods_UArray2 codewords = methods_p->new(pixmap.width / BLOCKSIZE, pixmap.height / BLOCKSIZE, sizeof(int64_t));

        // iterate through, filling with file data
        methods_p->map_row_major(codewords, populate_codewords, uncompressed);

        // methods_p->map_default(word_info_arr, test_print_word_info, NULL); 

        // for (int i = 0; i < 50; i++) {
        //         struct cvs_block *word = methods_p->at(word_info_arr, i, 0);
        //         fprintf(stdout, "word info from arr (D_input): \n");
        //         fprintf(stdout, "Pr: %u \n", word->Pr_index);
        //         fprintf(stdout, "Pb %u \n", word->Pb_index);
        //         fprintf(stdout, "d %d \n", word->d);
        //         fprintf(stdout, "c %d \n", word->c);
        //         fprintf(stdout, "b %d \n", word->b);
        //         fprintf(stdout, "a %u \n", word->a);

        // }


        /* Initialize codeword info array */
        A2Methods_UArray2 word_info_arr = methods_p->new(methods_p->width(codewords), methods_p->height(codewords), sizeof(struct cvs_block));

        /* Unpack codewords to codeword info array */
        methods_p->map_default(codewords, unpack_codewords, word_info_arr);

        // methods_p->map_default(word_info_arr, test_print_word_info, NULL); 

        /* Initialize blocked array with component video pixels */ 
        A2Methods_UArray2 comp_vid_arr = 
        create_blocked_arr((BLOCKSIZE * methods_p->width(word_info_arr)),
        (BLOCKSIZE * methods_p->height(word_info_arr)),
        sizeof(struct pixel_float), BLOCKSIZE, methods_b);

        /* D8 Convert cvs block ints to cvs floats */
        // iterate through comp_vid arr in block major order, and iterate 4 times more slowly through codeword_info_arr 
        methods_p->map_row_major(word_info_arr, fill_cvs_float_from_block, comp_vid_arr);

///////////////////////////////////////////////////////////////////
        /* D9 Convert CVS float to RGB floats */
        // initialize comp_vid_array info
        //struct copy_info *cvs_info = initialize_copy_info(comp_vid_arr, methods_b, transform_cvs_to_rgb_float); // todo free

        struct copy_info *cvs_info = malloc(sizeof(struct copy_info));
        assert(cvs_info != NULL);
        cvs_info->prev_arr = comp_vid_arr;
        cvs_info->prev_methods = methods_b;
        cvs_info->transFun = transform_cvs_to_rgb_float;

        // create rgb float array
        A2Methods_UArray2 rgb_float_arr = methods_p->new(methods_b->width(comp_vid_arr), methods_b->height(comp_vid_arr), sizeof(struct pixel_float)); 

        methods_p->map_default(rgb_float_arr, fill_transformed_from_rgb_flt, cvs_info);
//////////////////////////////////////////////////////////////////////////

        /* D11 Populate 2D array of RGB int vals */
        /* Initialize a new 2D array with new dims */
        ////////////////////////////////////////////////////////////////////////

        // creates pixels arr of pnm rgb ints
        A2Methods_UArray2 pixel = methods_p->new(methods_p->width(rgb_float_arr), methods_p->height(rgb_float_arr), sizeof(struct Pnm_rgb));

        /* Initialize rgb float to rgb int transfromation info */
//        struct copy_info *rgb_fl_info = initialize_copy_info(rgb_float_arr, methods_p, init_ints_from_rgb_floats); // todo free


        struct rgb_int_info *int_info = malloc(sizeof(struct rgb_int_info));
        int_info->maxval = pixmap.denominator;
        int_info->rgb_floats = rgb_float_arr;
        int_info->rgb_flt_meths = methods_p;


        // A2Methods_mapfun *map = methods_p->map_default; // row maj
        // methods_p->map_default(pixel, pop_with_rgb_ints, rgb_float_arr);

        /* D10 Populate RGB ints pixel array with RGB floats */
        methods_p->map_default(pixel, fill_rgb_ints_from_rgb_flt, int_info); // this is in C3 E test, which ppmdiffs to 0.0000

        ///////////////////////////////////////////////////////////////////////

        pixmap.width = methods_p->width(pixel);
        pixmap.height = methods_p->height(pixel);
        pixmap.pixels = pixel;
        
        /* D12 Print uncompressed PPM and free PPM */
        Pnm_ppmwrite(stdout, &pixmap);
//        Pnm_ppmfree(&pixmap);// todo review: no freeing bc local var


        // fclose(inputfile);  TODO put these in main after decompress
        // exit(EXIT_SUCCESS); TODO}
}

//fucntion contract
A2Methods_UArray2 create_blocked_arr(int width, int height, int size, int blocksize, A2Methods_T methods)
{
        assert(methods != NULL);

        A2Methods_UArray2 blocked_arr = methods->new_with_blocksize(width, height, size, blocksize);
        assert(blocked_arr != NULL);
         
        return blocked_arr;
}

//todo contract
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
        //struct copy_info *pixels_info = initialize_copy_info(image->pixels, uarray2_methods_plain, init_floats_from_rgb_ints);
        /*
        struct copy_info *pixels_info = malloc(sizeof(struct copy_info));
        assert(pixels_info != NULL);
        pixels_info->prev_arr = image->pixels;
        pixels_info->prev_methods = uarray2_methods_plain;
        pixels_info->transFun = init_floats_from_rgb_ints;
        */


        /* Populate with data from old array */
        A2Methods_mapfun *map = image->methods->map_default;
        map(trim_arr, fill_transformed_from_rgb_ints, image);

        //fprintf(stderr, "POST WIDTH AND HEIGHT: %d, %d \n", width, height);

        return trim_arr;
}

// Copy RGB values to pix in trimmed array
void init_floats_from_rgb_ints(void *elem, Pnm_rgb pix_val, unsigned maxval)
{
        assert(maxval == 255); // todo delete
        struct pixel_float *curr_pix = (struct pixel_float *)elem;

        curr_pix->r_Y = (float)pix_val->red / maxval;
        curr_pix->g_Pb = (float)pix_val->green / maxval;
        curr_pix->b_Pr = (float)pix_val->blue / maxval;        
}

// apply TODO (function contract)
// rgb ints -> floats
void fill_transformed_from_rgb_ints(int col, int row, A2Methods_UArray2 trim_arr, void *elem, void *og_img)
{
        Pnm_ppm orig = (Pnm_ppm)og_img;

        /* Get element from orig img arr analagous position */
        Pnm_rgb pix_val = orig->methods->at(orig->pixels, col, row);

        init_floats_from_rgb_ints(elem, pix_val, orig->denominator);

        (void) trim_arr;        
}

// todo contract
// RGB float to RGB int
void init_ints_from_rgb_floats(void *elem, struct pixel_float *float_rgb, unsigned maxval)
{
        Pnm_rgb int_pix = (struct Pnm_rgb *)elem;

        int_pix->red = (int)(float_rgb->r_Y * maxval);
        int_pix->green = (int)(float_rgb->g_Pb * maxval);
        int_pix->blue = (int)(float_rgb->b_Pr * maxval);
}

bool is_even(int x)
{
        return (x % 2 == 0);
}

//Initialize_ppm TODO (function contract)
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

// todo broken
void get_packed_cvs_block(int col, int row, A2Methods_UArray2 cvs_arr2b, void *elem, void *block_info)
{
        // fprintf(stderr, "1(col, row): (%d, %d)\n", col, row);

        struct block_info *info = block_info; //typecast cl struct
        struct pixel_float *cvs_curr = (struct pixel_float *)elem;

        // 1) Get average chroma values for a block
        /* If in a block, update averages */        
        if (info->counter < (BLOCKSIZE * BLOCKSIZE))
        {
                /* Increase numerators of Pb and Pr avgs*/
                info->Pr_avg += cvs_curr->b_Pr;
                
                //fprintf(stdout, "Pr %d: %.4f\n", info->counter, cvs_curr->b_Pr);

                info->Pb_avg += cvs_curr->g_Pb;
                info->counter++;
        }
        /* At the last cell of the block, calculate averages and cos coeffs */
        if (info->counter == (BLOCKSIZE * BLOCKSIZE)) {

                /* Get element at transformed index of cvs arr2p info array*/
                struct cvs_block *curr_iWord =  info->arr_methods->at(info->trans_cvs, col / BLOCKSIZE, row / BLOCKSIZE);

                //fprintf(stderr, "2actual (col, row): (%d, %d)\n", col, row);
                // fprintf(stderr, "(Col, Row): (%d, %d)\n", col / BLOCKSIZE, row / BLOCKSIZE);

                //fprintf(stdout, "Pr_numer: %.4f\n", info->Pr_avg);

                /* Calculate avg chroma float vals */
                info->Pr_avg = info->Pr_avg / (BLOCKSIZE * BLOCKSIZE);
                info->Pb_avg = info->Pb_avg / (BLOCKSIZE * BLOCKSIZE);

                // fprintf(stderr, "Pb_avg: %.4f\n", info->Pb_avg);

                /* Quantize chroma to int and populate field at mapped index*/
                // todo comment back in
                // curr_iWord->Pb_index = Arith40_index_of_chroma(info->Pb_avg);
                // curr_iWord->Pr_index = Arith40_index_of_chroma(info->Pr_avg);
                
                /* Populate Pb and Pr indices for codeword packing */
                curr_iWord->Pb_index = Arith40_index_of_chroma(info->Pb_avg);
                curr_iWord->Pr_index = Arith40_index_of_chroma(info->Pr_avg);

                /* Transform luminance values to cosine coefficients */
                transform_luminance_to_coeffs(cvs_arr2b, uarray2_methods_blocked, col, row, curr_iWord);
        /*
        fprintf(stdout, "Block coeff floats, signed a:\n");
        fprintf(stdout, "Sa: %d\n", curr_iWord->a);
        fprintf(stdout, "fb: %.4f\n", curr_iWord->b);
        fprintf(stdout, "fc: %.4f\n", curr_iWord->c);
        fprintf(stdout, "fd: %.4f\n", curr_iWord->d);
        */

                /* Reset vars restart for next block */
                info->counter = 0;
                info->Pb_avg = 0;
                info->Pr_avg = 0;
        }
        (void)col;
        (void)row;
        (void)cvs_arr2b;
}

void test_print_word_info(int col, int row, A2Methods_UArray2 word_info, void *elem, void *cl)
{
        struct cvs_block *curr = elem;
        (void) curr;
 
        fprintf(stderr, "w info (COL, ROW): (%d, %d)\n", col, row);
        fprintf(stderr, "block Pb: %u\n", curr->Pb_index);
        fprintf(stderr, "block Pr: %u\n", curr->Pr_index);
        fprintf(stderr, "block d: %d\n", curr->d);
        fprintf(stderr, "block c: %d\n", curr->c);
        fprintf(stderr, "block b: %d\n", curr->b);
        fprintf(stderr, "block a: %u\n", curr->a);

        (void)col;
        (void)row;
        (void)cl;
        (void)word_info;
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

/* todo come back to this at some point
*/
        curr_iWord->a = transform_a_to_unsigned(a);
        curr_iWord->b = convert_coeff_to_signed(b);
        curr_iWord->c = convert_coeff_to_signed(c);
        curr_iWord->d = convert_coeff_to_signed(d);

        /*
        fprintf(stderr, "US a: %u\n", curr_iWord->a);
        fprintf(stderr, "int b: %d\n", curr_iWord->b);
        fprintf(stderr, "int c: %d\n", curr_iWord->c);
        fprintf(stderr, "int d: %d\n", curr_iWord->d);
        */
}

unsigned transform_a_to_unsigned(float a)
{
        unsigned ua = (int)round(a * BITS9_MAX);

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

// returns float between -.3 and .3
// D_QUANT_RANGE is absolute value of max/min int for signed b,c,d
// FL_QUANT_RANGE is absolute value of max/min float for float b,c,d

float convert_signed_to_coeff(signed coeff)
{
        //fprintf(stdout, "b input signed: %d\n", coeff);
        
        float dequant_mapped = (FL_QUANT_RANGE * (coeff / D_QUANT_RANGE));
        
        if (dequant_mapped > FL_QUANT_RANGE) {
                dequant_mapped = FL_QUANT_RANGE;
        }
        //fprintf(stdout, "dequantized: %.4f\n", quant_mapped);

        return dequant_mapped;
}

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

        /* Set float avg Pb and Pr values for each block from US indices */
        set_unquantized_chroma_for_block(curr_iWord, b1_cvs, b2_cvs, b3_cvs, b4_cvs);

        /* Transform cosine coefficients to luminance values */
        transform_coeffs_to_luminance(curr_iWord, b1_cvs, b2_cvs, b3_cvs, b4_cvs);

        (void)codeword_info_arr2p;
}

// fills each one with the avg
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

void transform_coeffs_to_luminance(struct cvs_block *curr_iWord, struct pixel_float *b1_cvs, struct pixel_float *b2_cvs, struct pixel_float *b3_cvs, struct pixel_float *b4_cvs)
{
        /* Need to modify a, b, c, d values first */
        float a = ((float)curr_iWord->a) / BITS9_MAX; /* a in (-1,1) range */
        float b = convert_signed_to_coeff(curr_iWord->b);
        float c = convert_signed_to_coeff(curr_iWord->c);
        float d = convert_signed_to_coeff(curr_iWord->d);

        /* Initialize luminance cells of a block in row major order */
        b1_cvs->r_Y = (float)(a - b - c + d);
        b2_cvs->r_Y = (float)(a - b + c - d);
        b3_cvs->r_Y = (float)(a + b - c - d);
        b4_cvs->r_Y = (float)(a + b + c + d);        
}

void pack_codewords(int col, int row, A2Methods_UArray2 word_info_arr, void *elem, void *codewords_arr)
{
        A2Methods_T methods = uarray2_methods_plain;

        A2Methods_UArray2 codewords = (A2Methods_UArray2 *)codewords_arr;
        struct cvs_block *word_info = (struct cvs_block *)elem;

        // fprintf(stdout, "Pre-PACK (C) word info\n");
        // fprintf(stdout, "Pr: %u \n", word_info->Pr_index);
        // fprintf(stdout, "Pb %u \n", word_info->Pb_index);
        // fprintf(stdout, "d %d \n", word_info->d);
        // fprintf(stdout, "c %d \n", word_info->c);
        // fprintf(stdout, "b %d \n", word_info->b);
        // fprintf(stdout, "a %u \n", word_info->a);

        int64_t *curr_word = methods->at(codewords, col, row);

        /* Pack bits in Big-Endian Order (MSB first) */
        *curr_word = Bitpack_newu(*curr_word, Pr_WIDTH, Pr_LSB, word_info->Pr_index);
        *curr_word = Bitpack_newu(*curr_word, Pb_WIDTH, Pb_LSB, word_info->Pb_index);
        *curr_word = Bitpack_news(*curr_word, d_WIDTH, d_LSB, word_info->d);
        *curr_word = Bitpack_news(*curr_word, c_WIDTH, c_LSB, word_info->c);
        *curr_word = Bitpack_news(*curr_word, b_WIDTH, b_LSB, word_info->b);
        *curr_word = Bitpack_newu(*curr_word, a_WIDTH, a_LSB, word_info->a);

        //fprintf(stderr, "final codeword: %lu \n", *curr_word);

        (void)word_info_arr;
}

// codewords to codeword info array
void unpack_codewords(int col, int row, A2Methods_UArray2 codewords, void *elem, void *codeword_info)
{

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

        // fprintf(stdout, "unpack (d) word_info: \n");
        // fprintf(stdout, "Pr: %u \n", word_info->Pr_index);
        // fprintf(stdout, "Pb: %u \n", word_info->Pb_index);
        // fprintf(stdout, "d: %d \n", word_info->d);
        // fprintf(stdout, "c: %d \n", word_info->c);
        // fprintf(stdout, "b: %d \n", word_info->b);
        // fprintf(stdout, "a: %u \n", word_info->a);
        


       (void)codewords;
}

void print_codewords(A2Methods_UArray2 codewords, int width, int height)
{
        A2Methods_T methods = uarray2_methods_plain;

        printf("COMP40 Compressed image format 2\n%u %u\n", width, height);

        /* Iterate through the codewords and print in Big-Endian */
        methods->map_row_major(codewords, print_word, NULL);

        (void) codewords;
}

void print_word(int col, int row, A2Methods_UArray2 codewords, void *elem, void* cl)
{
        int64_t *curr_word = (int64_t *)elem;

        /* Fill chars by 8 bits starting at codeword MSB*/
        char one = Bitpack_getu(*curr_word, CHAR_BITS, WORD_BITS - (CHAR_BITS * 1));
        char two = Bitpack_getu(*curr_word, CHAR_BITS, WORD_BITS - (CHAR_BITS * 2));
        char three = Bitpack_getu(*curr_word, CHAR_BITS, WORD_BITS - (CHAR_BITS * 3));
        char four = Bitpack_getu(*curr_word, CHAR_BITS, WORD_BITS - (CHAR_BITS * 4));

        putchar(one);
        putchar(two);
        putchar(three);
        putchar(four);

        (void)col;
        (void)row;
        (void)cl;
        (void)codewords;
}

A2Methods_UArray2 create_pixels_from_header(FILE *in, unsigned *width, unsigned *height) 
{
        /* Set the width and height of pixels array and read up to newline */
        int read = fscanf(in, "COMP40 Compressed image format 2\n%u %u", width, height);
        assert(read == 2);
        int c = getc(in);
        assert(c == '\n');

        /* Initialize pixels array */
        A2Methods_T meths = uarray2_methods_plain;
        A2Methods_UArray2 pixels_arr = meths->new(*width, *height, sizeof(struct Pnm_rgb));

        return pixels_arr;
}

void populate_codewords(int col, int row, A2Methods_UArray2 codewords, void *elem, void *input_file)
{
        FILE *input = (FILE *)input_file;
        int64_t *curr_word = (int64_t *)elem;

        /* Read in character by character and fill codeword*/
        char c = getc(input);
        /* Fill chars by 8 bits starting at codeword MSB*/
        int64_t mod_one = Bitpack_news(*curr_word, CHAR_BITS, WORD_BITS - (CHAR_BITS * 1), c);
        fprintf(stderr, "TRACEE \n");
        c = getc(input);
        int64_t mod_two = Bitpack_news(mod_one, CHAR_BITS, WORD_BITS - (CHAR_BITS * 2), c);
        c = getc(input);
        int64_t mod_three = Bitpack_news(mod_two, CHAR_BITS, WORD_BITS - (CHAR_BITS * 3), c);
        c = getc(input);
        int64_t encoded_word = Bitpack_news(mod_three, CHAR_BITS, WORD_BITS - (CHAR_BITS * 4), c);

        *curr_word = encoded_word;

        fprintf(stderr, "word: %ld \n", *curr_word);

        (void)elem;
        (void)codewords;
        (void)col;
        (void)row;
}