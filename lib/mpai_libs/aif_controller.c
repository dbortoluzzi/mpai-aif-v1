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

/************* STATIC HEADER *************/
static int aiw_id;

/************* PRIVATE HEADER *************/
channel_map_element_t _linear_search_channel(const char *name);
aim_initialization_cb_t *_linear_search_aim_init(const char *name);

/* AIM initialization List */
aim_initialization_cb_t *MPAI_AIM_List[MPAI_AIF_AIM_MAX] = {};
/* Channel List*/
channel_map_element_t message_store_channel_list[MPAI_AIF_CHANNEL_MAX] = {};
/* Counters */
int mpai_controller_aim_count = 0;
int mpai_message_store_channel_count = 0;

#ifdef CONFIG_MPAI_AIM_CONTROL_UNIT_SENSORS_PERIODIC

/******** START PERIODIC MODE ***********/
void aim_timer_switch_status(struct k_work *work)
{
	if (MPAI_AIM_Is_Alive(aim_produce_sensors) == true)
	{
		MPAI_AIM_Pause(aim_produce_sensors);
	}
	else
	{
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
const char *const test_path[] = {"test", NULL};

const char *const large_path[] = {"large", NULL};

const char *const obs_path[] = {"obs", NULL};

static int send_simple_coap_msgs_and_wait_for_reply(uint8_t *data_result, const char *const *simple_path)
{
	uint8_t test_type = 0U;
	int r;

	while (1)
	{
		switch (test_type)
		{
		case 0:
			/* Test CoAP GET method */
			printk("\nCoAP client GET\n");
			r = send_simple_coap_request(COAP_METHOD_GET, simple_path);
			if (r < 0)
			{
				return r;
			}

			break;
		case 1:
			/* Test CoAP PUT method */
			printk("\nCoAP client PUT\n");
			r = send_simple_coap_request(COAP_METHOD_PUT, simple_path);
			if (r < 0)
			{
				return r;
			}

			break;
		case 2:
			/* Test CoAP POST method*/
			printk("\nCoAP client POST\n");
			r = send_simple_coap_request(COAP_METHOD_POST, simple_path);
			if (r < 0)
			{
				return r;
			}

			break;
		case 3:
			/* Test CoAP DELETE method*/
			printk("\nCoAP client DELETE\n");
			r = send_simple_coap_request(COAP_METHOD_DELETE, simple_path);
			if (r < 0)
			{
				return r;
			}

			break;
		default:
			return 0;
		}

		memset(data_result, 0, MAX_COAP_MSG_LEN);
		r = process_simple_coap_reply(data_result);
		if (r < 0)
		{
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
	if (!data)
	{
		return -ENOMEM;
	}

	r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN,
						 COAP_VERSION_1, COAP_TYPE_ACK, tkl, token, 0, id);
	if (r < 0)
	{
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
	const char *const *p;
	uint8_t *data;
	int r;

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data)
	{
		return -ENOMEM;
	}

	r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN,
						 COAP_VERSION_1, COAP_TYPE_CON,
						 COAP_TOKEN_MAX_LEN, coap_next_token(),
						 COAP_METHOD_GET, coap_next_id());
	if (r < 0)
	{
		LOG_ERR("Failed to init CoAP message");
		goto end;
	}

	r = coap_append_option_int(&request, COAP_OPTION_OBSERVE, 0);
	if (r < 0)
	{
		LOG_ERR("Failed to append Observe option");
		goto end;
	}

	for (p = obs_path; p && *p; p++)
	{
		r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
									  *p, strlen(*p));
		if (r < 0)
		{
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
	const char *const *p;
	uint8_t *data;
	int r;

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data)
	{
		return -ENOMEM;
	}

	r = coap_packet_init(&request, data, MAX_COAP_MSG_LEN,
						 COAP_VERSION_1, COAP_TYPE_RESET,
						 COAP_TOKEN_MAX_LEN, coap_next_token(),
						 0, coap_next_id());
	if (r < 0)
	{
		LOG_ERR("Failed to init CoAP message");
		goto end;
	}

	r = coap_append_option_int(&request, COAP_OPTION_OBSERVE, 0);
	if (r < 0)
	{
		LOG_ERR("Failed to append Observe option");
		goto end;
	}

	for (p = obs_path; p && *p; p++)
	{
		r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
									  *p, strlen(*p));
		if (r < 0)
		{
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

	while (1)
	{
		/* Test CoAP OBS GET method */
		if (!counter)
		{
			printk("\nCoAP client OBS GET\n");
			r = send_obs_coap_request();
			if (r < 0)
			{
				return r;
			}
		}
		else
		{
			printk("\nCoAP OBS Notification\n");
		}

		r = process_obs_coap_reply();
		if (r < 0)
		{
			return r;
		}

		counter++;

		/* Unregister */
		if (counter == 5U)
		{
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

	const struct device *flash_dev = init_flash();

	LOG_INF("Test 1: Flash erase\n");
	erase_flash(flash_dev);

	LOG_INF("Test 2: Flash write\n");
	int rc_write = write_flash(flash_dev, len, (void *)expected);
	if (rc_write != 0)
	{
		return;
	}

	int rc_read = read_flash(flash_dev, len, (void *)buf);
	if (rc_read != 0)
	{
		return;
	}
	if (memcmp(expected, buf, len) == 0)
	{
		LOG_INF("Data read matches data written. Good!!\n");
	}
	else
	{
		const char *wp = expected;
		const char *rp = buf;
		const char *rpe = rp + len;

		LOG_ERR("Data read does not match data written!!\n");
		while (rp < rpe)
		{
			LOG_ERR("%08x wrote %02x read %02x %s\n",
					(uint32_t)(FLASH_TEST_REGION_OFFSET + (rp - buf)),
					*wp, *rp, (*rp == *wp) ? "match" : "MISMATCH");
			++rp;
			++wp;
		}
	}
	LOG_INF("Hello, %s.", log_strdup(buf));
	/*** END SPI FLASH ***/
#endif

	wifi_connect();

#ifdef CONFIG_COAP_SERVER
	/*** START COAP ***/
	int r;

	LOG_DBG("Start CoAP-client sample");
	r = start_coap_client();
	if (r < 0)
	{
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

#if defined(CONFIG_MPAI_CONFIG_STORE) && defined(CONFIG_MPAI_CONFIG_STORE_USES_COAP)
	char *aif_result = MPAI_Config_Store_Get_AIF("demo");
	if (aif_result != NULL)
	{
		// printk("AIF RESULT: \n");
		// for ( size_t i = 0; i < strlen(aif_result); i++ )
		// {
		// 	printk("%c", (char)aif_result[i]);
		// 	k_sleep(K_MSEC(5));
		// }
		// printk("\n");

		JSON_Value *json_aif = json_parse_string(aif_result);
		char *aif_name = json_object_get_string(json_object(json_aif), "title");
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

#if defined(CONFIG_MPAI_CONFIG_STORE) && defined(CONFIG_MPAI_CONFIG_STORE_USES_COAP)
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

	memset(MPAI_AIM_List, 0, MPAI_AIF_AIM_MAX * sizeof(aim_initialization_cb_t *));

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return err;
}

#if defined(CONFIG_MPAI_CONFIG_STORE) && defined(CONFIG_MPAI_CONFIG_STORE_USES_COAP)
mpai_error_t MPAI_AIFU_AIW_Start_From_MPAI_Store(const char *name)
{
	char *aiw_result = MPAI_Config_Store_Get_AIW(name);
	// printk("AIW RESULT: \n");
	// for ( size_t i = 0; i < strlen(aiw_result); i++ )
	// {
	// 	printk("%c", (char)aiw_result[i]);
	// 	k_sleep(K_MSEC(5));
	// }
	// printk("\n");

	bool aif_ok = true;
	cJSON *root = cJSON_Parse(aiw_result);
	if (root != NULL)
	{
		// read aiw
		cJSON *aiw_name_cjson = cJSON_GetObjectItem(root, "title");
		if (aiw_name_cjson != NULL)
		{
			bool aiw_init_ok = true;
			char *aiw_name = aiw_name_cjson->valuestring;
			LOG_INF("Initializing AIW with title \"%s\"...", log_strdup(aiw_name));

			// read aiw topology
			cJSON *aiw_topology_cjson = cJSON_GetObjectItem(root, "Topology");
			if (aiw_topology_cjson != NULL)
			{
				if (cJSON_Array == aiw_topology_cjson->type)
				{
					int aiw_topology_el_count = cJSON_GetArraySize(aiw_topology_cjson);
					// read each topology element topology
					for (int idx = 0; idx < aiw_topology_el_count; idx++)
					{
						cJSON *aiw_topology_el_cjson = cJSON_GetArrayItem(aiw_topology_cjson, idx);
						// read input channel by aim (the json describe input channel match to output channel)
						cJSON *aiw_topology_output_cjson = cJSON_GetObjectItem(aiw_topology_el_cjson, "Output");
						if (aiw_topology_output_cjson != NULL)
						{

							cJSON *aiw_output_aim_name_cjson = cJSON_GetObjectItem(aiw_topology_output_cjson, "AIMName");
							cJSON *aiw_output_channel_cjson = cJSON_GetObjectItem(aiw_topology_output_cjson, "PortName");

							// search channel in config
							channel_map_element_t channel_map_element = _linear_search_channel(aiw_output_channel_cjson->valuestring);
							if (channel_map_element._channel_name != NULL && strcmp(aiw_output_aim_name_cjson->valuestring, "") != 0)
							{
								// search aim_init to add the input ports
								aim_initialization_cb_t *aim_init_cb = _linear_search_aim_init(aiw_output_aim_name_cjson->valuestring);
								if (aim_init_cb->_input_channels == NULL || sizeof(aim_init_cb->_input_channels) == 0)
								{
									aim_init_cb->_input_channels = (subscriber_channel_t *)k_malloc(sizeof(subscriber_channel_t));
									aim_init_cb->_input_channels[0] = channel_map_element._channel;
									aim_init_cb->_count_channels++;
								}
								else
								{
									int8_t old_size = aim_init_cb->_count_channels;
									subscriber_channel_t *channel_list_tmp = (subscriber_channel_t *)k_malloc((old_size + 1) * sizeof(subscriber_channel_t));
									memcpy(channel_list_tmp, aim_init_cb->_input_channels, old_size * sizeof(subscriber_channel_t));
									channel_list_tmp[old_size] = channel_map_element._channel;
									k_free(aim_init_cb->_input_channels);
									aim_init_cb->_input_channels = channel_list_tmp;
									aim_init_cb->_count_channels++;
								}
							}
							free(aiw_output_aim_name_cjson);
							free(aiw_output_channel_cjson);
						}
						free(aiw_topology_output_cjson);
						free(aiw_topology_el_cjson);
					}
				}
			}

			cJSON *aiw_subaims_cjson = cJSON_GetObjectItem(root, "SubAIMs");
			if (cJSON_Array == aiw_subaims_cjson->type)
			{
				int aims_count = cJSON_GetArraySize(aiw_subaims_cjson);
				for (int idx = 0; idx < aims_count; idx++)
				{
					bool aim_init_ok = false;
					cJSON *aim_cjson = cJSON_GetArrayItem(aiw_subaims_cjson, idx);
					cJSON *aim_identifier_cjson = cJSON_GetObjectItem(aim_cjson, "Identifier");
					if (aim_identifier_cjson != NULL)
					{
						cJSON *aim_specification_cjson = cJSON_GetObjectItem(aim_identifier_cjson, "Specification");
						if (aim_identifier_cjson != NULL)
						{
							cJSON *aim_name_cjson = cJSON_GetObjectItem(aim_specification_cjson, "AIM");
							char *aim_name = aim_name_cjson->valuestring;

							aim_initialization_cb_t *aim_init_cb = _linear_search_aim_init(aim_name);
							if (aim_init_cb != NULL)
							{
								LOG_INF("AIM %s found for AIW %s, now initializing...", log_strdup(aim_name), log_strdup(aiw_name));
								char *aim_result = MPAI_Config_Store_Get_AIM(aim_name);
								if (aim_result != NULL)
								{
									LOG_DBG("Calling AIM %s: success", log_strdup(aim_name));

									if (aim_init_cb->_input_channels != NULL)
									{
										for (size_t i = 0; i < aim_init_cb->_count_channels; i++)
										{
											LOG_INF("channel %d", aim_init_cb->_input_channels[i]);
										}
									}

									mpai_error_t err_aim = MPAI_AIFU_AIM_Start(AIW_CAE_REV, aim_init_cb);
									if (err_aim.code == MPAI_AIF_OK || err_aim.code == MPAI_AIM_CREATION_SKIPPED)
									{
										aim_init_ok = true;
									}
									else
									{
										LOG_ERR("Stop initialization");
										while (1)
										{
										};
									}
								}
								k_free(aim_result);
								free(aim_name_cjson);
							}
							else
							{
								LOG_ERR("AIM %s not found", log_strdup(aim_name));
							}
						}
						free(aim_specification_cjson);
					}
					free(aim_identifier_cjson);
					aiw_init_ok = aiw_init_ok & aim_init_ok;
				}
			}
			else
			{
				aiw_init_ok = false;
			}

			free(aiw_name_cjson);
			free(aiw_topology_cjson);
			free(root);
			aif_ok = aif_ok && aiw_init_ok;
		}
	}
	else
	{
		aif_ok = false;
	}
	if (aif_ok)
	{
		MPAI_ERR_INIT(err, MPAI_AIF_OK);
		return err;
	}
	else
	{
		MPAI_ERR_INIT(err, MPAI_ERROR);
		return err;
	}
}
#endif

mpai_error_t MPAI_AIFU_AIW_Start(const char *name, int *AIW_ID)
{
	LOG_INF("Starting AIW %s...", log_strdup(name));

	// At the moment, we handle only AIW CAE-REV
	if (strcmp(name, MPAI_LIBS_CAE_REV_AIW_NAME) == 0)
	{
#if defined(CONFIG_MPAI_CONFIG_STORE) && defined(CONFIG_MPAI_CONFIG_STORE_USES_COAP)
		int aiw_id = MPAI_AIW_CAE_REV_Init();
		*AIW_ID = aiw_id;

		mpai_error_t err_aiw = MPAI_AIFU_AIW_Start_From_MPAI_Store(name);

		if (err_aiw.code == MPAI_AIF_OK)
		{
#ifdef CONFIG_MPAI_AIM_CONTROL_UNIT_SENSORS_PERIODIC
			/* start periodic timer to switch status */
			k_timer_start(&aim_timer, K_SECONDS(5), K_SECONDS(5));
#endif
		}
		return err_aiw;
#endif
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

	memset(MPAI_AIM_List, 0, MPAI_AIF_AIM_MAX * sizeof(aim_initialization_cb_t *));

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return err;
}

// TODO: generalize the implementation. At the moment, we handle only AIW CAE-REV.
// maybe using "MPAI_AIM_List" of aiw_cae_rev.c
mpai_error_t MPAI_AIFU_AIM_GetStatus(int AIW_ID, const char *name, int *status)
{
	MPAI_Component_AIM_t *aim_to_check;
	if (AIW_ID == AIW_CAE_REV)
	{
		if (strcmp(name, MPAI_LIBS_CAE_REV_AIM_DATA_MIC_NAME) == 0)
		{
			aim_to_check = aim_data_mic;
		}
		else if (strcmp(name, MPAI_LIBS_CAE_REV_AIM_SENSORS_NAME) == 0)
		{
			aim_to_check = aim_produce_sensors;
		}
		else if (strcmp(name, MPAI_LIBS_CAE_REV_AIM_TEMP_LIMIT_NAME) == 0)
		{
			aim_to_check = aim_temp_limit;
		}
		else if (strcmp(name, MPAI_LIBS_CAE_REV_AIM_MOTION_NAME) == 0)
		{
			aim_to_check = aim_data_motion;
		}
		else if (strcmp(name, MPAI_LIBS_CAE_REV_AIM_REHABILITATION_NAME) == 0)
		{
			aim_to_check = aim_rehabilitation;
		}
	}
	if (MPAI_AIM_Is_Alive(aim_to_check))
	{
		*status = MPAI_AIM_ALIVE;
	}
	else
	{
		*status = MPAI_AIM_DEAD;
	}

	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return err;
}

// TODO: rename and check from MPAI specs
mpai_error_t MPAI_AIFU_AIM_Start(int aiw_id, aim_initialization_cb_t *aim_init)
{
	MPAI_Component_AIM_t *aim = MPAI_AIM_Creator(aim_init->_aim_name, aiw_id, aim_init->_subscriber, aim_init->_start, aim_init->_stop, aim_init->_resume, aim_init->_pause);
	bool post_cb_result = aim_init->_post_cb(aim);

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

aim_initialization_cb_t *_linear_search_aim_init(const char *name)
{
	for (size_t i = 0; i < mpai_controller_aim_count; i++)
	{
		// verify aim name
		if (strcmp(MPAI_AIM_Get_Component(MPAI_AIM_List[i]->_aim)->name, name) == 0 || strcmp(MPAI_AIM_List[i]->_aim_name, name) == 0)
		{
			return MPAI_AIM_List[i];
		}
	}
	return NULL;
}

channel_map_element_t _linear_search_channel(const char *name)
{
	for (size_t i = 0; i < mpai_message_store_channel_count; i++)
	{
		// verify aim name
		if (strcmp(message_store_channel_list[i]._channel_name, name) == 0)
		{
			return message_store_channel_list[i];
		}
	}
	channel_map_element_t empty = {};
	return empty;
}