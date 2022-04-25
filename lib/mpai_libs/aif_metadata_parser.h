/*
 * @file
 * @brief Headers of the parser of AIF/AIW/AIM metadata
 *
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef MPAI_AIF_METADATA_PARSER_H
#define MPAI_AIF_METADATA_PARSER_H

#include <core_common.h>
#include <cJSON.h>

/**
 * @brief Parse JSON coming from MPAI Store Config according with AIF specs
 * 
 * @param aif_result JSON string
 * @return true 
 * @return false 
 */
bool MPAI_Metadata_Parser_Parse_AIF_JSON(const char *aif_result);

/**
 * @brief Parse JSON coming from MPAI Store Config according with AIW specs, using callbacks
 * 
 * @param aiw_result JSON string
 * @param aiw_id ID of AIW
 * @param aim_callback callback called after extracting each AIM
 * @param topology_output_callback callback called after extracting the "Output" property of "Topology"
 * @return true 
 * @return false 
 */
bool MPAI_Metadata_Parser_Parse_AIW_JSON(const char *aiw_result, int aiw_id, aim_callback_t aim_callback, topology_output_callback_t topology_output_callback);

/**
 * @brief Parse JSON coming from MPAI Store Config according with AIW specs
 * 
 * @param aim_result 
 * @return true 
 * @return false 
 */
bool MPAI_Metadata_Parser_Parse_AIM_JSON(const char *aim_result);

#endif