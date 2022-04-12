#include "config_store.h"

LOG_MODULE_REGISTER(MPAI_CONFIG_STORE, LOG_LEVEL_INF);

/************* PRIVATE *************/

/************* PUBLIC **************/

void MPAI_Config_Store_get_AIF(uint8_t* data, uint16_t* len, char* aif_name)
{
	char* aif_full_name = append_strings(AIF_CONFIG[0], aif_name);
	char * aif_config_path[] = { aif_full_name, NULL };/*TODO: UNION OPTIONS*/
	int r = get_large_coap_msgs(aif_config_path, data, len);
	if (r < 0) {
		(void)close(get_coap_sock());
	}
} 