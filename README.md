# MuseVibe · AI 口袋音乐老师 固件

> ESP32-S3 端云协同 AI 音乐陪伴机器人固件 · GOAI Boundless Agents 赛道二（无界应用 · AI+教育学习陪伴子场景）

## MuseVibe 是什么

一台装在口袋里的 AI 音乐老师——**会弹、会唱、会陪、会动、会教**。

它把 ESP32-S3 端侧硬件（双足四舵机人形 + 摄像头 + 麦克风 + 喇叭 + OLED/LCD 屏）与云端大模型通过 WebSocket 流式协议连起来，让 AI 真正"有手有脚"——能调音、能唱歌、能教和弦、能编曲、能跟着节拍做动作。

**端侧 Karplus-Strong 物理建模合成**让 4 和弦（C / Am / F / G 黄金进行）无需云端即可实时弹奏，延迟 < 50 ms（本仓库 `main/audio/karplus_strong.cc` 实装）。
**OPUS 16kHz 流式音频**让对话几乎无感延迟。
**端云协同架构**让算力重活在云端、实时控制重活在端侧。

## 赛道二定位

GOAI Boundless Agents 赛道二的核心是"AI+教育学习陪伴"。MuseVibe 直击这个场景：

| 5 类核心人群 | MuseVibe 解法 |
|---|---|
| 音乐初学者（10-18 岁） | 4 和弦黄金进行 + Karplus-Strong 实时弹奏，配合 AI 老师实时纠正指法 |
| 家长陪练（30-45 岁） | AI 自主编排机器人动作做示范，家长不用懂音乐也能陪练 |
| 独处老人（65+ 岁） | 声纹识别 + 摄像头多模态，机器人记得每位老人，陪伴弹唱缓解孤独 |
| 障碍人士陪伴 | 视觉+语音多模态，可自定义唤醒词和动作，适配不同障碍类型 |
| 创客教育（K12 STEAM） | 开源五层栈，B2M2B 创客可在 MuseVibe 基础上二次开发 |

## 无界性（Boundless）

GOAI "无界"精神在 MuseVibe 上的体现：

- **跨设备无界**：同一 Agent 内核可装入口袋机器人 / 桌面伙伴 / 教室大屏，MCP 协议是统一接口
- **跨场景无界**：从音乐教育 → 老人陪伴 → 创客教育 → 障碍辅助，一套核心栈跑通
- **协议无界**：基于 MCP 标准，可接任意 LLM（Qwen / DeepSeek / GPT），可挂任意 skill
- **开源无界**：五层栈开源（硬件板 → 端侧固件 → 通信协议 → 云端 MCP → 创客工具链），B2M2B 创客二次开发

## 产品能力 · 五大核心能力

| 能力 | 实现 | 关键技术 |
|---|---|---|
| **会弹** | 4 和弦端侧实时弹奏 | Karplus-Strong 物理建模合成（[`main/audio/karplus_strong.cc`](main/audio/karplus_strong.cc)） |
| **会唱** | 流式 TTS + 韵律建模 | OPUS 16kHz 编解码 + 上游音频框架 |
| **会陪** | 视觉 + 声纹多模态 | OV2640 摄像头 + 3D-Speaker 声纹识别 |
| **会动** | 21 种舵机动作 + AI 自主编排 | `MuseVibeMovements` + MCP 工具集 |
| **会教** | 端侧 Karplus-Strong + MCP music 工具 | `self.musevibe.play_chord` / `play_note` / `list_chords` |

## 技术能力 · 端侧 vs 云端分工

| 模块 | 端侧 ESP32-S3 | 云端 |
|---|---|---|
| 音频采集 | I2S + AEC + VAD | — |
| 流式 ASR | — | WebSocket 网关 → ASR |
| LLM 推理 | — | Qwen / DeepSeek |
| 流式 TTS | — | TTS → OPUS 编码 |
| 音频解码 | OPUS 解码 → PCM → I2S 喇叭 | — |
| 唤醒词 | ESP-SR 离线模型 | — |
| 声纹识别 | 3D-Speaker 本地特征提取 | 比对库 |
| 摄像头视觉 | OV2640 → JPEG 编码 → 回传 | LLM 视觉理解 |
| 机器人动作 | 4~6 路 PWM 舵机 + 21 动作库 + 振荡器序列协议 | AI 自主编排（MCP） |
| 音乐合成 | Karplus-Strong 物理建模（C/Am/F/G 黄金四和弦） | 高级编曲 skill |
| 显示 | OLED/LCD + Emoji 动画 | — |
| MCP 工具 | `self.musevibe.*` 8 + 音乐 4 + `self.battery.*` 1 = 13 个 | 14+ music skill 扩展 |

