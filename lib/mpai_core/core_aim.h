/*
 * @file
 * @brief Headers of a MPAI AIM, according to the specs V1
 * 
 * Copyright (c) 2022 Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MPAI_CORE_AIM_H
#define MPAI_CORE_AIM_H

#include <core_common.h>
#include <message_store.h>

typedef struct MPAI_Component_AIM_t MPAI_Component_AIM_t;

/**
 * @brief Start the AIM
 * 
 */
mpai_error_t MPAI_AIM_Start(MPAI_Component_AIM_t* me);

/**
 * @brief Stop the AIM
 * 
 */
mpai_error_t MPAI_AIM_Stop(MPAI_Component_AIM_t* me);

/**
 * @brief Resume the AIM
 * 
 */
mpai_error_t MPAI_AIM_Resume(MPAI_Component_AIM_t* me);

/**
 * @brief Pause the AIM
 * 
 */
mpai_error_t MPAI_AIM_Pause(MPAI_Component_AIM_t* me);

/**
 * @brief Get the component data of the AIM
 * 
 */
component_t* MPAI_AIM_Get_Component(MPAI_Component_AIM_t* me);

/**
 * @brief Get the subscriber of the AIM
 * 
 */
module_t* MPAI_AIM_Get_Subscriber(MPAI_Component_AIM_t* me);

/**
 * @brief Get the status of the AIM
 * 
 */
bool MPAI_AIM_Is_Alive(MPAI_Component_AIM_t* me);

/**
 * Constructor and set up the AIM
 * @param name name of the AIM
 * @param aiw_id ID of the AIW
 * @param start start implementation of the custom AIM
 * @param stop stop implementation of the custom AIM
 * @param resume resume implementation of the custom AIM
 * @param pause pause implementation of the custom AIM
 * @return an instance of AIM
 */
MPAI_Component_AIM_t* MPAI_AIM_Creator(char* name, int aiw_id, 	module_t* subscriber, module_t* start, module_t* stop, module_t* resume, module_t* pause);

#endif