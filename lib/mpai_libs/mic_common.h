/*
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MPAI_LIBS_MIC_COMMON_H
#define MPAI_LIBS_MIC_COMMON_H

#include <core_common.h>

typedef struct mic_data_t{
	int16_t* data;
	size_t* size;
} mic_data_t;

typedef struct mic_peak_t{
	int32_t* data;
} mic_peak_t;

#endif