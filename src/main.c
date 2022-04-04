#include <zephyr.h>
#include <sys/printk.h>
#include <kernel.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(MAIN, LOG_LEVEL_INF);

#include <drivers/gpio.h>
#include <drivers/led.h>
#include <drivers/i2c.h>
#include <drivers/spi.h>
#include <drivers/sensor.h>
#include <usb/usb_device.h>
#include <drivers/uart.h>
#include <stdio.h>
#include <pubsub/pubsub.h>
#include <core_aim.h>
#include <sensors_aim.h>
#include <temp_limit_aim.h>
#include <data_mic_aim.h>
#include <motion_aim.h>
#include <rehabilitation_aim.h>
#include <message_store.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <logging/log.h>

#include <errno.h>
#include <sys/byteorder.h>

#include <net/socket.h>
#include <net/net_mgmt.h>
#include <net/net_ip.h>
#include <net/udp.h>
#include <net/coap.h>

#include <net_private.h>

#define PEER_PORT 5683
#define MAX_COAP_MSG_LEN 256

// TODO: move to Kconfig
#define PERIODIC_MODE_ENABLED false
#define SENSORS_DATA_ENABLED true
#define MIC_DATA_ENABLED true
#define TEMP_LIMIT_ENABLED false
#define MOTION_ENABLED true
#define REHABILITATION_ENABLED true
#define WRITE_TO_FLASH_ENABLED false

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include "button_svc.h"
#include "led_svc.h"
#ifdef WRITE_TO_FLASH_ENABLED == true
	#include "flash_store.h"
#endif
#include <parson.h>

#include <test_use_case_aiw.h>

#define WHOAMI_REG 0x0F
#define WHOAMI_ALT_REG 0x4F


MPAI_Component_AIM_t* aim_produce_sensors = NULL;
MPAI_Component_AIM_t* aim_temp_limit = NULL;
MPAI_Component_AIM_t* aim_data_mic = NULL;
MPAI_Component_AIM_t* aim_data_motion = NULL;
MPAI_Component_AIM_t* aim_rehabilitation = NULL;

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
static ssize_t bt_recv(struct bt_conn *conn,
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
		       BT_GATT_PERM_WRITE, NULL, bt_recv, (void *)1),
BT_GATT_CHARACTERISTIC(&but_notif_uuid.uuid, BT_GATT_CHRC_NOTIFY,
		       BT_GATT_PERM_READ, NULL, NULL, &but_val),
BT_GATT_CCC(mpu_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static ssize_t bt_recv(struct bt_conn *conn,
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


// /*** START COAP ***/


/* CoAP socket fd */
static int sock;

struct pollfd fds[1];
static int nfds;

/* CoAP Options */
static const char * const test_path[] = { "test", NULL };

static const char * const large_path[] = { "large", NULL };

static const char * const obs_path[] = { "obs", NULL };

#define BLOCK_WISE_TRANSFER_SIZE_GET 2048

static struct coap_block_context blk_ctx;

static void wait(void)
{
	if (poll(fds, nfds, -1) < 0) {
		LOG_ERR("Error in poll:%d", errno);
	}
}

static void prepare_fds(void)
{
	fds[nfds].fd = sock;
	fds[nfds].events = POLLIN;
	nfds++;
}

static int start_coap_client(void)
{
	int ret = 0;
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PEER_PORT);

	inet_pton(AF_INET, CONFIG_NET_CONFIG_PEER_IPV4_ADDR,
		  &addr.sin_addr);

	sock = socket(addr.sin_family, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create UDP socket %d", errno);
		return -errno;
	}

	ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		LOG_ERR("Cannot connect to UDP remote : %d", errno);
		return -errno;
	}

	prepare_fds();

	return 0;
}

static int process_simple_coap_reply(void)
{
	struct coap_packet reply;
	uint8_t *data;
	int rcvd;
	int ret;

	wait();

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	rcvd = recv(sock, data, MAX_COAP_MSG_LEN, MSG_DONTWAIT);
	if (rcvd == 0) {
		ret = -EIO;
		goto end;
	}

	if (rcvd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			ret = 0;
		} else {
			ret = -errno;
		}

		goto end;
	}

	net_hexdump("Response", data, rcvd);

	ret = coap_packet_parse(&reply, data, rcvd, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Invalid data received");
	}

end:
	k_free(data);

	return ret;
}