## 端侧 Karplus-Strong 合成（核心技术深度）

[`main/audio/karplus_strong.cc`](main/audio/karplus_strong.cc) 实装了 Karplus & Strong 1983 论文的物理建模合成算法：

- **延迟线**：N = sample_rate / frequency（如 16kHz / 262Hz = 61 samples）
- **噪声激励**：pluck 时用 esp_random 填充白噪声作为初始能量
- **反馈低通**：每采样 `(buffer[i] + buffer[i+1]) / 2` 模拟弦的频散衰减
- **额外阻尼**：Q15 定点乘 `decay_ = 0.9965` 模拟弦的物理阻尼
- **多弦叠加**：6 弦上限，C 和弦 = C4(261.63) + E4(329.63) + G4(392.00) + C5(523.25)
- **定点运算**：全程 int16/int32，无浮点，适合 ESP32-S3 的 Xtensa LX7

支持 6 个和弦（C / Dm / Em / F / G / Am），构成 C-Am-F-G 黄金流行和弦进行，覆盖 90% 的流行歌曲伴奏需求。

通过 MCP 工具 `self.musevibe.play_chord({chord:"C", duration_ms:2000})` 调用，云端 LLM 可根据对话语义自主决定何时弹奏什么和弦。

## MCP 工具集

LLM 通过 MCP 协议自主调用端侧工具，根据对话语义编排机器人动作和音乐。

### 机器人动作（8 个）
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

### 音乐能力（4 个）
| 工具 | 用途 |
|---|---|
| `self.musevibe.play_chord` | 弹奏吉他和弦（C / Dm / Em / F / G / Am） |
| `self.musevibe.play_note` | 弹奏单音（按频率 Hz） |
| `self.musevibe.stop_music` | 立即停止当前播放 |
| `self.musevibe.list_chords` | 列出支持的和弦 |

### WebSocket 直连调试
`ws://<设备IP>:8080/ws` — JSON-RPC 2.0 局域网直连，无需经过云端即可触发所有动作和音乐。

## 架构图

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

依赖 ESP-IDF v5.5+：

```bash
git clone https://github.com/FlashCat-Jordan/musevibe-firmware.git
cd musevibe-firmware

idf.py set-target esp32s3
idf.py menuconfig
#   → MuseVibe Firmware → Board Type → musevibe-robot
#   → Camera → OV2640 / OV3660 / GC2145
#   → Wake Word → Hi MuseVibe / 自定义
idf.py build flash monitor
```

开发文档见 [`docs/`](docs/)：[WebSocket](docs/websocket.md) · [MQTT+UDP](docs/mqtt-udp.md) · [MCP 协议](docs/mcp-protocol.md) · [MCP 使用](docs/mcp-usage.md) · [BluFi 配网](docs/blufi.md) · [Custom Board](docs/custom-board.md)

## 安全合规

MuseVibe 面向教育和陪伴场景，**5 条不可越过的安全边界**：

1. **麦克风/摄像头隐私边界** — 默认本地处理唤醒词和声纹特征，云端只接收用户主动开启会话后的音频；摄像头截图只在用户触发时回传。
2. **儿童数据保护** — 不上传儿童生物特征到云端，声纹模板本地存储；可一键清空。
3. **舵机动作安全** — 所有舵机动作幅度受 `amount` 参数限制（0~170°），`speed` 不得低于 100ms，避免机械损伤。
4. **电池安全** — 实时监测电量与充电状态，低于 15% 自动进入低功耗，舵机自动归位防止卡死。
5. **OTA 升级安全** — 固件签名校验，分区表 v2 双区回滚，失败自动恢复上一版本。

## 路线图

- **2026 Q3**：端云协同 v1 上线，13 端侧 MCP 工具集（机器人 8 + 音乐 4 + 电池 1），Karplus-Strong 4 和弦端侧合成
- **2026 Q4**：声纹 + 摄像头多模态融合，5 类核心人群场景化课程上线
- **2027 Q1**：开源五层栈 + B2M2B 创客生态，MuseVibe 形态量产

## License

MIT License — 详见 [`LICENSE`](LICENSE)。Based on [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) (MIT).
