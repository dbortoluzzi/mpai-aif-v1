#ifndef MPAI_MESSAGE_STORE_H
#define MPAI_MESSAGE_STORE_H

#include <core_common.h>
#include <core_aim.h>
#include <pubsub/pubsub.h>
#include <errno.h>
#include <sys/slist.h>

#define PUB_SUB_MAX_SUBSCRIBERS 20
#define PUB_SUB_DEFAULT_CHANNEL 0

// overwrite MAX channel in pubsub library
#undef MAX_CHANNELS
#define MAX_CHANNELS 20

typedef uint16_t subscriber_channel_t;
typedef struct _subscriber_item{
    module_t* key;
    struct pubsub_subscriber_s* value;
	subscriber_channel_t channel;
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
 * @brief Register to a topic from message store 
 */
mpai_error_t MPAI_MessageStore_register(MPAI_AIM_MessageStore_t* me, module_t* subscriber, subscriber_channel_t channel);

/**
 * @brief Publish a message into a message store 
 */
mpai_error_t MPAI_MessageStore_publish(MPAI_AIM_MessageStore_t* me, mpai_parser_t* message, subscriber_channel_t channel);

/**
 * @brief Poll a message message store
 */
int MPAI_MessageStore_poll(MPAI_AIM_MessageStore_t* me, module_t* subscriber, k_timeout_t timeout, subscriber_channel_t channel);

/**
 * @brief Copy a message message store when is available
 */
mpai_error_t MPAI_MessageStore_copy(MPAI_AIM_MessageStore_t* me, module_t* subscriber, mpai_parser_t* message);

/**
 * @brief Create the message store
 */
MPAI_AIM_MessageStore_t* MPAI_MessageStore_Creator(int aiw_id, char* topic_name, size_t topic_size);

#endif