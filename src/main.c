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
#include <parson.h>

#include "stm32l475e_iot01_audio.h"

LOG_MODULE_REGISTER(MAIN, LOG_LEVEL_INF);

#define WHOAMI_REG 0x0F
#define WHOAMI_ALT_REG 0x4F

/*** START MIC ***/
static size_t TARGET_AUDIO_BUFFER_NB_SAMPLES = AUDIO_SAMPLING_FREQUENCY * 2;
static int16_t *TARGET_AUDIO_BUFFER;
static size_t TARGET_AUDIO_BUFFER_IX = 0;

static uint16_t PCM_Buffer[PCM_BUFFER_LEN / 2];
static BSP_AUDIO_Init_t MicParams;

// we skip the first 50 events (100 ms.) to not record the button click
static size_t SKIP_FIRST_EVENTS = 50;
static size_t half_transfer_events = 0;
static size_t transfer_complete_events = 0;

// callback that gets invoked when TARGET_AUDIO_BUFFER is full
void target_audio_buffer_full() {
    // pause audio stream
    int32_t ret = BSP_AUDIO_IN_Pause(AUDIO_INSTANCE);
    if (ret != BSP_ERROR_NONE) {
        printk("Error Audio Pause (%d)\n", ret);
    }
    else {
        printk("OK Audio Pause\n");
    }
}

/**
* @brief  Half Transfer user callback, called by BSP functions.
* @param  None
* @retval None
*/
void BSP_AUDIO_IN_HalfTransfer_CallBack(uint32_t Instance) {
    half_transfer_events++;
    if (half_transfer_events < SKIP_FIRST_EVENTS) return;

    uint32_t buffer_size = PCM_BUFFER_LEN / 2; /* Half Transfer */
    uint32_t nb_samples = buffer_size / sizeof(int16_t); /* Bytes to Length */

    if ((TARGET_AUDIO_BUFFER_IX + nb_samples) > TARGET_AUDIO_BUFFER_NB_SAMPLES) {
        return;
    }
    /* Copy first half of PCM_Buffer from Microphones onto Fill_Buffer */
   memcpy(((uint8_t*)TARGET_AUDIO_BUFFER) + (TARGET_AUDIO_BUFFER_IX * 2), PCM_Buffer, buffer_size);
   TARGET_AUDIO_BUFFER_IX += nb_samples;

    if (TARGET_AUDIO_BUFFER_IX >= TARGET_AUDIO_BUFFER_NB_SAMPLES) {
        target_audio_buffer_full();
        return;
    }
}

/**
* @brief  Transfer Complete user callback, called by BSP functions.
* @param  None
* @retval None
*/
void BSP_AUDIO_IN_TransferComplete_CallBack(uint32_t Instance) {
    transfer_complete_events++;
    if (transfer_complete_events < SKIP_FIRST_EVENTS) return;

    uint32_t buffer_size = PCM_BUFFER_LEN / 2; /* Half Transfer */
    uint32_t nb_samples = buffer_size / sizeof(int16_t); /* Bytes to Length */

    if ((TARGET_AUDIO_BUFFER_IX + nb_samples) > TARGET_AUDIO_BUFFER_NB_SAMPLES) {
        return;
    }

    /* Copy second half of PCM_Buffer from Microphones onto Fill_Buffer */
   memcpy((uint8_t*)TARGET_AUDIO_BUFFER + (TARGET_AUDIO_BUFFER_IX * 2),
       ((uint8_t*)PCM_Buffer) + (nb_samples * 2), buffer_size);
    TARGET_AUDIO_BUFFER_IX += nb_samples;

    if (TARGET_AUDIO_BUFFER_IX >= TARGET_AUDIO_BUFFER_NB_SAMPLES) {
		target_audio_buffer_full();
        return;
    }
}

/**
  * @brief  Manages the BSP audio in error event.
  * @param  Instance Audio in instance.
  * @retval None.
  */
void BSP_AUDIO_IN_Error_CallBack(uint32_t Instance) {
    printk("BSP_AUDIO_IN_Error_CallBack\n");
}

void print_stats() {
    printk("Half %lu, Complete %lu, IX %lu\n", half_transfer_events, transfer_complete_events,
        TARGET_AUDIO_BUFFER_IX);
}

