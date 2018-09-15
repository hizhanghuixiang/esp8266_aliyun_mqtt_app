/*
 * user_main.c
 */

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "user_interface.h"
#include "mem.h"
#include "driver/uart.h"

#include "mqtt/mqtt.h"
#include "mqtt/debug.h"

#include "user_config.h"
#include "aliyun_mqtt.h"
#include "user_wifi.h"

MQTT_Client mqttClient;

// topic
#define BASE_TOPIC   "/"PRODUCT_KEY"/"DEVICE_NAME
#define GET_TOPIC     BASE_TOPIC"/get"
#define UPDATE_TOPIC  BASE_TOPIC"/update"

/*****************************************************************************/
// 以下是esp8266例程里的mqtt示例，只改动了小部分。

void wifiConnectCb(uint8_t status) {
	if (status == STATION_GOT_IP) {
		MQTT_Connect(&mqttClient);
	} else {
		MQTT_Disconnect(&mqttClient);
	}
}

void mqttConnectedCb(uint32_t *args) {
	MQTT_Client* client = (MQTT_Client*) args;
	INFO("MQTT: Connected\r\n");

	MQTT_Subscribe(client, GET_TOPIC, 0);

	/*
	 * MQTT_Publish函数参数说明
	 * @param  client: 	    MQTT_Client reference
	 * @param  topic: 		string topic will publish to
	 * @param  data: 		buffer data send point to
	 * @param  data_length: length of data
	 * @param  qos:		    qos
	 * @param  retain:      retain
	 */
	MQTT_Publish(client, UPDATE_TOPIC, "hello", 6, 0, 0);
}

void mqttDisconnectedCb(uint32_t *args) {
	MQTT_Client* client = (MQTT_Client*) args;
	INFO("MQTT: Disconnected\r\n");
}

void mqttPublishedCb(uint32_t *args) {
	MQTT_Client* client = (MQTT_Client*) args;
	INFO("MQTT: Published\r\n");
}

void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len,
		const char *data, uint32_t data_len) {
	char *topicBuf = (char*) os_zalloc(topic_len + 1), *dataBuf =
			(char*) os_zalloc(data_len + 1);

	MQTT_Client* client = (MQTT_Client*) args;

	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;

	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;

	INFO("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);
	os_free(topicBuf);
	os_free(dataBuf);
}

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
 *******************************************************************************/
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void) {
	enum flash_size_map size_map = system_get_flash_size_map();
	uint32 rf_cal_sec = 0;

	switch (size_map) {
	case FLASH_SIZE_4M_MAP_256_256:
		rf_cal_sec = 128 - 5;
		break;

	case FLASH_SIZE_8M_MAP_512_512:
		rf_cal_sec = 256 - 5;
		break;

	case FLASH_SIZE_16M_MAP_512_512:
	case FLASH_SIZE_16M_MAP_1024_1024:
		rf_cal_sec = 512 - 5;
		break;

	case FLASH_SIZE_32M_MAP_512_512:
	case FLASH_SIZE_32M_MAP_1024_1024:
		rf_cal_sec = 1024 - 5;
		break;

	default:
		rf_cal_sec = 0;
		break;
	}

	return rf_cal_sec;
}

/*****************************************************************************/

void user_init(void) {
	uart_init(BIT_RATE_74880, BIT_RATE_74880);
	//uart_init(BIT_RATE_115200, BIT_RATE_115200);

	// 测试 hmacmd5 生成mqtt passwrod
	//test_hmac_md5();

	aliyun_mqtt_init();

	//MQTT_InitConnection(&mqttClient, "192.168.11.122", 1880, 0);
	MQTT_InitConnection(&mqttClient, g_aliyun_mqtt.host, g_aliyun_mqtt.port, 0);

	//MQTT_InitClient(&mqttClient, "client_id", "user", "pass", 120, 1);
	MQTT_InitClient(&mqttClient, g_aliyun_mqtt.client_id,
			g_aliyun_mqtt.username, g_aliyun_mqtt.password,
			g_aliyun_mqtt.keepalive, 1);

	// 遗愿消息
	// 阿里云mqtt不需要设置遗愿消息
	//MQTT_InitLWT(&mqttClient, "/lwt", "offline", 0, 0);

	// 设置mqtt的回调函数
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	MQTT_OnData(&mqttClient, mqttDataCb);

	// 连接wifi
	wifi_connect(wifiConnectCb);

	INFO("\r\nSystem started ...\r\n");
}