static int send_simple_coap_request(uint8_t method)
{
	uint8_t payload[] = "payload";
	struct coap_packet request;
	const char * const *p;
	uint8_t *data;
	int r;

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, COAP_TYPE_CON,
			     COAP_TOKEN_MAX_LEN, coap_next_token(),
			     method, coap_next_id());
	if (r < 0) {
		LOG_ERR("Failed to init CoAP message");
		goto end;
	}

	for (p = test_path; p && *p; p++) {
		r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
					      *p, strlen(*p));
		if (r < 0) {
			LOG_ERR("Unable add option to request");
			goto end;
		}
	}

	switch (method) {
	case COAP_METHOD_GET:
	case COAP_METHOD_DELETE:
		break;

	case COAP_METHOD_PUT:
	case COAP_METHOD_POST:
		r = coap_packet_append_payload_marker(&request);
		if (r < 0) {
			LOG_ERR("Unable to append payload marker");
			goto end;
		}

		r = coap_packet_append_payload(&request, (uint8_t *)payload,
					       sizeof(payload) - 1);
		if (r < 0) {
			LOG_ERR("Not able to append payload");
			goto end;
		}

		break;
	default:
		r = -EINVAL;
		goto end;
	}

	net_hexdump("Request", request.data, request.offset);

	r = send(sock, request.data, request.offset, 0);

end:
	k_free(data);

	return 0;
}

static int send_simple_coap_msgs_and_wait_for_reply(void)
{
	uint8_t test_type = 0U;
	int r;

	while (1) {
		switch (test_type) {
		case 0:
			/* Test CoAP GET method */
			printk("\nCoAP client GET\n");
			r = send_simple_coap_request(COAP_METHOD_GET);
			if (r < 0) {
				return r;
			}

			break;
		case 1:
			/* Test CoAP PUT method */
			printk("\nCoAP client PUT\n");
			r = send_simple_coap_request(COAP_METHOD_PUT);
			if (r < 0) {
				return r;
			}

			break;
		case 2:
			/* Test CoAP POST method*/
			printk("\nCoAP client POST\n");
			r = send_simple_coap_request(COAP_METHOD_POST);
			if (r < 0) {
				return r;
			}

			break;
		case 3:
			/* Test CoAP DELETE method*/
			printk("\nCoAP client DELETE\n");
			r = send_simple_coap_request(COAP_METHOD_DELETE);
			if (r < 0) {
				return r;
			}

			break;
		default:
			return 0;
		}

		r = process_simple_coap_reply();
		if (r < 0) {
			return r;
		}

		test_type++;
	}

	return 0;
}

static int process_large_coap_reply(void)
{
	struct coap_packet reply;
	uint8_t *data;
	bool last_block;
	int rcvd;
	int ret;

	wait();

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	rcvd = recv(sock, data, MAX_COAP_MSG_LEN, MSG_DONTWAIT);
	if (rcvd == 0) {
		ret = -EIO;
		goto end;
	}

	if (rcvd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			ret = 0;
		} else {
			ret = -errno;
		}

		goto end;
	}

	net_hexdump("Response", data, rcvd);

	ret = coap_packet_parse(&reply, data, rcvd, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Invalid data received");
		goto end;
	}

	ret = coap_update_from_block(&reply, &blk_ctx);
	if (ret < 0) {
		goto end;
	}

	last_block = coap_next_block(&reply, &blk_ctx);
	if (!last_block) {
		ret = 1;
		goto end;
	}

	ret = 0;

end:
	k_free(data);

	return ret;
}

static int send_large_coap_request(void)
{
	struct coap_packet request;
	const char * const *p;
	uint8_t *data;
	int r;

	if (blk_ctx.total_size == 0) {
		coap_block_transfer_init(&blk_ctx, COAP_BLOCK_64,
					 BLOCK_WISE_TRANSFER_SIZE_GET);
	}

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, COAP_TYPE_CON,
			     COAP_TOKEN_MAX_LEN, coap_next_token(),
			     COAP_METHOD_GET, coap_next_id());
	if (r < 0) {
		LOG_ERR("Failed to init CoAP message");
		goto end;
	}

	for (p = large_path; p && *p; p++) {
		r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
					      *p, strlen(*p));
		if (r < 0) {
			LOG_ERR("Unable add option to request");
			goto end;
		}
	}

	r = coap_append_block2_option(&request, &blk_ctx);
	if (r < 0) {
		LOG_ERR("Unable to add block2 option.");
		goto end;
	}

	net_hexdump("Request", request.data, request.offset);

	r = send(sock, request.data, request.offset, 0);

end:
	k_free(data);

	return r;
}

static int get_large_coap_msgs(void)
{
	int r;

	while (1) {
		/* Test CoAP Large GET method */
		printk("\nCoAP client Large GET (block %zd)\n",
		       blk_ctx.current / 64 /*COAP_BLOCK_64*/);
		r = send_large_coap_request();
		if (r < 0) {
			return r;
		}

		r = process_large_coap_reply();
		if (r < 0) {
			return r;
		}

		/* Received last block */
		if (r == 1) {
			memset(&blk_ctx, 0, sizeof(blk_ctx));
			return 0;
		}
	}

	return 0;
}

