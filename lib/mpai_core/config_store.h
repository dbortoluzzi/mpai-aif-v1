#ifndef MPAI_CONFIG_STORE_H
#define MPAI_CONFIG_STORE_H

#include <device.h>
#include <devicetree.h>
#include <stdio.h>
#include <string.h>
#include <logging/log.h>
#include <coap_connect.h>
#include <misc_utils.h>

static const char * const AIF_CONFIG[] = { "config/aif/", NULL }; /*demo*/
static const char * const AIW_CAE_REV_CONFIG[] = { "config/aiw/", NULL }; /*cae_rev*/
static const char * const AIM_VOLUMEPEAKANALYSIS_CONFIG[] = { "config/aim/", NULL }; /*volume_peak_analysis*/

void MPAI_Config_Store_get_AIF(uint8_t* data, uint16_t* len, char* aif_name);

#endif