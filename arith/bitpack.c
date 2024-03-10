
/*
 *     bitpack.c
 *     Authors: Yvonne Lopez Davila and Caroline Shaw
 *     ylopez02 and cshaw03
 *     HW4 / Arith
 *
 *     todo
 *
 * 
 *     Notes:
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "assert.h"
#include "compress40.h"
#include "pnm.h"
#include "a2methods.h"
#include "a2plain.h"
#include "a2blocked.h"
#include "arith40.h"

const unsigned MAX_WIDTH = 64;
const uint64_t U1 = 1;
const int64_t S1 = 1;

// You are to implement this interface in file bitpack.c. Unless otherwise indicated below, your functions should use
// Hanson assertions (from assert.h) to ensure that shift values are for ≤ 64 bits, that widths are ≤ 64, and that bit
// fields to be accessed or updated fit entirely within the supplied 64 bit word.

/****************************
 *   Width Test Functions  *
 ***************************/
Except_T Bitpack_Overflow = { "Overflow packing bits" };

/*
B) Unsigned ints
range: 0 to (2^n) - 1
(ie: for 8 bits, 0 to 2^8 - 1 = 255)
*/
// test to see if an integer can be represented in k bits

// "can n be represented in width bits?"
// edge cases to test: n = 0, width = 64, n = -1, n = 2.2 
// (n = 256, w = 8) --> Out of range // this edge passed
/********** Bitpack_fitsu ********
 *
 * Description: 
 *      Checks if an unsigned integer value fits within the specified width.
 * Parameters:
 *      n: The unsigned integer value to be checked.
 *      width: The width of the representation in bits.
 * Return: 
 *      True if the value can be represented within the specified width; 
 *      false otherwise.
 * Expects:
 *      The width parameter is less than or equal to the maximum 
 *      width (MAX_WIDTH).
 * Notes:
 *      function checks if the unsigned integer value falls within the range
 *      that can be represented by the specified width.
 ************************/

bool Bitpack_fitsu(uint64_t n, unsigned width)
{
    assert(width <= MAX_WIDTH);

    uint64_t u_bound;

    /* Set inclusive upper bound of n values that fit */
    if (width == MAX_WIDTH) {
        /* Handle shifts by 64 */
        u_bound = (U1 << (width - 1)); /* (2^63) */
        u_bound = (u_bound * 2) - 1; /* 2-64 -1*/
    } 
    else {
        u_bound = ((U1 << width) - U1); /* (2^n) - 1, inclusive */
    }    

    return (n <= u_bound);
}

/*
A) Signed ints
range: -2^(n-1) to (2^(n-1) - 1)
(ex: for 8 bits, -128 to 127) // passed
*/
// todo this works for 64... but prev does not
/********** Bitpack_fitss ********
 *
 * Description: 
 *      Checks if a signed integer value fits within the specified width when
 *      represented in two's complement format.
 * Parameters:
 *      n: The signed integer value to be checked.
 *      width: The width of the representation in bits.
 * Return: 
 *      True if the value can be represented within the specified width; 
 *      false otherwise.
 * Expects:
 *      The width parameter is less than or equal to the maximum width 
 *      (MAX_WIDTH).
 * Notes:
 *      function checks if the signed integer value falls within the range that
 *      can be represented by the specified width using two's complement
 *      representation.
 ************************/

bool Bitpack_fitss( int64_t n, unsigned width)
{  
    assert(width <= MAX_WIDTH);
    
    if (width == 0) {
        return 0;
    }

    // todo: do we need to use 2s complement here or is *(-1) fine?
    // todo: verify that these equations work
    int64_t neg_bound = -1 * (S1 << (width - S1));  /* (2^(n-1) inclusive */
    int64_t pos_bound = ~(neg_bound); /* (2^(n-1) - 1) inclusive */ 

    /* Return if in range that can be represented by width bits*/
    return ((n >= neg_bound) && (n <= pos_bound)); 
}


/*********************************
 ** Field-extraction functions **
 ********************************/

// The next functions you are to define extract values from a word. Values extracted may be signed or unsigned, but by
// programming convention we use only unsigned types to represent words.

// n extracts a field from a word given the width of the field and the location of the field’s least significant
// bit
// todo test this
/********** Bitpack_getu ********
 *
 * Description: 
 *      Extracts an unsigned value from a word with the specified width and
 *      least significant bit (lsb) position.
 * Parameters:
 *      word: The word from which the value will be extracted.
 *      width: The width of the value.
 *      lsb: The least significant bit position of the value within the word.
 * Return: 
 *      The extracted unsigned value.
 * Expects:
 *      -The width parameter is less than or equal to 64.
 *      -The sum of width and lsb is less than or equal to 64.
 * Notes:
 *      extracts the value from the word by masking it and then shifting it
 *      to the least significant bit position.
 ************************/

uint64_t Bitpack_getu(uint64_t word, unsigned width, unsigned lsb)
{
    assert(width <= 64);
    assert((width + lsb) <= 64);

    /* Create mask with ones at indices of word and zeroes elsewhere */
    uint64_t mask = ~0; /* 64-bit binary with all ones */
    mask = mask >> (64 - width) << lsb;

    /* Extract word using mask (shifted to ones place) */
    uint64_t subword = (mask & word) >> lsb;

    return subword;
}

