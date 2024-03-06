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

const int BLOCKSIZE = 2;

// hold floating pt representations of a pixel
        // rgb floats
        // cvs floats
struct pixel_float
{
        float r_Y;
        float g_Pb;
        float b_Pr;
};

struct cvs_block
{
        unsigned quant_Pb;
        unsigned quant_Pr;
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
        int counter;
        float Pb_avg;
        float Pr_avg; 
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

/* Transformation (helpers) */
void transform_rgb_floats_to_cvs(void *elem, struct pixel_float *rgb_pix);
void init_floats_from_rgb_ints(void *elem, Pnm_rgb pix_val);
void init_ints_from_rgb_floats(void *elem, struct pixel_float *float_rgb);
void transform_cvs_to_rgb_float(void *elem, struct pixel_float *cvs_pix);
A2Methods_UArray2 create_blocked_arr(int width, int height, int size, int blocksize, A2Methods_T methods);

int make_even(int dim);
struct copy_info *initialize_copy_info(A2Methods_UArray2 prev_arr, A2Methods_T prev_methods, void (*transFun)());

/* Decompression functions */
void decompress40 (FILE *input);
//void pop_with_rgb_ints(int col, int row, A2Methods_UArray2 rgb_int_arr, void *elem, void *rgb_float_arr); // deleted

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
        // decompressTest(uncompressed); // passed C1 todo del E = 0.0000
     
        fprintf(stderr, "Maxval: %u \n", uncompressed->denominator);

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

////////////////////////////////////////////////////////////////////////////
        /* Initialize blocked array with component video pixels */ 
         A2Methods_UArray2 comp_vid_arr = 
         create_blocked_arr(methods_p->width(trimmed_rgb_flts),
         methods_p->height(trimmed_rgb_flts),
         sizeof(struct pixel_float), BLOCKSIZE, methods_b);
         //decompressTest(comp_vid_arr, uncompressed); // E = .0023
/*
         fprintf(stderr, "WIDTH: %d \n", methods_b->width(comp_vid_arr));
         fprintf(stderr, "HEIGHT: %d \n", methods_b->height(comp_vid_arr));
*/

         /* C4 Updates CVS arr to hold values transformed from RGB floats */
         /* Initialize rgb to cvs transfromation info */
         struct copy_info *info = initialize_copy_info(trimmed_rgb_flts, methods_p, transform_rgb_floats_to_cvs); // todo free
         /* Map rgb floats to cvs*/
         methods_b->map_default(comp_vid_arr, fill_transformed_from_rgb_flt, info);
  //       printf("COMPRESSION done \n");
        // decompressTest(comp_vid_arr, uncompressed); E = .0023
////////////////////////////////////////////////////////////////////////////

        /* C6 a Convert chroma averages to quantized*/
        /* Initialize array of cvs block coordinates with 1/BLOCKSIZE dims*/
        A2Methods_UArray2 cvs_arr2b = methods_p->new((methods_p->width(trimmed_rgb_flts)) / BLOCKSIZE, (methods_p->height(trimmed_rgb_flts)) / BLOCKSIZE, sizeof(struct cvs_block));

        (void)cvs_arr2b;
        
        /* Initialize cl info for cvs transformation */
        struct cvs_block *block_info = malloc(sizeof(struct cvs_block)); 
        assert(block_info != NULL);
        block_info->a = 0;
        block_info->b = 0;
        block_info->c = 0;
        block_info->d = 0;
        block_info->quant_Pb = 0;
        block_info->quant_Pr = 0;
        
        methods_p->map_default(comp_vid_arr, get_packed_cvs_block, block_info);

/*  
        printf("W: %d \n ", uncompressed->width);
        printf("H: %d \n ", uncompressed->height);
*/
        (void) comp_vid_arr;
        (void) trimmed_rgb_flts; // array of rgb floats

//todo comment back in after tests 
        // Pnm_ppmfree(&uncompressed);
        // uncompressed = NULL; //todo ptr make null after 
}

