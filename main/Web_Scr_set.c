#include "Web_Scr_set.h"
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "WEB_SERVER";

// AP模式的SSID和密码
const char *ap_ssid = "ESP32-Setup";
const char *ap_password = "12345678";

// Web服务器句柄
httpd_handle_t server = NULL;
bool isWebServerStarted = false;
bool isSoftAPStarted = false;

// NVS操作辅助函数
esp_err_t nvs_get_str_alloc(nvs_handle_t handle, const char* key, char** out_value) {
    size_t required_size = 0;
    esp_err_t err = nvs_get_str(handle, key, NULL, &required_size);
    if (err == ESP_OK) {
        *out_value = malloc(required_size);
        if (*out_value == NULL) {
            return ESP_ERR_NO_MEM;
        }
        err = nvs_get_str(handle, key, *out_value, &required_size);
        if (err != ESP_OK) {
            free(*out_value);
            *out_value = NULL;
        }
    }
    return err;
}

// 字符串辅助函数
char* url_decode(const char* str) {
    int len = strlen(str);
    char* decoded = malloc(len + 1);
    int i, j = 0;

    for (i = 0; i < len; i++) {
        if (str[i] == '%' && i + 2 < len) {
            int hex_val;
            sscanf(str + i + 1, "%2x", &hex_val);
            decoded[j++] = hex_val;
            i += 2;
        } else if (str[i] == '+') {
            decoded[j++] = ' ';
        } else {
            decoded[j++] = str[i];
        }
    }
    decoded[j] = '\0';
    return decoded;
}

// 解析POST数据
esp_err_t parse_post_data(httpd_req_t *req, char* buf, size_t buf_len) {
    int total_len = req->content_len;
    int cur_len = 0;
    int received = 0;

    if (total_len >= buf_len) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Content too long");
        return ESP_FAIL;
    }

    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';
    return ESP_OK;
}

