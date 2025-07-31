# ESP32 AI LLM 音频处理与语音识别技术文档

## 项目概述

ESP32 AI LLM 是一个基于 ESP32 微控制器的智能语音助手项目，支持多种大语言模型，提供完整的语音交互体验。本文档重点分析项目中的音频接收和语音识别API调用部分的技术实现。

## 系统架构

### 硬件架构
- **主控制器**: ESP32/ESP32-S3
- **音频输入**: INMP441 全向麦克风
- **音频输出**: MAX98357 I2S音频放大器
- **显示**: 1.8寸RGB_TFT屏幕
- **离线唤醒**: ASRPRO开发板（可选）

### 软件架构
```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   音频采集层    │    │   音频处理层    │    │   语音识别层    │
│   (I2S + INMP441)│ -> │   (Audio1类)    │ -> │  (讯飞STT API)  │
└─────────────────┘    └─────────────────┘    └─────────────────┘
                                                       │
                                                       v
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   语音合成层    │ <- │   大模型处理    │ <- │   文本处理层    │
│  (百度TTS API)  │    │   (多种LLM)     │    │ (指令识别/处理) │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## 音频接收系统详细分析

### 1. I2S接口配置

I2S（Inter-IC Sound）是用于数字音频设备之间传输的串行总线接口。

#### I2S类实现 (I2S.cpp)

```cpp
// I2S配置参数
#define SAMPLE_RATE (16000)  // 采样率16kHz
const i2s_port_t I2S_PORT = I2S_NUM_0;

I2S::I2S(uint8_t PIN_I2S_BCLK, uint8_t PIN_I2S_LRC, uint8_t PIN_I2S_DIN)
{
    BITS_PER_SAMPLE = I2S_BITS_PER_SAMPLE_32BIT;
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),  // 主模式，接收
        .sample_rate = SAMPLE_RATE,                           // 16kHz采样率
        .bits_per_sample = BITS_PER_SAMPLE,                   // 32位采样
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,         // 立体声格式
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = 0,
        .dma_buf_count = 16,                                  // DMA缓冲区数量
        .dma_buf_len = 60                                     // 每个缓冲区长度
    };
}
```

**技术要点**：
- **采样率**: 16kHz，符合语音识别的标准要求
- **位深度**: 32位采样，提供高精度音频数据
- **DMA缓冲**: 16个缓冲区，每个60字节，确保音频流的连续性
- **通信格式**: 标准I2S格式，MSB优先

#### 引脚配置

```cpp
// ESP32-S3版本的引脚定义
#define PIN_I2S_BCLK 2   // 时钟线，对应INMP441的SCK
#define PIN_I2S_LRC  1   // 声道选择线，对应INMP441的WS
#define PIN_I2S_DIN 42   // 数据线，对应INMP441的SD

// ESP32版本的引脚定义
#define PIN_I2S_BCLK 4
#define PIN_I2S_LRC 15
#define PIN_I2S_DIN 22
```

### 2. Audio1音频录制类

Audio1类负责音频数据的采集、处理和格式转换。

#### 类结构设计

```cpp
class Audio1
{
    I2S *i2s;                                    // I2S接口对象
    static const int headerSize = 44;            // WAV文件头大小
    static const int i2sBufferSize = 5120;       // I2S缓冲区大小
    char i2sBuffer[i2sBufferSize];               // I2S数据缓冲区
    
public:
    static const int wavDataSize = 30000;        // WAV数据总大小
    static const int dividedWavDataSize = i2sBufferSize / 4;  // 分段大小
    char **wavData;                              // 分段存储音频数据
    byte paddedHeader[headerSize + 4] = {0};     // WAV文件头
    
