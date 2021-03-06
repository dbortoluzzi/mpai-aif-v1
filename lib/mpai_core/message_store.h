/*
 * @file
 * @brief Headers of a message store comunication between AIMs
 * 
 * Copyright (c) 2022 University of Turin, Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MPAI_MESSAGE_STORE_H
#define MPAI_MESSAGE_STORE_H

#include <core_common.h>
#include <core_aim.h>
#include <pubsub/pubsub.h>
#include <errno.h>
#include <sys/slist.h>

#define PUB_SUB_MAX_SUBSCRIBERS 20
#define PUB_SUB_DEFAULT_CHANNEL 0

typedef uint16_t subscriber_channel_t;
typedef struct _subscriber_item{
    module_t* subscriber_key;
	subscriber_channel_t channel;
    struct pubsub_subscriber_s* value;
} subscriber_item;

typedef struct MPAI_AIM_MessageStore_t
{
	struct pubsub_topic_s* _topic;
	char* _topic_name;
	int _aiw_id;
	subscriber_item* message_store_subscribers;
} MPAI_AIM_MessageStore_t; 

static int subscriber_item_count = 0;
static subscriber_channel_t subscriber_channel_current = 1;

/**
 * @brief Create a new channel for message stores
 * 
 * @return subscriber_channel_t 
 */
subscriber_channel_t MPAI_MessageStore_new_channel();

/**
 * @brief Register to a topic channel of message store 
 */
mpai_error_t MPAI_MessageStore_register(MPAI_AIM_MessageStore_t* me, module_t* subscriber, subscriber_channel_t channel);

/**
 * @brief Publish a message to a specified channel of a message store 
 */
mpai_error_t MPAI_MessageStore_publish(MPAI_AIM_MessageStore_t* me, mpai_message_t* message, subscriber_channel_t channel);

/**
 * @brief Poll a message message store from a specified channel
 */
int MPAI_MessageStore_poll(MPAI_AIM_MessageStore_t* me, module_t* subscriber, k_timeout_t timeout, subscriber_channel_t channel);

/**
 * @brief Copy a message message store when is available
 */
mpai_error_t MPAI_MessageStore_copy(MPAI_AIM_MessageStore_t* me, module_t* subscriber, subscriber_channel_t channel, mpai_message_t* message);

/**
 * @brief Create the message store
 */
// TODO: remove topic name, because now the topic is switched to channel
MPAI_AIM_MessageStore_t* MPAI_MessageStore_Creator(int aiw_id, char* topic_name, size_t topic_size);

/**
 * @brief Destroy data of MPAI MessageStore
 * 
 * @return mpai_error_t 
 */
mpai_error_t MPAI_MessageStore_Destructor(MPAI_AIM_MessageStore_t* me);

#endif