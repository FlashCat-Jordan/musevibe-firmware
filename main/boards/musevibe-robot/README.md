# musevibe-robot 板型

MuseVibe 双足四舵机人形机器人板型，基于 ESP32-S3，支持 OV2640/OV3660/GC2145 三种摄像头（自动探测）、4~6 路 PWM 舵机、OLED/LCD 显示屏。

## 硬件配置

详见 [`config.h`](config.h) 与 [`config.json`](config.json)。

| 资源 | 引脚 |
|---|---|
| 左腿舵机 | `right_leg_pin` |
| 右腿舵机 | `left_foot_pin` |
| 左脚舵机 | `right_foot_pin` |
| 右脚舵机 | `left_leg_pin` |
| 左手舵机（可选） | `left_hand_pin` |
| 右手舵机（可选） | `right_hand_pin` |
| 音频 I2S | 详见 `audio_i2s_*_pin` |
| 显示屏 | `display_*_pin` |
| 摄像头 | 自动探测 OV2640 / OV3660 / GC2145 |

摄像头型号通过 PID 探测：
- `0x2640` / `0x2626` → OV2640
- `0x3660` → OV3660
- `0x2145` → GC2145

## MCP 工具集

| 工具 | 用途 |
|---|---|
| `self.musevibe.action` | 执行动作 |
| `self.musevibe.stop` | 立即停止并复位 |
| `self.musevibe.get_status` | 查询运动状态（moving / idle） |
| `self.musevibe.set_trim` | 校准舵机微调（±50°） |
| `self.musevibe.get_trims` | 读取所有舵机微调 |
| `self.musevibe.get_ip` | 获取 WiFi IP |
| `self.musevibe.servo_sequences` | 自编程舵机序列 |
| `self.battery.get_level` | 电池电量 |

### `self.musevibe.action` 参数

| 参数 | 说明 | 范围 | 默认 |
|---|---|---|---|
| `action` | 动作名（必填） | 见下表 | — |
| `steps` | 步数 | 1~100 | 3 |
| `speed` | 周期 ms（越小越快） | 100~3000 | 700 |
| `direction` | 1=前/左，-1=后/右，0=双手 | -1~1 | 1 |
| `amount` | 幅度 | 0~170 | 30 |
| `arm_swing` | 手臂摆动幅度 | 0~170 | 50 |

### 支持的动作

**基础移动**：`walk` · `turn` · `jump`

**特殊动作**：`swing` · `moonwalk` · `bend` · `shake_leg` · `updown` · `whirlwind_leg`

**固定动作**：`sit` · `showcase` · `home`

**手部动作（需配置手部舵机）**：`hands_up` · `hands_down` · `hand_wave` · `windmill` · `takeoff` · `fitness` · `greeting` · `shy` · `radio_calisthenics` · `magic_circle`

### 调用示例

```json
{"name": "self.musevibe.action", "arguments": {"action": "walk", "steps": 5, "speed": 800}}
{"name": "self.musevibe.action", "arguments": {"action": "swing", "steps": 5, "amount": 50}}
{"name": "self.musevibe.action", "arguments": {"action": "moonwalk", "steps": 3, "speed": 800, "direction": 1, "amount": 30}}
{"name": "self.musevibe.action", "arguments": {"action": "home"}}
{"name": "self.musevibe.stop", "arguments": {}}
```

## WebSocket 直连调试

`ws://<设备IP>:8080/ws` — JSON-RPC 2.0，无需经过云端即可触发所有动作。

```json
{"jsonrpc":"2.0","method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{}},"id":1}
{"jsonrpc":"2.0","method":"tools/list","params":{},"id":2}
{"jsonrpc":"2.0","method":"tools/call","params":{"name":"self.musevibe.action","arguments":{"action":"walk","steps":3,"speed":700,"direction":1}},"id":3}
{"jsonrpc":"2.0","method":"tools/call","params":{"name":"self.musevibe.get_status","arguments":{}},"id":4}
{"jsonrpc":"2.0","method":"tools/call","params":{"name":"self.musevibe.get_trims","arguments":{}},"id":5}
```

### 自定义舵机序列

**普通移动模式**：
```json
{"jsonrpc":"2.0","method":"tools/call","params":{"name":"self.musevibe.servo_sequences","arguments":{"sequence":"{\"a\":[{\"s\":{\"ll\":110,\"rl\":70},\"v\":800},{\"s\":{\"ll\":90,\"rl\":90},\"v\":800}],\"d\":0}"}},"id":6}
```

**振荡器模式（双臂摆动）**：
```json
{"jsonrpc":"2.0","method":"tools/call","params":{"name":"self.musevibe.servo_sequences","arguments":{"sequence":"{\"a\":[{\"osc\":{\"a\":{\"lh\":30,\"rh\":30},\"o\":{\"lh\":90,\"rh\":90},\"ph\":{\"rh\":180},\"p\":500,\"c\":5.0}}]}"}},"id":7}
```

### 舵机键名

| 键名 | 舵机 | 范围 |
|---|---|---|
| `ll` | 左腿 | 0=外展 / 90=中立 / 180=内收 |
| `rl` | 右腿 | 0=内收 / 90=中立 / 180=外展 |
| `lf` | 左脚 | 0=向上 / 90=水平 / 180=向下 |
| `rf` | 右脚 | 0=向下 / 90=水平 / 180=向上 |
| `lh` | 左手 | 0=向下 / 90=水平 / 180=向上 |
| `rh` | 右手 | 0=向上 / 90=水平 / 180=向下 |

## 动作参数建议

- **低速**：`speed` = 1200~1500（精确控制）
- **中速**：`speed` = 900~1200（日常推荐）
- **高速**：`speed` = 500~800（表演娱乐）
- **小幅度**：`amount` = 10~30
- **中幅度**：`amount` = 30~60
- **大幅度**：`amount` = 60~120

## 备注

- 每个动作执行完成后，机器人会自动回到初始位置（`home`），以便执行下一个动作；`sit` 与 `showcase` 例外。
- 所有参数都有合理默认值，可省略不需要自定义的参数。
- 动作在后台任务中执行，不阻塞主程序；支持动作队列，可连续执行多个动作。
- 手部动作需要配置手部舵机；未配置时，相关动作会被跳过。
