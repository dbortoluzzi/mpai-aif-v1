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
/* search channel by name*/
channel_map_element_t _linear_search_channel(const char *name);
/* init and start aim after parsing from MPAI Store Config*/
bool _start_aim_after_parsing_callback(const char * aim_name); 
/* update input channels in MPAI_AIM_List */
void _update_input_channels_after_parsing_callback(const char * aim_name, const char* port_name); 
/* search message store by aiw_id*/
message_store_map_element_t _linear_search_message_store(int aiw_id);

/* AIM initialization List */
aim_initialization_cb_t *MPAI_AIM_List[MPAI_AIF_AIM_MAX] = {};
/* Channel List*/
channel_map_element_t message_store_channel_list[MPAI_AIF_CHANNEL_MAX] = {};
message_store_map_element_t message_store_list[MPAI_AIF_AIW_MAX] = {};
/* Counters */
int mpai_controller_aim_count = 0;
int mpai_message_store_channel_count = 0;
int mpai_message_store_count = 0;

/*** START COAP ***/
#ifdef CONFIG_COAP_SERVER
const char *const test_path[] = {"test", NULL};

const char *const large_path[] = {"large", NULL};

const char *const obs_path[] = {"obs", NULL};
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

	bool aif_ok = false;
#if defined(CONFIG_MPAI_CONFIG_STORE) && defined(CONFIG_MPAI_CONFIG_STORE_USES_COAP)
	char *aif_result = MPAI_Config_Store_Get_AIF(MPAI_LIBS_AIF_NAME);
	aif_ok = MPAI_Metadata_Parser_Parse_AIF_JSON(aif_result);
#endif

	if (aif_ok)
	{
		mpai_error_t err_aiw = MPAI_AIFU_AIW_Start(MPAI_LIBS_CAE_REV_AIW_NAME, &aiw_id);
		if (err_aiw.code != MPAI_AIF_OK)
		{
			LOG_ERR("Error starting AIW %s: %s", MPAI_LIBS_CAE_REV_AIW_NAME, log_strdup(MPAI_ERR_STR(err_aiw.code)));
			return;
		}

		LOG_INF("MPAI_AIF initialized correctly");
	}


#if defined(CONFIG_MPAI_CONFIG_STORE) && defined(CONFIG_MPAI_CONFIG_STORE_USES_COAP)
	/* Close the socket when it's no longer usefull*/
	(void)close(get_coap_sock());
#endif

	// k_sleep(K_SECONDS(5));

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

