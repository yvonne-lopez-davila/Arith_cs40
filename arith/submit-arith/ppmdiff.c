/*
 *     ppmdiff.c
 *     Authors: Yvonne Lopez Davila and Caroline Shaw
 *     ylopez02 and cshaw03
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
        Pnm_ppm imgBig;
        unsigned maxval_small;
        A2Methods_T methods;
        float sum;
        float denom;
};

int main(int argc, char *argv[])
{
        FILE *one_input;
        FILE *two_input;
        bool big_first = true;

        // todo test with diff combos of stdin//
        
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
        assert(info != NULL);
        info->sum = 0;
        info->methods = methods;
        info->denom = 0;

        /* Iterate through smaller img and calc cum. sum of squared diffs */
        if (big_first) {
                info->imgBig = image_1;
                info->maxval_small = image_2->denominator;
                map(image_2->pixels, get_zDiff, info);
        }
        else {
                info->imgBig = image_2;
                info->maxval_small = image_1->denominator;
                map(image_1->pixels, get_zDiff, info);
        }

        float z = sqrt(info->sum / info->denom);

        free(info);
        Pnm_ppmfree(&image_1);
        Pnm_ppmfree(&image_2);

        printf("%.4f\n", z);

        return EXIT_SUCCESS;
}


// "apply" function
// at each index of the pixel, we want to add the equation to the summation  
void get_zDiff(int col, int row, A2Methods_UArray2 imgSmall_arr, void *elem, void *cl)
{
        /* Initialize curr pix in smaller photo in comparison*/
        struct z_info *info = (struct z_info *)cl;
        Pnm_rgb small_pix = (Pnm_rgb)elem;

        /* Get analagous pixel in larger photo */
        Pnm_rgb big_pix = info->methods->at(info->imgBig->pixels, col, row);
        
        float r_diff = ((float)(big_pix->red) / (float)
        (info->imgBig->denominator) - ((float)(small_pix->red) / (float)
        (info->maxval_small)));

        float g_diff = ((float)(big_pix->green) / (float)
        (info->imgBig->denominator) - ((float)(small_pix->green) / (float)
        (info->maxval_small)));
        float b_diff = ((float)(big_pix->blue) / (float)(info 
        ->imgBig->denominator) - ((float)(small_pix->blue) / (float)
        (info->maxval_small)));
       
        float numer = (r_diff * r_diff) + (g_diff * g_diff) + (b_diff * b_diff);
   
        info->denom = (3 * info->methods->width(imgSmall_arr) * info->methods->height(imgSmall_arr));
        info->sum += numer;

}

void check_width_height_diff(Pnm_ppm *img_one, Pnm_ppm *img_two, bool *big_first)
{
        int diff = ((*img_one)->height - (*img_two)->height);

        /* Swap arrays if image two has larger dimensions*/
        if (diff < 1) {
                *big_first = false;     

        }

        if (abs(diff) > MAX_DIFF) {
                fprintf(stderr, "Error: Difference is too large\n");
                float one = 1.0;
                printf("%.1f\n", one);
        }

}

        /* Optionally, one or the other argument (but not both) may be the C string "-", which stands for standard input. */
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