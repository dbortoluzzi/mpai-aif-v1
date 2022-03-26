#include <zephyr.h>
#include <sys/printk.h>
#include <kernel.h>

#include <drivers/gpio.h>
#include <drivers/led.h>
#include <drivers/i2c.h>
#include <drivers/spi.h>
#include <drivers/sensor.h>
#include <usb/usb_device.h>
#include <drivers/uart.h>
#include <stdio.h>
#include <logging/log.h>
#include <pubsub/pubsub.h>
#include <core_aim.h>
#include <sensors_aim.h>
#include <temp_limit_aim.h>
#include <data_mic_aim.h>
#include <message_store.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <logging/log.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include "button_svc.h"
#include "led_svc.h"
#include "flash_store.h"
#include <parson.h>

LOG_MODULE_REGISTER(MAIN, LOG_LEVEL_INF);

#define WHOAMI_REG 0x0F
#define WHOAMI_ALT_REG 0x4F

// TODO: move to Kconfig
#define PERIODIC_MODE_ENABLED false
#define SENSORS_DATA_ENABLED true
#define WRITE_TO_FLASH_ENABLED false

static int AIW_TEMP_LIMIT_DETECTION = 1;

MPAI_Component_AIM_t* aim_produce_sensors = NULL;
MPAI_Component_AIM_t* aim_temp_limit = NULL;
MPAI_Component_AIM_t* aim_data_mic = NULL;

MPAI_AIM_MessageStore_t* message_store;

#if PERIODIC_MODE_ENABLED == true
/******** START PERIODIC MODE ***********/
void aim_timer_switch_status(struct k_work *work)
{
	if (MPAI_AIM_Is_Alive(aim_produce_sensors) == true) {
		MPAI_AIM_Pause(aim_produce_sensors);
	} else {
		MPAI_AIM_Resume(aim_produce_sensors);
	}
}

K_WORK_DEFINE(my_work, aim_timer_switch_status);

void aim_timer_handler(struct k_timer *dummy)
{
    k_work_submit(&my_work);
}

K_TIMER_DEFINE(aim_timer, aim_timer_handler, NULL);
/******** END PERIODIC MODE ***********/
#endif	

/*** START BT ***/
/* Button value. */
static uint16_t but_val;

/* Prototype */
static ssize_t recv(struct bt_conn *conn,
		    const struct bt_gatt_attr *attr, const void *buf,
		    uint16_t len, uint16_t offset, uint8_t flags);

/* ST Custom Service  */
static struct bt_uuid_128 st_service_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x0000fe40, 0xcc7a, 0x482a, 0x984a, 0x7f2ed5b3e58f));

/* ST LED service */
static struct bt_uuid_128 led_char_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x0000fe41, 0x8e22, 0x4541, 0x9d4c, 0x21edae82ed19));

/* ST Notify button service */
static struct bt_uuid_128 but_notif_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x0000fe42, 0x8e22, 0x4541, 0x9d4c, 0x21edae82ed19));

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
#define ADV_LEN 12

/* Advertising data */
static uint8_t manuf_data[ADV_LEN] = {
	0x01 /*SKD version */,
	0x83 /* STM32WB - P2P Server 1 */,
	0x00 /* GROUP A Feature  */,
	0x00 /* GROUP A Feature */,
	0x00 /* GROUP B Feature */,
	0x00 /* GROUP B Feature */,
	0x00, /* BLE MAC start -MSB */
	0x00,
	0x00,
	0x00,
	0x00,
	0x00, /* BLE MAC stop */
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
	BT_DATA(BT_DATA_MANUFACTURER_DATA, manuf_data, ADV_LEN)
};

/* BLE connection */
struct bt_conn *conn;
/* Notification state */
volatile bool notify_enable;

static void mpu_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);
	notify_enable = (value == BT_GATT_CCC_NOTIFY);
	LOG_INF("Notification %s", notify_enable ? "enabled" : "disabled");
}

