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

/* Compression functions */
Pnm_ppm initialize_ppm(FILE *input);
void compress40 (FILE *input);
void trim_odd_dims(Pnm_ppm image, int width, int height);
void apply(int col, int row, A2Methods_UArray2 trim_arr, void *elem, void *og_img);
bool is_even(int x);

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

// Compress40 TODO (function contract)
void compress40 (FILE *input)
{
        //grid of RGB int pixels todo del
        /* Initialize and populate pnm for uncompressed file */ 
        Pnm_ppm uncompressed = initialize_ppm(input); /* C1 */
        trim_odd_dims(uncompressed, uncompressed->width, uncompressed->height); /* C2 */

        printf("W: %d \n ", uncompressed->width);
        printf("H: %d \n ", uncompressed->height);
}

// Decompress40 TODO (function contract)
void decompress40 (FILE *input)
{
        /* Print uncompressed PPM */
        Pnm_ppm uncompressed = initialize_ppm(input);

        (void) input;       

}

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
        map(trim_arr, apply, image);

        /* Swap arrays and free untrimmed */
        A2Methods_UArray2 temp = image->pixels;
        image->pixels = trim_arr;
        image->width = width;
        image->height = height;
        image->methods->free(&temp);
}

// apply TODO (function contract)
void apply(int col, int row, A2Methods_UArray2 trim_arr, void *elem, void *og_img)
{
        //printf("in apply \n");
        Pnm_ppm orig = (Pnm_ppm)og_img;

        /* Get element from orig img arr analagous position */
        Pnm_rgb pix_val = orig->methods->at(orig->pixels, col, row);

        /* Copy RGB values to pix in trimmed array */
        Pnm_rgb curr_pix = (struct Pnm_rgb *) elem;
        //printf("\n(col, row) = %d, %d \n", col, row);

        // todo these lines are seg-faulting
        curr_pix->red = pix_val->red;
        curr_pix->green = pix_val->green;
        curr_pix->blue = pix_val->blue;

        //printf("r_val old = %u\n", pix_val->red);
        //printf("r_val new = %u\n", curr_pix->red);

        (void) trim_arr;        
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

        return image;
}