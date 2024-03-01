/*
 *     ppmdiff.c
 *     Authors: Yvonne Lopez Davila and
 *     ylopez02 and
 *     HW4 / Arith
 *
 *     The ppmdiff program quantifies the similarity between two user-specified 
 *     PPM images
 *
 *      Usage: 
 *          1) ./ppmdiff -[ppm_filename_1] -[ppm_filename_2]
 *            (Specify the PGM image file as a command-line argument.)
 *
 *          2) ./ppmdiff -[ppm_filename_1] -
 *      One or the other argument (but not both) may be the C string "-", 
 *      which stands for standard input
 * 
 *       - The program accepts 2 commandline arguments, which
 *       should be the names of two valid PGM image files, or one valid 
 *       filename and stdin.
 *
 *     Notes:
 *
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

#include "a2methods.h"
#include "a2plain.h"
#include "a2blocked.h"
#include "pnm.h"

const int MAX_ARGS = 3;
const int MAX_DIFF = 1; 

void open_valid_ppm_files(int argc, char *argv[], FILE **one_input,
                          FILE **two_input);
void check_width_height_diff(Pnm_ppm *img_one, Pnm_ppm *img_two);

int main(int argc, char *argv[])
{
        FILE *one_input;
        FILE *two_input;

        // todo test with diff combos of stdin
        /* Updates file pointers for valid file input */
        open_valid_ppm_files(argc, argv, &one_input, &two_input);

        /* default to UArray2 methods */
        A2Methods_T methods = uarray2_methods_plain;
        assert(methods != NULL);

        /* Populate 2 ppm image objects */
        Pnm_ppm image_1 = Pnm_ppmread(one_input, methods);
        Pnm_ppm image_2 = Pnm_ppmread(two_input, methods);

        check_width_height_diff(&image_1, &image_2);



        return 0;
}


// use the smaller dimensions in summations
float get_diff_coeff(Pnm_ppm *image_one, Pnm_ppm *image_two, int small_width, int small_height)
{// todo these should be pixels 
        unsigned r_diff = ((*image_one)->pixels->red - (*image_two)->pixels->red);
        unsigned g_diff = ((*image_one)->pixels->green - (*image_two)->pixels->green);
        unsigned b_diff = ((*image_one)->pixels->blue - (*image_two)->pixels->blue);

        unsigned numer = (r_diff * r_diff) + (g_diff * g_diff) + (b_diff * b_diff);
        unsigned denom = (3 * small_width * small_height);

        float E = 0;

        for (int i = 0; i < small_width; i++) {
                for (int j = 0; j < small_height; j++) {
                        E += sqrt(numer / denom ); // todo check type, might need to be a float
                } 
        }

        return E;
}
/*
*/

void check_width_height_diff(Pnm_ppm *img_one, Pnm_ppm *img_two)
{
        int diff = ((*img_one)->height - (*img_two)->height);
        printf("Testing PRE swap: %d\n", (*img_one)->width);

        /* Set larger image to img_one*/
        if (diff < 1) {
                Pnm_ppm *temp = img_one;
                Pnm_ppm img_one = img_two;
                Pnm_ppm img_two = temp;
                Pnm_ppmfree(temp);
        }

        printf("Testing swap: %d\n", (*img_one)->width);

        if (abs(diff) > MAX_DIFF) {
                fprintf(stderr, "Error: Difference is too large\n");
                float one = 1.0;
                printf("%.1f\n", one);
        }

}

        //Optionally, one or the other argument (but not both) may be the C string "-", which stands for standard input.
void open_valid_ppm_files(int argc, char *argv[], FILE **one_input, FILE 
                          **two_input)
{
        /* User should always provide two commandline args*/
        assert(argc == MAX_ARGS); /* MAX_ARGS == 3 */
        assert(one_input != NULL && two_input != NULL);


        
        /* Open commandline files, or read from stdin if applicable */
// todo move these two checks in a helper function that uses i and a file**
        if (strcmp(argv[1], "-") == 0) {
                *one_input = stdin;
                assert(!(strcmp(argv[2], argv[1]) == 0)); // both files cannot come from stdin
        } 
        else {
                *one_input = fopen(argv[1], "rb");
        }
//
        if (strcmp(argv[2], "-") == 0) {
                *two_input = stdin;
        } 
        else {
                *two_input = fopen(argv[2], "rb");
        }

        printf("Trace %s\n", argv[1]);
        printf("Trace %s\n", argv[2]);

        assert(*one_input != NULL);
        assert(*two_input != NULL); 
}