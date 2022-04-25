/*
 * @file
 * @brief Headers of MPAI Config Store
 * 
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MPAI_CONFIG_STORE_H
#define MPAI_CONFIG_STORE_H

#include <device.h>
#include <devicetree.h>
#include <stdio.h>
#include <string.h>
#include <logging/log.h>
#include <misc_utils.h>
#ifdef CONFIG_MPAI_CONFIG_STORE_USES_COAP
    #include <coap_connect.h>

    static const char * const AIF_CONFIG[] = { "config/aif/", NULL };
    static const char * const AIW_CONFIG[] = { "config/aiw/", NULL };
    static const char * const AIM_CONFIG[] = { "config/aim/", NULL };
#endif

/**
 * @brief Retrieve AIF configuration in a JSON format
 * 
 * @param aif_name 
 * @return char* 
 */
char* MPAI_Config_Store_Get_AIF(const char* aif_name);

/**
 * @brief Retrieve AIW configuration in a JSON format
 * 
 * @param aiw_name 
 * @return char* 
 */
char* MPAI_Config_Store_Get_AIW(const char* aiw_name);

/**
 * @brief Retrieve AIM configuration in a JSON format
 * 
 * @param aim_name 
 * @return char* 
 */
char* MPAI_Config_Store_Get_AIM(const char* aim_name);

#endif