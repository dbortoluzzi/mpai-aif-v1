#ifndef COAP_CONNECT_H_
#define COAP_CONNECT_H_

#include <net/socket.h>
#include <net/net_mgmt.h>
#include <net/net_ip.h>
#include <net/udp.h>
#include <net/coap.h>

#include <net/net_core.h>
#include <net/net_if.h>

#define PEER_PORT 5683
#define MAX_COAP_MSG_LEN 256

#define BLOCK_WISE_TRANSFER_SIZE_GET 2048
#define IP_ADDRESS_COAP_SERVER "134.102.218.18"/*coap.me*/

int get_coap_sock(void);

struct coap_block_context get_block_context(void);

struct coap_block_context* get_block_context_ptr(void);

void wait(void);

int start_coap_client(void);

int process_simple_coap_reply(void);

int process_large_coap_reply(void);

int process_obs_coap_reply(void);

#endif /* COAP_CONNECT_H_ */