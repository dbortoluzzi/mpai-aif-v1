/*
 * wifi_config.h
 *
 * Based on the official sample by Intel at https://github.com/zephyrproject-rtos/zephyr/tree/master/samples/net/wifi
 *
 * Created on: Dec 27, 2020
 * Author: Joao Carreira
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef SRC_WIFI_CONFIG_H_
#define SRC_WIFI_CONFIG_H_

// Auto join 0 - Disabled, 1 - Enable
#define AUTO_CONNECT  		1

// SSID
extern char* AUTO_CONNECT_SSID;

// Password
extern char* AUTO_CONNECT_SSID_PSK;

//Security type WIFI_SECURITY_TYPE_NONE = 0, WIFI_SECURITY_TYPE_PSK = 1
#define SSID_SECURITY	WIFI_SECURITY_TYPE_PSK

#endif /* SRC_WIFI_CONFIG_H_ */