// todo: not sure if this should be more different from prev (only change is the type of subword?)
/********** Bitpack_gets ********
 *
 * Description: 
 *      Extracts a signed value from a word with the specified width and least 
 *      significant bit (lsb) position.
 * Parameters:
 *      word: The word from which the value will be extracted.
 *      width: The width of the value.
 *      lsb: The least significant bit position of the value within the word.
 * Return: 
 *      The extracted signed value.
 * Expects:
 *      -The width parameter is less than or equal to 64.
 *      -The sum of width and lsb is less than or equal to 64.
 * Notes:
 *      function extracts the value from the word by masking it and then
 *      shifting it to the least significant bit position.
 ************************/

int64_t Bitpack_gets(uint64_t word, unsigned width, unsigned lsb)
{
    assert(width <= 64);
    assert((width + lsb) <= 64);
    
    /* Create mask with ones at indices of word and zeroes elsewhere */
    int64_t mask = ~0; /* 64-bit binary with all ones */
    mask = mask >> (64 - width) << lsb;

    /* Extract word using mask (shifted to ones place) */
    int64_t subword = (mask & word) >> lsb;

    subword = (subword << (MAX_WIDTH - width)) >> (MAX_WIDTH - width);

    // /* Determine if subword is negative by checking subword MSB */
    // int64_t sub_msb = subword >> (width - 1); /* Move MSB in lsb == 0*/

    // /* Initialize negative bit representation if subword MSB == 1 */
    // if (sub_msb == 1) {
    //     subword = ~subword + 1;
    // }

    return subword;
}

/********** Bitpack_newu ********
 *
 * Description: 
 *      Updates a word by inserting a new unsigned value with the specified
 *      width at the specified least significant bit (lsb) position.
 * Parameters:
 *      word: The original word to be updated.
 *      width: The width of the new value.
 *      lsb: The least significant bit position where the new value will be 
 *                                                                  inserted.
 *      value: The new unsigned value to be inserted.
 * Return: 
 *      The updated word with the new value inserted at the specified position.
 * Expects:
 *      -The width parameter is less than or equal to 64.
 *      -The value fits within the specified width.
 * Notes:
 *      updates the word by clearing bits from lsb to (lsb + width - 1) and 
 *      then inserting the new value at the cleared positions.
 ************************/

uint64_t Bitpack_newu(uint64_t word, unsigned width, unsigned lsb, uint64_t value)
{
    /* Check that width is in range and value fits in width bits */
    assert(width <= 64);
    if (!Bitpack_fitsu(value, width)) {
        RAISE(Bitpack_Overflow);
    }


    /* Create mask2 with ones at indices of word and zeroes elsewhere */
    uint64_t mask2 = ~0; /* 64-bit binary with all ones */
    mask2 = mask2 >> (64 - width) << lsb;

    /* Create mask2 with zeroes at indices of word and ones elsewhere */
    /* mask 1 = mask with errors */
    uint64_t mask1 = ~mask2;

    /* Set word with all zeroes except for value starting at lsb index */
    value = (value << lsb);
    fprintf(stderr, "\n unsigned rep: %ld \n", value);
    /* Set 0s at indices of original word which will be replaced */
    uint64_t m_word = (mask1 & word);

    /* Write word with value at zeroed indices */
    uint64_t new_word = (m_word | value); 

    return new_word;
}

/********** Bitpack_news ********
 *
 * Description: 
 *      Updates a word by inserting a new value with the specified width at 
 *      the specified least significant bit (lsb) position.
 * Parameters:
 *      word: The original word to be updated.
 *      width: The width of the new value.
 *      lsb: The least significant bit position where the new value will be 
 *                                                                  inserted.
 *      value: The new value to be inserted.
 * Return: 
 *      The updated word with the new value inserted at the specified position.
 * Expects:
 *      -The width parameter is less than or equal to 64.
 *      -The value fits within the specified width.
 * Notes:
 *      updates the word by clearing bits from lsb to (lsb + width - 1) and then 
 *      inserting the new value at the cleared positions.
 ************************/

uint64_t Bitpack_news(uint64_t word, unsigned width, unsigned lsb,  int64_t value)
{
    /* Check that width is in range and value fits in width bits */
    assert(width <= 64);
    if (!Bitpack_fitss(value, width)) {
        RAISE(Bitpack_Overflow);
    }

    /* Create mask1 with zeroes at indices of word and ones elsewhere */
    uint64_t mask2 = ~0; /* 64-bit binary with all ones */
    mask2 = mask2 >> (64 - width) << lsb; /* ones only at indices to replace */
    uint64_t mask1 = ~mask2;

    /* Set word with all zeroes except for value starting at lsb index */
    uint64_t u_val = value; // todo not preserving bits
    u_val = (u_val << lsb);
    u_val = (mask2 & u_val);

    /* Set 0s at indices of original word which will be replaced */
    uint64_t m_word = (mask1 & word);

    /* Write word with value at zeroed indices */
    uint64_t new_word = (m_word | u_val);

    return new_word;
}

