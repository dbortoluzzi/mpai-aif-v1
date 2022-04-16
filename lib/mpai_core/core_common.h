

/*
 * @file
 * @brief Common functions implementations of MPAI
 * 
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MPAI_CORE_COMMON_H
#define MPAI_CORE_COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <kernel.h>
#include <logging/log.h>
#include <pubsub/pubsub.h>

#define OK EXIT_SUCCESS
#define KO EXIT_FAILURE
/* size of stack area used by each thread */
#define STACKSIZE 1024

/********** GLOBAL STRUCT **********/
typedef enum
{ 
    MPAI_AIF_OK,
    MPAI_AIM_ALIVE, 
    MPAI_AIM_DEAD,
    MPAI_ERROR
} MPAI_RETURN_CODE;

typedef enum
{ 
    AIM_TYPE, 
    AIW_TYPE, 
    AIF_TYPE
} MPAI_COMPONENT_TYPE;

typedef struct __mpai_error {
    MPAI_RETURN_CODE code;
} mpai_error_t;

typedef struct __mpai_component {
    MPAI_COMPONENT_TYPE type;
    char* name;
} component_t;

typedef struct _mpai_parser_t 
{
	void* data;
	int64_t timestamp;
} mpai_parser_t;

typedef mpai_error_t *(module_t)();

/********** MACRO ***********/
#define MPAI_ERR_INIT(sname, ...) mpai_error_t sname __VA_OPT__(= { __VA_ARGS__ })
#define MPAI_ERR_STR(err)                            \
    (MPAI_AIF_OK       == err ? "MPAI_AIF_OK"    :                \
     (MPAI_AIM_ALIVE     == err ? "MPAI_AIM_ALIVE"   :                \
      (MPAI_AIM_DEAD   == err ? "MPAI_AIM_DEAD"  :                \
       (MPAI_ERROR == err ? "MPAI_ERROR" : "unknown"))))           

#endif