void start_recording() {
    int32_t ret;
    uint32_t state;

    ret = BSP_AUDIO_IN_GetState(AUDIO_INSTANCE, &state);
    if (ret != BSP_ERROR_NONE) {
        printk("Cannot start recording: Error getting audio state (%d)\n", ret);
        return;
    }
    if (state == AUDIO_IN_STATE_RECORDING) {
        printk("Cannot start recording: Already recording\n");
        return;
    }

    // reset audio buffer location
    TARGET_AUDIO_BUFFER_IX = 0;
    transfer_complete_events = 0;
    half_transfer_events = 0;

    ret = BSP_AUDIO_IN_Record(AUDIO_INSTANCE, (uint8_t *) PCM_Buffer, PCM_BUFFER_LEN);
    if (ret != BSP_ERROR_NONE) {
        printk("Error Audio Record (%ld)\n", ret);
        return;
    }
    else {
        printk("OK Audio Record\n");
    }
}

int myaudiomain() {

static size_t TARGET_AUDIO_BUFFER_IX = 0;


printk("Hello from the B-L475E-IOT01A microphone \n");


	// static uint8_t *buf = TARGET_AUDIO_BUFFER;
   TARGET_AUDIO_BUFFER = (int16_t*)k_calloc(TARGET_AUDIO_BUFFER_NB_SAMPLES, sizeof(int16_t));

   if (!TARGET_AUDIO_BUFFER) {
       printk("Failed to allocate TARGET_AUDIO_BUFFER buffer\n");
       return 0;
   }

    // set up the microphone
    MicParams.BitsPerSample = AUDIO_RESOLUTION_16b; // AUDIO_RESOLUTION_16b
    MicParams.ChannelsNbr = AUDIO_CHANNELS;
    MicParams.Device = AUDIO_IN_DIGITAL_MIC1; 
    MicParams.SampleRate =  AUDIO_SAMPLING_FREQUENCY;
    MicParams.Volume = AUDIO_VOLUME_VALUE;

    int32_t ret = BSP_AUDIO_IN_Init(AUDIO_INSTANCE, &MicParams);

    if (ret != BSP_ERROR_NONE) {
        printk("Error Audio Init (%ld)\r\n", ret);
        return 1;
    } else {
        printk("OK Audio Init\t(Audio Freq=%ld)\r\n", AUDIO_SAMPLING_FREQUENCY);
    }

	start_recording();

}

void print_wav() 
{
	// create WAV file
    size_t wavFreq = AUDIO_SAMPLING_FREQUENCY;
    size_t dataSize = (TARGET_AUDIO_BUFFER_NB_SAMPLES * 2);
    size_t fileSize = 44 + (TARGET_AUDIO_BUFFER_NB_SAMPLES * 2);

    uint8_t wav_header[44] = {
        0x52, 0x49, 0x46, 0x46, // RIFF
        fileSize & 0xff, (fileSize >> 8) & 0xff, (fileSize >> 16) & 0xff, (fileSize >> 24) & 0xff,
        0x57, 0x41, 0x56, 0x45, // WAVE
        0x66, 0x6d, 0x74, 0x20, // fmt
        0x10, 0x00, 0x00, 0x00, // length of format data
        0x01, 0x00, // type of format (1=PCM)
        0x01, 0x00, // number of channels
        wavFreq & 0xff, (wavFreq >> 8) & 0xff, (wavFreq >> 16) & 0xff, (wavFreq >> 24) & 0xff,
        0x00, 0x7d, 0x00, 0x00, // 	(Sample Rate * BitsPerSample * Channels) / 8
        0x02, 0x00, 0x10, 0x00,
        0x64, 0x61, 0x74, 0x61, // data
        dataSize & 0xff, (dataSize >> 8) & 0xff, (dataSize >> 16) & 0xff, (dataSize >> 24) & 0xff,
    };

    printk("Total complete events: %lu, index is %lu\n", transfer_complete_events, TARGET_AUDIO_BUFFER_IX);

    // print both the WAV header and the audio buffer in HEX format to serial
    // you can use the script in `hex-to-buffer.js` to make a proper WAV file again
    printk("WAV file:\n");
    for (size_t ix = 0; ix < 44; ix++) {
        printk("%02x", wav_header[ix]);
    }

	for (size_t iy=0; iy<(TARGET_AUDIO_BUFFER_IX*2)/64; iy++) {
		char logstring[64*2] = {};
		for (size_t ix = 0; ix < 64; ix++) {
			sprintf(logstring+2*ix, "%02x", ((uint8_t *)TARGET_AUDIO_BUFFER)[64*iy+ix]);
		}
		printk(logstring);
		k_sleep(K_MSEC(50));
	}
    printk("\n");
}
/*** END MIC ***/

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

	/* Start microphone recording */
	myaudiomain();	
	
	k_sleep(K_MSEC(5000));

	print_wav();
	/* End microphone recording */

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
}
