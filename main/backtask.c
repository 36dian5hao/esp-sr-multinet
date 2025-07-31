#include "backtask.h"

// static const char *TAG = "anjian";

int wake = 0;

// 全局变量定义
TaskQueue taskQueue = {0};

//标志位初始化
void initFlagStruct(FlagStruct* flagStruct) {
    flagStruct->task1_flag = 0;
    flagStruct->task2_flag = 0;
    flagStruct->task3_flag = 0;
}

// 队列操作函数
void initQueue(TaskQueue* queue) {
    queue->front = 0;
    queue->rear = 0;
    queue->count = 0;
}

// 判断队列是否为空
bool isQueueEmpty(TaskQueue* queue) {
    return queue->count == 0;
}

// 添加任务到队列
void enqueue(TaskQueue* queue, TaskCallback task) {
    if (queue->count < 10) {
        queue->tasks[queue->rear] = task;
        queue->rear = (queue->rear + 1) % 10;
        queue->count++;
    }
}

// 删除队列中的任务
TaskCallback dequeue(TaskQueue* queue) {
    if (!isQueueEmpty(queue)) {
        TaskCallback task = queue->tasks[queue->front];
        queue->front = (queue->front + 1) % 10;
        queue->count--;
        return task;
    }
    return NULL;
}

// 任务函数
void task1() {
    printf("任务一正在运行\n");
}

void task2() {
    printf("任务二正在运行\n");
}

void task3() {
    printf("任务三正在运行\n");
}

// 执行队列中所有任务的函数
void executeAllTasks() {
    printf("开始执行任务队列\n");

    while (!isQueueEmpty(&taskQueue)) {
        // 执行队列中的下一个任务
        TaskCallback task = dequeue(&taskQueue);
        if (task != NULL) {
            task();
            vTaskDelay(pdMS_TO_TICKS(1000)); // 任务间延迟1秒
        }
    }

    printf("所有任务执行完成\n");
}

// 初始化任务队列并添加三个任务
void initTaskQueue() {
    initQueue(&taskQueue);

    // 添加三个任务到队列
    enqueue(&taskQueue, task1);
    enqueue(&taskQueue, task2);
    enqueue(&taskQueue, task3);

    //printf("任务队列初始化完成，已添加3个任务\n");
}

// FreeRTOS任务一
void freertos_task1(void *pvParameters) {
    int counter = 0;

    while (1) {
        printf("FreeRTOS任务一正在运行\n");

        // 检查wake标志位
        if (wake == 1) {
            ESP_LOGI("wakeup_task", "检测到唤醒信号!");
            // 这里可以添加您的唤醒处理逻辑
            //wake_main(); // 如果有这个函数的话
            wake = 0; // 重置标志位
        } else {
            printf("没有检测到唤醒信号\n");
        }
        vTaskDelay(pdMS_TO_TICKS(2000)); // 每2秒执行一次
    }
}

// FreeRTOS任务二
void freertos_task2(void *pvParameters) {
    int counter = 0;

    while (1) {
        printf("FreeRTOS任务二正在运行 - 计数: %d\n", counter++);
        vTaskDelay(pdMS_TO_TICKS(3000)); // 每3秒执行一次
    }
}



// 按键中断处理函数
void IRAM_ATTR boot_button_isr_handler(void* arg) {
    // 在中断中只做最简单的操作
    //printf("Boot按键按下！\n");
    //ESP_LOGI(TAG, "Boot按键按下!");
    wake = 1;  // 设置唤醒标志
}

// 初始化按键中断
void init_boot_button_interrupt(void) {
    // 配置GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << GPIO_BOOT,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE  // 下降沿触发（按键按下）
    };
    gpio_config(&io_conf);

    // 安装GPIO中断服务
    gpio_install_isr_service(0);

    // 添加中断处理函数
    gpio_isr_handler_add(GPIO_BOOT, boot_button_isr_handler, NULL);

    //printf("Boot按键中断初始化完成\n");
}