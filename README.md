# ESP-SR 拼音唤醒词识别项目

###注意：观看我视频来的小伙伴请去下拉版本2.0.0的代码。

## 📖 项目简介

这是一个基于ESP32的智能语音识别项目，使用Espressif的ESP-SR语音识别框架，实现了纯拼音唤醒词识别功能。项目摒弃了传统的WakeNet预设唤醒词模式，直接使用MultiNet模型进行自定义拼音唤醒词检测。

## 🛠️ 技术栈

### 硬件平台
- **主控芯片**: ESP32系列 (ESP32/ESP32-S3推荐)
- **音频输入**: I2S数字麦克风或模拟麦克风+ADC
- **内存要求**: 至少4MB Flash (用于存储语音模型)
- **RAM要求**: 建议512KB以上SRAM

### 软件框架
- **开发框架**: ESP-IDF v4.4+/最好是v5.4.1
- **语音识别**: ESP-SR (Espressif Speech Recognition)
- **实时操作系统**: FreeRTOS
- **音频处理**: ESP-AFE (Audio Front-End)
- **语音模型**: MultiNet5 量化版本 (mn5q8_cn)

### 核心组件
```
├── ESP-IDF Framework
├── ESP-SR Components
│   ├── ESP-AFE (音频前端处理)
│   ├── MultiNet5 (中文语音识别模型)
│   └── Speech Commands (命令管理系统)
├── FreeRTOS Tasks
│   ├── Audio Feed Task (音频数据输入)
│   └── Detection Task (语音检测与识别)
└── I2S Driver (音频接口驱动)
```

## ✨ 核心特性

- 🎯 **纯拼音识别**: 支持自定义中文拼音唤醒词
- 🚀 **低延迟响应**: 实时音频处理，快速识别响应
- 💾 **内存优化**: 使用量化模型，减少内存占用
- 🔧 **易于扩展**: 简单的API接口，方便添加新的唤醒词
- 📱 **无需网络**: 完全离线运行，保护隐私
- ⚡ **低功耗**: 优化的算法，适合电池供电设备

## 🎵 当前支持的唤醒词

| 唤醒词ID | 拼音格式 | 中文含义 | 功能说明 |
|---------|----------|----------|----------|
| 1 | `ni hao xiao yu` | 你好小余 | 示例功能1 |
| 2 | `xiao yu xiao yu` | 小余小余 | 示例功能2 |
| 3 | `san shi liu hao` | 36号 | 示例功能3 |

## 🚀 快速开始

### 环境准备

1. **安装ESP-IDF**
   ```bash
   # 下载ESP-IDF
   git clone --recursive https://github.com/espressif/esp-idf.git
   cd esp-idf
   ./install.sh
   . ./export.sh
   ```

2. **克隆项目**
   ```bash
   git clone <your-project-repo>
   cd esp-sr-test002
   ```

### 配置项目

1. **配置目标芯片**
   ```bash
   idf.py set-target esp32s3  # 或 esp32
   ```

2. **配置项目参数**
   ```bash
   idf.py menuconfig
   ```
   
   在menuconfig中进行以下配置：
   - 进入 `ESP Speech Recognition`
   - 启用 `chinese_recognition(mn5q8_cn)`
   - 禁用所有WakeNet模型
   - 确保 `CONFIG_SR_MN_CN_MULTINET5_RECOGNITION_QUANT8=y`

3. **分区配置**
   
   确保`partitions.csv`包含足够的模型存储空间：
   ```csv
   # Name,   Type, SubType, Offset,  Size, Flags
   nvs,      data, nvs,     0x9000,  0x6000,
   phy_init, data, phy,     0xf000,  0x1000,
   factory,  app,  factory, 0x10000, 1M,
   model,    data, spiffs,  ,        4M,
   ```

### 编译和烧录

```bash
# 编译项目
idf.py build

# 烧录到设备
idf.py flash

# 监控串口输出
idf.py monitor

# 或者一次性执行
idf.py build flash monitor
```

## 📋 使用方法

### 基本使用

