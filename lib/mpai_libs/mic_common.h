#ifndef MPAI_LIBS_MIC_COMMON_H
#define MPAI_LIBS_MIC_COMMON_H

#include <core_common.h>

typedef struct mic_data_t{
	int16_t* data;
	size_t* size;
} mic_data_t;

typedef struct mic_peak_t{
	int32_t* data;
	size_t* size;
} mic_peak_t;

#endif