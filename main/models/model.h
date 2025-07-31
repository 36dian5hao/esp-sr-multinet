#ifndef _MODEL_H_
#define _MODEL_H_


#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "sys/socket.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "netdb.h"
#include "arpa/inet.h"
#include "I2S/I2Sto.h"
#include <string.h>
#include "esp_log.h"

#include "esp_mn_models.h"
#include "model_path.h"
#include "esp_mn_iface.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_models.h"
#include "esp_afe_sr_iface.h"

#include "driver/i2s.h"
#include "esp_mn_speech_commands.h"
#include "driver/gpio.h"

#define TAG "app"
#define ESP_MN_PREFIX "mn"
#define LED_GPIO 40

void feed_Task(void *arg);
void detect_Task(void *arg);
int wake_main();






#endif