    void Record();                               // 录音函数
    float calculateRMS(uint8_t *buffer, int bufferSize);  // 计算RMS值
};
```

#### 音频录制核心函数

```cpp
void Audio1::Record()
{
    // 从I2S接口读取原始音频数据
    i2s->Read(i2sBuffer, i2sBufferSize);
    
    // 数据格式转换：从32位立体声转换为16位单声道
    for (int i = 0; i < i2sBufferSize / 8; ++i)
    {
        wavData[0][2 * i] = i2sBuffer[8 * i + 2];      // 低字节
        wavData[0][2 * i + 1] = i2sBuffer[8 * i + 3];  // 高字节
    }
}
```

**数据转换逻辑**：
- 原始数据：32位立体声，8字节/样本
- 目标数据：16位单声道，2字节/样本
- 提取策略：从每8字节中提取第2、3字节作为16位音频样本

#### RMS值计算

```cpp
float Audio1::calculateRMS(uint8_t *buffer, int bufferSize)
{
    float sum = 0;
    int16_t sample;
    
    // 每次处理两个字节，16位
    for (int i = 0; i < bufferSize; i += 2)
    {
        // 从缓冲区中读取16位样本，注意字节顺序
        sample = (buffer[i + 1] << 8) | buffer[i];
        
        // 计算平方和
        sum += sample * sample;
    }
    
    // 计算平均值并返回RMS值
    sum /= (bufferSize / 2);
    return sqrt(sum);
}
```

**RMS用途**：
- 音频信号强度检测
- 噪声门限判断
- 语音活动检测（VAD）

## 语音识别API调用系统

### 1. WebSocket连接管理

项目使用WebSocket与讯飞语音识别服务进行实时通信。

#### 连接初始化

```cpp
void ConnServer1()
{
    // 设置回调函数
    webSocketClient1.onMessage(onMessageCallback1);    // 消息回调
    webSocketClient1.onEvent(onEventsCallback1);       // 事件回调
    
    // 连接到讯飞STT服务
    Serial.println("开始连接讯飞STT语音转文字服务......");
    if (webSocketClient1.connect(url1.c_str()))
    {
        Serial.println("连接成功！Connected to server1(Xunfei STT)!");
    }
}
```

#### URL生成与鉴权

```cpp
// 讯飞API参数
String APPID = "60118755";                             // App ID
String APISecret = "NDVmNzUwYTlkMjM2ZjA1MzJjYWE0MTdk"; // API Secret
String APIKey = "beb4e4bdecc5404c140f17ad37b4ee8f";    // API Key
String websockets_server1 = "ws://iat-api.xfyun.cn/v2/iat";