// todo remove this function, just here for testing
// todo change input to correct input after compression has been fully implemented --> FILE *input
void decompressTest(A2Methods_UArray2 comp_vid_arr, Pnm_ppm uncompressed)
{
        /* Initialize plain and blocked methods */
        A2Methods_T methods_p = uarray2_methods_plain;        
        A2Methods_T methods_b = uarray2_methods_blocked;
        assert(methods_b != NULL && methods_p != NULL);

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
/*
        fprintf(stderr, "w: %d, h: %d \n", methods_b->width(comp_vid_arr), methods_b->height(comp_vid_arr));
*/

        methods_p->map_default(rgb_float_arr, fill_transformed_from_rgb_flt, cvs_info);
//////////////////////////////////////////////////////////////////////////

        /* D11 Populate 2D array of RGB int vals */
        /* Initialize a new 2D array with new dims */
        ////////////////////////////////////////////////////////////////////////

        // creates pixels arr of pnm rgb ints
        A2Methods_UArray2 pixel = methods_p->new(methods_p->width(rgb_float_arr), methods_p->height(rgb_float_arr), sizeof(struct Pnm_rgb));

        /* Initialize rgb float to rgb int transfromation info */
        struct copy_info *rgb_fl_info = initialize_copy_info(rgb_float_arr, methods_p, init_ints_from_rgb_floats); // todo free

        // A2Methods_mapfun *map = methods_p->map_default; // row maj
        // methods_p->map_default(pixel, pop_with_rgb_ints, rgb_float_arr);

        /* D10 Populate RGB ints pixel array with RGB floats */
        // could lose a bit more data here because rounding floats to ints
        methods_p->map_default(pixel, fill_transformed_from_rgb_flt, rgb_fl_info); // this is in C3 E test, which ppmdiffs to 0.0000

        ///////////////////////////////////////////////////////////////////////

        uncompressed->width = methods_p->width(pixel);
        uncompressed->height = methods_p->height(pixel);
        uncompressed->pixels = pixel;
        
        /* D12 Print uncompressed PPM and free PPM */
        Pnm_ppmwrite(stdout, uncompressed);
        Pnm_ppmfree(&uncompressed);


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
        fprintf(stderr, "R new = %u\n", ((struct Pnm_rgb *)elem)->red);
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

        fprintf(stderr, "CVS Y: %.4f \n", cvs_pix->r_Y);
        fprintf(stderr, "RGB float r: %.4f \n", rgb_pix->r_Y);
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
        /* Initialize a new 2D array with new dims */
        A2Methods_UArray2 trim_arr = image->methods->new(width, height, sizeof(struct pixel_float));

        /* Initialize prev array info */
        // todo helper
        //struct copy_info *pixels_info = initialize_copy_info(image->pixels, uarray2_methods_plain, init_floats_from_rgb_ints);
        struct copy_info *pixels_info = malloc(sizeof(struct copy_info));
        assert(pixels_info != NULL);
        pixels_info->prev_arr = image->pixels;
        pixels_info->prev_methods = uarray2_methods_plain;
        pixels_info->transFun = init_floats_from_rgb_ints;
        /*
        */


        /* Populate with data from old array */
        A2Methods_mapfun *map = image->methods->map_default;
        map(trim_arr, fill_transformed_from_rgb_ints, pixels_info);

        //fprintf(stderr, "POST WIDTH AND HEIGHT: %d, %d \n", width, height);

        return trim_arr;
}

// Copy RGB values to pix in trimmed array
void init_floats_from_rgb_ints(void *elem, Pnm_rgb pix_val)
{
        struct pixel_float *curr_pix = (struct pixel_float *)elem;

        curr_pix->r_Y = (float)pix_val->red;
        curr_pix->g_Pb = (float)pix_val->green;
        curr_pix->b_Pr = (float)pix_val->blue;        

        fprintf(stderr, "Red in transform: %u\n", pix_val->red);
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
*/
        fprintf(stderr, "comp r_val int = %u\n", pix_val->red);
        //fprintf(stderr, "r_val new = %.4f\n", ((struct pixel_float *)elem)->r_Y);
        (void) trim_arr;        
}

// todo contract
// updates int pixel to hold float values (rounded)
// RGB float to RGB int
void init_ints_from_rgb_floats(void *elem, struct pixel_float *float_rgb)
{
        Pnm_rgb int_pix = (struct Pnm_rgb *)elem;
/*
        fprintf(stderr, "FLOAT VAL r: %.4f\n", float_rgb->r_Y);
        fprintf(stderr, "FLOAT VAL g: %.4f\n", float_rgb->g_Pb);                fprintf(stderr, "FLOAT VAL b: %.4f\n", float_rgb->b_Pr);
        // /fprintf(stderr, "FLOAT VAL: %.4f\n", float_rgb->r_Y)
*/

        int_pix->red = (int)float_rgb->r_Y;
        int_pix->green = (int)float_rgb->g_Pb;
        int_pix->blue = (int)float_rgb->b_Pr;
/*
        fprintf(stderr, "INT VAL r: %u\n", int_pix->red);
        fprintf(stderr, "INT VAL g: %u\n", int_pix->green);
        fprintf(stderr, "INT VAL b: %u\n", int_pix->blue);
*/
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

// C6

// This is what will be packed
// for each CVS block: get averages for Pb and Pr
// get a, b, c, d from Y1, Y2, Y3, Y4
        // convert b, c, d to signed 

// cl parameter-->
        // counter--> int 
        // pb average tracker--> float
        // pr average tracker--> float
        // or maybe just update struct directly --> cl

// "apply function" as we iterate through cvs_arr in block major order
// expects counter to begin at 0
void get_packed_cvs_block(int col, int row, A2Methods_UArray2 cvs_arr2b, void *elem, void *block_info)
{
        struct block_info *info = block_info; //typecast cl struct

        struct pixel_float *cvs_curr = (struct pixel_float *)elem;

        // 1) Get average chroma values for a block
        /* If in a block, update averages */        
        if (info->counter < (BLOCKSIZE * BLOCKSIZE))
        {
                // add the current Pb value
                info->Pb_avg += cvs_curr->g_Pb;


                //info->Pr_avg +=     
                info->counter++;
        }

        (void)col;
        (void)row;
        (void)cvs_arr2b;
}