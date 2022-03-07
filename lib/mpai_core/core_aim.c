#include "core_aim.h"

LOG_MODULE_REGISTER(MPAI_CORE_AIM, LOG_LEVEL_INF);

struct MPAI_Component_AIM_t
{
	component_t* _component;
	int _aiw_id;
	module_t* _subscriber;
	module_t* _start;
	module_t* _stop;
	module_t* _resume;
	module_t* _pause;
	bool _active;
};

mpai_error_t MPAI_AIM_Start(MPAI_Component_AIM_t* me)
{
	if (me == NULL) {
		MPAI_ERR_INIT(err, MPAI_ERROR);
		LOG_ERR("Found a failure starting AIM: %s.", log_strdup(MPAI_ERR_STR(MPAI_ERROR)));
		return err;
	}
	
	*(me->_start)();
	me->_active = true;

	LOG_INF("AIM %s started with success.", log_strdup(me->_component->name));
	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return err;
}

mpai_error_t MPAI_AIM_Stop(MPAI_Component_AIM_t* me)
{
	if (me == NULL) {
		MPAI_ERR_INIT(err, MPAI_ERROR);
		LOG_ERR("Found a failure stopping AIM: %s.", log_strdup(MPAI_ERR_STR(MPAI_ERROR)));
		return err;
	}
	
	*(me->_stop)();
	me->_active = false;

	LOG_INF("AIM %s stopped with success.", log_strdup(me->_component->name));
	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return err;
}

mpai_error_t MPAI_AIM_Resume(MPAI_Component_AIM_t* me)
{
	if (me == NULL) {
		MPAI_ERR_INIT(err, MPAI_ERROR);
		LOG_ERR("Found a failure resuming AIM: %s.", log_strdup(MPAI_ERR_STR(MPAI_ERROR)));
		return err;
	}
	
	*(me->_resume)();
	me->_active = true;

	LOG_INF("AIM %s resumed with success.", log_strdup(me->_component->name));
	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return err;
}

mpai_error_t MPAI_AIM_Pause(MPAI_Component_AIM_t* me)
{
	if (me == NULL) {
		MPAI_ERR_INIT(err, MPAI_ERROR);
		LOG_ERR("Found a failure pausing AIM: %s.", log_strdup(MPAI_ERR_STR(MPAI_ERROR)));
		return err;
	}
	
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

struct module_t* MPAI_AIM_Get_Subscriber(MPAI_Component_AIM_t* me)
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
	MPAI_Component_AIM_t* this = (MPAI_Component_AIM_t*) k_malloc(sizeof(MPAI_Component_AIM_t));
	this->_component = (component_t*) k_malloc(sizeof(component_t));
	// this->_message_store = (MPAI_AIM_MessageStore_t*) k_malloc(sizeof(MPAI_AIM_MessageStore_t));
	this->_component->name = (char*) k_malloc(strlen(name));

	// TODO: add checks
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