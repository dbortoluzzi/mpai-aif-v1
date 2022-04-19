/*
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MPAI_LIBS_MOTION_COMMON_H
#define MPAI_LIBS_MOTION_COMMON_H

#include <core_common.h>

typedef enum
{ 
    UNKNOWN,
	STOPPED,
	STARTED
} MOTION_TYPE;

typedef struct _motion_data_t{
	MOTION_TYPE motion_type;
	float accel_total;

} motion_data_t;

#endif