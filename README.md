# MuseVibe · AI 口袋音乐老师 固件

> ESP32-S3 端云协同 AI 音乐陪伴机器人固件，参加 GOAI Boundless Agents 赛道二（无界应用 · AI+教育学习陪伴子场景）。

## 项目简介

MuseVibe 是一台装在口袋里的 AI 音乐老师：会弹、会唱、会陪。它把 ESP32-S3 端侧硬件（双足四舵机人形 + 摄像头 + 麦克风 + 喇叭 + OLED/LCD 屏）与云端大模型（流式 ASR + LLM + TTS）通过 WebSocket 流式协议连起来，并通过 14+ MCP music skill 工具集让 AI 真正"有手有脚"——能调音、能唱歌、能教和弦、能编曲、能跟着节拍做动作。

端侧 Karplus-Strong 物理建模合成让 4 和弦（C / Am / F / G 黄金进行）可以无云端、低延迟地实时弹奏；OPUS 16kHz 流式音频让对话几乎无感延迟；端云协同架构让算力重活在云端、实时控制重活在端侧。

## 上游致谢

本固件基于开源项目 **[xiaozhi-esp32](https://github.com/78/xiaozhi-esp32)**（作者 78，MIT License，版权 © 2025 Shenzhen Xinzhi Future Technology Co., Ltd.）二次开发。上游提供了完整的语音交互框架（ASR + LLM + TTS）、MCP 协议栈、音频编解码、显示框架、多语言资源等基础设施，MuseVibe 在此之上增加了：

- **musevibe-robot 板型适配**：双足四舵机（左右腿/左右脚）+ 双手舵机 + OV2640/OV3660/GC2145 摄像头版本硬件抽象，电源管理、I2C 设备树、按键、背光等。
- **MuseVibe 动作系统**：`MuseVibeMovements` / `MuseVibeController` 实现行走、转向、跳跃、摇摆、太空步、弯腰、摇腿、旋风腿、坐下、展示、复位等 11 种基础动作，以及举手、挥手、大风车、起飞、健身、打招呼、害羞、广播体操、爱的魔力转圈圈等 10 种手部动作。
- **AI 自主编排动作 Skill**：把舵机序列封装成 MCP 工具 `self.musevibe.action` / `self.musevibe.stop` / `self.musevibe.get_status` / `self.musevibe.set_trim` / `self.musevibe.get_trims` / `self.musevibe.get_ip` / `self.musevibe.servo_sequences`，云端 LLM 可以根据对话语义自主编排机器人动作。
- **WebSocket 直连调试接口**：JSON-RPC 2.0 局域网直连调试，无需经过云端即可触发所有动作。
- **振荡器舵机序列协议**：支持普通逐步移动与振荡器双臂摆动两种模式，可编程自定义动作序列。

上游 LICENSE 完整保留于本仓库根目录 `LICENSE`，谨此致谢。

## 硬件

MuseVibe 当前支持的板型：

| 板型 | 芯片 | 摄像头 | 舵机 | 屏 |
|---|---|---|---|---|
| `musevibe-robot` | ESP32-S3 | OV2640 / OV3660 / GC2145（自动探测） | 4~6 路 PWM 舵机 | OLED / LCD |

板型配置文件：[`main/boards/musevibe-robot/config.h`](main/boards/musevibe-robot/config.h) · [`config.json`](main/boards/musevibe-robot/config.json)。

## 核心能力

### 1. 端云协同语音交互
- **流式 ASR + LLM + TTS**：基于 WebSocket 的全双工音频流，OPUS 16kHz 编解码，对话延迟低于 800 ms。
- **离线唤醒词**：ESP-SR 模型，本地唤醒不依赖云端。
- **声纹识别**：3D-Speaker 识别当前说话人身份。
- **多语言**：中文 / 英文 / 日文等 30+ 语种。

### 2. AI 自主编排机器人动作（MCP 工具集）
LLM 通过 MCP 协议调用端侧工具：

| 工具 | 用途 |
|---|---|
| `self.musevibe.action` | 执行动作：walk / turn / jump / swing / moonwalk / bend / shake_leg / updown / whirlwind_leg / sit / showcase / home / hands_up / hands_down / hand_wave / windmill / takeoff / fitness / greeting / shy / radio_calisthenics / magic_circle |
| `self.musevibe.stop` | 立即停止并复位 |
| `self.musevibe.get_status` | 查询运动状态（moving / idle） |
| `self.musevibe.set_trim` | 校准舵机微调（±50°） |
| `self.musevibe.get_trims` | 读取所有舵机微调 |
| `self.musevibe.get_ip` | 获取 WiFi IP |
| `self.musevibe.servo_sequences` | 自编程舵机序列（普通 / 振荡器两种模式） |
| `self.battery.get_level` | 电池电量 |

### 3. 端侧 Karplus-Strong 物理建模合成
4 和弦（C / Am / F / G 黄金进行）端侧实时合成，无需云端即可弹唱，延迟低于 50 ms。

### 4. 摄像头视觉
ESP32-S3 + OV2640/OV3660/GC2145 自动探测，可截图回传给 LLM 做视觉理解。

## 架构

```
┌─────────────────────────── 端侧 ESP32-S3 ───────────────────────────┐
│  麦克风 ─ I2S ─ AudioProcessor(AEC+VAD) ─ OpusEncoder ─┐             │
│  唤醒词 ESP-SR ─ 声纹 3D-Speaker                       │ WebSocket   │
│  摄像头 OV2640 ─ JPEG 编码 ────────────────────────────┼──流────┐    │
│  OLED/LCD 显示 + Emoji 动画                            │        │    │
│  4~6 路 PWM 舵机 ─ MuseVibeMovements ─ MCP 工具调用 ◀───┼────────┼────│
│  Karplus-Strong 合成 ─ 4 和弦实时弹奏                  │        │    │
│  OPUS 解码 ◀─ PCM ◀ AudioOutputTask ◀──────────────────┘        │    │
└──────────────────────────────────────────────────────────────────┼────┘
                                                                    │
┌──────────────────────────────── 云端 ──────────────────────────────▼────┐
│  WebSocket 网关 ─ 流式 ASR ─ LLM (Qwen / DeepSeek) ─ TTS ─ OpusEncoder  │
│  MCP Server ─ 14+ music skill 工具（调音 / 教和弦 / 编曲 / 节拍器 …）    │
│  RAG 知识库 ─ 语义检索召回                                              │
└─────────────────────────────────────────────────────────────────────────┘
```

## 构建

依赖 ESP-IDF v5.3+：

```bash
# 克隆
git clone https://github.com/FlashCat-Jordan/musevibe-firmware.git
cd musevibe-firmware

# 设置目标芯片
idf.py set-target esp32s3

# 配置（选择 musevibe-robot 板型、摄像头型号、唤醒词等）
idf.py menuconfig
#   → MuseVibe Firmware → Board Type → musevibe-robot
#   → Camera → OV2640 / OV3660 / GC2145
#   → Wake Word → Hi MuseVibe / 自定义

# 编译 + 烧录
idf.py build flash monitor
```

更多开发文档见 [`docs/`](docs/)：
- 通信协议：[WebSocket](docs/websocket.md) · [MQTT+UDP](docs/mqtt-udp.md)
- MCP 协议：[MCP 协议](docs/mcp-protocol.md) · [MCP 使用](docs/mcp-usage.md)
- 蓝牙配网：[BluFi](docs/blufi.md)
- 自定义板型：[Custom Board](docs/custom-board.md)

## 板型文档

`musevibe-robot` 板型的硬件配置、动作参数、MCP 工具调用示例、WebSocket 直连调试接口详见 [`main/boards/musevibe-robot/README.md`](main/boards/musevibe-robot/README.md)。

## 路线图

- **2026 Q3**：端云协同 v1 上线，14+ MCP music skill 工具集，4 和弦端侧合成
- **2026 Q4**：声纹 + 摄像头多模态融合，5 类核心人群场景化课程
- **2027 Q1**：开源五层栈 + B2M2B 创客生态，MuseVibe 形态量产

## License

MIT License — 详见 [`LICENSE`](LICENSE)。

本固件基于 [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32)（MIT，版权 © 2025 Shenzhen Xinzhi Future Technology Co., Ltd.）二次开发，谨向上游作者 78 致谢。
