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

void apply(int col, int row, A2Methods_UArray2 trim_arr, void *elem, void *og_img);
void compress40 (FILE *input);
void decompress40 (FILE *input);
Pnm_ppm initialize_ppm(FILE *input);
bool is_even(int x);
void trim_odd_dims(Pnm_ppm image, int width, int height);

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
        Pnm_ppm uncompressed = initialize_ppm(input);
        trim_odd_dims(uncompressed, uncompressed->width, uncompressed->height);

        printf("W: %d \n ", uncompressed->width);
        printf("H: %d \n ", uncompressed->height);
}

// Decompress40 TODO (function contract)
void decompress40 (FILE *input)
{
        (void) input;       

}

//trim_odd_dims TODO (function contract)
void trim_odd_dims(Pnm_ppm image, int width, int height)
{
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

        printf("testing width pre : %d \n", image->width); 
        printf("testing height pre: %d \n", image->height); 

        /* Initialize a new 2D array with new dims */
        A2Methods_UArray2 trim_arr = image->methods->new(width, height, sizeof(struct Pnm_rgb));

        /* Populate with data from old array */
        A2Methods_mapfun *map = image->methods->map_default;
        map(trim_arr, apply, image);

        /* Swap arrays and free untrimmed */
        Pnm_ppm temp = image->pixels;
        image->pixels = trim_arr;
        Pnm_ppmfree(&temp);

        printf("testing width after : %d \n", image->width); 
        printf("testing height after: %d \n", image->height); 

}

// apply TODO (function contract)
void apply(int col, int row, A2Methods_UArray2 trim_arr, void *elem, void *og_img)
{
        Pnm_ppm orig = (Pnm_ppm)og_img;

        /* Get element from orig img arr analagous position */
        Pnm_rgb *pix_val = orig->methods->at(orig->pixels, col, row);

        /* Copy RGB values to pix in trimmed array */
        Pnm_rgb curr_pix = (struct Pnm_rgb *) elem;
        printf("\nTRACE 1\n");
        // todo these lines are seg-faulting
        curr_pix->red = (*pix_val)->red;
        curr_pix->green = (*pix_val)->green;
        curr_pix->blue = (*pix_val)->blue;

        (void) trim_arr;        
}

bool is_even(int x)
{
        return (x % 2 == 0);
}

//Initialize_ppm TODO (function contract)
Pnm_ppm initialize_ppm(FILE *input)
{
        printf("TRACE 2\n");
        assert(input != NULL);
        
        /* default to UArray2 methods */
        A2Methods_T methods = uarray2_methods_plain;
        assert(methods != NULL);

        Pnm_ppm image = Pnm_ppmread(input, methods);

        return image;
}