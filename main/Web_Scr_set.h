#ifndef WEB_SCR_SET_H
#define WEB_SCR_SET_H

// ESP-IDF 标准库
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

// 如果需要Web服务器功能，需要添加相应的ESP-IDF组件
// 注意：ESP-IDF没有直接的AsyncWebServer等效库
// 可以使用esp_http_server组件

// 基本类型定义
typedef struct {
    char ssid[32];
    char password[64];
} wifi_config_t_custom;

typedef struct {
    char name[64];
    char id[32];
} music_config_t;

typedef struct {
    char supplier[32];
    char api_key[128];
    char api_url[256];
} llm_config_t;

// 全局变量声明
extern const char *ap_ssid;
extern const char *ap_password;
extern bool isWebServerStarted;
extern bool isSoftAPStarted;

// ESP-IDF HTTP服务器相关
#include "esp_http_server.h"

// 函数声明
esp_err_t openWeb(void);
esp_err_t init_wifi_ap(void);
esp_err_t init_web_server(void);

// HTTP处理器函数声明
esp_err_t root_get_handler(httpd_req_t *req);
esp_err_t wifi_get_handler(httpd_req_t *req);
esp_err_t music_get_handler(httpd_req_t *req);
esp_err_t model_get_handler(httpd_req_t *req);
esp_err_t favicon_get_handler(httpd_req_t *req);
esp_err_t error_handler(httpd_req_t *req, httpd_err_code_t err);
esp_err_t simple_request_handler(httpd_req_t *req, const char* response_html);
esp_err_t handle_large_header_error(httpd_req_t *req, httpd_err_code_t err);

// 辅助函数声明
esp_err_t nvs_get_str_alloc(nvs_handle_t handle, const char* key, char** out_value);
char* url_decode(const char* str);
esp_err_t parse_post_data(httpd_req_t *req, char* buf, size_t buf_len);

// 配置保存/加载函数声明
esp_err_t save_wifi_config(const char* ssid, const char* password);
esp_err_t load_wifi_config(wifi_config_t_custom* config);

#endif