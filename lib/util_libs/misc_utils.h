/*
 * Copyright (c) 2022 Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MPAI_LIBS_SENSORS_AIM_H
#define MPAI_LIBS_SENSORS_AIM_H

#include <stdlib.h>
#include <stdio.h>

#define STOPPER 0                                /* Smaller than any datum */
#define MEDIAN_FILTER_SIZE    (20)

char * append_strings(const char * old, const char * new);

#endif