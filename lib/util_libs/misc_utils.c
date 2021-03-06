/*
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "misc_utils.h"

struct pair
{
    struct pair   *point;        /* Pointers forming list linked in sorted order */
    int32_t  value;             /* Values to sort */
};


void median_filter(int32_t datum, int32_t *calc_median, int32_t *calc_peak)
{
    static struct pair buffer[MEDIAN_FILTER_SIZE] = {0}; /* Buffer of nwidth pairs */
    static struct pair *datpoint = buffer;   /* Pointer into circular buffer of data */
    static struct pair small = {NULL, STOPPER};          /* Chain stopper */
    static struct pair big = {&small, 0}; /* Pointer to head (largest) of linked list.*/
    
    struct pair *successor;         /* Pointer to successor of replaced data item */
    struct pair *scan;              /* Pointer used to scan down the sorted list */
    struct pair *scanold;           /* Previous value of scan */
    struct pair *median;            /* Pointer to median */
    
    uint16_t i;
    
    if (datum == STOPPER)
    {
        datum = STOPPER + 1;                             /* No stoppers allowed. */
    }
    
    if ( (++datpoint - buffer) >= MEDIAN_FILTER_SIZE)
    {
        datpoint = buffer;                  /* Increment and wrap data in pointer.*/
    }
    
    datpoint->value = datum;                /* Copy in new datum */
    successor = datpoint->point;            /* Save pointer to old value's successor */
    median = &big;                          /* Median initially to first in chain */
    scanold = NULL;                         /* Scanold initially null. */
    scan = &big;              /* Points to pointer to first (largest) datum in chain */
    
    /* Handle chain-out of first item in chain as special case */
    if (scan->point == datpoint)
    {
        scan->point = successor;
    }
    scanold = scan;                                     /* Save this pointer and   */
    scan = scan->point ;                                /* step down chain */
    
    /* Loop through the chain, normal loop exit via break. */
    for (i = 0 ; i < MEDIAN_FILTER_SIZE; ++i)
    {
        /* Handle odd-numbered item in chain  */
        if (scan->point == datpoint)
        {
            scan->point = successor;                      /* Chain out the old datum.*/
        }
        
        if (scan->value < datum)         /* If datum is larger than scanned value,*/
        {
            datpoint->point = scanold->point;             /* Chain it in here.  */
            scanold->point = datpoint;                    /* Mark it chained in. */
            datum = STOPPER;
        };
        
        /* Step median pointer down chain after doing odd-numbered element */
        median = median->point;                       /* Step median pointer.  */
        if (scan == &small)
        {
            break;                                      /* Break at end of chain  */
        }
        scanold = scan;                               /* Save this pointer and   */
        scan = scan->point;                           /* step down chain */
        
        /* Handle even-numbered item in chain.  */
        if (scan->point == datpoint)
        {
            scan->point = successor;
        }
        
        if (scan->value < datum)
        {
            datpoint->point = scanold->point;
            scanold->point = datpoint;
            datum = STOPPER;
        }
        
        if (scan == &small)
        {
            break;
        }
        
        scanold = scan;
        scan = scan->point;
    }
    *calc_median = median->value;
    *calc_peak = big.point->value;
}

char * append_strings(const char * old, const char * new)
{
    // find the size of the string to allocate
    const size_t old_len = strlen(old), new_len = strlen(new);
    const size_t out_len = old_len + new_len + 1;

    // allocate a pointer to the new string
    char *out = k_malloc(out_len);

    // concat both strings and return
    memcpy(out, old, old_len);
    memcpy(out + old_len, new, new_len + 1);

    return out;
}