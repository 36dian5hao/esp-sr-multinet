# Web页面配置模块说明

## 📁 文件结构

```
main/
├── web_yemian.c        # 网页界面实现文件
├── Web_Scr_set.h       # 头文件声明
├── main.c              # 主程序文件
└── CMakeLists.txt      # 构建配置
```

## 🎯 功能概述

这个模块为ESP32热点配网系统提供了完整的Web界面，包括：

### 1. 音乐配置管理
- **添加/更新音乐**：保存音乐名称和对应的ID
- **删除音乐**：根据音乐名称删除配置
- **查看音乐列表**：显示所有已保存的音乐配置

### 2. 大模型配置管理
- **添加/更新大模型**：配置API Key和API URL
- **删除大模型配置**：根据供应商名称删除配置
- **查看配置列表**：显示所有已保存的大模型配置
- **支持的供应商**：OpenAI、Claude、Gemini、通义千问、文心一言、智谱AI等

## 🔧 主要函数说明

### 音乐配置相关函数

#### `handleMusicManagement()`
- **功能**：显示音乐配置页面
- **路由**：`/music`
- **方法**：GET
- **界面包含**：
  - 添加音乐表单（音乐名称 + 音乐ID）
  - 删除音乐表单
  - 查看已保存音乐按钮

#### `handleListMusic()`
- **功能**：显示已保存的音乐列表
- **路由**：`/listMusic`
- **方法**：GET
- **数据存储**：使用Preferences存储在"music_store"命名空间

#### `handleDeleteMusicFixed()`
- **功能**：删除指定的音乐配置
- **路由**：`/deleteMusic`
- **方法**：POST
- **参数**：`musicName` - 要删除的音乐名称

### 大模型配置相关函数

#### `handleLLMManagement()`
- **功能**：显示大模型配置页面
- **路由**：`/model`
- **方法**：GET
- **界面包含**：
  - 添加大模型配置表单（供应商 + API Key + API URL）
  - 删除大模型配置表单
  - 查看已保存配置按钮

#### `handleSaveLLMComplete()`
- **功能**：保存或更新大模型配置
- **路由**：`/saveLLM`
- **方法**：POST
- **参数**：
  - `modelSupplier` - 模型供应商
  - `apiKey` - API密钥
  - `apiUrl` - API地址

#### `handleListLLM()`
- **功能**：显示已保存的大模型配置列表
- **路由**：`/listLLM`
- **方法**：GET
- **数据存储**：使用Preferences存储在"llm_store"命名空间

#### `handleDeleteLLM()`
- **功能**：删除指定的大模型配置
- **路由**：`/deleteLLM`
- **方法**：POST
- **参数**：`modelSupplier` - 要删除的供应商名称

## 💾 数据存储结构

### 音乐配置存储
```
命名空间: "music_store"
键值对:
- numMusic: 音乐数量
- musicName0, musicName1, ... : 音乐名称
- musicId0, musicId1, ... : 音乐ID
```

### 大模型配置存储
```
命名空间: "llm_store"
键值对:
- numLLM: 大模型配置数量
- llmName0, llmName1, ... : 供应商名称
- llmApiKey0, llmApiKey1, ... : API密钥
- llmApiUrl0, llmApiUrl1, ... : API地址
```

## 🎨 界面特点

### 响应式设计
- 适配移动设备和桌面设备
- 使用CSS Flexbox布局
- 固定底部导航按钮

### 用户体验
- 表单验证和必填字段提示
- 操作成功/失败的JavaScript弹窗提示
- 清晰的页面导航和返回按钮
- 删除按钮使用红色警告样式

### 样式特色
- 现代化的卡片式设计
- 统一的配色方案（深灰色主题）
- 悬停效果和过渡动画
- 滚动区域支持长列表显示

## 🔗 路由映射

| 路由 | 方法 | 功能 | 对应函数 |
|------|------|------|----------|
| `/music` | GET | 音乐配置页面 | `handleMusicManagement()` |
| `/saveMusic` | POST | 保存音乐配置 | `handleSaveMusic()` |
| `/deleteMusic` | POST | 删除音乐配置 | `handleDeleteMusicFixed()` |
| `/listMusic` | GET | 音乐列表页面 | `handleListMusic()` |
| `/model` | GET | 大模型配置页面 | `handleLLMManagement()` |
| `/saveLLM` | POST | 保存大模型配置 | `handleSaveLLMComplete()` |
| `/deleteLLM` | POST | 删除大模型配置 | `handleDeleteLLM()` |
| `/listLLM` | GET | 大模型列表页面 | `handleListLLM()` |

## 🚀 使用方法

1. **包含头文件**：
   ```c
   #include "Web_Scr_set.h"
   ```

2. **在Web服务器中注册路由**：
   ```c
   server.on("/music", HTTP_GET, handleMusicManagement);
   server.on("/saveMusic", HTTP_POST, handleSaveMusic);
   server.on("/deleteMusic", HTTP_POST, handleDeleteMusicFixed);
   server.on("/listMusic", HTTP_GET, handleListMusic);
   server.on("/model", HTTP_GET, handleLLMManagement);
   server.on("/saveLLM", HTTP_POST, handleSaveLLMComplete);
   server.on("/deleteLLM", HTTP_POST, handleDeleteLLM);
   server.on("/listLLM", HTTP_GET, handleListLLM);
   ```

3. **确保依赖库**：
   - AsyncWebServer
   - Preferences
   - WiFi

## ⚠️ 注意事项

1. **线程安全**：当前实现未考虑多线程访问，建议在单线程环境下使用
2. **存储限制**：Preferences有存储大小限制，注意API Key长度
3. **输入验证**：建议在实际使用中添加更严格的输入验证
4. **错误处理**：可以根据需要增强错误处理和用户反馈机制

## 🔧 扩展建议

1. **添加编辑功能**：支持直接在列表页面编辑配置
2. **批量操作**：支持批量删除和导入/导出配置
3. **配置验证**：添加API连接测试功能
4. **用户认证**：添加管理员密码保护
5. **配置备份**：支持配置的备份和恢复功能
