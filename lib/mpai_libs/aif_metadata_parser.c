/*
 * @file
 * @brief Implementations of the parser of AIF/AIW/AIM metadata
 *
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "aif_metadata_parser.h"

LOG_MODULE_REGISTER(MPAI_LIBS_AIF_METADATA_PARSER, LOG_LEVEL_INF);

bool MPAI_Metadata_Parser_Parse_AIF_JSON(const char *aif_result)
{
	if (aif_result != NULL)
	{
		// TODO: validate according with JSON schema

		// Parse AIF Json Metadata
		cJSON *root_aif = cJSON_Parse(aif_result);
		if (root_aif != NULL)
		{
			// read aif
			cJSON *aif_name_cjson = cJSON_GetObjectItem(root_aif, "title");
			if (aif_name_cjson != NULL)
			{
				char *aif_name = aif_name_cjson->valuestring;
				LOG_INF("Initializing AIF with title \"%s\"...", log_strdup(aif_name));
			} 
			else 
			{
				return false;
			}
			free(aif_name_cjson);
		} 
		else 
		{
			return false;
		}
		free(root_aif);
		k_free(aif_result);

		return true;
	}
	else 
	{
		return false;
	}
}

bool MPAI_Metadata_Parser_Parse_AIW_JSON(const char *aiw_result, int aiw_id, aim_callback_t aim_callback, topology_output_callback_t topology_output_callback)
{
	bool aiw_ok = true;
	cJSON *root = cJSON_Parse(aiw_result);
	if (root != NULL)
	{
		// TODO: validate according with JSON schema

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

							const char* aim_name = aiw_output_aim_name_cjson->valuestring;
							const char* output_port_name = aiw_output_channel_cjson->valuestring;

							topology_output_callback(aim_name, output_port_name);

							free(aiw_output_aim_name_cjson);
							free(aiw_output_channel_cjson);
						}
						free(aiw_topology_output_cjson);
						free(aiw_topology_el_cjson);
					}
				}
			}

			// read AIMs of AIW
			cJSON *aiw_subaims_cjson = cJSON_GetObjectItem(root, "SubAIMs");
			if (cJSON_Array == aiw_subaims_cjson->type)
			{
				int aims_count = cJSON_GetArraySize(aiw_subaims_cjson);
				// loop on AIMs
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

							aim_init_ok = aim_callback(aim_name);
							free(aim_name_cjson);
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
			aiw_ok = aiw_ok && aiw_init_ok;
		}
		else 
		{
			aiw_ok = false;
		}
	}
	else
	{
		aiw_ok = false;
	}
	return aiw_ok;
}

// TODO: at the moment, we only check the AIM exists
bool MPAI_Metadata_Parser_Parse_AIM_JSON(const char *aim_result)
{
	// TODO: validate according with JSON schema
	return aim_result != NULL;
}