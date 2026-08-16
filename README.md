# MuseVibe · AI 口袋音乐老师 固件

> ESP32-S3 端云协同 AI 音乐陪伴机器人固件 · GOAI Boundless Agents 赛道二（无界应用 · AI+教育学习陪伴子场景）

## MuseVibe 是什么

一台装在口袋里的 AI 音乐老师——**会弹、会唱、会陪**。

它把 ESP32-S3 端侧硬件（双足四舵机人形 + 摄像头 + 麦克风 + 喇叭 + OLED/LCD 屏）与云端大模型通过 WebSocket 流式协议连起来，让 AI 真正"有手有脚"——能调音、能唱歌、能教和弦、能编曲、能跟着节拍做动作。

**端侧 Karplus-Strong 物理建模合成**让 4 和弦（C / Am / F / G 黄金进行）无需云端即可实时弹奏，延迟 < 50 ms。
**OPUS 16kHz 流式音频**让对话几乎无感延迟。
**端云协同架构**让算力重活在云端、实时控制重活在端侧。

## 产品能力

### 1. 五大核心能力
- **会弹** — 端侧物理建模合成，4 和弦实时弹奏，节拍器同步
- **会唱** — 流式 TTS + 韵律建模，可克隆音色
- **会陪** — 摄像头视觉 + 声纹识别，记住每个用户
- **会动** — 21 种舵机动作 + AI 自主编排
- **会教** — 14+ MCP music skill 工具集，从识谱到编曲

### 2. AI 自主编排机器人动作（MCP 工具集）
LLM 通过 MCP 协议自主调用端侧工具，根据对话语义编排机器人动作：

| 工具 | 用途 |
|---|---|
| `self.musevibe.action` | 21 种动作：walk / turn / jump / swing / moonwalk / bend / shake_leg / updown / whirlwind_leg / sit / showcase / home / hands_up / hands_down / hand_wave / windmill / takeoff / fitness / greeting / shy / radio_calisthenics / magic_circle |
| `self.musevibe.stop` | 立即停止并复位 |
| `self.musevibe.get_status` | 运动状态（moving / idle） |
| `self.musevibe.set_trim` | 舵机微调（±50°） |
| `self.musevibe.get_trims` | 读取所有微调 |
| `self.musevibe.get_ip` | WiFi IP |
| `self.musevibe.servo_sequences` | 自编程舵机序列（普通 / 振荡器两种模式） |
| `self.battery.get_level` | 电池电量 |

### 3. WebSocket 直连调试
`ws://<设备IP>:8080/ws` — JSON-RPC 2.0 局域网直连，无需经过云端即可触发所有动作。

## 技术架构

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

## 硬件

| 板型 | 芯片 | 摄像头 | 舵机 | 屏 |
|---|---|---|---|---|
| `musevibe-robot` | ESP32-S3 | OV2640 / OV3660 / GC2145（自动探测） | 4~6 路 PWM | OLED / LCD |

板型配置：[`main/boards/musevibe-robot/config.h`](main/boards/musevibe-robot/config.h) · [`config.json`](main/boards/musevibe-robot/config.json)
板型文档：[`main/boards/musevibe-robot/README.md`](main/boards/musevibe-robot/README.md)

## 构建

依赖 ESP-IDF v5.3+：

```bash
git clone https://github.com/FlashCat-Jordan/musevibe-firmware.git
cd musevibe-firmware

idf.py set-target esp32s3
idf.py menuconfig
#   → MuseVibe Firmware → Board Type → musevibe-robot
#   → Camera → OV2640 / OV3660 / GC2145
idf.py build flash monitor
```

开发文档见 [`docs/`](docs/)：[WebSocket](docs/websocket.md) · [MQTT+UDP](docs/mqtt-udp.md) · [MCP 协议](docs/mcp-protocol.md) · [MCP 使用](docs/mcp-usage.md) · [BluFi 配网](docs/blufi.md) · [Custom Board](docs/custom-board.md)

## 路线图

- **2026 Q3**：端云协同 v1 上线，14+ MCP music skill 工具集，4 和弦端侧合成
- **2026 Q4**：声纹 + 摄像头多模态融合，5 类核心人群场景化课程
- **2027 Q1**：开源五层栈 + B2M2B 创客生态，MuseVibe 形态量产

## License

MIT License — 详见 [`LICENSE`](LICENSE)。Based on [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) (MIT).
