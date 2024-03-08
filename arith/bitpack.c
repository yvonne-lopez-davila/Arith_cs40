
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
//function contract- Bitpack_fitsu
bool Bitpack_fitsu(uint64_t n, unsigned width)
{
    assert(width <= MAX_WIDTH);

    uint64_t u_bound;

    /* Set inclusive upper bound of n values that fit */
    if (width == MAX_WIDTH) {
        /* Handle shifts by 64 */
        u_bound = (U1 << (width - 1)); /* (2^63) */
        u_bound = (u_bound * 2) - 1;
    } 
    else {
        u_bound = ((U1 << width) - U1); /* (2^n) - 1, inclusive */
    }    

    fprintf(stderr, "ubound: %lu \n", u_bound);

    return (n <= u_bound);
}

/*
A) Signed ints
range: -2^(n-1) to (2^(n-1) - 1)
(ex: for 8 bits, -128 to 127) // passed
*/
// todo this works for 64... but prev does not
//function contract- Bitpack_fitss
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

    fprintf(stderr, "ubound, lbound: %ld, %ld\n", pos_bound, neg_bound);

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
//function contract- Bitpack_getu
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
//function contract- Bitpack_gets
int64_t Bitpack_gets(uint64_t word, unsigned width, unsigned lsb)
{
    assert(width <= 64);
    assert((width + lsb) <= 64);
    
    /* Create mask with ones at indices of word and zeroes elsewhere */
    uint64_t mask = ~0; /* 64-bit binary with all ones */
    mask = mask >> (64 - width) << lsb;

    /* Extract word using mask (shifted to ones place) */
    int64_t subword = (mask & word) >> lsb;

    return subword;
}

//function contract- Bitpack_newu
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

//function contract- Bitpack_news
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

