#ifndef BACKTASK_H
#define BACKTASK_H

#include <stdio.h>
#include <stdbool.h>  // 添加bool类型支持
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "spi_flash_mmap.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "Web_Scr_set.h"  // 现在可以包含了
#include "driver/gpio.h"  // 包含按键的头文件
#include "esp_log.h"
#include "wifi/wifi.h"
#include "models/model.h"
#include "I2S/I2Sto.h"


#define GPIO_BOOT 0  // boot按键


extern int wake;


// 定义任务回调函数类型
typedef void (*TaskCallback)(void);



// 标志位结构
typedef struct {
    int task1_flag;  // 任务1的标志位
    int task2_flag;  // 任务2的标志位
    int task3_flag;  // 任务3的标志位
} FlagStruct;



// 任务队列结构
typedef struct {
    TaskCallback tasks[10];  // 最多支持10个任务
    int front;  // 队列头索引
    int rear;   // 队列尾索引
    int count;  // 当前队列中任务数量
} TaskQueue;

// 函数声明
void initFlagStruct(FlagStruct* flagStruct);
void initQueue(TaskQueue* queue);
bool isQueueEmpty(TaskQueue* queue);
void enqueue(TaskQueue* queue, TaskCallback task);
TaskCallback dequeue(TaskQueue* queue);
void task1(void);
void task2(void);
void task3(void);
void executeAllTasks(void);
void initTaskQueue(void);

// 按键中断处理函数声明
void IRAM_ATTR boot_button_isr_handler(void* arg);
void init_boot_button_interrupt(void);

// FreeRTOS任务函数声明
void freertos_task1(void *pvParameters);
void freertos_task2(void *pvParameters);



// 全局变量声明
extern TaskQueue taskQueue;





#endif