static int send_obs_reply_ack(uint16_t id, uint8_t *token, uint8_t tkl)
{
	struct coap_packet request;
	uint8_t *data;
	int r;

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, COAP_TYPE_ACK, tkl, token, 0, id);
	if (r < 0) {
		LOG_ERR("Failed to init CoAP message");
		goto end;
	}

	net_hexdump("Request", request.data, request.offset);

	r = send(sock, request.data, request.offset, 0);
end:
	k_free(data);

	return r;
}

static int process_obs_coap_reply(void)
{
	struct coap_packet reply;
	uint16_t id;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t *data;
	uint8_t type;
	uint8_t tkl;
	int rcvd;
	int ret;

	wait();

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	rcvd = recv(sock, data, MAX_COAP_MSG_LEN, MSG_DONTWAIT);
	if (rcvd == 0) {
		ret = -EIO;
		goto end;
	}

	if (rcvd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			ret = 0;
		} else {
			ret = -errno;
		}

		goto end;
	}

	net_hexdump("Response", data, rcvd);

	ret = coap_packet_parse(&reply, data, rcvd, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Invalid data received");
		goto end;
	}

	tkl = coap_header_get_token(&reply, token);
	id = coap_header_get_id(&reply);

	type = coap_header_get_type(&reply);
	if (type == COAP_TYPE_ACK) {
		ret = 0;
	} else if (type == COAP_TYPE_CON) {
		ret = send_obs_reply_ack(id, token, tkl);
	}
end:
	k_free(data);

	return ret;
}

static int send_obs_coap_request(void)
{
	struct coap_packet request;
	const char * const *p;
	uint8_t *data;
	int r;

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, COAP_TYPE_CON,
			     COAP_TOKEN_MAX_LEN, coap_next_token(),
			     COAP_METHOD_GET, coap_next_id());
	if (r < 0) {
		LOG_ERR("Failed to init CoAP message");
		goto end;
	}

	r = coap_append_option_int(&request, COAP_OPTION_OBSERVE, 0);
	if (r < 0) {
		LOG_ERR("Failed to append Observe option");
		goto end;
	}

	for (p = obs_path; p && *p; p++) {
		r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
					      *p, strlen(*p));
		if (r < 0) {
			LOG_ERR("Unable add option to request");
			goto end;
		}
	}

	net_hexdump("Request", request.data, request.offset);

	r = send(sock, request.data, request.offset, 0);

end:
	k_free(data);

	return r;
}

static int send_obs_reset_coap_request(void)
{
	struct coap_packet request;
	const char * const *p;
	uint8_t *data;
	int r;

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, COAP_TYPE_RESET,
			     COAP_TOKEN_MAX_LEN, coap_next_token(),
			     0, coap_next_id());
	if (r < 0) {
		LOG_ERR("Failed to init CoAP message");
		goto end;
	}

	r = coap_append_option_int(&request, COAP_OPTION_OBSERVE, 0);
	if (r < 0) {
		LOG_ERR("Failed to append Observe option");
		goto end;
	}

	for (p = obs_path; p && *p; p++) {
		r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
					      *p, strlen(*p));
		if (r < 0) {
			LOG_ERR("Unable add option to request");
			goto end;
		}
	}

	net_hexdump("Request", request.data, request.offset);

	r = send(sock, request.data, request.offset, 0);

end:
	k_free(data);

	return r;
}

static int register_observer(void)
{
	uint8_t counter = 0U;
	int r;

	while (1) {
		/* Test CoAP OBS GET method */
		if (!counter) {
			printk("\nCoAP client OBS GET\n");
			r = send_obs_coap_request();
			if (r < 0) {
				return r;
			}
		} else {
			printk("\nCoAP OBS Notification\n");
		}

		r = process_obs_coap_reply();
		if (r < 0) {
			return r;
		}

		counter++;

		/* Unregister */
		if (counter == 5U) {
			/* TODO: Functionality can be verified byt waiting for
			 * some time and make sure client shouldn't receive
			 * any notifications. If client still receives
			 * notifications means, Observer is not removed.
			 */
			return send_obs_reset_coap_request();
		}
	}

	return 0;
}

