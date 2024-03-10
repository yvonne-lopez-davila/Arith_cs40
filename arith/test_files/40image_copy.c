/* changes as of 3/4 meeting
 *     40image.c
 *     Authors: Yvonne Lopez Davila and Caroline Shaw
 *     ylopez02 and cshaw03
 *     HW4 / Arith
 *
 *     TO DO:
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

static void (*compress_or_decompress)(FILE *input) = compress40;

/* Compression functions */
Pnm_ppm initialize_ppm(FILE *input);
void compress40 (FILE *input);
A2Methods_UArray2 create_trimmed_float_arr(Pnm_ppm image, int width, int height);
void pop_with_rgb_floats(int col, int row, A2Methods_UArray2 trim_arr, void *elem, void *og_img);
bool is_even(int x);
A2Methods_UArray2 create_blocked_arr(int width, int height, int size, int blocksize, A2Methods_T methods);
void pop_cvs_from_rgb(int col, int row, A2Methods_UArray2 comp_vid_arr, void *elem, void *og_img);
void rgb_to_comp_vid_space(struct component_vid *cvs_pix, struct rgb_float *rgb_pix);

/* Decompression functions */
void decompress40 (FILE *input);
void pop_with_rgb_ints(int col, int row, A2Methods_UArray2 rgb_int_arr, void *elem, void *rgb_float_arr);

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
        //todo declare a methods out here
 

        /* C1 Initialize and populate pnm for uncompressed file */ 
        Pnm_ppm uncompressed = initialize_ppm(input); // rgb ints
        // //decompressTest(uncompressed); // passed C1
        A2Methods_T methods_p = uarray2_methods_plain; // todo review this

        /* C2/C3 Create trimmed arr with rgb float types */
        A2Methods_UArray2 trimmed_rgb_flts = create_trimmed_float_arr(uncompressed, uncompressed->width, uncompressed->height); // todo maybe make width and height pointers so that those variables update
        //decompressTest(trimmed_rgb_flts, uncompressed); // C3 passed
        
        /* C4 Transform RGB floats to component video space */
                //initialize new blocked arr with 2 blocksize
                // apply--> transform to Y, Pb, Pr (float to float) 
        /* Initialize blocked array with component video pixels */
        A2Methods_T methods_b = uarray2_methods_blocked; // todo review this

        A2Methods_UArray2 comp_vid_arr = create_blocked_arr(methods_p->width(trimmed_rgb_flts), methods_p->height(trimmed_rgb_flts), sizeof(struct component_vid), BLOCKSIZE, methods_b);

        fprintf(stderr, "WIDTH: %d \n", methods_b->width(comp_vid_arr));
        fprintf(stderr, "HEIGHT: %d \n", methods_b->height(comp_vid_arr));

        A2Methods_mapfun *map = methods_b->map_block_major; // row maj
        map(comp_vid_arr, pop_cvs_from_rgb, trimmed_rgb_flts);
/*  
        printf("W: %d \n ", uncompressed->width);
        printf("H: %d \n ", uncompressed->height);
*/
        (void) comp_vid_arr;
        (void) trimmed_rgb_flts;

        Pnm_ppmfree(&uncompressed);
        uncompressed = NULL; //todo ptr make null after 
}

// todo remove this function, just here for testing
// todo change input to correct input after compression has been fully implemented --> FILE *input
void decompressTest(A2Methods_UArray2 rgb_float_arr, Pnm_ppm uncompressed)
{
        // TODO works when called from commandline, but not when called from compress

        /* D10 Convert RGB floats to RGB ints*/

        /* D11 Populate 2D array of RGB int vals */
        /* Initialize a new 2D array with new dims */
        ////////////////////////////////////////////////////////////////////////
        A2Methods_T methods = uarray2_methods_plain; // todo review this
        assert(methods != NULL);

        A2Methods_UArray2 pixel = methods->new(methods->width(rgb_float_arr), methods->height(rgb_float_arr), sizeof(struct Pnm_rgb));

        A2Methods_mapfun *map = methods->map_default; // row maj
        map(pixel, pop_with_rgb_ints, rgb_float_arr);

        ///////////////////////////////////////////////////////////////////////

        uncompressed->width = methods->width(pixel);
        uncompressed->pixels = pixel;
        uncompressed->height = methods->height(pixel);
        
        /* D12 Print uncompressed PPM and free PPM */
        Pnm_ppmwrite(stdout, uncompressed);
        Pnm_ppmfree(&uncompressed);


        // fclose(inputfile);  TODO put these in main after decompress
        // exit(EXIT_SUCCESS); TODO 
}