/* The embedded board is acting as GATT server.
 * The ST BLE Android app is the BLE GATT client.
 */

/* ST BLE Sensor GATT services and characteristic */

BT_GATT_SERVICE_DEFINE(stsensor_svc,
BT_GATT_PRIMARY_SERVICE(&st_service_uuid),
BT_GATT_CHARACTERISTIC(&led_char_uuid.uuid,
		       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
		       BT_GATT_PERM_WRITE, NULL, recv, (void *)1),
BT_GATT_CHARACTERISTIC(&but_notif_uuid.uuid, BT_GATT_CHRC_NOTIFY,
		       BT_GATT_PERM_READ, NULL, NULL, &but_val),
BT_GATT_CCC(mpu_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static ssize_t recv(struct bt_conn *conn,
		    const struct bt_gatt_attr *attr, const void *buf,
		    uint16_t len, uint16_t offset, uint8_t flags)
{
	led_update();

	return 0;
}

static void button_callback(const struct device *gpiob, struct gpio_callback *cb,
		     uint32_t pins)
{
	int err;

	LOG_INF("Button pressed");
	if (conn) {
		if (notify_enable) {
			err = bt_gatt_notify(NULL, &stsensor_svc.attrs[4],
					     &but_val, sizeof(but_val));
			if (err) {
				LOG_ERR("Notify error: %d", err);
			} else {
				LOG_INF("Send notify ok");
				but_val = (but_val == 0) ? 0x100 : 0;
			}
		} else {
			LOG_INF("Notify not enabled");
		}
	} else {
		LOG_INF("BLE not connected");
	}
}

static void bt_ready(int err)
{
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}
	LOG_INF("Bluetooth initialized");
	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return;
	}

	LOG_INF("Configuration mode: waiting connections...");
}

static void connected(struct bt_conn *connected, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection failed (err %u)", err);
	} else {
		LOG_INF("Connected");
		if (!conn) {
			conn = bt_conn_ref(connected);
		}
	}
}

