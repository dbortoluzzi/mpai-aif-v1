/*
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sensors_aim.h"

LOG_MODULE_REGISTER(MPAI_LIBS_SENSORS_AIM, LOG_LEVEL_INF);

/*************** DEFINE ***************/

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

/* delay between reads from sensors (in ms) */
#define CONFIG_SENSORS_RATE_MS 100

/*************** STATIC ***************/
// INITIALIZE STRUCT
#ifdef CONFIG_HTS221
	struct sensor_value hts221_hum = {0,0};
	struct sensor_value hts221_temp = {0,0};
#endif

#ifdef CONFIG_LPS22HH
	struct sensor_value lps22hh_press = {0,0};
	struct sensor_value lps22hh_temp = {0,0};
#endif
#ifdef CONFIG_LPS22HB
	struct sensor_value lps22hb_press = {0,0};
	struct sensor_value lps22hb_temp = {0,0};
#endif
#ifdef CONFIG_STTS751
	struct sensor_value stts751_temp = {0,0};
#endif

static sensor_result_t sensor_result = {
	// TODO: at the moment, the first sensor is mandatory
	#ifdef CONFIG_HTS221
		.hts221_hum = &hts221_hum,
		.hts221_temp = &hts221_temp
	#endif
	#ifdef CONFIG_LPS22HH
		,
		.lps22hh_press = &lps22hh_press,
		.lps22hh_temp = &lps22hh_temp
	#endif	
	#ifdef CONFIG_LPS22HB
		,
		.lps22hb_press = &lps22hb_press,
		.lps22hb_temp = &lps22hb_temp
	#endif		
	#ifdef CONFIG_LIS2DW12
		,
		.lis2dw12_accel = {{0,0}, {0,0}, {0,0}}
	#endif
	#ifdef CONFIG_IIS3DHHC
		,
		.iis3dhhc_accel = {{0,0}, {0,0}, {0,0}}
	#endif
	#ifdef CONFIG_LSM6DSO
		,
		.lsm6dso_accel = {{0,0}, {0,0}, {0,0}},
		.lsm6dso_gyro = {{0,0}, {0,0}, {0,0}}
	#endif
	#ifdef CONFIG_LSM6DSL
		,
		.lsm6dsl_accel = {{0,0}, {0,0}, {0,0}},
		.lsm6dsl_gyro = {{0,0}, {0,0}, {0,0}}
	#endif
	#ifdef CONFIG_STTS751
		,
		.stts751_temp = &stts751_temp
	#endif
	#ifdef CONFIG_LIS2MDL
		,
		.lis2mdl_magn = {{0,0}, {0,0}, {0,0}}
	#endif
	#ifdef CONFIG_LIS3MDL
		,
		.lis3mdl_magn = {{0,0}, {0,0}, {0,0}}
	#endif
};

#ifdef CONFIG_LPS22HH_TRIGGER
static int lps22hh_trig_cnt;

static void lps22hh_trigger_handler(const struct device *dev,
				    const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_PRESS);
	lps22hh_trig_cnt++;
}
#endif

#ifdef CONFIG_LIS2DW12_TRIGGER
static int lis2dw12_trig_cnt;

static void lis2dw12_trigger_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	lis2dw12_trig_cnt++;
}
#endif

#ifdef CONFIG_LSM6DSO_TRIGGER
static int lsm6dso_acc_trig_cnt;
static int lsm6dso_gyr_trig_cnt;
static int lsm6dso_temp_trig_cnt;

static void lsm6dso_acc_trig_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	lsm6dso_acc_trig_cnt++;
}

static void lsm6dso_gyr_trig_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_GYRO_XYZ);
	lsm6dso_gyr_trig_cnt++;
}

static void lsm6dso_temp_trig_handler(const struct device *dev,
				      const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_DIE_TEMP);
	lsm6dso_temp_trig_cnt++;
}
#endif

