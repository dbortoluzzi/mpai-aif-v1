#include <coap_connect.h>

#include <zephyr.h>
#include <sys/printk.h>
#include <kernel.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(COAP_CONNECT, LOG_LEVEL_INF);

#include <net_private.h>

/* CoAP socket fd */
struct pollfd fds[1];
int nfds;
int coap_sock;

struct coap_block_context blk_ctx;

/*** PRIVATE ***/
void extract_data_result(struct coap_packet packet, char* data_result, bool add_termination);

/*** PUBLIC ***/
int get_coap_sock(void)
{
	return coap_sock;
}

struct coap_block_context get_block_context(void)
{
	return blk_ctx;
}

struct coap_block_context* get_block_context_ptr(void)
{
	return &blk_ctx;
}

void wait(void)
{
	if (poll(fds, nfds, -1) < 0) {
		LOG_ERR("Error in poll:%d", errno);
	}
}

void prepare_fds(void)
{
	fds[nfds].fd = coap_sock;
	fds[nfds].events = POLLIN;
	nfds++;
}

int start_coap_client(void)
{
	int ret = 0;
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PEER_PORT);

	inet_pton(AF_INET, IP_ADDRESS_COAP_SERVER,
		  &addr.sin_addr);

	coap_sock = socket(addr.sin_family, SOCK_DGRAM, IPPROTO_UDP);
	if (coap_sock < 0) {
		LOG_ERR("Failed to create UDP socket %d", errno);
		return -errno;
	}

	ret = connect(coap_sock, (struct sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		LOG_ERR("Cannot connect to UDP remote : %d", errno);
		return -errno;
	}

	prepare_fds();

	return 0;
}

int process_simple_coap_reply(char * data_result)
{
	struct coap_packet reply = {};
	uint8_t *data;
	int rcvd;
	int ret;

	wait();

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	rcvd = recv(coap_sock, data, MAX_COAP_MSG_LEN, MSG_DONTWAIT);
	if (rcvd == 0) {
		ret = -EIO;
		goto end;
	}

	if (rcvd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			ret = 0;
		} else {
			ret = -errno;
		}

		goto end;
	}

	net_hexdump("Raw response", data, rcvd);

	ret = coap_packet_parse(&reply, data, rcvd, NULL, 0);
	extract_data_result(reply, data_result, true);
	printk("Response %s\n", data_result);

	if (ret < 0) {
		LOG_ERR("Invalid data received");
	}

end:
	k_free(data);

	return ret;
}

int process_large_coap_reply(char * data_result)
{
	struct coap_packet reply = {};
	uint8_t *data;
	bool last_block;
	int rcvd;
	int ret;

	wait();

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	rcvd = recv(coap_sock, data, MAX_COAP_MSG_LEN, MSG_DONTWAIT);
	if (rcvd == 0) {
		ret = -EIO;
		goto end;
	}

	if (rcvd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			ret = 0;
		} else {
			ret = -errno;
		}

		goto end;
	}

	// net_hexdump("Raw response", data, rcvd);

	ret = coap_packet_parse(&reply, data, rcvd, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Invalid data received");
		goto end;
	}

	ret = coap_update_from_block(&reply, &blk_ctx);
	if (ret < 0) {
		goto end;
	}

	extract_data_result(reply, data_result, false);
	printk("Response block %zd: %s\n", get_block_context().current / 64, data_result);

	last_block = coap_next_block(&reply, &blk_ctx);
	if (!last_block) {
		ret = 1;
		goto end;
	}

	ret = 0;

end:
	k_free(data);

	return ret;
}

int process_obs_coap_reply(void)
{
	struct coap_packet reply;
	uint16_t id;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t *data;
	uint8_t type;
	uint8_t tkl;
	int rcvd;
	int ret;

	wait();

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	rcvd = recv(coap_sock, data, MAX_COAP_MSG_LEN, MSG_DONTWAIT);
	if (rcvd == 0) {
		ret = -EIO;
		goto end;
	}

	if (rcvd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			ret = 0;
		} else {
			ret = -errno;
		}

		goto end;
	}

	// net_hexdump("Response", data, rcvd);

	ret = coap_packet_parse(&reply, data, rcvd, NULL, 0);
	if (ret < 0) {
		LOG_ERR("Invalid data received");
		goto end;
	}

	tkl = coap_header_get_token(&reply, token);
	id = coap_header_get_id(&reply);

	type = coap_header_get_type(&reply);
	if (type == COAP_TYPE_ACK) {
		ret = 0;
	} else if (type == COAP_TYPE_CON) {
		ret = send_obs_reply_ack(id, token, tkl);
	}
end:
	k_free(data);

	return ret;
}

void extract_data_result(struct coap_packet packet, char* data_result, bool add_termination)
{
	if (packet.offset>=21) 
	{
		// Start from 21 byte according with specs, but there is again garbage at the start
		memcpy(data_result, packet.data+21, packet.offset-21); 
		if (add_termination) 
		{
			char tmp[2] = ""; 
			strcat(data_result, tmp);
		}
	}
	else {
		memset(data_result, 0, MAX_COAP_MSG_LEN);
	} 
}