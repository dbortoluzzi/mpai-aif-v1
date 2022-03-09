#ifndef MPAI_LIBS_SENSORS_COMMON_H
#define MPAI_LIBS_SENSORS_COMMON_H

#include <drivers/i2c.h>
#include <drivers/spi.h>
#include <drivers/sensor.h>
#include <core_common.h>

typedef struct _sensor_devices_t{
	#ifdef CONFIG_HTS221
		const struct device *hts221;
	#endif
	#ifdef CONFIG_LPS22HB
		const struct device *lps22hb;
	#endif
	#ifdef CONFIG_LIS2DW12
		const struct device *lis2dw12;
	#endif
	#ifdef CONFIG_LPS22HH
		const struct device *lps22hh;
	#endif
	#ifdef CONFIG_LSM6DSO
		const struct device *lsm6dso;
	#endif
	#ifdef CONFIG_LSM6DSL
		const struct device *lsm6dsl;
	#endif
	#ifdef CONFIG_STTS751
		const struct device *stts751;
	#endif
	#ifdef CONFIG_IIS3DHHC
		const struct device *iis3dhhc;
	#endif
	#ifdef CONFIG_LIS2MDL
		const struct device *lis2mdl;
	#endif
	#ifdef CONFIG_LIS3MDL
		const struct device *lis3mdl;
	#endif
} sensor_devices_t;

typedef struct _sensor_result_t{
	#ifdef CONFIG_HTS221
		struct sensor_value* hts221_hum;
		struct sensor_value* hts221_temp; 
	#endif
	#ifdef CONFIG_LPS22HH
		struct sensor_value* lps22hh_press;
		struct sensor_value* lps22hh_temp;
	#endif
	#ifdef CONFIG_LPS22HB
		struct sensor_value* lps22hb_press;
		struct sensor_value* lps22hb_temp;
	#endif
	#ifdef CONFIG_LIS2DW12
		struct sensor_value lis2dw12_accel[3];
	#endif
	#ifdef CONFIG_IIS3DHHC
		struct sensor_value iis3dhhc_accel[3];
	#endif
	#ifdef CONFIG_LSM6DSO
		struct sensor_value lsm6dso_accel[3];
		struct sensor_value lsm6dso_gyro[3];
	#endif
	#ifdef CONFIG_LSM6DSL
		struct sensor_value lsm6dsl_accel[3];
		struct sensor_value lsm6dsl_gyro[3];
	#endif
	#ifdef CONFIG_STTS751
		struct sensor_value* stts751_temp;
	#endif
	#ifdef CONFIG_LIS2MDL
		struct sensor_value lis2mdl_magn[3];
	#endif
	#ifdef CONFIG_LIS3MDL
		struct sensor_value lis3mdl_magn[3];
	#endif
} sensor_result_t;

#endif