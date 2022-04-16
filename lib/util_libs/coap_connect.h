/*
 * Copyright (c) 2022 Daniele Bortoluzzi <danieleb88@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COAP_CONNECT_H_
#define COAP_CONNECT_H_

#include <net/socket.h>
#include <net/net_mgmt.h>
#include <net/net_ip.h>
#include <net/udp.h>
#include <net/coap.h>

#include <net/net_core.h>
#include <net/net_if.h>

/* COAP Port used */
#define PEER_PORT CONFIG_COAP_SERVER_PORT
#define MAX_COAP_MSG_LEN 256

#define BLOCK_WISE_TRANSFER_SIZE_GET 2048
#define IP_ADDRESS_COAP_SERVER CONFIG_COAP_SERVER_IPV4_ADDR

/**
 * @brief Get the coap sock object
 * 
 * @return int 
 */
int get_coap_sock(void);

/**
 * @brief Get the block context object
 * 
 * @return struct coap_block_context 
 */
struct coap_block_context get_block_context(void);

/**
 * @brief Get the block context pointer object
 * 
 * @return struct coap_block_context* 
 */
struct coap_block_context* get_block_context_ptr(void);

void wait(void);

/**
 * @brief Initialize coap client
 * 
 * @return int 
 */
int start_coap_client(void);

/**
 * @brief Send a COAP request with the specified method and path
 * 
 * @param method COAP METHOD (GET, POST ecc..)
 * @param simple_path COAP URL
 * @return int 
 */
int send_simple_coap_request(uint8_t method, char ** simple_path);

/**
 * @brief Process simple result returning from coap server
 * 
 * @param data_result data binary
 * @return int 
 */
int process_simple_coap_reply(uint8_t * data_result);

/**
 * @brief Process large result (grouped in blocks) from coap server
 * 
 * @param data_result data binary
 * @return int 
 */
int process_large_coap_reply(uint8_t * data_result);

int process_obs_coap_reply(void);

#endif /* COAP_CONNECT_H_ */