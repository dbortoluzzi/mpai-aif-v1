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
} motion_data_t;

#endif