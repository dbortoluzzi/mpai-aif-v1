/*
 * @file
 * @brief Implementation of a MPAI AIM, according to the specs V1
 * 
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core_aim.h"

LOG_MODULE_REGISTER(MPAI_CORE_AIM, LOG_LEVEL_INF);

/**
 * @brief AIM structural representation
 * 
 */
struct MPAI_Component_AIM_t
{
	component_t* _component; // MPAI component (in this case, it will set to AIM)
	int _aiw_id; 			 // related AIW identifier
	module_t* _subscriber; 	 // related AIM subscriber identifier
	module_t* _start;		 // AIM's start function
	module_t* _stop;		 // AIM's stop function
	module_t* _resume;		 // AIM's resume function
	module_t* _pause;		 // AIM's pause function
	bool _active;		 	 // indicates if AIM is started or not		
};

mpai_error_t MPAI_AIM_Start(MPAI_Component_AIM_t* me)
{
	// check errors
	if (me == NULL) {
		MPAI_ERR_INIT(err, MPAI_ERROR);
		LOG_ERR("Found a failure starting AIM: %s.", log_strdup(MPAI_ERR_STR(MPAI_ERROR)));
		return err;
	}
	
	// let's start the AIM
	*(me->_start)();
	me->_active = true;

	LOG_INF("AIM %s started with success.", log_strdup(me->_component->name));
	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return err;
}

mpai_error_t MPAI_AIM_Stop(MPAI_Component_AIM_t* me)
{
	// check errors
	if (me == NULL) {
		MPAI_ERR_INIT(err, MPAI_ERROR);
		LOG_ERR("Found a failure stopping AIM: %s.", log_strdup(MPAI_ERR_STR(MPAI_ERROR)));
		return err;
	}

	// let's stop the AIM
	*(me->_stop)();
	me->_active = false;

	LOG_INF("AIM %s stopped with success.", log_strdup(me->_component->name));
	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return err;
}

mpai_error_t MPAI_AIM_Resume(MPAI_Component_AIM_t* me)
{
	// check errors
	if (me == NULL) {
		MPAI_ERR_INIT(err, MPAI_ERROR);
		LOG_ERR("Found a failure resuming AIM: %s.", log_strdup(MPAI_ERR_STR(MPAI_ERROR)));
		return err;
	}
	
	// let's resume the AIM
	*(me->_resume)();
	me->_active = true;

	LOG_INF("AIM %s resumed with success.", log_strdup(me->_component->name));
	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return err;
}

mpai_error_t MPAI_AIM_Pause(MPAI_Component_AIM_t* me)
{
	// check errors
	if (me == NULL) {
		MPAI_ERR_INIT(err, MPAI_ERROR);
		LOG_ERR("Found a failure pausing AIM: %s.", log_strdup(MPAI_ERR_STR(MPAI_ERROR)));
		return err;
	}
	
	// let's pause the AIM
	*(me->_pause)();
	me->_active = false;

	LOG_INF("AIM %s paused with success.", log_strdup(me->_component->name));
	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return err;
}

component_t* MPAI_AIM_Get_Component(MPAI_Component_AIM_t* me)
{
	return me->_component;
}

module_t* MPAI_AIM_Get_Subscriber(MPAI_Component_AIM_t* me)
{
	return me->_subscriber;
}

bool MPAI_AIM_Is_Alive(MPAI_Component_AIM_t* me)
{
	return me->_active;
}

MPAI_Component_AIM_t* MPAI_AIM_Creator(
	char* name, 
	int aiw_id,
	module_t* subscriber,
	module_t* start, 	
	module_t* stop, 
	module_t* resume, 
	module_t* pause)
{
	// allocate in the heap
	MPAI_Component_AIM_t* this = (MPAI_Component_AIM_t*) k_malloc(sizeof(MPAI_Component_AIM_t));
	this->_component = (component_t*) k_malloc(sizeof(component_t));
	this->_component->name = (char*) k_malloc(strlen(name));

	// TODO: add checks

	// initialize the struct fields
	strcpy(this->_component->name, name);
	this->_component->type = AIM_TYPE;
	this->_aiw_id = aiw_id;
	this->_subscriber = subscriber;
	this->_start = start;
	this->_stop = stop;
	this->_resume = resume;
	this->_pause = pause;
	this->_active = false;

	return this;
}

mpai_error_t MPAI_AIM_Destructor(MPAI_Component_AIM_t* me)
{
	// check errors
	if (me == NULL) {
		MPAI_ERR_INIT(err, MPAI_ERROR);
		LOG_ERR("Found a failure destructing AIM: %s.", log_strdup(MPAI_ERR_STR(MPAI_ERROR)));
		return err;
	}

	k_free(me->_component->name);
	k_free(me->_component);
	k_free(me);
}