// 简化的请求处理器 - 忽略大部分请求头
esp_err_t simple_request_handler(httpd_req_t *req, const char* response_html) {
    // 直接发送响应，不处理复杂的请求头
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_set_hdr(req, "Expires", "0");
    httpd_resp_send(req, response_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// 处理请求头过长的错误
esp_err_t handle_large_header_error(httpd_req_t *req, httpd_err_code_t err) {
    if (err == HTTPD_431_REQ_HDR_FIELDS_TOO_LARGE) {
        ESP_LOGW(TAG, "Request header too large, sending simplified response");
        const char* simple_html = "<!DOCTYPE html><html><head><title>ESP32</title></head><body><h1>ESP32配置</h1><p><a href='/'>首页</a> | <a href='/wifi'>WiFi</a> | <a href='/music'>音乐</a> | <a href='/model'>AI</a></p></body></html>";
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, simple_html, HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    return ESP_FAIL;
}

// 处理根路径的请求
esp_err_t root_get_handler(httpd_req_t *req)
{
    const char* html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1.0'><title>ESP32配置中心</title><style>body{font-family:Arial,sans-serif;background-color:#f0f0f0;display:flex;justify-content:center;align-items:center;height:100vh;margin:0}.container{background-color:#fff;padding:20px;border-radius:8px;box-shadow:0 0 10px rgba(0,0,0,0.1);text-align:center;width:90%;max-width:400px;display:flex;flex-direction:column;gap:20px}h1{color:#333}.button{display:inline-block;padding:10px 20px;margin:10px;border:none;background-color:#333;color:white;text-decoration:none;cursor:pointer;border-radius:5px;transition:background-color 0.3s}.button:hover{background-color:#555}</style></head><body><div class='container'><h1>ESP32配置中心</h1><a href='/wifi'class='button'>Wi-Fi配置</a><a href='/music'class='button'>音乐配置</a><a href='/model'class='button'>大模型配置</a></div></body></html>";

    return simple_request_handler(req, html);
}

// Wi-Fi配置页面处理
esp_err_t wifi_get_handler(httpd_req_t *req)
{
    const char* html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1.0'><title>Wi-Fi配置</title><style>body{font-family:Arial,sans-serif;background-color:#f0f0f0;display:flex;justify-content:center;align-items:center;height:100vh;margin:0}.container{background-color:#fff;padding:20px;border-radius:8px;box-shadow:0 0 10px rgba(0,0,0,0.1);text-align:center;width:90%;max-width:400px;display:flex;flex-direction:column;gap:20px}h1{color:#333}.button{display:inline-block;padding:10px 20px;margin:10px;border:none;background-color:#333;color:white;text-decoration:none;cursor:pointer;border-radius:5px;transition:background-color 0.3s}.button:hover{background-color:#555}form{display:flex;flex-direction:column;gap:10px}label{text-align:left}input[type='text'],input[type='password']{padding:10px;border:1px solid#ccc;border-radius:5px}input[type='submit']{padding:10px 20px;border:none;background-color:#333;color:white;cursor:pointer;border-radius:5px;transition:background-color 0.3s}input[type='submit']:hover{background-color:#555}.fixed-button{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);z-index:1000}</style></head><body><div class='container'><h1>Wi-Fi 配置</h1><form action='/save'method='post'><label for='ssid'>Wi-Fi 名称:</label><input type='text'id='ssid'name='ssid'required><label for='password'>Wi-Fi 密码:</label><input type='password'id='password'name='password'required><input type='submit'value='保存 / 更新'class='button'></form><form action='/delete'method='post'><label for='ssid_delete'>要删除的 Wi-Fi 名称:</label><input type='text'id='ssid_delete'name='ssid'><input type='submit'value='删除'class='button'></form><a href='/list'class='button'>已保存的 Wi-Fi 网络</a><a href='/'class='button fixed-button'>返回首页</a></div></body></html>";

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// 音乐配置页面处理
esp_err_t music_get_handler(httpd_req_t *req)
{
    const char* html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1.0'><title>音乐配置</title><style>body{font-family:Arial,sans-serif;background-color:#f0f0f0;display:flex;justify-content:center;align-items:center;height:100vh;margin:0}.container{background-color:#fff;padding:20px;border-radius:8px;box-shadow:0 0 10px rgba(0,0,0,0.1);text-align:center;width:90%;max-width:400px;display:flex;flex-direction:column;gap:20px}h1{color:#333}.button{display:inline-block;padding:10px 20px;margin:10px;border:none;background-color:#333;color:white;text-decoration:none;cursor:pointer;border-radius:5px;transition:background-color 0.3s}.button:hover{background-color:#555}form{display:flex;flex-direction:column;gap:10px}label{text-align:left}input[type='text'],input[type='password']{padding:10px;border:1px solid#ccc;border-radius:5px}input[type='submit']{padding:10px 20px;border:none;background-color:#333;color:white;cursor:pointer;border-radius:5px;transition:background-color 0.3s}input[type='submit']:hover{background-color:#555}.fixed-button{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);z-index:1000}</style></head><body><div class='container'><h1>音乐配置</h1><form action='/saveMusic'method='post'><label for='musicName'>音乐名称:</label><input type='text'id='musicName'name='musicName'required><label for='musicId'>音乐 ID:</label><input type='text'id='musicId'name='musicId'required><input type='submit'value='保存 / 更新'class='button'></form><form action='/deleteMusic'method='post'><label for='musicName'>要删除的音乐名称:</label><input type='text'id='musicName'name='musicName'><input type='submit'value='删除'class='button'></form><a href='/listMusic'class='button'>已保存的音乐</a><a href='/'class='button fixed-button'>返回首页</a></div></body></html>";

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// 大模型配置页面处理
esp_err_t model_get_handler(httpd_req_t *req)
{
    const char* html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport'content='width=device-width, initial-scale=1.0'><title>大模型配置</title><style>body{font-family:Arial,sans-serif;background-color:#f0f0f0;display:flex;justify-content:center;align-items:center;height:100vh;margin:0}.container{background-color:#fff;padding:20px;border-radius:8px;box-shadow:0 0 10px rgba(0,0,0,0.1);text-align:center;width:90%;max-width:400px;display:flex;flex-direction:column;gap:20px}h1{color:#333}.button{display:inline-block;padding:10px 20px;margin:10px;border:none;background-color:#333;color:white;text-decoration:none;cursor:pointer;border-radius:5px;transition:background-color 0.3s}.button:hover{background-color:#555}form{display:flex;flex-direction:column;gap:10px}label{text-align:left}input[type='text'],input[type='password'],select{padding:10px;border:1px solid#ccc;border-radius:5px}input[type='submit']{padding:10px 20px;border:none;background-color:#333;color:white;cursor:pointer;border-radius:5px;transition:background-color 0.3s}input[type='submit']:hover{background-color:#555}.fixed-button{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);z-index:1000}</style></head><body><div class='container'><h1>大模型配置</h1><form action='/saveLLM'method='post'><label for='modelSupplier'>模型供应商:</label><select id='modelSupplier'name='modelSupplier'><option value='ChatGPT'>ChatGPT</option><option value='Claude'>Claude</option><option value='Gemini'>Gemini</option><option value='通义千问'>通义千问</option><option value='文心一言'>文心一言</option><option value='智谱AI'>智谱AI</option></select><label for='apiUrl'>API URL:</label><input type='text'id='apiUrl'name='apiUrl'required><label for='apiKey'>API KEY:</label><input type='text'id='apiKey'name='apiKey'required><input type='submit'value='保存 / 更新'class='button'></form><a href='/listLLM'class='button'>已保存的LLM参数</a><a href='/'class='button fixed-button'>返回首页</a></div></body></html>";

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// favicon.ico处理器
esp_err_t favicon_get_handler(httpd_req_t *req)
{
    // 返回一个简单的1x1像素透明图标
    const char* favicon_data = "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A\x00\x00\x00\x0D\x49\x48\x44\x52\x00\x00\x00\x01\x00\x00\x00\x01\x08\x06\x00\x00\x00\x1F\x15\xC4\x89\x00\x00\x00\x0A\x49\x44\x41\x54\x78\x9C\x63\x00\x01\x00\x00\x05\x00\x01\x0D\x0A\x2D\xB4\x00\x00\x00\x00\x49\x45\x4E\x44\xAE\x42\x60\x82";

    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, favicon_data, 67);
    return ESP_OK;
}

// 通用错误处理器
esp_err_t error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    const char* error_page = "<!DOCTYPE html><html><head><title>错误</title></head><body><h1>页面未找到</h1><p><a href='/'>返回首页</a></p></body></html>";

    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_status(req, "404 Not Found");
    httpd_resp_send(req, error_page, HTTPD_RESP_USE_STRLEN);
    return ESP_FAIL;
}

// 初始化WiFi AP模式
esp_err_t init_wifi_ap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = strlen(ap_ssid),
            .channel = 1,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    strcpy((char*)wifi_config.ap.ssid, ap_ssid);
    strcpy((char*)wifi_config.ap.password, ap_password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    isSoftAPStarted = true;
    ESP_LOGI(TAG, "WiFi AP started. SSID:%s password:%s", ap_ssid, ap_password);
    return ESP_OK;
}

// 初始化Web服务器
esp_err_t init_web_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // 增加缓冲区大小以处理较长的HTTP请求头
    config.stack_size = 16384;         // 大幅增加任务栈大小
    config.task_priority = 5;          // 设置任务优先级
    config.recv_wait_timeout = 10;     // 接收超时时间
    config.send_wait_timeout = 10;     // 发送超时时间

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");

        // 注册URI处理器
        httpd_uri_t root = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = root_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &root);

        httpd_uri_t wifi = {
            .uri       = "/wifi",
            .method    = HTTP_GET,
            .handler   = wifi_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &wifi);

        httpd_uri_t music = {
            .uri       = "/music",
            .method    = HTTP_GET,
            .handler   = music_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &music);

        httpd_uri_t model = {
            .uri       = "/model",
            .method    = HTTP_GET,
            .handler   = model_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &model);

        // 注册favicon处理器
        httpd_uri_t favicon = {
            .uri       = "/favicon.ico",
            .method    = HTTP_GET,
            .handler   = favicon_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &favicon);

        // 注册错误处理器
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, error_handler);
        httpd_register_err_handler(server, HTTPD_431_REQ_HDR_FIELDS_TOO_LARGE, handle_large_header_error);

        isWebServerStarted = true;
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return ESP_FAIL;
}

// 主要的Web初始化函数
esp_err_t openWeb(void)
{
    // 初始化NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 初始化WiFi AP
    ESP_ERROR_CHECK(init_wifi_ap());

    // 初始化Web服务器
    ESP_ERROR_CHECK(init_web_server());

    ESP_LOGI(TAG, "Web 服务器成功运行");
    return ESP_OK;
}