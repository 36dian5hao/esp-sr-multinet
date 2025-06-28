
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
#include "I2Sto.h"
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


#define TAG "app"
#define ESP_MN_PREFIX "mn"

int detect_flag = 0;
static esp_afe_sr_iface_t *afe_handle = NULL;
static volatile int task_flag = 0;

// Multinet相关变量
static esp_mn_iface_t *multinet = NULL;
static model_iface_data_t *model_data = NULL;
static afe_fetch_result_t *res = NULL;  // 全局变量，供检测任务使用



void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;

    int feed_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int feed_nch = afe_handle->get_feed_channel_num(afe_data);
    // assert(feed_nch<feed_channel);
    int16_t *feed_buff = (int16_t *) malloc(feed_chunksize * feed_nch * sizeof(int16_t));
            

    assert(feed_buff);
    while (task_flag) {
        size_t bytesIn = 0;
        // esp_err_t result = i2s_read(I2S_NUM_0, &sBuffer, bufferLen * sizeof(int16_t), &bytesIn, portMAX_DELAY);
        
        esp_err_t result = i2s_read(I2S_NUM_0, feed_buff, feed_chunksize * feed_nch * sizeof(int16_t), &bytesIn, portMAX_DELAY);
    
        afe_handle->feed(afe_data, feed_buff);
    }
    if (feed_buff) {
        free(feed_buff);
        feed_buff = NULL;
    }
    vTaskDelete(NULL);
}

void detect_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    int16_t *buff = malloc(afe_chunksize * sizeof(int16_t));
    assert(buff);
    printf("------------MultiNet detect start------------\n");

    while (task_flag) {
        res = afe_handle->fetch(afe_data);  // 使用全局变量res

        if (!res || res->ret_value == ESP_FAIL) {
            printf("fetch error!\n");
            break;
        }

        // 直接使用MultiNet进行拼音唤醒词检测，不依赖WakeNet
        if (model_data && res->data) {
            esp_mn_state_t mn_state = multinet->detect(model_data, res->data);

            if (mn_state == ESP_MN_STATE_DETECTED) {
                esp_mn_results_t *mn_result = multinet->get_results(model_data);

                if (mn_result != NULL && mn_result->num > 0) {
                    int command_id = mn_result->phrase_id[0];
                    printf("检测到拼音唤醒词 ID: %d\n", command_id);

                    if (command_id == 0) {
                        printf("执行命令: 你好小余\n");
                        // 在这里添加您的处理逻辑
                    }
                    else if (command_id == 1) {
                        printf("执行命令: 小余小余\n");
                        // 在这里添加您的处理逻辑
                    }
                    else if (command_id == 2) {
                        printf("执行命令: 36号\n");
                        // 在这里添加您的处理逻辑
                    }
                }
            }
        }
    }
    if (buff) {
        free(buff);
        buff = NULL;
    }
    vTaskDelete(NULL);
}





int app_main()
{
    /* 初始化NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    Init_i2s();
  
    srmodel_list_t *models = esp_srmodel_init("model");

    // 遍历所有加载的模型，初始化multinet
    for (int i = 0; i < models->num; i++) {
        ESP_LOGI(TAG, "Model %d: %s", i, models->model_name[i]);

        // 查找multinet模型 (mn5q8_cn)
        if (strstr(models->model_name[i], ESP_MN_PREFIX) != NULL) {
            // 获取multinet模型句柄
            multinet = esp_mn_handle_from_name(models->model_name[i]);
            if (!multinet) {
                ESP_LOGI("MULTINET", "failed to create multinet handle");
                continue;
            }

            // 加载模型数据
            model_data = multinet->create(models->model_name[i], 6000);
            if (!model_data) {
                ESP_LOGI("MULTINET", "failed to create model_data handle");
                continue;
            }

            ESP_LOGI("MULTINET", "Successfully loaded model: %s", models->model_name[i]);
        }
    }

    // 如果成功加载了multinet模型，添加自定义唤醒词
    if (model_data) {
        // 初始化命令系统
        esp_mn_commands_alloc(multinet, model_data);

        // 清除现有命令并添加自定义唤醒词（使用拼音+空格的方式）
        esp_mn_commands_clear();
        esp_mn_commands_add(1, "ni hao xiao yu");    // 你好小余
        esp_mn_commands_add(2, "xiao yu xiao yu");  // 小余小余
        esp_mn_commands_add(3, "san shi liu hao");   // 36号
        esp_mn_commands_update();

        // 打印添加的唤醒词
        esp_mn_active_commands_print();
    }

    // 配置AFE为仅MultiNet模式，不使用WakeNet
    afe_config_t *afe_config = afe_config_init("M", models, AFE_TYPE_SR, AFE_MODE_LOW_COST);
    if (afe_config) {
        // 禁用WakeNet，仅使用MultiNet
        afe_config->wakenet_init = false;
        afe_config->aec_init = false;  // 如果不需要回声消除
    }

    afe_handle = esp_afe_handle_from_config(afe_config);
    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(afe_config);

    afe_config_free(afe_config);
    
    task_flag = 1;

    


    xTaskCreatePinnedToCore(&feed_Task, "feed", 8 * 1024, (void*)afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(&detect_Task, "detect", 4 * 1024, (void*)afe_data, 5, NULL, 1);

    return 0;
}