// 生成带鉴权的WebSocket URL
url1 = getUrl(websockets_server1, "iat-api.xfyun.cn", "/v2/iat", Date);
```

### 2. 音频数据流式传输

#### 录音事件处理

```cpp
void onEventsCallback1(WebsocketsEvent event, String data)
{
    if (event == WebsocketsEvent::ConnectionOpened)
    {
        // 初始化录音参数
        int silence = 0;      // 静音计数
        int firstframe = 1;   // 首帧标识
        int voicebegin = 0;   // 语音开始标识
        int voice = 0;        // 语音计数
        int null_voice = 0;   // 静音计数
        
        // 创建JSON文档用于数据传输
        StaticJsonDocument<2000> doc;
        
        // 录音循环
        while (1)
        {
            // 录制音频数据
            audio1.Record();
            
            // 计算RMS值进行语音活动检测
            float rms = audio1.calculateRMS((uint8_t *)audio1.wavData[0], 1280);
            
            // 噪声抑制
            if (rms > 1000) rms = 8.6;
            
            // 语音活动检测逻辑
            if (rms < noise)  // 低于噪声门限
            {
                null_voice++;
                if (voicebegin == 1) silence++;
            }
            else  // 检测到语音
            {
                if (null_voice > 0) null_voice--;
                voice++;
                if (voice >= 10) voicebegin = 1;
                else voicebegin = 0;
                silence = 0;
            }
            
            // 静音检测：连续8个周期静音则结束录音
            if (silence == 8)
            {
                // 发送结束帧
                data["status"] = 2;
                data["format"] = "audio/L16;rate=16000";
                data["audio"] = base64::encode((byte *)audio1.wavData[0], 1280);
                data["encoding"] = "raw";
                
                String jsonString;
                serializeJson(doc, jsonString);
                webSocketClient1.send(jsonString);
                break;
            }
        }
    }
}
```

#### 音频数据帧格式

**首帧数据**：
```cpp
if (firstframe == 1)
{
    data["status"] = 0;                    // 开始帧
    data["format"] = "audio/L16;rate=16000";  // 16位线性PCM，16kHz
    data["audio"] = base64::encode((byte *)audio1.wavData[0], 1280);  // Base64编码音频
    data["encoding"] = "raw";              // 原始编码
    
    JsonObject common = doc.createNestedObject("common");
    common["app_id"] = appId1.c_str();     // 应用ID
    
    JsonObject business = doc.createNestedObject("business");
    business["domain"] = "iat";            // 语音转写领域
    business["language"] = language.c_str(); // 语言设置
    business["accent"] = "mandarin";       // 口音设置
    business["dwa"] = "wpgs";              // 动态修正
    business["vad_eos"] = 2000;            // 静音检测时长
}
```

**后续帧数据**：
```cpp
else
{
    data["status"] = 1;                    // 中间帧
    data["format"] = "audio/L16;rate=16000";
    data["audio"] = base64::encode((byte *)audio1.wavData[0], 1280);
    data["encoding"] = "raw";
}
```

### 3. 语音识别结果处理

#### 消息回调函数

```cpp
void onMessageCallback1(WebsocketsMessage message)
{
    // 解析JSON响应
    DynamicJsonDocument jsonDocument(4096);
    DeserializationError error = deserializeJson(jsonDocument, message.data());
    
    if (error)
    {
        Serial.println("JSON解析错误");
        return;
    }
    
    // 检查返回码
    if (jsonDocument["code"] != 0)
    {
        Serial.println("API调用失败");
        webSocketClient1.close();
        return;
    }
    
    // 提取识别结果
    JsonArray ws = jsonDocument["data"]["result"]["ws"].as<JsonArray>();
    
    // 处理流式返回内容
    if (jsonDocument["data"]["status"] != 2)
    {
        askquestion = "";  // 清空之前的结果
    }
    
    // 拼接识别文本
    for (JsonVariant i : ws)
    {
        for (JsonVariant w : i["cw"].as<JsonArray>())
        {
            askquestion += w["w"].as<String>();
        }
    }
    
    // 检查是否识别完成
    if (jsonDocument["data"]["status"] == 2)
    {
        Serial.println("识别完成: " + askquestion);
        webSocketClient1.close();
        processAskquestion();  // 处理识别结果
    }
}
```

## 语音活动检测（VAD）算法

### 检测逻辑

```cpp
// VAD参数
int noise = 50;        // 噪声门限值
int silence = 0;       // 静音计数器
int voice = 0;         // 语音计数器
int voicebegin = 0;    // 语音开始标识

// 检测算法
if (rms < noise)  // 信号强度低于门限
{
    null_voice++;
    if (voicebegin == 1) silence++;  // 语音开始后的静音
}
else  // 检测到语音信号
{
    if (null_voice > 0) null_voice--;
    voice++;
    if (voice >= 10) voicebegin = 1;  // 连续10帧确认语音开始
    else voicebegin = 0;
    silence = 0;
}

// 结束条件：连续8帧静音
if (silence == 8)
{
    // 发送结束帧，停止录音
}
```

### 超时处理

```cpp
if(null_voice >= 80)  // 8秒无语音输入
{
    shuaxin = 1;
    awake_flag = 0;
    Answer = "主人，我先退下了，有事再找我喵~";
    response();
    recording = 0;
    webSocketClient1.close();
    return;
}
```

## 系统集成与流程控制

### 对话流程

1. **唤醒检测** → 2. **建立WebSocket连接** → 3. **开始录音** → 4. **实时传输音频** → 5. **接收识别结果** → 6. **处理用户指令** → 7. **生成回复** → 8. **语音合成播放**

### 连续对话机制

```cpp
// 连续对话检测
if (audio2.isplaying == 0 && Answer == "" && subindex == subAnswers.size() && 
    musicplay == 0 && conflag == 1 && hint == 0)
{
    loopcount++;
    StartConversation();  // 自动开始下一轮对话
}
```

## 性能优化与注意事项

### 内存管理
- 使用分段存储避免大块内存分配
- 及时释放WebSocket连接
- 合理设置JSON文档大小

### 实时性保证
- 40ms音频帧间隔
- DMA缓冲区优化
- 中断优先级设置

### 错误处理
- 网络连接异常恢复
- 音频数据校验
- API调用失败重试

## 音频输出系统分析

### Audio2音频播放类

Audio2类负责音频输出，支持多种音频源和格式。

#### 核心配置

```cpp
// Audio2初始化参数
Audio2 audio2(false, 3, I2S_NUM_1);
// 参数说明：
// false: 不使用内部DAC，使用外部I2S设备
// 3: 启用左右声道
// I2S_NUM_1: 使用I2S端口1（避免与录音冲突）
```

#### I2S输出配置

```cpp
// I2S输出配置（Audio2.cpp中）
m_i2s_config.sample_rate = 16000;                    // 采样率16kHz
m_i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;  // 16位采样
m_i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;   // 立体声
m_i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;       // 中断优先级
m_i2s_config.dma_buf_count = 16;                     // DMA缓冲区数量
m_i2s_config.dma_buf_len = 512;                      // 缓冲区长度
m_i2s_config.use_apll = APLL_DISABLE;                // 禁用APLL
m_i2s_config.tx_desc_auto_clear = true;              // 自动清除发送描述符
```

### 语音合成集成

#### TTS API调用

```cpp
// 百度TTS语音合成
audio2.connecttospeech(Answer.c_str(), "zh");