1. **启动设备**: 上电后设备自动初始化
2. **等待就绪**: 看到 `MultiNet detect start` 表示系统就绪
3. **语音唤醒**: 清晰说出配置的拼音唤醒词
4. **查看响应**: 串口会输出识别结果和执行的操作

### 添加自定义唤醒词

在 `main/blink_example_main.c` 中修改：

```c
// 添加新的唤醒词
esp_mn_commands_clear();
esp_mn_commands_add(1, "ni hao xiao yu");     // 你好小余
esp_mn_commands_add(2, "xiao yu xiao yu");   // 小余小余
esp_mn_commands_add(3, "san shi liu hao");   // 36号
esp_mn_commands_add(4, "kai deng");          // 开灯 (新增)
esp_mn_commands_update();
```

然后在检测任务中添加对应的处理逻辑：

```c
if (command_id == 2) {
    printf("36号\n");
}
if (command_id == 3) {
    printf("执行命令: 开灯\n");
    // 添加开灯的具体实现
    gpio_set_level(LED_PIN, 1);
}
```

## 🔧 项目结构

```
esp-sr-test002/
├── main/
│   ├── blink_example_main.c    # 主程序文件
│   ├── I2S/
│   │   └── I2Sto.h            # I2S音频接口头文件
│   └── CMakeLists.txt
├── managed_components/
│   └── espressif__esp-sr/     # ESP-SR组件
├── partitions.csv             # 分区表配置
├── sdkconfig                  # 项目配置
├── CMakeLists.txt            # 构建配置
└── README.md                 # 项目说明
```

## 🎛️ 核心API说明

### 模型初始化
```c
srmodel_list_t *models = esp_srmodel_init("model");
multinet = esp_mn_handle_from_name(model_name);
model_data = multinet->create(model_name, 6000);
```

### 命令管理
```c
esp_mn_commands_alloc(multinet, model_data);  // 初始化命令系统
esp_mn_commands_add(id, "pinyin_string");     // 添加命令
esp_mn_commands_update();                     // 更新命令列表
```

### 语音检测
```c
esp_mn_state_t state = multinet->detect(model_data, audio_data);
if (state == ESP_MN_STATE_DETECTED) {
    esp_mn_results_t *result = multinet->get_results(model_data);
    // 处理识别结果
}
```

## 📊 性能指标

- **识别延迟**: < 500ms
- **内存占用**: ~50KB SRAM
- **模型大小**: ~2.3MB Flash
- **采样率**: 16kHz
- **音频格式**: 16-bit PCM
- **支持语言**: 中文拼音

## 🔍 故障排除

### 常见问题

1. **模型加载失败**
   - 检查分区空间是否足够
   - 确认模型文件完整性

2. **无法识别语音**
   - 检查麦克风连接--INMP441_SLK_IO1 G15
                    INMP441_WS_IO1 G16
                    INMP441_SD_IO1 G17
                    INMP441_L/R  悬空不接
                    vcc 3.3v    GND  GND
   - 确认I2S配置正确
   - 在安静环境下测试

3. **编译错误**
   - 确认ESP-IDF版本兼容
   - 检查组件依赖关系

### 调试技巧

- 启用详细日志: `idf.py menuconfig` → `Component config` → `Log output`
- 监控内存使用: 添加 `esp_get_free_heap_size()` 调用
- 检查音频数据: 在feed任务中添加数据验证

## 🤝 贡献指南

欢迎提交Issue和Pull Request来改进项目！

1. Fork 项目
2. 创建特性分支
3. 提交更改
4. 推送到分支
5. 创建Pull Request

## 来源

本项目基于https://blog.csdn.net/m0_75132712/article/details/148940472，薛定谔的寄存器博主的博文，和哔站博主“亥时弈输灯花”【ESP-SR从零开始的（语音唤醒和命令识别）】https://www.bilibili.com/video/BV1g49tYqEVe?vd_source=806b84ec9ea531067da4ce307042a4a4两人的教程，感谢两位作者的分享。

---

**注意**: 本项目仅供学习和研究使用，商业使用请遵循相关许可证要求。
