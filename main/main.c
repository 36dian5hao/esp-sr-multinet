#include <stdio.h>
#include <stdbool.h>  // 添加bool类型支持
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"//
#include "spi_flash_mmap.h"//
#include "esp_mac.h"//
#include "esp_wifi.h"
#include "Web_Scr_set.h"  // 现在可以包含了

#include "backtask.h"
#include "wifi/wifi.h"
#include "models/model.h"
#include "I2S/I2Sto.h"



void app_main(void)
{

    // printf("开始执行WiFi任务\n");
    // ESP_ERROR_CHECK( nvs_flash_init() );//初始化NVS
    // initialise_wifi();


    printf("开始唤醒模式\n");
    wake_main();

    // printf("开始执行热点任务\n");
    // //openWeb();  // 启动Web服务器

    // printf("开始执行按键任务\n");


    // // 初始化按键中断
    // init_boot_button_interrupt();

    // printf("开始执行任务队列\n");

    // // 初始化任务队列并添加三个任务
    // initTaskQueue();

    // // 执行队列中的所有任务
    // executeAllTasks();


    // //printf("多任务开始执行\n");

    
    //  // 创建FreeRTOS任务一
    // xTaskCreate(freertos_task1,"FreeRTOS_Task1",2048,NULL,5,NULL);

    // // 创建FreeRTOS任务二
    // xTaskCreate(
    //     freertos_task2,     // 任务函数
    //     "FreeRTOS_Task2",   // 任务名称
    //     2048,               // 堆栈大小
    //     NULL,               // 任务参数
    //     5,                  // 任务优先级
    //     NULL                // 任务句柄
    // );

    printf("程序执行完成，进入循环等待\n");

    // 保持主任务运行
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(5000)); // 每5秒打印一次状态
        printf("主程序运行中...\n");
    }
}