// 支持的功能：
// - 中文语音合成
// - 实时流式播放
// - 音量控制
// - 播放状态监控
```

## 唤醒系统双模式设计

### 1. 在线唤醒模式

使用讯飞STT服务进行唤醒词识别：

```cpp
// 在线唤醒检测
if (audio2.isplaying == 0 && awake_flag == 0 && await_flag == 1)
{
    awake_flag = 1;     // 在线唤醒模式，唤醒成功
    StartConversation();
}
```

**优点**：
- 识别准确率高
- 支持自定义唤醒词
- 无需额外硬件

**缺点**：
- 消耗网络流量
- 依赖网络连接
- 消耗STT服务配额

### 2. 离线唤醒模式（ASRPRO）

使用ASRPRO开发板进行离线唤醒：

```cpp
#ifdef ASRPRO
    #define awake 3     // 唤醒引脚
    #define RX2 19      // UART接收
    #define TX2 20      // UART发送

    // UART接收缓冲区
    const int BUFFER_SIZE = 5;
    char buffer[BUFFER_SIZE];
    int globalIndex = 0;
#endif

// 离线唤醒检测
if (digitalRead(awake) == LOW)  // PA2引脚低电平信号
{
    awake_flag = 1;
    StartConversation();
}
```

**优点**：
- 无需网络连接
- 不消耗在线服务配额
- 响应速度快
- 功耗较低

**缺点**：
- 需要额外硬件
- 唤醒词相对固定
- 增加系统复杂度

### ASRPRO指令处理

```cpp
// 指令验证
bool isValidCommand(const char cmdBuffer[])
{
    return (cmdBuffer[0] == 0xAA && cmdBuffer[1] == 0x55 &&
            cmdBuffer[3] == 0x55 && cmdBuffer[4] == 0xAA);
}

// 指令处理
void processCommand(uint8_t command)
{
    switch (command) {
    case 0x01:  // 唤醒指令
        Serial.println("Wake up command received.");
        conflag = 0;
        awake_flag = 1;
        loopcount++;
        StartConversation();
        break;
    case 0x02:  // 打断指令
        Serial.println("Interrupt command received.");
        // 处理打断逻辑
        break;
    default:
        Serial.println("Unknown command received.");
        break;
    }
}
```

## 数据流与协议分析

### WebSocket数据格式

#### 请求格式（发送到讯飞STT）

```json
{
    "common": {
        "app_id": "应用ID"
    },
    "business": {
        "domain": "iat",
        "language": "zh_cn",
        "accent": "mandarin",
        "dwa": "wpgs",
        "vad_eos": 2000
    },
    "data": {
        "status": 0,  // 0:首帧 1:中间帧 2:尾帧
        "format": "audio/L16;rate=16000",
        "encoding": "raw",
        "audio": "base64编码的音频数据"
    }
}
```

#### 响应格式（讯飞STT返回）

```json
{
    "code": 0,
    "message": "success",
    "sid": "会话ID",
    "data": {
        "status": 2,  // 2表示识别完成
        "result": {
            "ws": [
                {
                    "cw": [
                        {
                            "w": "识别的文字",
                            "wp": "n"  // 词性
                        }
                    ]
                }
            ]
        }
    }
}
```

### Base64音频编码

```cpp
// 音频数据Base64编码
data["audio"] = base64::encode((byte *)audio1.wavData[0], 1280);