#ifdef CONFIG_LSM6DSL_TRIGGER
static int lsm6dsl_trig_cnt;
static void lsm6dsl_trigger_handler(const struct device *dev,
				    struct sensor_trigger *trig)
{
	static struct sensor_value accel_x, accel_y, accel_z;
	static struct sensor_value gyro_x, gyro_y, gyro_z;
	lsm6dsl_trig_cnt++;

	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_X, &accel_x);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Y, &accel_y);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Z, &accel_z);

	/* lsm6dsl gyro */
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_GYRO_XYZ);
	sensor_channel_get(dev, SENSOR_CHAN_GYRO_X, &gyro_x);
	sensor_channel_get(dev, SENSOR_CHAN_GYRO_Y, &gyro_y);
	sensor_channel_get(dev, SENSOR_CHAN_GYRO_Z, &gyro_z);
}
#endif

#ifdef CONFIG_STTS751_TRIGGER
static int stts751_trig_cnt;

static void stts751_trigger_handler(const struct device *dev,
				    const struct sensor_trigger *trig)
{
	stts751_trig_cnt++;
}
#endif

#ifdef CONFIG_IIS3DHHC_TRIGGER
static int iis3dhhc_trig_cnt;

static void iis3dhhc_trigger_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	iis3dhhc_trig_cnt++;
}
#endif

static void lps22hh_config(const struct device *lps22hh)
{
	struct sensor_value odr_attr;

	/* set LPS22HH sampling frequency to 50 Hz */
	odr_attr.val1 = 50;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lps22hh, SENSOR_CHAN_ALL,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LPS22HH\n");
		return;
	}

#ifdef CONFIG_LPS22HH_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ALL;
	sensor_trigger_set(lps22hh, &trig, lps22hh_trigger_handler);
#endif
}

static void lps22hb_config(const struct device *lps22hb)
{
	struct sensor_value odr_attr;

	/* set LPS22HB sampling frequency to 50 Hz */
	odr_attr.val1 = 50;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lps22hb, SENSOR_CHAN_ALL,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LPS22HB\n");
		return;
	}
}

static void lis2dw12_config(const struct device *lis2dw12)
{
	struct sensor_value odr_attr, fs_attr;

	/* set LIS2DW12 accel/gyro sampling frequency to 100 Hz */
	odr_attr.val1 = 100;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lis2dw12, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LIS2DW12 accel\n");
		return;
	}

	sensor_g_to_ms2(16, &fs_attr);

	if (sensor_attr_set(lis2dw12, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set sampling frequency for LIS2DW12 gyro\n");
		return;
	}

#ifdef CONFIG_LIS2DW12_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(lis2dw12, &trig, lis2dw12_trigger_handler);
#endif
}

static void lsm6dso_config(const struct device *lsm6dso)
{
	struct sensor_value odr_attr, fs_attr;

	/* set LSM6DSO accel sampling frequency to 208 Hz */
	odr_attr.val1 = 208;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lsm6dso, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LSM6DSO accel\n");
		return;
	}

	sensor_g_to_ms2(16, &fs_attr);

	if (sensor_attr_set(lsm6dso, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set fs for LSM6DSO accel\n");
		return;
	}

	/* set LSM6DSO gyro sampling frequency to 208 Hz */
	odr_attr.val1 = 208;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lsm6dso, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for LSM6DSO gyro\n");
		return;
	}

	sensor_degrees_to_rad(250, &fs_attr);

	if (sensor_attr_set(lsm6dso, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		printk("Cannot set fs for LSM6DSO gyro\n");
		return;
	}

#ifdef CONFIG_LSM6DSO_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(lsm6dso, &trig, lsm6dso_acc_trig_handler);

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_GYRO_XYZ;
	sensor_trigger_set(lsm6dso, &trig, lsm6dso_gyr_trig_handler);

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_DIE_TEMP;
	sensor_trigger_set(lsm6dso, &trig, lsm6dso_temp_trig_handler);
#endif
}