static void disconnected(struct bt_conn *disconn, uint8_t reason)
{
	if (conn) {
		bt_conn_unref(conn);
		conn = NULL;
	}

	LOG_INF("Disconnected (reason %u)", reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};


/*** END BT ***/


void main(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	static const struct device *led0, *led1;
	int i, on = 1;
	int cnt = 1;
	uint32_t dtr = 0;

	// LED
	led0 = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(led0), gpios));
	gpio_pin_configure(led0, DT_GPIO_PIN(DT_ALIAS(led0), gpios),
					   GPIO_OUTPUT_ACTIVE |
						   DT_GPIO_FLAGS(DT_ALIAS(led0), gpios));

	led1 = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(led1), gpios));
	gpio_pin_configure(led1, DT_GPIO_PIN(DT_ALIAS(led1), gpios),
					   GPIO_OUTPUT_INACTIVE |
						   DT_GPIO_FLAGS(DT_ALIAS(led1), gpios));

	for (i = 0; i < 6; i++)
	{
		gpio_pin_set(led0, DT_GPIO_PIN(DT_ALIAS(led0), gpios), on);
		gpio_pin_set(led1, DT_GPIO_PIN(DT_ALIAS(led1), gpios), !on);
		k_sleep(K_MSEC(100));
		on = (on == 1) ? 0 : 1;
	}

	gpio_pin_set(led0, DT_GPIO_PIN(DT_ALIAS(led0), gpios), 0);
	gpio_pin_set(led1, DT_GPIO_PIN(DT_ALIAS(led1), gpios), 1);

	printk("IoT node INITIALIZING...\n");

	int err;

	err = button_init(button_callback);
	if (err) {
		return;
	}

	err = led_init();
	if (err) {
		return;
	}

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
	}

	#if WRITE_TO_FLASH_ENABLED == true
		/*** START SPI FLASH ***/
		const char expected[] = "{\"name\":\"Daniele\"}";
		const size_t len = sizeof(expected);
		char buf[sizeof(expected)];

		const struct device* flash_dev = init_flash();

		LOG_INF("Test 1: Flash erase\n");
		erase_flash(flash_dev);

		LOG_INF("Test 2: Flash write\n");
		int rc_write = write_flash(flash_dev, len, (void*) expected);
		if (rc_write != 0) 
		{
			return;
		}

		int rc_read = read_flash(flash_dev, len, (void*) buf);
		if (rc_read != 0)
		{
			return;
		}
		if (memcmp(expected, buf, len) == 0) 
		{
			LOG_INF("Data read matches data written. Good!!\n");
		} else 
		{
			const char* wp = expected;
			const char* rp = buf;
			const char* rpe = rp + len;

			LOG_ERR("Data read does not match data written!!\n");
			while (rp < rpe) {
				LOG_ERR("%08x wrote %02x read %02x %s\n",
					(uint32_t)(FLASH_TEST_REGION_OFFSET + (rp - buf)),
					*wp, *rp, (*rp == *wp) ? "match" : "MISMATCH");
				++rp;
				++wp;
			}
		}
		JSON_Value* json = json_parse_string(buf);
		char* name = json_object_get_string(json_object(json), "name");
		LOG_INF("Hello, %s.", log_strdup(name));
		json_value_free(json);
		/*** END SPI FLASH ***/
	#endif

	message_store = MPAI_MessageStore_Creator(AIW_TEMP_LIMIT_DETECTION, "AIF_USE_CASE", sizeof(mpai_parser_t));

	aim_data_mic = MPAI_AIM_Creator("AIM_DATA_MIC", AIW_TEMP_LIMIT_DETECTION, data_mic_aim_subscriber, data_mic_aim_start, data_mic_aim_stop, data_mic_aim_resume, data_mic_aim_pause);
	mpai_error_t err_data_mic = MPAI_AIM_Start(aim_data_mic);	

	if (err_data_mic.code != MPAI_AIF_OK)
	{
		LOG_ERR("Error starting AIM %s: %s", MPAI_AIM_Get_Component(aim_data_mic)->name, log_strdup(MPAI_ERR_STR(err_data_mic.code)));
		return;
	} 
	
	#if SENSORS_DATA_ENABLED == true
		
		aim_produce_sensors = MPAI_AIM_Creator("AIM_PRODUCE_SENSORS_DATA", AIW_TEMP_LIMIT_DETECTION, sensors_aim_subscriber, sensors_aim_start, sensors_aim_stop, sensors_aim_resume, sensors_aim_pause);
		mpai_error_t err_sens_aim = MPAI_AIM_Start(aim_produce_sensors);

		if (err_sens_aim.code != MPAI_AIF_OK) 
		{
			LOG_ERR("Error starting AIM %s: %s", MPAI_AIM_Get_Component(aim_produce_sensors)->name, log_strdup(MPAI_ERR_STR(err_sens_aim.code)));
			return;
		}

		aim_temp_limit = MPAI_AIM_Creator("AIM_TEMP_LIMIT", AIW_TEMP_LIMIT_DETECTION, temp_limit_aim_subscriber, temp_limit_aim_start, temp_limit_aim_stop, temp_limit_aim_resume, temp_limit_aim_pause);
		MPAI_MessageStore_register(message_store, MPAI_AIM_Get_Subscriber(aim_temp_limit));
		mpai_error_t err_temp_limit = MPAI_AIM_Start(aim_temp_limit);	

		if (err_temp_limit.code != MPAI_AIF_OK)
		{
			LOG_ERR("Error starting AIM %s: %s", MPAI_AIM_Get_Component(aim_temp_limit)->name, log_strdup(MPAI_ERR_STR(err_temp_limit.code)));
			return;
		} 

		LOG_INF("MPAI_AIF initialized correctly");

		#if PERIODIC_MODE_ENABLED == true
			/* start periodic timer to switch status */
			k_timer_start(&aim_timer, K_SECONDS(5), K_SECONDS(5));		
		#endif
	#endif
}