// 编码参数：
// - 输入：1280字节原始音频数据
// - 输出：Base64编码字符串
// - 对应：约80ms的音频（16kHz采样率）
```

## 错误处理与异常恢复

### 网络异常处理

```cpp
// WebSocket连接失败处理
if (!webSocketClient1.connect(url1.c_str()))
{
    Serial.println("连接STT失败！");
    // 重试机制或降级处理
    awake_flag = 0;
    await_flag = 1;
    return;
}

// 连接断开处理
void onEventsCallback1(WebsocketsEvent event, String data)
{
    if (event == WebsocketsEvent::ConnectionClosed)
    {
        Serial.println("STT连接已断开");
        recording = 0;
        // 清理资源，准备重连
    }
}
```

### 音频异常处理

```cpp
// RMS值异常抑制
float rms = audio1.calculateRMS((uint8_t *)audio1.wavData[0], 1280);
if (rms > 1000)  // 抑制录音奇奇怪怪的噪声
{
    rms = 8.6;
}

// 录音超时处理
if(null_voice >= 80)  // 8秒无有效语音
{
    shuaxin = 1;
    Answer = "主人，我先退下了，有事再找我喵~";
    response();
    recording = 0;
    webSocketClient1.close();
    return;
}
```

### JSON解析异常

```cpp
// JSON解析错误处理
DeserializationError error = deserializeJson(jsonDocument, message.data());
if (error)
{
    Serial.println("JSON解析错误:");
    Serial.println(error.c_str());
    Serial.println(message.data());
    return;
}

// API返回码检查
if (jsonDocument["code"] != 0)
{
    Serial.println("API调用失败");
    Serial.println(message.data());
    webSocketClient1.close();
}
```

## 性能监控与调试

### 音频质量监控

```cpp
// RMS值监控
printf("RMS: %f\n", rms);

// 录音状态监控
Serial.println("开始录音");
Serial.println("录音结束");

// 识别结果监控
Serial.println("识别完成: " + askquestion);
```

### 时间性能分析

```cpp
// URL生成时间记录
urlTime = millis();

// 录音时长统计
unsigned long startTime = 0;
unsigned long endTime = 0;

startTime = millis();  // 录音开始
// ... 录音过程 ...
endTime = millis();    // 录音结束

Serial.printf("录音时长: %lu ms\n", endTime - startTime);
```

### 内存使用监控

```cpp
// JSON文档大小控制
StaticJsonDocument<2000> doc;  // 限制2KB
DynamicJsonDocument jsonDocument(4096);  // 限制4KB

// 音频缓冲区管理
static const int wavDataSize = 30000;     // 30KB音频缓冲
static const int i2sBufferSize = 5120;    // 5KB I2S缓冲
```

## 系统优化建议

### 1. 实时性优化

- **减少延迟**：优化音频帧间隔（当前40ms）
- **缓冲优化**：调整DMA缓冲区大小和数量
- **中断优化**：合理设置中断优先级

### 2. 稳定性优化

- **重连机制**：实现WebSocket自动重连
- **状态管理**：完善状态机设计
- **资源清理**：及时释放内存和连接

### 3. 功耗优化

- **休眠模式**：实现深度睡眠机制
- **动态调频**：根据负载调整CPU频率
- **外设管理**：按需启用/禁用外设

## 总结

ESP32 AI LLM项目的音频处理与语音识别系统采用了模块化设计，通过I2S接口实现高质量音频采集，结合讯飞语音识别API提供准确的语音转文字服务。系统具有以下特点：

### 技术亮点

1. **双模式唤醒**：支持在线和离线两种唤醒方式
2. **实时音频处理**：基于I2S的高质量音频采集
3. **智能VAD算法**：有效的语音活动检测
4. **流式数据传输**：WebSocket实时音频流传输
5. **完善的错误处理**：多层次的异常处理机制

### 应用价值

- 为物联网设备提供自然语音交互能力
- 支持多种大语言模型的语音对话
- 具有良好的扩展性和可维护性
- 适合智能家居、语音助手等应用场景

该系统为智能语音交互提供了坚实的技术基础，具有很高的实用价值和参考意义。