// /*** END COAP ***/

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

	/*** START COAP ***/
	int r;

	LOG_DBG("Start CoAP-client sample");
	r = start_coap_client();
	if (r < 0) {
		(void)close(sock);
	}

	/* GET, PUT, POST, DELETE */
	r = send_simple_coap_msgs_and_wait_for_reply();
	if (r < 0) {
		(void)close(sock);
	}

	/* Block-wise transfer */
	r = get_large_coap_msgs();
	if (r < 0) {
		(void)close(sock);
	}

	/* Register observer, get notifications and unregister */
	r = register_observer();
	if (r < 0) {
		(void)close(sock);
	}

	/* Close the socket */
	(void)close(sock);

	LOG_DBG("Done");

	/*** END COAP ***/


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

	INIT_Test_Use_Case_AIW();

	#if MIC_DATA_ENABLED == true
		aim_data_mic = MPAI_AIM_Creator("AIM_DATA_MIC", AIW_USE_CASE_ID, data_mic_aim_subscriber, data_mic_aim_start, data_mic_aim_stop, data_mic_aim_resume, data_mic_aim_pause);
		mpai_error_t err_data_mic = MPAI_AIM_Start(aim_data_mic);	

		if (err_data_mic.code != MPAI_AIF_OK)
		{
			LOG_ERR("Error starting AIM %s: %s", MPAI_AIM_Get_Component(aim_data_mic)->name, log_strdup(MPAI_ERR_STR(err_data_mic.code)));
			return;
		} 
	#endif
	
	#if SENSORS_DATA_ENABLED == true
		
		aim_produce_sensors = MPAI_AIM_Creator("AIM_PRODUCE_SENSORS_DATA", AIW_USE_CASE_ID, sensors_aim_subscriber, sensors_aim_start, sensors_aim_stop, sensors_aim_resume, sensors_aim_pause);
		mpai_error_t err_sens_aim = MPAI_AIM_Start(aim_produce_sensors);

		if (err_sens_aim.code != MPAI_AIF_OK) 
		{
			LOG_ERR("Error starting AIM %s: %s", MPAI_AIM_Get_Component(aim_produce_sensors)->name, log_strdup(MPAI_ERR_STR(err_sens_aim.code)));
			return;
		}

		#if TEMP_LIMIT_ENABLED == true
			aim_temp_limit = MPAI_AIM_Creator("AIM_TEMP_LIMIT", AIW_USE_CASE_ID, temp_limit_aim_subscriber, temp_limit_aim_start, temp_limit_aim_stop, temp_limit_aim_resume, temp_limit_aim_pause);
			MPAI_MessageStore_register(message_store_test_case_aiw, MPAI_AIM_Get_Subscriber(aim_temp_limit), SENSORS_DATA_CHANNEL);
			mpai_error_t err_temp_limit = MPAI_AIM_Start(aim_temp_limit);	

			if (err_temp_limit.code != MPAI_AIF_OK)
			{
				LOG_ERR("Error starting AIM %s: %s", MPAI_AIM_Get_Component(aim_temp_limit)->name, log_strdup(MPAI_ERR_STR(err_temp_limit.code)));
				return;
			} 
		#endif

		#if MOTION_ENABLED == true
			aim_data_motion = MPAI_AIM_Creator("AIM_MOTION", AIW_USE_CASE_ID, motion_aim_subscriber, motion_aim_start, motion_aim_stop, motion_aim_resume, motion_aim_pause);
			MPAI_MessageStore_register(message_store_test_case_aiw, MPAI_AIM_Get_Subscriber(aim_data_motion), SENSORS_DATA_CHANNEL);
			mpai_error_t err_motion = MPAI_AIM_Start(aim_data_motion);	

			if (err_motion.code != MPAI_AIF_OK)
			{
				LOG_ERR("Error starting AIM %s: %s", MPAI_AIM_Get_Component(aim_data_motion)->name, log_strdup(MPAI_ERR_STR(err_motion.code)));
				return;
			} 
		#endif

		#if REHABILITATION_ENABLED == true
			aim_rehabilitation = MPAI_AIM_Creator("AIM_REHABILITATION", AIW_USE_CASE_ID, rehabilitation_aim_subscriber, rehabilitation_aim_start, rehabilitation_aim_stop, rehabilitation_aim_resume, rehabilitation_aim_pause);
			MPAI_MessageStore_register(message_store_test_case_aiw, MPAI_AIM_Get_Subscriber(aim_rehabilitation), MOTION_DATA_CHANNEL);
			MPAI_MessageStore_register(message_store_test_case_aiw, MPAI_AIM_Get_Subscriber(aim_rehabilitation), MIC_PEAK_DATA_CHANNEL);
			mpai_error_t err_rehabilitation = MPAI_AIM_Start(aim_rehabilitation);	

			if (err_rehabilitation.code != MPAI_AIF_OK)
			{
				LOG_ERR("Error starting AIM %s: %s", MPAI_AIM_Get_Component(aim_rehabilitation)->name, log_strdup(MPAI_ERR_STR(err_rehabilitation.code)));
				return;
			} 
		#endif

		LOG_INF("MPAI_AIF initialized correctly");

		#if PERIODIC_MODE_ENABLED == true
			/* start periodic timer to switch status */
			k_timer_start(&aim_timer, K_SECONDS(5), K_SECONDS(5));		
		#endif
	#endif
}
