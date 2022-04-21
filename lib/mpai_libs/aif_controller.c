/*
 * @file
 * @brief Implementation of a MPAI AIF Controller, according to the specs V1
 * 
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "aif_controller.h"

LOG_MODULE_REGISTER(MPAI_LIBS_AIF_CONTROLLER, LOG_LEVEL_INF);

#include <wifi_connect.h>
#include <net_private.h>

static int aiw_id;

#ifdef CONFIG_MPAI_AIM_CONTROL_UNIT_SENSORS_PERIODIC

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

/*** START COAP ***/
#ifdef CONFIG_COAP_SERVER
	const char * const test_path[] = { "test", NULL };

	const char * const large_path[] = { "large", NULL };

	const char * const obs_path[] = { "obs", NULL };

	static int send_simple_coap_msgs_and_wait_for_reply(uint8_t * data_result, const char * const * simple_path)
	{
		uint8_t test_type = 0U;
		int r;

		while (1) {
			switch (test_type) {
			case 0:
				/* Test CoAP GET method */
				printk("\nCoAP client GET\n");
				r = send_simple_coap_request(COAP_METHOD_GET, simple_path);
				if (r < 0) {
					return r;
				}

				break;
			case 1:
				/* Test CoAP PUT method */
				printk("\nCoAP client PUT\n");
				r = send_simple_coap_request(COAP_METHOD_PUT, simple_path);
				if (r < 0) {
					return r;
				}

				break;
			case 2:
				/* Test CoAP POST method*/
				printk("\nCoAP client POST\n");
				r = send_simple_coap_request(COAP_METHOD_POST, simple_path);
				if (r < 0) {
					return r;
				}

				break;
			case 3:
				/* Test CoAP DELETE method*/
				printk("\nCoAP client DELETE\n");
				r = send_simple_coap_request(COAP_METHOD_DELETE, simple_path);
				if (r < 0) {
					return r;
				}

				break;
			default:
				return 0;
			}

			memset(data_result, 0, MAX_COAP_MSG_LEN);
			r = process_simple_coap_reply(data_result);
			if (r < 0) {
				return r;
			}
			test_type++;
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

		r = send(get_coap_sock(), request.data, request.offset, 0);
	end:
		k_free(data);

		return r;
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

		r = send(get_coap_sock(), request.data, request.offset, 0);

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

		r = send(get_coap_sock(), request.data, request.offset, 0);

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
	//*** END COAP ***/
#endif

mpai_error_t MPAI_AIFU_Controller_Initialize()
{

	#ifdef CONFIG_APP_TEST_WRITE_TO_FLASH
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
		const JSON_Object* root_json = json_object(json);
		const char* name = json_object_get_string(root_json, "name");
		LOG_INF("Hello, %s.", log_strdup(name));
		json_value_free(json);
		/*** END SPI FLASH ***/
	#endif

	wifi_connect();

	#ifdef CONFIG_COAP_SERVER
		/*** START COAP ***/
		int r;

		LOG_DBG("Start CoAP-client sample");
		r = start_coap_client();
		if (r < 0) {
			(void)close(get_coap_sock());
		}

		// // /* GET, PUT, POST, DELETE */
		// uint8_t* data_result = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN * sizeof(uint8_t));
		// r = send_simple_coap_msgs_and_wait_for_reply(data_result, test_path);
		// k_free(data_result);

		// /* Block-wise transfer */
		// char* data_large_result = get_large_coap_msgs(large_path);
		// k_free(data_large_result);

		/* Register observer, get notifications and unregister */
		// r = register_observer();
		// if (r < 0) {
		// 	(void)close(get_coap_sock());
		// }
	#endif

	#if defined(CONFIG_MPAI_CONFIG_STORE) && defined (CONFIG_MPAI_CONFIG_STORE_USES_COAP)
		char* aif_result = MPAI_Config_Store_Get_AIF("demo");
		if (aif_result != NULL) {
			// printk("AIF RESULT: \n");
			// for ( size_t i = 0; i < strlen(aif_result); i++ )
			// {
			// 	printk("%c", (char)aif_result[i]);
			// 	k_sleep(K_MSEC(5));
			// }
			// printk("\n");

			JSON_Value* json_aif = json_parse_string(aif_result);
			char* aif_name = json_object_get_string(json_object(json_aif), "title");
			LOG_INF("Initializing AIF with title \"%s\"...", log_strdup(aif_name));

			k_free(aif_result);
		}

	#endif

	mpai_error_t err_aiw = MPAI_AIFU_AIW_Start(MPAI_LIBS_CAE_REV_AIW_NAME, &aiw_id);
	if (err_aiw.code != MPAI_AIF_OK)
	{
		LOG_ERR("Error starting AIW %s: %s", MPAI_LIBS_CAE_REV_AIW_NAME, log_strdup(MPAI_ERR_STR(err_aiw.code)));
		return;
	} 

	LOG_INF("MPAI_AIF initialized correctly");

	#if defined(CONFIG_MPAI_CONFIG_STORE) && defined (CONFIG_MPAI_CONFIG_STORE_USES_COAP)
		/* Close the socket when it's no longer usefull*/
		(void)close(get_coap_sock());
	#endif

	// k_sleep(K_SECONDS(2));

	// MPAI_AIFU_Controller_Destroy();

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return err;
}

mpai_error_t MPAI_AIFU_Controller_Destroy() 
{
	DESTROY_MPAI_AIW_CAE_REV();
	
	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return err;
}

mpai_error_t MPAI_AIFU_AIW_Start(const char* name, int* AIW_ID)
{
	LOG_INF("Starting AIW %s...", log_strdup(name));
	
	// At the moment, we handle only AIW CAE-REV 
	if (strcmp(name, MPAI_LIBS_CAE_REV_AIW_NAME) == 0)
	{
		int aiw_id = MPAI_AIW_CAE_REV_Init();
		*AIW_ID = aiw_id;

		MPAI_AIW_CAE_REV_Start();

		MPAI_ERR_INIT(err, MPAI_AIF_OK);
		return err;
	}

	MPAI_ERR_INIT(err, MPAI_ERROR);
	return err;
}

mpai_error_t MPAI_AIFU_AIW_Pause(int AIW_ID)
{
	LOG_INF("Pausing AIW %d...", AIW_ID);
	
	// At the moment, we handle only AIW CAE-REV 
	if (AIW_ID == AIW_CAE_REV)
	{
		MPAI_AIW_CAE_REV_Pause();
		
		MPAI_ERR_INIT(err, MPAI_AIF_OK);
		return err;
	}

	MPAI_ERR_INIT(err, MPAI_ERROR);
	return err;
}

mpai_error_t MPAI_AIFU_AIW_Resume(int AIW_ID)
{
	LOG_INF("Resuming AIW %d...", AIW_ID);
	
	// At the moment, we handle only AIW CAE-REV 
	if (AIW_ID == AIW_CAE_REV)
	{
		MPAI_AIW_CAE_REV_Resume();
		
		MPAI_ERR_INIT(err, MPAI_AIF_OK);
		return err;
	}

	MPAI_ERR_INIT(err, MPAI_ERROR);
	return err;
}

mpai_error_t MPAI_AIFU_AIW_Stop(int AIW_ID)
{
	LOG_INF("Stopping AIW %d...", AIW_ID);
	
	// At the moment, we handle only AIW CAE-REV 
	if (AIW_ID == AIW_CAE_REV)
	{
		MPAI_AIW_CAE_REV_Stop();
		
		MPAI_ERR_INIT(err, MPAI_AIF_OK);
		return err;
	}

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return err;
}

// TODO: generalize the implementation. At the moment, we handle only AIW CAE-REV.
// maybe using "MPAI_AIM_List" of aiw_cae_rev.c
mpai_error_t MPAI_AIFU_AIM_GetStatus(int AIW_ID, const char* name, int* status)
{
	MPAI_Component_AIM_t* aim_to_check;
	if (AIW_ID == AIW_CAE_REV)
	{	
		if (strcmp(name, MPAI_LIBS_CAE_REV_AIM_DATA_MIC_NAME) == 0)
		{
			aim_to_check = aim_data_mic;
		} else if (strcmp(name, MPAI_LIBS_CAE_REV_AIM_SENSORS_NAME) == 0)
		{
			aim_to_check = aim_produce_sensors;
		} else if (strcmp(name, MPAI_LIBS_CAE_REV_AIM_TEMP_LIMIT_NAME) == 0)
		{
			aim_to_check = aim_temp_limit;
		} else if (strcmp(name, MPAI_LIBS_CAE_REV_AIM_MOTION_NAME) == 0)
		{
			aim_to_check = aim_data_motion;
		} else if (strcmp(name, MPAI_LIBS_CAE_REV_AIM_REHABILITATION_NAME) == 0)
		{
			aim_to_check = aim_rehabilitation;
		}
	}
	if (MPAI_AIM_Is_Alive(aim_to_check))
	{
		*status = MPAI_AIM_ALIVE;
	} else 
	{
		*status = MPAI_AIM_DEAD;
	}

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return err;
}

mpai_error_t MPAI_AIFU_AIM_Start(int aiw_id, aim_initialization_cb_t aim_init)
{
	MPAI_Component_AIM_t* aim = MPAI_AIM_Creator(aim_init._aim_name, aiw_id, aim_init._subscriber, aim_init._start, aim_init._stop, aim_init._resume, aim_init._pause);
	bool post_cb_result = aim_init._post_cb(aim);

	if (post_cb_result) 
	{
		mpai_error_t err_aim = MPAI_AIM_Start(aim);	

		if (err_aim.code != MPAI_AIF_OK)
		{
			LOG_ERR("Error starting AIM %s: %s", log_strdup(MPAI_AIM_Get_Component(aim)->name), log_strdup(MPAI_ERR_STR(err_aim.code)));
		} 
		return err_aim;
	}
	else 
	{
		LOG_WRN("Skipped creation AIM %s", log_strdup(MPAI_AIM_Get_Component(aim)->name));
		MPAI_ERR_INIT(err, MPAI_AIM_CREATION_SKIPPED);
		return err;
	}
}