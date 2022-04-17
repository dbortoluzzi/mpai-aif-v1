/*
 * @file
 * @brief Implementation of a message store comunication between AIMs
 * 
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "message_store.h"

LOG_MODULE_REGISTER(MPAI_MESSAGE_STORE, LOG_LEVEL_INF);

K_SEM_DEFINE(subscriber_channel_sem, 0, 1);

/************* PRIVATE HEADER *************/
subscriber_item* _linear_search(subscriber_item *items, size_t size, module_t *subscriber_key, subscriber_channel_t channel);

/************* PUBLIC **************/

mpai_error_t MPAI_MessageStore_register(MPAI_AIM_MessageStore_t *me, module_t *subscriber, subscriber_channel_t channel)
{
	// check errors
	if (me == NULL) {
		MPAI_ERR_INIT(err, MPAI_ERROR);
		LOG_ERR("Found a failure registering AIM: %s.", log_strdup(MPAI_ERR_STR(MPAI_ERROR)));
		return err;
	}

	if (subscriber == NULL) {
		MPAI_ERR_INIT(err, MPAI_ERROR);
		LOG_ERR("Found a failure registering AIM of the AIW %d: %s.", me->_aiw_id, log_strdup(MPAI_ERR_STR(MPAI_ERROR)));
		return err;
	}
	
	// TODO: check if subscriber is not already created before

	// create AIM subscriber for the specified channel (using PubSub library)
	struct pubsub_subscriber_s *sub = (struct pubsub_subscriber_s *)k_malloc(sizeof(struct pubsub_subscriber_s));
	if (sub == NULL) {
		MPAI_ERR_INIT(err, MPAI_ERROR);
		LOG_ERR("Found a failure allocating memory for AIM of the AIW %d: %s.", me->_aiw_id, log_strdup(MPAI_ERR_STR(MPAI_ERROR)));
		return err;
	}
	// save created topic
	sub->topic = me->_topic;

	// register the topic for subscriber and specified channel (using PubSub library)
	pubsub_subscriber_register(me->_topic, sub, channel);

	// create subscriber_item to have in memory a list of all subscribers
	subscriber_item item = {.subscriber_key = subscriber, .channel = channel, .value = sub};
	// add subscriber_item to this list
	me->message_store_subscribers[subscriber_item_count++] = item;

	// TODO: error management
	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return err;
}

mpai_error_t MPAI_MessageStore_publish(MPAI_AIM_MessageStore_t *me, mpai_parser_t *message, subscriber_channel_t channel)
{
	// check errors
	if (me == NULL) {
		MPAI_ERR_INIT(err, MPAI_ERROR);
		LOG_ERR("Found a failure publishing message: %s.", log_strdup(MPAI_ERR_STR(MPAI_ERROR)));
		return err;
	}

	// publish message to a specified topic and channel (using PubSub library)
	pubsub_publish(me->_topic, channel, message);

	// TODO: error management
	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return err;
}

int MPAI_MessageStore_poll(MPAI_AIM_MessageStore_t *me, module_t *subscriber, k_timeout_t timeout, subscriber_channel_t channel)
{
	// check errors
	if (me == NULL) {
		LOG_ERR("Found a failure polling: %s.", log_strdup(MPAI_ERR_STR(MPAI_ERROR)));
		return -EINVAL;
	}

	if (subscriber == NULL) {
		LOG_ERR("Found a failure polling for the AIW %d: %s.", me->_aiw_id, log_strdup(MPAI_ERR_STR(MPAI_ERROR)));
		return -EINVAL;
	}
	
	// find subscriber to poll if there are messages for it
	subscriber_item *sub_found = _linear_search(me->message_store_subscribers, (size_t)subscriber_item_count, subscriber, channel);
	if (sub_found != NULL) {
		return pubsub_poll(sub_found->value, timeout);
	}
	return -EINVAL;
}

mpai_error_t MPAI_MessageStore_copy(MPAI_AIM_MessageStore_t *me, module_t *subscriber, subscriber_channel_t channel, mpai_parser_t *message)
{
	// check errors
	if (me == NULL) {
		MPAI_ERR_INIT(err, MPAI_ERROR);
		LOG_ERR("Found a failure copying message for the AIM: %s.", log_strdup(MPAI_ERR_STR(MPAI_ERROR)));
		return err;
	}

	if (subscriber == NULL) {
		MPAI_ERR_INIT(err, MPAI_ERROR);
		LOG_ERR("Found a failure copying message for the AIW %d: %s.", me->_aiw_id, log_strdup(MPAI_ERR_STR(MPAI_ERROR)));
		return err;
	}
	
	// find subscriber to retrieve the message
	subscriber_item *sub_found = _linear_search(me->message_store_subscribers, (size_t)subscriber_item_count, subscriber, channel);
	if (sub_found != NULL) {
		pubsub_copy(sub_found->value, message);
	}

	// TODO: error management
	MPAI_ERR_INIT(err, MPAI_AIF_OK);
	return err;
}

subscriber_channel_t MPAI_MessageStore_new_channel()
{
	// mutual exclusion lock to permit the creation of a new channel identifier
	if (k_sem_take(&subscriber_channel_sem, K_NO_WAIT) != 0) {
        subscriber_channel_t new_channel = subscriber_channel_current++;
		k_sem_reset(&subscriber_channel_sem);
		return new_channel;
    } else {
        return -1;
    }
}

MPAI_AIM_MessageStore_t *MPAI_MessageStore_Creator(int aiw_id, char *topic_name, size_t topic_size)
{
	MPAI_AIM_MessageStore_t *this = (MPAI_AIM_MessageStore_t *)k_malloc(sizeof(MPAI_AIM_MessageStore_t));
	this->_topic = (struct pubsub_topic_s *)k_malloc(sizeof(struct pubsub_topic_s));
	this->_topic_name = (char *)k_malloc(strlen(topic_name));
	this->message_store_subscribers = (subscriber_item *)k_calloc(PUB_SUB_MAX_SUBSCRIBERS, sizeof(subscriber_item));

	strcpy(this->_topic_name, topic_name);
	this->_aiw_id = aiw_id;

	pubsub_topic_init(this->_topic, topic_size);

	return this;
}

mpai_error_t MPAI_MessageStore_Destructor(MPAI_AIM_MessageStore_t* me)
{
	// check errors
	if (me == NULL) {
		MPAI_ERR_INIT(err, MPAI_ERROR);
		LOG_ERR("Found a failure destroying MPAI MessageStore: %s.", log_strdup(MPAI_ERR_STR(MPAI_ERROR)));
		return err;
	}

	k_free(me->_topic_name);
	k_free(me->_topic);
	k_free(me->message_store_subscribers);
}

/************* PRIVATE IMPLEMENTATION *************/
subscriber_item* _linear_search(subscriber_item *items, size_t size, module_t *subscriber_key, subscriber_channel_t channel)
{
	for (size_t i = 0; i < size; i++)
	{
		// verify subscriber and channel identifiers
		if (items[i].subscriber_key == subscriber_key && items[i].channel == channel)
		{
			return &items[i];
		}
	}
	return NULL;
}