// cl take in a struct with og_img and a function pointer,
// apply TODO (function contract)
void pop_cvs_from_rgb(int col, int row, A2Methods_UArray2 comp_vid_arr, void *elem, void *og_arr)
{
        //fprintf(stderr, "(col, row) = %d, %d\n", col, row);

        A2Methods_UArray2 orig = (A2Methods_UArray2)og_arr;

        A2Methods_T methods_plain = uarray2_methods_plain; // todo review this
        assert(methods_plain != NULL);

        /* Get element from orig img arr analagous position */
        struct rgb_float *pix_val = methods_plain->at(orig, col, row);
        //fprintf(stderr, "TRACE AA\n");

        /* Populate new arr with transformed float vals*/
        /* Copy RGB values to pix in trimmed array */
        struct component_vid *cvs_pix = (struct component_vid *)elem;
        rgb_to_comp_vid_space(cvs_pix, pix_val);

/*
        fprintf(stderr, "r_val old = %.4f\n", pix_val->red);
        fprintf(stderr, "r_val new = %.4f\n", cvs_pix->Y);
*/

        (void) comp_vid_arr;        
}

// cvs = component video space
void rgb_to_comp_vid_space(struct component_vid *cvs_pix, struct rgb_float *rgb_pix)
{
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


//trim_odd_dims TODO (function contract)
// todo can probably make this and above a single function if add size parameter and some way of knowing whether to set block size (pass in methods)
A2Methods_UArray2 create_trimmed_float_arr(Pnm_ppm image, int width, int height)
{
        ///// todo maybe put this part in separate helper function get trimmed dims////////////////////
        fprintf(stderr, "ORIGINAL WIDTH AND HEIGHT: %d, %d \n", width, height);
        /* Set width and height to trimmed dimensions */
        if (!is_even(width))
        {
                width--;
        }
        if (!is_even(height))
        {  
                height--;
        }

        /* Initialize a new 2D array with new dims */
        A2Methods_UArray2 trim_arr = image->methods->new(width, height, sizeof(struct rgb_float));

        /* Populate with data from old array */
        A2Methods_mapfun *map = image->methods->map_default;
        map(trim_arr, pop_with_rgb_floats, image);

        fprintf(stderr, "POST WIDTH AND HEIGHT: %d, %d \n", width, height);

        return trim_arr;
}


// todo cleanup merge copying functions: cl take in a struct with og_img and a function pointer,
// apply TODO (function contract)
void pop_with_rgb_floats(int col, int row, A2Methods_UArray2 trim_arr, void *elem, void *og_img)
{
        //printf("in apply \n");
        Pnm_ppm orig = (Pnm_ppm)og_img;

        /* Get element from orig img arr analagous position */
        Pnm_rgb pix_val = orig->methods->at(orig->pixels, col, row);

        //////////// todo put in helper function (passed as ptr) /////////
        /* Copy RGB values to pix in trimmed array */
        struct rgb_float *curr_pix = (struct rgb_float *)elem; // would need to change typecast here
        curr_pix->red = (float)pix_val->red; // curr_pix->Y = transformed(pix_val->red)
        curr_pix->green = (float)pix_val->green;
        curr_pix->blue = (float)pix_val->blue;
        ////////////////////////////////////////////////////////////////
/*
        fprintf(stderr, "r_val old = %u\n", pix_val->red);
        fprintf(stderr, "r_val new = %.4f\n", curr_pix->red);
*/

        (void) trim_arr;        
}

// todo contract
// updates int pixel to hold float values (rounded)
void pop_rgb_ints_from_floats(Pnm_rgb int_struct, struct rgb_float *float_rgb)
{
        int_struct->red = (int)float_rgb->red;
        int_struct->green = (int)float_rgb->green;
        int_struct->blue = (int)float_rgb->blue;
}

//todo contract
void pop_with_rgb_ints(int col, int row, A2Methods_UArray2 rgb_int_arr, void *elem, void *rgb_float_arr)
{
        //fprintf(stderr, "(col, row) = %d, %d\n", col, row);
        A2Methods_UArray2 orig = (A2Methods_UArray2)rgb_float_arr;

        A2Methods_T methods = uarray2_methods_plain; // todo review this
        assert(methods != NULL);

        /* Get element from orig img arr analagous position */
        struct rgb_float *pix_val = methods->at(orig, col, row);

        //struct *rgb_float

        Pnm_rgb int_pix = (struct Pnm_rgb *)elem;
        
        /* Copy RGB float values to RGB int vals in new array */
        pop_rgb_ints_from_floats(int_pix, pix_val);
        
        (void) rgb_int_arr;        
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