mpai_error_t MPAI_AIFU_AIW_Start(const char *name, int *AIW_ID)
{
	LOG_INF("Starting AIW %s...", log_strdup(name));

	// At the moment, we handle only AIW CAE-REV
	if (strcmp(name, MPAI_LIBS_CAE_REV_AIW_NAME) == 0)
	{
		int aiw_id = MPAI_AIW_CAE_REV_Init();
		*AIW_ID = aiw_id;

#if defined(CONFIG_MPAI_CONFIG_STORE)
		mpai_error_t err_aiw = MPAI_Controller_Start_Loading_AIW_From_MPAI_Store(name, aiw_id);
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

mpai_error_t MPAI_AIFU_AIM_GetStatus(int AIW_ID, const char *name, int *status)
{
	aim_initialization_cb_t* aim_init = MPAI_AIFU_AIM_Find_AIM(name);
	if (aim_init != NULL)
	{
		if (MPAI_AIM_Is_Alive(aim_init->_aim))
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

	MPAI_ERR_INIT(err, MPAI_ERROR);
	return err;
}

mpai_error_t MPAI_AIFM_AIM_Start(const char *name)
{
	aim_initialization_cb_t* aim_init = MPAI_Controller_Find_AIM_Init_Config(name);
	if (aim_init == NULL)
	{
		MPAI_ERR_INIT(err, MPAI_ERROR);
		return err;
	}
	return MPAI_AIM_Start(aim_init->_aim);
}

mpai_error_t MPAI_AIFM_AIM_Stop(const char *name)
{
	aim_initialization_cb_t* aim_init = MPAI_Controller_Find_AIM_Init_Config(name);
	if (aim_init == NULL)
	{
		MPAI_ERR_INIT(err, MPAI_ERROR);
		return err;
	}
	return MPAI_AIM_Stop(aim_init->_aim);
}

mpai_error_t MPAI_AIFM_AIM_Pause(const char *name)
{
	aim_initialization_cb_t* aim_init = MPAI_Controller_Find_AIM_Init_Config(name);
	if (aim_init == NULL)
	{
		MPAI_ERR_INIT(err, MPAI_ERROR);
		return err;
	}
	return MPAI_AIM_Pause(aim_init->_aim);
}

mpai_error_t MPAI_AIFM_AIM_Resume(const char *name)
{
	aim_initialization_cb_t* aim_init = MPAI_Controller_Find_AIM_Init_Config(name);
	if (aim_init == NULL)
	{
		MPAI_ERR_INIT(err, MPAI_ERROR);
		return err;
	}
	return MPAI_AIM_Resume(aim_init->_aim);
}

#if defined(CONFIG_MPAI_CONFIG_STORE)
mpai_error_t MPAI_Controller_Start_Loading_AIW_From_MPAI_Store(const char *name, int aiw_id)
{
	char *aiw_result = MPAI_Config_Store_Get_AIW(name);
	// printk("AIW RESULT: \n");
	// for ( size_t i = 0; i < strlen(aiw_result); i++ )
	// {
	// 	printk("%c", (char)aiw_result[i]);
	// 	k_sleep(K_MSEC(5));
	// }
	// printk("\n");

	bool aiw_ok = MPAI_Metadata_Parser_Parse_AIW_JSON(aiw_result, aiw_id, _start_aim_after_parsing_callback, _update_input_channels_after_parsing_callback);
	if (aiw_ok)
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

mpai_error_t MPAI_Controller_Start_Loading_AIM_From_Config_Init(int aiw_id, aim_initialization_cb_t *aim_init)
{
	// create AIM
	MPAI_Component_AIM_t *aim = MPAI_AIM_Creator(aim_init->_aim_name, aiw_id, aim_init->_subscriber, aim_init->_start, aim_init->_stop, aim_init->_resume, aim_init->_pause);
	// set AIM in init configuration
	aim_init->_aim = aim;

	// check if there are input channels configured
	if (aim_init->_input_channels != NULL)
	{
		// search message_store of the AIW
		message_store_map_element_t message_store_map_el = _linear_search_message_store(aiw_id);
		if (message_store_map_el._message_store != NULL)
		{
			// loop on channels and register to the AIM
			for (size_t i = 0; i < aim_init->_count_channels; i++)
			{
				LOG_INF("Registring channel %d for AIM %s", aim_init->_input_channels[i], log_strdup(aim_init->_aim_name));
				MPAI_MessageStore_register(message_store_map_el._message_store, aim_init->_subscriber, aim_init->_input_channels[i]);
			}
		}
	}
	// start the AIM
	mpai_error_t err_aim = MPAI_AIM_Start(aim);

	if (err_aim.code != MPAI_AIF_OK)
	{
		LOG_ERR("Error starting AIM %s: %s", log_strdup(MPAI_AIM_Get_Component(aim)->name), log_strdup(MPAI_ERR_STR(err_aim.code)));
	}
	return err_aim;
}

aim_initialization_cb_t *MPAI_Controller_Find_AIM_Init_Config(const char *name)
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

message_store_map_element_t _linear_search_message_store(int aiw_id)
{
	for (size_t i = 0; i < mpai_message_store_count; i++)
	{
		if (message_store_list[i]._aiw_id == aiw_id)
		{
			return message_store_list[i];
		}
	}
	message_store_map_element_t empty = {};
	return empty;
}

bool _start_aim_after_parsing_callback(const char * aim_name)
{
	aim_initialization_cb_t *aim_init_cb = MPAI_Controller_Find_AIM_Init_Config(aim_name);
	if (aim_init_cb != NULL)
	{
		LOG_INF("AIM %s found, now initializing...", log_strdup(aim_name));
		char *aim_result = MPAI_Config_Store_Get_AIM(aim_name);
		bool aim_parse_ok = MPAI_Metadata_Parser_Parse_AIM_JSON(aim_result);
		if (aim_parse_ok)
		{
			LOG_DBG("Calling AIM %s: success", log_strdup(aim_name));

			// start AIM according with the aim_init configuration
			mpai_error_t err_aim = MPAI_Controller_Start_Loading_AIM_From_Config_Init(aiw_id, aim_init_cb);
			if (err_aim.code == MPAI_AIF_OK)
			{
				return true;
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
	}
	else
	{
		LOG_ERR("AIM %s not found", log_strdup(aim_name));
	}
}

void _update_input_channels_after_parsing_callback(const char * aim_name, const char* output_port_name)
{
	// search channel in config
	channel_map_element_t channel_map_element = _linear_search_channel(output_port_name);
	if (channel_map_element._channel_name != NULL && strcmp(aim_name, "") != 0)
	{
		// search aim_init to add the input ports
		aim_initialization_cb_t *aim_init_cb = MPAI_Controller_Find_AIM_Init_Config(aim_name);
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
}