static void lsm6dsl_config(const struct device *lsm6dsl)
{
	struct sensor_value odr_attr;

	/* set accel/gyro sampling frequency to 104 Hz */
	odr_attr.val1 = 104;
	odr_attr.val2 = 0;

	if (sensor_attr_set(lsm6dsl, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for accelerometer.\n");
		return;
	}

	if (sensor_attr_set(lsm6dsl, SENSOR_CHAN_GYRO_XYZ,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for gyro.\n");
		return;
	}

	#ifdef CONFIG_LSM6DSL_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;

	if (sensor_trigger_set(lsm6dsl, &trig, lsm6dsl_trigger_handler) != 0) {
		printk("Could not set sensor type and channel\n");
		return;
	}
	#endif
}

static void stts751_config(const struct device *stts751)
{
	struct sensor_value odr_attr;

	/* set STTS751 conversion rate to 16 Hz */
	odr_attr.val1 = 16;
	odr_attr.val2 = 0;

	if (sensor_attr_set(stts751, SENSOR_CHAN_ALL,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for STTS751\n");
		return;
	}

#ifdef CONFIG_STTS751_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_THRESHOLD;
	trig.chan = SENSOR_CHAN_ALL;
	sensor_trigger_set(stts751, &trig, stts751_trigger_handler);
#endif
}

static void iis3dhhc_config(const struct device *iis3dhhc)
{
	struct sensor_value odr_attr;

	/* enable IIS3DHHC conversion */
	odr_attr.val1 = 1000; /* enable sensor at 1KHz */
	odr_attr.val2 = 0;

	if (sensor_attr_set(iis3dhhc, SENSOR_CHAN_ALL,
			    SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
		printk("Cannot set sampling frequency for IIS3DHHC\n");
		return;
	}

#ifdef CONFIG_IIS3DHHC_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(iis3dhhc, &trig, iis3dhhc_trigger_handler);
#endif
}

/**************** THREADS **********************/

static k_tid_t producer_mic_thread_id;

K_THREAD_STACK_DEFINE(thread_prod_stack_area, STACKSIZE);
static struct k_thread thread_prod_mic_data;

/* PRODUCER */
void produce_sensors_data(void *arg1, void *arg2) {

	sensor_result_t *sensor_result_ptr = (sensor_result_t*) arg1;
	sensor_devices_t *sensor_devices_ptr = (sensor_devices_t*) arg2;

	LOG_DBG("Producing......\n\n");

	/* handle HTS221 sensor */
	#ifdef CONFIG_HTS221
		if (sensor_sample_fetch(sensor_devices_ptr->hts221) < 0) {
			LOG_ERR("HTS221 Sensor sample update error\n");
			return;
		}
	#endif

	#ifdef CONFIG_LIS2DW12
		/* handle LIS2DW12 sensor */
		if (sensor_sample_fetch(sensor_devices_ptr->lis2dw12) < 0) {
			LOG_ERR("LIS2DW12 Sensor sample update error\n");
			return;
		}
	#endif

	#ifdef CONFIG_LSM6DSO
		if (sensor_sample_fetch(sensor_devices_ptr->lsm6dso) < 0) {
			LOG_ERR("LSM6DSO Sensor sample update error\n");
			return;
		}
	#endif

	#ifdef CONFIG_LSM6DSO
		if (sensor_sample_fetch(sensor_devices_ptr->lsm6dsl) < 0) {
			LOG_ERR("LSM6DSL Sensor sample update error\n");
			return;
		}
	#endif

	#ifdef CONFIG_LPS22HH
		if (sensor_sample_fetch(sensor_devices_ptr->lps22hh) < 0) {
			LOG_ERR("LPS22HH Sensor sample update error\n");
			return;
		}
	#endif

	#ifdef CONFIG_LPS22HB
		if (sensor_sample_fetch(sensor_devices_ptr->lps22hb) < 0) {
			LOG_ERR("LPS22HB Sensor sample update error\n");
			return;
		}
	#endif

	#ifdef CONFIG_STTS751
		if (sensor_sample_fetch(sensor_devices_ptr->stts751) < 0) {
			LOG_ERR("STTS751 Sensor sample update error\n");
			return;
		}
	#endif

	#ifdef CONFIG_IIS3DHHC
		if (sensor_sample_fetch(sensor_devices_ptr->iis3dhhc) < 0) {
			LOG_ERR("IIS3DHHC Sensor sample update error\n");
			return;
		}
	#endif

	#ifdef CONFIG_LIS2MDL
		if (sensor_sample_fetch(sensor_devices_ptr->lis2mdl) < 0) {
			LOG_ERR("LIS2MDL Sensor sample update error\n");
			return;
		}
	#endif

	#ifdef CONFIG_LIS3MDL
		if (sensor_sample_fetch(sensor_devices_ptr->lis3mdl) < 0) {
			LOG_ERR("LIS3MDL Sensor sample update error\n");
			return;
		}
	#endif

	// GET SENSORS DATA
	#ifdef CONFIG_HTS221
		sensor_channel_get(sensor_devices_ptr->hts221, SENSOR_CHAN_HUMIDITY, sensor_result_ptr->hts221_hum);
		sensor_channel_get(sensor_devices_ptr->hts221, SENSOR_CHAN_AMBIENT_TEMP, sensor_result_ptr->hts221_temp);
	#endif
	#ifdef CONFIG_LPS22HH
		sensor_channel_get(sensor_devices_ptr->lps22hh, SENSOR_CHAN_AMBIENT_TEMP, sensor_result_ptr->lps22hh_temp);
		sensor_channel_get(sensor_devices_ptr->lps22hh, SENSOR_CHAN_PRESS, sensor_result_ptr->lps22hh_press);
	#endif	
	#ifdef CONFIG_LPS22HB
		sensor_channel_get(sensor_devices_ptr->lps22hb, SENSOR_CHAN_PRESS, sensor_result_ptr->lps22hb_press);
		sensor_channel_get(sensor_devices_ptr->lps22hb, SENSOR_CHAN_AMBIENT_TEMP, sensor_result_ptr->lps22hb_temp);
	#endif		
	#ifdef CONFIG_LIS2DW12
		struct sensor_value lis2dw12_accel[3];
		sensor_channel_get(sensor_devices_ptr->lis2dw12, SENSOR_CHAN_ACCEL_XYZ, lis2dw12_accel);
		memcpy(sensor_result_ptr->lis2dw12_accel, lis2dw12_accel, sizeof(lis2dw12_accel));
	#endif
	#ifdef CONFIG_LSM6DSO
		struct sensor_value lsm6dso_accel[3], lsm6dso_gyro[3];
		sensor_channel_get(sensor_devices_ptr->lsm6dso, SENSOR_CHAN_ACCEL_XYZ, lsm6dso_accel);
		sensor_channel_get(sensor_devices_ptr->lsm6dso, SENSOR_CHAN_GYRO_XYZ, lsm6dso_gyro);

		memcpy(sensor_result_ptr->lsm6dso_accel, lsm6dso_accel, sizeof(lsm6dso_accel));
		memcpy(sensor_result_ptr->lsm6dso_gyro, lsm6dso_gyro, sizeof(lsm6dso_gyro));
	#endif
	#ifdef CONFIG_LSM6DSL
		struct sensor_value lsm6dsl_accel[3], lsm6dsl_gyro[3];
		sensor_channel_get(sensor_devices_ptr->lsm6dsl, SENSOR_CHAN_ACCEL_XYZ, lsm6dsl_accel);
		sensor_channel_get(sensor_devices_ptr->lsm6dsl, SENSOR_CHAN_GYRO_XYZ, lsm6dsl_gyro);

		memcpy(sensor_result_ptr->lsm6dsl_accel, lsm6dsl_accel, sizeof(lsm6dsl_accel));
		memcpy(sensor_result_ptr->lsm6dsl_gyro, lsm6dsl_gyro, sizeof(lsm6dsl_gyro));
	#endif
	#ifdef CONFIG_STTS751
		sensor_channel_get(sensor_devices_ptr->stts751, SENSOR_CHAN_AMBIENT_TEMP, sensor_result_ptr->stts751_temp);
	#endif
	#ifdef CONFIG_IIS3DHHC
		struct sensor_value iis3dhhc_accel[3];
		sensor_channel_get(sensor_devices_ptr->iis3dhhc, SENSOR_CHAN_ACCEL_XYZ, iis3dhhc_accel);
		memcpy(sensor_result_ptr->iis3dhhc_accel, iis3dhhc_accel, sizeof(iis3dhhc_accel));
	#endif
	#ifdef CONFIG_LIS2MDL
		struct sensor_value lis2mdl_magn[3];
		sensor_channel_get(sensor_devices_ptr->lis2mdl, SENSOR_CHAN_MAGN_XYZ, lis2mdl_magn);
		memcpy(sensor_result_ptr->lis2mdl_magn, lis2mdl_magn, sizeof(lis2mdl_magn));
	#endif
	#ifdef CONFIG_LIS3MDL
		struct sensor_value lis3mdl_magn[3];
		sensor_channel_get(sensor_devices_ptr->lis3mdl, SENSOR_CHAN_MAGN_XYZ, lis3mdl_magn);
		memcpy(sensor_result_ptr->lis3mdl_magn, lis3mdl_magn, sizeof(lis3mdl_magn));
	#endif

	// Publish sensor message 
	mpai_parser_t msg = {
		.data = sensor_result_ptr,
		.timestamp = k_uptime_get()
	};

	MPAI_MessageStore_publish(message_store_sensors_aim, &msg, SENSORS_DATA_CHANNEL);
}

void th_produce_sensors_data(void *arg1, void *dummy2, void *dummy3)
{

	sensor_result_t *sensor_result_ptr = (sensor_result_t*) arg1;

	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	// GET SENSORS
	sensor_devices_t sensor_devices = {
		#ifdef CONFIG_HTS221
		.	hts221 = device_get_binding(DT_LABEL(DT_INST(0, st_hts221)))
		#endif
		#ifdef CONFIG_LIS2DW12
			,
			.lis2dw12 = device_get_binding(DT_LABEL(DT_INST(0, st_lis2dw12)))
		#endif
		#ifdef CONFIG_LPS22HH
			,
			.lps22hh = device_get_binding(DT_LABEL(DT_INST(0, st_lps22hh)))
		#endif
		#ifdef CONFIG_LPS22HB
			,
			.lps22hb = device_get_binding(DT_LABEL(DT_INST(0, st_lps22hb_press)))
		#endif
		#ifdef CONFIG_LSM6DSO
			,
			.lsm6dso = device_get_binding(DT_LABEL(DT_INST(0, st_lsm6dso)))
		#endif
		#ifdef CONFIG_LSM6DSL
			,
			.lsm6dsl = device_get_binding(DT_LABEL(DT_INST(0, st_lsm6dsl)))
		#endif
		#ifdef CONFIG_STTS751
			,
			.stts751 = device_get_binding(DT_LABEL(DT_INST(0, st_stts751)))
		#endif
		#ifdef CONFIG_IIS3DHHC
			,
			.iis3dhhc = device_get_binding(DT_LABEL(DT_INST(0, st_iis3dhhc)))
		#endif
		#ifdef CONFIG_LIS2MDL
			,
			.lis2mdl = device_get_binding(DT_LABEL(DT_INST(0, st_lis2mdl)))
		#endif
		#ifdef CONFIG_LIS3MDL
			,
			.lis3mdl = device_get_binding(DT_LABEL(DT_INST(0, st_lis3mdl_magn)))
		#endif
	};
	const sensor_devices_t *sensor_devices_ptr = &sensor_devices;

	// CHECK AND CONFIG SENSORS
	#ifdef CONFIG_HTS221
		if (!sensor_devices_ptr->hts221) {
			LOG_ERR("Could not get pointer to %s sensor\n",
				DT_LABEL(DT_INST(0, st_hts221)));
			return;
		}
	#endif

	#ifdef CONFIG_LIS2DW12
		if (!sensor_devices_ptr->lis2dw12) {
			LOG_ERR("Could not get LIS2DW12 device\n");
			return;
		}
		lis2dw12_config(sensor_devices_ptr->lis2dw12);
	#endif

	#ifdef CONFIG_LPS22HH
		if (sensor_devices_ptr->lps22hh == NULL) {
			LOG_ERR("Could not get LPS22HH device\n");
			return;
		}
		lps22hh_config(sensor_devices_ptr->lps22hh);
	#endif

	#ifdef CONFIG_LPS22HB
		if (sensor_devices_ptr->lps22hb == NULL) {
			LOG_ERR("Could not get LPS22HB device\n");
			return;
		}
	#endif

	#ifdef CONFIG_LSM6DSO
		if (sensor_devices_ptr->lsm6dso == NULL) {
			LOG_ERR("Could not get LSM6DSO device\n");
			return;
		}
		lsm6dso_config(sensor_devices_ptr->lsm6dso);
	#endif

	#ifdef CONFIG_LSM6DSL
		if (sensor_devices_ptr->lsm6dsl == NULL) {
			LOG_ERR("Could not get LSM6DSL device\n");
			return;
		}
		lsm6dsl_config(sensor_devices_ptr->lsm6dsl);
	#endif

	#ifdef CONFIG_STTS751
		if (sensor_devices_ptr->stts751 == NULL) {
			LOG_ERR("Could not get STTS751 device\n");
			return;
		}
		stts751_config(sensor_devices_ptr->stts751);
	#endif

	#ifdef CONFIG_IIS3DHHC
		if (sensor_devices_ptr->iis3dhhc == NULL) {
			LOG_ERR("Could not get IIS3DHHC device\n");
			return;
		}
		iis3dhhc_config(sensor_devices_ptr->iis3dhhc);
	#endif

	#ifdef CONFIG_LIS2MDL
		if (sensor_devices_ptr->lis2mdl == NULL) {
			LOG_ERR("Could not get LIS2MDL device\n");
			return;
		}
	#endif
	#ifdef CONFIG_LIS3MDL
		if (sensor_devices_ptr->lis3mdl == NULL) {
			LOG_ERR("Could not get LIS3MDL device\n");
			return;
		}
	#endif

	while(1) {

		produce_sensors_data((void*) sensor_result_ptr, (void*) sensor_devices_ptr);
		k_sleep(K_MSEC(CONFIG_SENSORS_RATE_MS));
		
	}
}

/************** EXECUTIONS ***************/
mpai_error_t* sensors_aim_subscriber()
{
	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t* sensors_aim_start()
{
	// CREATE PRODUCER
	producer_mic_thread_id = k_thread_create(&thread_prod_mic_data, thread_prod_stack_area,
			K_THREAD_STACK_SIZEOF(thread_prod_stack_area),
			th_produce_sensors_data, (void*) &sensor_result, NULL, NULL,
			PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&thread_prod_mic_data, "thread_prod");
	
	// START THREAD
	k_thread_start(producer_mic_thread_id);

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t* sensors_aim_stop() 
{
	k_thread_abort(producer_mic_thread_id);
	LOG_INF("Execution stopped");

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t* sensors_aim_resume() 
{
	k_thread_resume(producer_mic_thread_id);
	LOG_INF("Execution resumed");

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}

mpai_error_t* sensors_aim_pause() 
{
	k_thread_suspend(producer_mic_thread_id);
	LOG_INF("Execution paused");

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return &err;
}