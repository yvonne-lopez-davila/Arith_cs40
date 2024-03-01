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
//#include <pnmrdr.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "a2methods.h"
#include "a2plain.h"
#include "a2blocked.h"
#include "pnm.h"

const int MAX_ARGS = 3;

void open_valid_ppm_files(int argc, char *argv[], FILE **one_input,
                          FILE **two_input);
int main(int argc, char *argv[])
{
        FILE *one_input;
        FILE *two_input;

        open_valid_ppm_files(argc, argv, &one_input, &two_input);

        return 0;
}

        //Optionally, one or the other argument (but not both) may be the C string "-", which stands for standard input.
void open_valid_ppm_files(int argc, char *argv[], FILE **one_input, FILE 
                          **two_input)
{
        /* User should always provide two commandline args*/
        assert(argc == MAX_ARGS); /* MAX_ARGS == 3 */
        assert(one_input != NULL && two_input != NULL);
        assert(!(strcmp(argv[2], argv[1]))); /* Both files cannot be stdin*/

        
        /* Open commandline files, or read from stdin if applicable */
// todo move these two checks in a helper function that uses i and a file**
        if (strcmp(argv[1], "-") == 0) {
            *one_input = stdin;
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


        /* default to UArray2 methods */
        A2Methods_T methods = uarray2_methods_plain;
        assert(methods != NULL);

        Pnm_ppm image_1 = Pnm_ppmread(*one_input, methods);

        Pnm_ppm image_2 = Pnm_ppmread(*two_input, methods);

        (void) image_1;
        (void) image_2;

}