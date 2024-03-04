/*
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


static void (*compress_or_decompress)(FILE *input) = compress40;

const int BLOCKSIZE = 2;

/* Compression functions */
Pnm_ppm initialize_ppm(FILE *input);
void compress40 (FILE *input);
void trim_odd_dims(Pnm_ppm image, int width, int height);
void copy_pix_to_new_arr(int col, int row, A2Methods_UArray2 trim_arr, void *elem, void *og_img);
bool is_even(int x);
A2Methods_UArray2 intialize_blocked_arr(int width, int height, int size, int blocksize);

/* Decompression functions */
void decompress40 (FILE *input);

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


// todo contract
// note: at an intermed step, this will maybe hold converted float rep of rgb ints, is the name still okay?
struct Pnm_component_vid
{
        float Y;
        float Pb;
        float Pr;
};

// Compress40 TODO (function contract)
void compress40 (FILE *input)
{

        //grid of RGB int pixels todo del
        /* C1 Initialize and populate pnm for uncompressed file */ 
        Pnm_ppm uncompressed = initialize_ppm(input);
/*
printf("TRACE a\n");
        decompress40(input); // test todo del
printf("TRACE b\n");
*/

        /* C2 Update pnm pixels arr to have even dims */
        trim_odd_dims(uncompressed, uncompressed->width, uncompressed->height); /* C2 */

        printf("MODIF WIDTH AND HEIGHT: %d, %d \n", uncompressed->width, uncompressed->height);

        /* C5 Initialize 2D blocked array with trimmed dims*/
        A2Methods_UArray2 comp_vid_arr = intialize_blocked_arr(uncompressed->width, uncompressed->height, sizeof(struct Pnm_component_vid), BLOCKSIZE);
        /* Populate with data from old array */
        // A2Methods_mapfun *map = image->methods->;
        // map(trim_arr, copy_pix_to_new_arr, image);

        (void)comp_vid_arr;

        /* C3 Transform RGB ints to RGB floats */

        /* C4 Transform RGB floats to component video */


/*
        printf("W: %d \n ", uncompressed->width);
        printf("H: %d \n ", uncompressed->height);
*/
}

// todo contract
A2Methods_UArray2 intialize_blocked_arr(int width, int height, int size, int blocksize)
{
        /* Use blocked methods */
        A2Methods_T methods = uarray2_methods_blocked;
        assert(methods != NULL);

        /* Initialize blocked 2D with specified dimensions*/
        A2Methods_UArray2 blocked_arr = methods->new_with_blocksize(width, height, size, blocksize);

        return blocked_arr;
}

// Decompress40 TODO (function contract)
// todo change input to correct input after compression has been fully implemented --> FILE *input
void decompress40 (FILE *input)
{
        // TODO works when called from commandline, but not when called from compress



        /* D11 Populate 2D array of RGB int vals */
        Pnm_ppm uncompressed = initialize_ppm(input);

        printf("TRACE c \n");
        /* D12 Print uncompressed PPM and free PPM */
        Pnm_ppmwrite(stdout, uncompressed);
        Pnm_ppmfree(&uncompressed);


        // fclose(inputfile);  TODO put these in main after decompress
        // exit(EXIT_SUCCESS); TODO   
        (void) input;       

}


// apply TODO (function contract) 
/*
void apply(int col, int row, A2Methods_UArray2 trim_arr, void *elem, void *og_img)
{
        // for each pixel in trimmed array:
                // transform int rgbs to floats
                // transform float rgbs to component vid Y, Pb, Pr
                // make a blocked 2d array with converted values (where each cell holds Y, Pb, Pr (struct) )

}
*/


//trim_odd_dims TODO (function contract)
void trim_odd_dims(Pnm_ppm image, int width, int height)
{
        printf("ORIGINAL WIDTH AND HEIGHT: %d, %d \n", width, height);
        /* Set width and height to trimmed dimensions */
        if (is_even(width) && is_even(height))
        {
                return;
        }
        if (!is_even(width))
        {
                width--;
        }
        if (!is_even(height))
        {  
                height--;
        }

        /* Initialize a new 2D array with new dims */
        A2Methods_UArray2 trim_arr = image->methods->new(width, height, sizeof(struct Pnm_rgb));

        /* Populate with data from old array */
        A2Methods_mapfun *map = image->methods->map_default;
        map(trim_arr, copy_pix_to_new_arr, image);

        /* Swap arrays and free untrimmed */
        A2Methods_UArray2 temp = image->pixels;
        image->pixels = trim_arr;
        image->width = width;
        image->height = height;
        image->methods->free(&temp);
}

// apply TODO (function contract)
void copy_pix_to_new_arr(int col, int row, A2Methods_UArray2 new_arr, void *elem, void *og_img)
{
        //printf("in apply \n");
        Pnm_ppm orig = (Pnm_ppm)og_img;

        /* Get element from orig img arr analagous position */
        Pnm_rgb pix_val = orig->methods->at(orig->pixels, col, row);

        // todo thought: what if this is already the 2D blocked array and we 
        // just put in floats here?

        /* Copy vals from prev array to new array */
        Pnm_rgb curr_pix = (struct Pnm_rgb *) elem;
        //printf("\n(col, row) = %d, %d \n", col, row);

        curr_pix->red = pix_val->red;
        curr_pix->green = pix_val->green;
        curr_pix->blue = pix_val->blue;

        //printf("r_val old = %u\n", pix_val->red);
        //printf("r_val new = %u\n", curr_pix->red);

        (void) new_arr;        
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
        printf("trace d\n");

        return image;
}

// D12