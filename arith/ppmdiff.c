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

void check_width_height_diff(Pnm_ppm *img_one, Pnm_ppm *img_two, bool *big_first);
void get_zDiff(int col, int row, A2Methods_UArray2 imgTwo_arr, void *elem, void *cl);
void open_valid_ppm_files(int argc, char *argv[], FILE **one_input,
                          FILE **two_input);

struct z_info
{
        A2Methods_UArray2 imgBig_arr;
        A2Methods_T methods;
        float sum;
};

int main(int argc, char *argv[])
{
        FILE *one_input;
        FILE *two_input;
        bool big_first = true;

        // todo test with diff combos of stdin
        /* Updates file pointers for valid file input */
        open_valid_ppm_files(argc, argv, &one_input, &two_input);

        /* default to UArray2 methods */
        A2Methods_T methods = uarray2_methods_plain;
        assert(methods != NULL);
        A2Methods_mapfun *map = methods->map_default;

        /* Populate 2 ppm image objects */
        Pnm_ppm image_1 = Pnm_ppmread(one_input, methods);
        Pnm_ppm image_2 = Pnm_ppmread(two_input, methods);

        check_width_height_diff(&image_1, &image_2, &big_first);
        
        struct z_info *info = malloc(sizeof(struct z_info)); // todo free
        info->sum = 0;
        info->methods = methods;

        /* Iterate through smaller img and calc cum. sum of squared diffs */
        if (big_first) {
                info->imgBig_arr = image_1->pixels;
                map(image_2->pixels, get_zDiff, info);
        }
        else {
                info->imgBig_arr = image_2->pixels;
                map(image_1->pixels, get_zDiff, info);
        }

        printf("Z coefficient is: %.4f\n", info->sum);

        return 0;
}


// "apply" function
// at each index of the pixel, we want to add the equation to the summation  
void get_zDiff(int col, int row, A2Methods_UArray2 imgSmall_arr, void *elem, void *cl)
{
        /* Initialize curr pix in smaller photo in comparison*/
        struct z_info *info = (struct z_info *)cl;
        Pnm_rgb small_pix = (Pnm_rgb)elem;

        /* Get analagous pixel in larger photo */
        Pnm_rgb big_pix = info->methods->at(info->imgBig_arr, col, row);
        
        unsigned r_diff = (big_pix->red - (small_pix)->red);
        unsigned g_diff = (big_pix->green - (small_pix)->green);
        unsigned b_diff = (big_pix->blue - (small_pix)->blue);
        
        unsigned numer = (r_diff * r_diff) + (g_diff * g_diff) + (b_diff * b_diff);
        unsigned denom = (3 * info->methods->width(imgSmall_arr) * info->methods->height(imgSmall_arr));

        info->sum += sqrt(numer / denom ); // todo check type, might need to be a float

        printf("Tracing curr sum... (col, row): (%d, %d), sum: %.4f \n", col, row, info->sum);

}

void check_width_height_diff(Pnm_ppm *img_one, Pnm_ppm *img_two, bool *big_first)
{
        int diff = ((*img_one)->height - (*img_two)->height);

        /* Swap arrays if image two has larger dimensions*/
        if (diff < 1) {
                *big_first = false;     

        }

//        printf("Testing swap: %d\n", (*img_one)->width);

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

        assert(*one_input != NULL);
        assert(*two_input != NULL); 
}