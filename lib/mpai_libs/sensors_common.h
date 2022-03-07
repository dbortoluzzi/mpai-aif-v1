#ifndef MPAI_LIBS_SENSORS_COMMON_H
#define MPAI_LIBS_SENSORS_COMMON_H

#include <drivers/i2c.h>
#include <drivers/spi.h>
#include <drivers/sensor.h>
#include <core_common.h>

typedef struct _sensor_devices_t{
	const struct device *hts221;
} sensor_devices_t;

typedef struct _sensor_result_t{
	struct sensor_value *hts221_hum;
	struct sensor_value *hts221_temp; 
} sensor_result_t;

#endif