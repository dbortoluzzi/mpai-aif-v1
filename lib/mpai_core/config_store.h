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

    static const char * const AIF_CONFIG[] = { "config/aif/", NULL }; /*demo*/
    static const char * const AIW_CAE_REV_CONFIG[] = { "config/aiw/", NULL }; /*cae_rev*/
    static const char * const AIM_VOLUMEPEAKANALYSIS_CONFIG[] = { "config/aim/", NULL }; /*volume_peak_analysis*/
#endif

char* MPAI_Config_Store_Get_AIF(char* aif_name);

#endif