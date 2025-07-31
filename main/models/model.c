#include "model.h"

int detect_flag = 0;
esp_afe_sr_iface_t *afe_handle = NULL;
volatile int task_flag = 0;

// Multinet相关变量
esp_mn_iface_t *multinet = NULL;
model_iface_data_t *model_data = NULL;
afe_fetch_result_t *res = NULL;  // 全局变量，供检测任务使用

// LED初始化函数
void init_led_gpio(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_GPIO, 0); // 初始状态关闭
}

// LED控制函数
void set_led_state(bool state)
{
    gpio_set_level(LED_GPIO, state ? 1 : 0);
    printf("LED状态: %s\n", state ? "开启" : "关闭");
}

void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;

    int feed_chunksize = afe_handle->get_feed_chunksize(afe_data);//获取feed_chunksize
    int feed_nch = afe_handle->get_feed_channel_num(afe_data);//获取feed_nch
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
            // 检测拼音唤醒词
            esp_mn_state_t mn_state = multinet->detect(model_data, res->data);

            if (mn_state == ESP_MN_STATE_DETECTED) {
                // 获取检测结果
                esp_mn_results_t *mn_result = multinet->get_results(model_data);

                // 检查结果是否有效
                if (mn_result != NULL && mn_result->num > 0) {
                    int command_id = mn_result->phrase_id[0];
                    printf("检测到拼音唤醒词 ID: %d\n", command_id);

                    if (command_id == 0) {
                        printf("执行命令: 关闭灯光\n");
                        set_led_state(false); // 关闭LED
                    }
                    else if (command_id == 1) {
                        printf("执行命令: 小余小余\n");
                        set_led_state(true); // 开启LED
                        vTaskDelay(pdMS_TO_TICKS(500));
                        set_led_state(false); // 关闭LED
                        // 在这里添加您的处理逻辑
                    }
                    else if (command_id == 2) {
                        printf("执行命令: 打开灯光\n");
                        set_led_state(true); // 开启LED
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





int wake_main()
{
    /* 初始化NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    
    // 初始化LED GPIO
    init_led_gpio();
    
    Init_i2s();//初始化I2S
  
    // 初始化模型列表
    srmodel_list_t *models = esp_srmodel_init("model");

    // 遍历所有加载的模型，初始化multinet
    for (int i = 0; i < models->num; i++) {
        ESP_LOGI(TAG, "Model %d: %s", i, models->model_name[i]);

        // 查找multinet模型 (mn5q8_cn)
        if (strstr(models->model_name[i], ESP_MN_PREFIX) != NULL) {
            // 获取multinet模型句柄 
            multinet = esp_mn_handle_from_name(models->model_name[i]);
            if (!multinet) {
                ESP_LOGI("MULTINET", "创建多网络句柄失败");
                continue;
            }

            // 加载模型数据
            model_data = multinet->create(models->model_name[i], 6000);
            if (!model_data) {
                ESP_LOGI("MULTINET", "创建 model_data 句柄失败");
                continue;
            }

            ESP_LOGI("MULTINET", "模型已成功加载: %s", models->model_name[i]);
        }
    }

    // 如果成功加载了multinet模型，添加自定义唤醒词
    if (model_data) {
        // 初始化命令系统
        esp_mn_commands_alloc(multinet, model_data);

        // 清除现有命令并添加自定义唤醒词（使用拼音+空格的方式）
        esp_mn_commands_clear();
        esp_mn_commands_add(1, "guan bi deng guang");    // 关闭灯光
        esp_mn_commands_add(2, "xiao yu xiao yu");  // 小余小余
        esp_mn_commands_add(3, "da kai deng guang");   // 打开灯光
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

    afe_handle = esp_afe_handle_from_config(afe_config);//创建AFE句柄
    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(afe_config);//创建AFE数据

    afe_config_free(afe_config);//释放配置内存
    
    task_flag = 1;

    xTaskCreatePinnedToCore(&feed_Task, "feed", 8 * 1024, (void*)afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(&detect_Task, "detect", 4 * 1024, (void*)afe_data, 5, NULL, 1);

    return 0;
}
