# ESP32-S31 智能语音聊天项目

基于 **ESP32-S31** 芯片和 **ESP-IDF v6.1** 的智能语音对话助手，支持语音唤醒、大模型语音对话、LVGL 显示及摄像头视觉功能。

## 硬件平台

- **主控**: ESP32-S31 (esp32_s31_korvo_1)
- **PSRAM**: 16MB Octal PSRAM @200MHz
- **Flash**: 16MB QIO
- **音频编解码**: ES8389（支持双麦克风 + 扬声器）
- **显示屏**: 800×480 RGB LCD，RGB565
- **触摸**: GT1151 电容触摸（I2C）
- **摄像头**: OV3660（DVP 接口）
- **WiFi**: 2.4GHz 802.11 b/g/n
- **SD 卡**: SDMMC 接口（4-bit 模式）

## 功能特性

- **语音唤醒**: 内置 WakeNet（"你好小智"），支持 AFE 前处理（AEC/NS/VAD/AGC）
- **语音对话**: 通过 MQTT 连接大模型语音服务器（小智协议），支持 OPUS 音频编解码
- **LVGL 显示**: 显示对话状态、聊天消息、表情动画
- **显示主题**: 默认 Dark 主题，支持通过 MCP 在 Light/Dark 之间切换
- **电容触摸**: 板级适配层支持通过 Board Manager 和 `esp_lvgl_port` 将 GT1151 接入 LVGL
- **摄像头**: DVP 摄像头录制，支持视觉解释功能
- **音频通路**: 完整录音 → OPUS 编码 → 网络发送 → OPUS 解码 → 播放链路

## 项目结构

```
esp32s31/
├── CMakeLists.txt                    # 项目根 CMake
├── main/
│   ├── CMakeLists.txt                # 主组件注册
│   ├── idf_component.yml             # 外部依赖声明
│   ├── Kconfig.projbuild             # 传输方式配置（MQTT/WebSocket）
│   ├── main.c                        # app_main() 入口
│   ├── esp_xiaozhi_chat_app.c/.h     # 语音聊天核心逻辑
│   └── esp_xiaozhi_chat_display.c/.h # LVGL 界面显示
├── components/
│   ├── esp_board_manager_adapter/    # 板级管理器适配层
│   └── gen_bmgr_codes/              # 板配置代码生成
├── managed_components/               # 自动下载的外部组件（勿手动编辑）
├── partitions.csv                    # 自定义分区表（双 OTA 6MB）
├── sdkconfig.defaults                # Kconfig 默认配置
└── dependencies.lock                 # 组件依赖锁文件
```

## 构建与烧录

### 环境要求

- ESP-IDF v6.1-beta1 或更新版本

### 构建

```bash
# 激活 IDF 环境
& "C:\Program Files\Espressif\idf_cmd_init\idf_cmd_init.bat"

# 构建
python -m idf_py build

# 烧录并监视串口
python -m idf_py -p COM? flash monitor

# 清理并重新构建
python -m idf_py fullclean && python -m idf_py build

# 打开配置菜单
python -m idf_py menuconfig

# 更新组件依赖
python -m idf_py update-dependencies
```

## 依赖组件

| 组件名 | 版本 | 用途 |
|--------|------|------|
| `espressif/esp_xiaozhi` | ^0.1.1 | 小智语音对话 SDK |
| `jason-mao/av_processor` | ^0.7.0 | 音频处理（录放音、编解码） |
| `espressif/network_provisioning` | ^1.2.4 | 网络配网 |
| `78/xiaozhi-fonts` | ~1.3.2 | LVGL 中文字库 |
| `protocol_examples_common` | 内建 | 网络连接示例组件 |

## 分区布局

```
nvs:        data/nvs       — 16KB    NVS 存储
otadata:    data/ota       — 8KB     OTA 信息
phy_init:   data/phy       — 4KB     射频校准
model:      data/spiffs    — 1MB     模型/资源分区
ota_0:      app/ota_0      — 6MB     固件槽 A
ota_1:      app/ota_1      — 6MB     固件槽 B（OTA 升级）
```

## Kconfig 关键选项

`sdkconfig.defaults` 中预置了针对 ESP32-S31 的优化配置：

- CPU 频率 320MHz，双核启用
- PSRAM 启用（16MB Octal SPI）
- LVGL 中文 CJK 字库（Source Han Sans SC）
- WiFi 内存优化（减少缓冲区、关闭企业认证）
- UART ISR 放在 IRAM（防止 ML307 FIFO 溢出）
- 禁用不必要的 LVGL 控件以减小固件体积

## 显示与触摸适配说明

- LCD 是无独立 `io_handle` 的 RGB 面板，注册 LVGL display 时必须使用 `lvgl_port_add_disp_rgb()`，不能使用 SPI/I80 屏对应的 `lvgl_port_add_disp()`。
- Xiaozhi 应用设置 `bsp_config.enable_lvgl = false`，由 `esp_xiaozhi_chat_display.c` 唯一负责初始化 LVGL 和注册 RGB display，避免同一 panel 被重复注册后出现裂屏、刷新竞争或颜色异常。
- 当前显示缓冲为一帧 `800×480` RGB565 数据，分配在 PSRAM，约占 750KB；`swap_bytes` 必须与面板的 RGB565 字节序保持一致。
- 触摸配置宏使用 `CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUB_I2C_SUPPORT`，运行时句柄类型为 `dev_lcd_touch_handles_t`，公开头文件为 `dev_lcd_touch.h`。只有同时启用适配层的 LVGL 和 `enable_touch` 时，适配层才会调用 `lvgl_port_add_touch()`。
- 默认主题为 Dark：主背景 `#121212`、聊天区域 `#1E1E1E`、正文白色、用户气泡深绿色、助手气泡深灰色。

## 最近更新（2026-07-12）

- 修复 RGB LCD 使用错误注册接口导致的 `io_handle != NULL` 断言。
- 修复触摸适配层配置宏和 Board Manager 新版 API 名称不一致的问题。
- 避免 Board Manager 与 Xiaozhi 显示层重复初始化 LVGL；显示缓冲改为 PSRAM 全屏缓冲。
- Xiaozhi UI 默认切换为 Dark 主题。
- 为 `main` 目录的入口、聊天、音频、显示和构建配置补充中文注释。

## 许可

Apache-2.0 License
## 参考来源

本项目基于乐鑫官方示例 [esp_xiaozhi xiaozhi_chat](https://components.espressif.com/components/espressif/esp_xiaozhi/versions/0.1.1/examples/xiaozhi_chat?language=zh) 进行适配和二次开发。
