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

const int BLOCKSIZE = 2;

//todo contract
struct rgb_float
{
        float red;
        float green;
        float blue;
};


// todo contract
// todo consider: at an intermed step, this could hold converted float rep of rgb ints, is the name still okay?
struct component_vid
{
        float Y;
        float Pb;
        float Pr;
};

struct copy_info
{
        A2Methods_UArray2 prev_arr;
        A2Methods_T prev_methods;
        void (*transFun)();
};

static void (*compress_or_decompress)(FILE *input) = compress40;

/* Compression functions */
Pnm_ppm initialize_ppm(FILE *input);
void compress40 (FILE *input);
A2Methods_UArray2 create_trimmed_float_arr(Pnm_ppm image, int width, int height);
bool is_even(int x);
A2Methods_UArray2 create_blocked_arr(int width, int height, int size, int blocksize, A2Methods_T methods);
void fill_transformed_from_rgb_flt(int col, int row, A2Methods_UArray2 curr_arr, void *elem, void *trans_info);
void init_cvs_from_rgb_floats(void *elem, struct rgb_float *rgb_pix);

void fill_transformed_from_rgb_ints(int col, int row, A2Methods_UArray2 trim_arr, void *elem, void *trans_info); // todo maybe merge
void init_floats_from_rgb_ints(void *elem, Pnm_rgb pix_val);
int make_even(int dim);

/* Decompression functions */
void decompress40 (FILE *input);
//void pop_with_rgb_ints(int col, int row, A2Methods_UArray2 rgb_int_arr, void *elem, void *rgb_float_arr); // deleted
void init_ints_from_rgb_floats(void *elem, struct rgb_float *float_rgb);

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
        // decompressTest(uncompressed); // passed C1 todo del
     
        /* Initialize blocked methods */
        // plain methods: uncompressed->methods // todo del this
        A2Methods_T methods_b = uarray2_methods_blocked;
        //A2Methods_mapfun *map_b = methods_b->map_default; // block maj
        A2Methods_T methods_p = uarray2_methods_plain;
        //A2Methods_mapfun *map_p = methods_p->map_default; // row maj
        assert(methods_b != NULL && methods_p != NULL);

        /* C2/C3 Create trimmed arr of rgb floats */
        A2Methods_UArray2 trimmed_rgb_flts = 
        create_trimmed_float_arr(uncompressed,
        make_even(uncompressed->width), 
        make_even(uncompressed->height)); // C3 passed, trimmed is p
        decompressTest(trimmed_rgb_flts, uncompressed); 

        /* Initialize blocked array with component video pixels */ 
//         A2Methods_UArray2 comp_vid_arr = 
//         create_blocked_arr(methods_p->width(trimmed_rgb_flts),
//         methods_p->height(trimmed_rgb_flts),
//         sizeof(struct component_vid), BLOCKSIZE, methods_b);

//         fprintf(stderr, "WIDTH: %d \n", methods_b->width(comp_vid_arr));
//         fprintf(stderr, "HEIGHT: %d \n", methods_b->height(comp_vid_arr));

//         /* Initialize rgb to cvs transfromation info */
//         // todo could put this init'n in helper func
//         struct copy_info *info = malloc(sizeof(info));
//         info->prev_arr = trimmed_rgb_flts;
//         info->prev_methods = methods_p;
//         info->transFun = init_cvs_from_rgb_floats;

//         /* C4 Transform RGB floats to component video space */
//         methods_b->map_default(comp_vid_arr, fill_transformed_from_rgb_flt, info);

// /*  
//         printf("W: %d \n ", uncompressed->width);
//         printf("H: %d \n ", uncompressed->height);
// */
//         (void) comp_vid_arr;
        (void) trimmed_rgb_flts;

//todo comment back in after tests 
        //Pnm_ppmfree(&uncompressed);
        //uncompressed = NULL; //todo ptr make null after 
}

// todo remove this function, just here for testing
// todo change input to correct input after compression has been fully implemented --> FILE *input
void decompressTest(A2Methods_UArray2 rgb_float_arr, Pnm_ppm uncompressed)
{

        /* D10 Convert RGB floats to RGB ints*/

        /* D11 Populate 2D array of RGB int vals */
        /* Initialize a new 2D array with new dims */
        ////////////////////////////////////////////////////////////////////////
        A2Methods_T methods_p = uarray2_methods_plain; // todo review this
        assert(methods_p != NULL);

        // creates pixels arr of pnm rgb ints
        A2Methods_UArray2 pixel = methods_p->new(methods_p->width(rgb_float_arr), methods_p->height(rgb_float_arr), sizeof(struct Pnm_rgb));

        /* Initialize rgb to cvs transfromation info */
        // todo helper (2nd time)
        struct copy_info *info = malloc(sizeof(info));
        info->prev_arr = rgb_float_arr;
        info->prev_methods = methods_p;
        info->transFun = init_ints_from_rgb_floats;

        //A2Methods_mapfun *map = methods_p->map_default; // row maj
        // methods_p->map_default(pixel, pop_with_rgb_ints, rgb_float_arr);
        methods_p->map_default(pixel, fill_transformed_from_rgb_flt, info);

        ///////////////////////////////////////////////////////////////////////

        uncompressed->width = methods_p->width(pixel);
        uncompressed->pixels = pixel;
        uncompressed->height = methods_p->height(pixel);
        
        /* D12 Print uncompressed PPM and free PPM */
        Pnm_ppmwrite(stdout, uncompressed);
        Pnm_ppmfree(&uncompressed);

        fprintf(stderr, "HEREEE \n");

        // fclose(inputfile);  TODO put these in main after decompress
        // exit(EXIT_SUCCESS); TODO 
}

// cl take in a struct with og_img and a function pointer,
// apply TODO (function contract)
// curr array is comp_vid_arr
void fill_transformed_from_rgb_flt(int col, int row, A2Methods_UArray2 curr_arr, void *elem, void *trans_info)
{
        fprintf(stderr, "(col, row) = %d, %d\n", col, row);
        struct copy_info *info = trans_info;

        /* Get element from orig img arr analagous position */
        struct rgb_float *pix_val = info->prev_methods->at(info->prev_arr, col, row);

        /* Populate new arr with transformed float vals*/
        info->transFun(elem, pix_val);
/*
        fprintf(stderr, "r_val old = %.4f\n", pix_val->red);
        fprintf(stderr, "r_val new = %.4f\n", ((struct component_vid *)elem)->Y);
*/
        (void) curr_arr;        
}

// todo contract
void init_cvs_from_rgb_floats(void *elem, struct rgb_float *rgb_pix)
{
        struct component_vid *cvs_pix = (struct component_vid *)elem;
        cvs_pix->Y = .299 * rgb_pix->red + 0.587 * rgb_pix->green + 0.114 * rgb_pix->blue;
        cvs_pix->Pb = -0.168736 * rgb_pix->red - 0.331264 *  rgb_pix->green + 0.5 * rgb_pix->blue;
        cvs_pix->Pr = 0.5 * rgb_pix->red - 0.418688 * rgb_pix->green - 0.081312 * rgb_pix->blue;
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
        ///// todo maybe put this part in separate helper function get trimmed dims////////////////////
        //fprintf(stderr, "TRIM WIDTH AND HEIGHT: %d, %d \n", width, height);
        
        /* Initialize a new 2D array with new dims */
        A2Methods_UArray2 trim_arr = image->methods->new(width, height, sizeof(struct rgb_float));

        /* Initialize prev array info */
        // todo helper
        struct copy_info *pixels_info = malloc(sizeof(pixels_info));
        assert(pixels_info != NULL);
        pixels_info->prev_arr = image->pixels;
        pixels_info->prev_methods = uarray2_methods_plain;
        pixels_info->transFun = init_floats_from_rgb_ints;


        /* Populate with data from old array */
        A2Methods_mapfun *map = image->methods->map_default;
        map(trim_arr, fill_transformed_from_rgb_ints, pixels_info);

        fprintf(stderr, "POST WIDTH AND HEIGHT: %d, %d \n", width, height);

        return trim_arr;
}

// Copy RGB values to pix in trimmed array
void init_floats_from_rgb_ints(void *elem, Pnm_rgb pix_val)
{
        struct rgb_float *curr_pix = (struct rgb_float *)elem;

        curr_pix->red = (float)pix_val->red;
        curr_pix->green = (float)pix_val->green;
        curr_pix->blue = (float)pix_val->blue;        
}

// todo cleanup merge copying functions: cl take in a struct with og_img and a function pointer,
// TODO MERGE
// apply TODO (function contract)
// rgb ints -> floats
void fill_transformed_from_rgb_ints(int col, int row, A2Methods_UArray2 trim_arr, void *elem, void *trans_info)
{
        struct copy_info *info = trans_info;
        //Pnm_ppm orig = (Pnm_ppm)og_img;

        /* Get element from orig img arr analagous position */
        // todo: if we initialize this as a void ptr, can we reuse other func?
        Pnm_rgb pix_val = info->prev_methods->at(info->prev_arr, col, row);

        // make this a function ptr instead
        //init_floats_from_rgb_ints(elem, pix_val);
        info->transFun(elem, pix_val);
/*
        fprintf(stderr, "r_val old = %u\n", pix_val->red);
        fprintf(stderr, "r_val new = %.4f\n", ((struct rgb_float *)elem)->red);
*/
        (void) trim_arr;        
}

// todo contract
// updates int pixel to hold float values (rounded)
void init_ints_from_rgb_floats(void *elem, struct rgb_float *float_rgb)
{
        Pnm_rgb int_pix = (struct Pnm_rgb *)elem;

        int_pix->red = (int)float_rgb->red;
        int_pix->green = (int)float_rgb->green;
        int_pix->blue = (int)float_rgb->blue;
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

// D12



//todo contract
// void pop_with_rgb_ints(int col, int row, A2Methods_UArray2 rgb_int_arr, void *elem, void *rgb_float_arr)
// {
//         //fprintf(stderr, "(col, row) = %d, %d\n", col, row);
//         A2Methods_UArray2 orig = (A2Methods_UArray2)rgb_float_arr;

//         A2Methods_T methods = uarray2_methods_plain; // todo review this
//         assert(methods != NULL);

//         /* Get element from orig img arr analagous position */
//         struct rgb_float *pix_val = methods->at(orig, col, row);

//         //struct *rgb_float

//         Pnm_rgb int_pix = (struct Pnm_rgb *)elem;
        
//         /* Copy RGB float values to RGB int vals in new array */
//         init_ints_from_rgb_floats(int_pix, pix_val);
        
//         (void) rgb_int_arr;        
// }


// ONE FUNCTION: fill_transformed_arr(int col, int row, A2Methods_UArray2 curr_arr, void *elem, void *trans_info)
        // populates an array of rgb ints from rgb floats
        // populates an array of cvs floats from rgb floats

// SIMILAR:
        // fill_transformed_from_rgb_ints: pop's arr rgb floats from rgb ints