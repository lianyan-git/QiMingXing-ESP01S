# QiMingXing ESP-01S 自定义 AT 固件

ESP-01S 自定义固件：**网页托管 + WebSocket 透传 + OTA 转发 + 音乐固件上传 + WiFi 配网存参**。

架构（当前）：

- **网页托管**：OTA 上传页 / WiFi 配网页 / 数据仪表盘 / 音乐固件上传页 全部由 ESP 托管（`PROGMEM` 存网页，仪表盘 43KB 分块发送），STM32 不再解析 HTTP。
- **WebSocket 透传**：浏览器 WS 文本帧（端口 81 `/ws`）↔ UART 行协议（JSON 原样透传）——STM32 的 `{` 开头的 JSON 行广播给所有 WS 客户端，浏览器的 JSON 命令原样转发给 STM32，STM32 下发的 `AT+...` 行由本机当作指令处理。
- **OTA 转发**：网页上传 App 固件，ESP 重组为 1KB 定长包（包级 ACK + CRC16 + 整包 CRC32）经 UART 转发给 STM32。
- **音乐固件上传**：复用 OTA 同款二进制帧，把编好的音乐固件（MUB1）转发给 STM32 写入外部 Flash。
- **WiFi 配网存参**：EEPROM 存 5 组 `{SSID, PASS}`，自动联网失败自动转配网 AP。

串口连接：ESP `GPIO1(TX)` / `GPIO3(RX)` → STM32 `USART1`（`PA10`/`PA9`），**115200 8N1**。

---

## 自定义 AT 指令

| 指令 | 说明 | STM32 用法 |
|------|------|------------|
| `AT` | 探测 ESP 是否就绪，回 `OK` | 上电先发 `AT` 等 `OK` |
| `AT+OTAAP` | 开 SoftAP（`QIMINGXING`/`12345678`）+ OTA 上传页；上传完按二进制协议经 UART 转发 | Bootloader 发 `AT+OTAAP\r\n` 后进入 UART 接收状态 |
| `AT+CFGAP` | 开 AP 配网：网页选周边 WiFi、输密码；连接成功后写入空槽位 | 需配网时发 `AT+CFGAP\r\n` |
| `AT+CFGCLR` | 清空全部 WiFi 存参 | 重新配网前清除 |
| `AT+WEBSTART`（同 `AT+STARTWEB`） | 自动联网：无参→AP 配网；逐组尝试已存 SSID 连接；全部失败→AP 配网；成功回 `+IP:x.x.x.x` 并开仪表盘 | App 运行时发 `AT+WEBSTART\r\n` 开启数据仪表盘 |
| `AT+MUSICAP` | 开音乐上传 AP（已连 STA 时用 AP+STA 共存，不抢占路由器信道）；服务音乐上传页 | 音乐页"上传音乐"时发 `AT+MUSICAP\r\n` |
| `AT+MUSICCLOSE` | 关闭音乐 AP（不关 STA），回 `+MUSICCLOSED` | 取消/完成音乐上传后发 |
| `AT+PUSHDATA=<string>` | 缓存数据（数据展示兼容，主要数据走 WebSocket） | APP 定时发送 |
| `AT+WEBCLOSE` | 关闭所有 Web Server 与 WS，ESP 进入 Modem-Sleep 低功耗 | 无需网络时由 STM32 发送（**休眠时机由 STM32 控制**） |

> 所有指令以 `\r\n` 结尾；除 `AT+MUSICCLOSE` 外均回 `OK\r\n` 或 `ERROR\r\n`。

关键响应（STM32 据此判断状态）：`+IP:<ip>`（联网成功）、`+AP`（进入配网 AP）、`+CFG:<n>`（当前存参数）、`+TRY:<i>:<ssid>`（正在尝试某组）、`+CFGFAIL:<n>`（全部失败转配网）、`+MUSICAP`（音乐 AP 已开）、`+MUSICOK`（音乐固件转发完成）、`+MUSICERR`（音乐转发失败）、`+MUSICCLOSED`（音乐 AP 已关）。

网页地址（HTTP 端口 80，根路径按当前模式自动返回对应页）：

- `http://192.168.4.1/` — 随模式：OTA 上传页 / WiFi 配网页 / 数据仪表盘 / 音乐固件上传页
- `http://192.168.4.1/scan` — 返回扫描状态与结果 `{"scanning":<bool>,"nets":[{"ssid":"..."},...]}`
- `http://192.168.4.1/startscan` — 触发一次 WiFi 扫描（空闲时才发起，<10s 复用缓存）
- `ws://<ip>:81/ws` — WebSocket 透传端点（JSON 原样双向透传）

---

## WiFi 凭据持久化（EEPROM）

存参存于 **专用扇区 0xEB**（1M64 布局的 SPIFFS 保留区，本固件从不挂载 SPIFFS），`CFG_MAGIC=QMX2`，最多 **5 组** `{SSID(33B), PASS(65B)}`。配网页 `/connect` 连接成功后写入一个空槽位（满则替换槽 0）。

> 旧版曾用默认 EEPROM 扇区 `0xFB`，该区与 SDK 分区表的 PHY_DATA 叠加，**真实断电冷启动会扰动存参**（不断电时 ESP 未真冷启动而表现正常）。改存 `0xEB` 并升 magic 作废旧区，修复"不断电能连、真断电重启需重新配网"。

---

## OTA 串口协议（1KB/包，包级 ACK）

```
阶段1 握手： ESP → STM32: [0xAA 0x55 0x01] + [4字节固件大小 大端]
             STM32 → ESP:   0x06 (ACK)
             ※ STM32 收到握手后先整区擦除内部 App 分区（约 2s），擦完才回 ACK。
               故 ESP 握手后等待 ACK 放宽到 10s，覆盖擦除耗时后再发数据包，
               避免 ESP 提前发包撞上擦除（单字节 USART 缓冲会丢字节）。

阶段2 数据包：ESP → STM32: [0xAA] + [2字节包序号大端] + [≤1024数据]
                          + [2字节CRC16] + [0x55]
             STM32 → ESP:   0x06 (ACK) 或 0x15 (NAK，要求重传当前包)

阶段3 结束： ESP → STM32: [0xAA 0x55 0x02] + [4字节总CRC32大端]
             STM32 → ESP:   0x06 (ACK)

阶段4 完成： ESP → STM32: 0xDD
```

- **CRC16** = Modbus 标准（多项式 `0x8005`/反射 `0xA001`，初值 `0xFFFF`），覆盖 **包序号(2B) + 数据(NB)**。
- **CRC32** = IEEE 802.3（与 STM32 端 `CRC32_Calculate` 一致），覆盖 **整个固件**。
- 包内不含长度字段，STM32 根据握手得到的总大小与包序号推算每包数据长度：`包总数 = (总大小 + 1023) / 1024`；第 `i` 包(0基) 数据长度 = `i==包总数-1 ? 总大小 - i*1024 : 1024`。

### 浏览器端 CRC 校验（挡住 WiFi 上传污染）

网页在上传前用 JS 计算文件 CRC32 并随 `?crc=` 上传。ESP 收到文件后**再次计算 CRC32**，若与浏览器上报值不一致，判定为 WiFi 上传链路污染，拒绝转发（返回 `FIRMWARE CRC MISMATCH`），避免把坏固件传给 STM32 导致其"下载成功"却黑屏。

音乐固件上传走 `/music?size=`（multipart，`server.upload()` 必需），校验由 App 端完成，ESP 仅表示已转发（成功回 `+MUSICOK` 后自动关 AP，失败回 `+MUSICERR`）。

---

## 烧录（Arduino IDE）

1. Arduino IDE → 文件 → 首选项 → 附加开发板管理器网址，添加：
   `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
2. 开发板管理器安装 **esp8266 by ESP8266 Community**。
3. 工具 → 开发板 → **Generic ESP8266 Module**，配置：
   - Flash Size：`1M (no SPIFFS)`（或 `1M`）
   - CPU Frequency：`80 MHz`
   - Upload Speed：`115200`
   - 复位方式/烧录模式：按你的 USB-TTL 模块选择（通常 `esp01`/`dio`）
4. 用 USB-TTL 接 ESP-01S：`TX→RX`、`RX→TX`、`GND→GND`、`VCC→3.3V`；烧录时 `GPIO0` 拉低（接 GND）进入下载模式，烧完断开。
5. 打开 `QiMingXing_ESP_AT.ino`，编译并上传。

> ESP-01S 仅 1MB Flash / ~40KB 空闲 RAM：网页以 `PROGMEM` 存放（仪表盘 `web_page.h` 分 13 块 ×3800B），固件采用边收边转发（不缓存整个文件）。

---

## 与 STM32 固件的配合

- **Bootloader**（`bootloader/bl_esp01s.c`）：上电发 `AT` 等 `OK` → `AT+OTAAP` → 按二进制协议收固件直写内部 App 分区。
- **App**（`module/esp_link.c`）：运行时发 `AT+WEBSTART` 联网开仪表盘，JSON 行命令（`HELLO/PRESET_*/PARAM_SET/RUN/GLOBAL/CAN_MODE`）经 WS 透传，`AT+CFGAP` 配网、`AT+MUSICAP` 上传音乐、`AT+WEBCLOSE` 休眠。

---

## 更新日志

### 2026-09-18

#### 变更
- **协议助手全部改为 bool 返回，失败立即停传**：`uart_send_handshake/packet/end`、`uart_music_handshake/end`、`uart_lang_handshake/end` 重试 5 次仍无 ACK 返回 `false`；`handle_music_upload` / `handle_lang_upload` 在**握手失败或任一分包 ACK 失败时立即终止传输**（`musicInProgress/langInProgress=false`），后续 WRITE 只吞字节不再往 UART 灌，END 回 400 —— 避免"STM32 等 ACK、ESP 盲发下一包"导致两边状态机失步（此前是 5 次失败后静默继续上传的经典故障源）

#### 修复
- 与 STM32 端同步的失败路径闭环：STM32 上传溢出/超时会 Abort 并停止 ACK，ESP 侧在 5 次重试耗尽后终止并以 `+MUSICERR`/`+LANGERR` + 400 结束，不再无限续传

### 2026-09-16

#### 新增
- **语言字库上传（`WEB_LANG` 模式 + `AT+LANGAP`/`AT+LANGCLOSE`）**：新增语言字库上传页 `/lang?size=`（PROGMEM，文本框选 `.bin` 字库 + 进度条），AP+STA 共存；复用 `0xAA 0x55` 二进制帧但**帧型独立**：握手 `0x13`、结束 `0x14`（与 OTA `0x01/0x02`、音乐 `0x11/0x12` 区分开）；握手等待 ACK 放宽到 30s（覆盖 STM32 预擦整段字库 10-20s），包级 ACK/NAK 与 OTA 一致
- **音乐握手 ACK 等待放宽**：`uart_music_handshake` 从 3s 提至 10s（等待 ACK 打开窗口一致覆盖 STM32 按包擦写后的首包 ACK 时序）

#### 修复
- **真断电/会话残留导致 AP 打不开**：确认 `AT+MUSICAP`/`AT+LANGAP` 命令的执行顺序为**先返回 `+MUSICAP`/`+LANGAP` 确认行、再回 `OK`**——STM32 端已改按确认行决定显示热点与开传，本固件侧命令语义不变（供主机严格按返回行判断 AP 真实状态）

### 2026-09-13

#### 新增
- **WebSocket 双向透传**：浏览器 WS 文本帧（端口 81 `/ws`）↔ UART 行协议 JSON 原样透传；STM32 的 `{` 行广播给全部 WS 客户端，浏览器的 JSON 命令原样转发给 STM32，替代旧的"网页轮询 PUSHDATA"数据展示方案
- **数据仪表盘**：`web_page.h`（`web2c.py` 由 `web.txt` 生成，43KB PROGMEM 分 13 块），含设备卡片/温湿度曲线/预设管理/级联开关；根路径 `WEB_DASH` 模式分块 `sendContent_P` 发送
- **音乐固件上传**：`WEB_MUSIC` 模式 + `AT+MUSICAP`/`AT+MUSICCLOSE`，AP+STA 共存（softAP 信道自动随 STA，不抢占路由器信道）；音乐上传页 `/music?size=`（FormData multipart，修复 `server.upload()` 不解析裸文件导致进度/列表异常）；复用 OTA `0xAA` 二进制帧转发，成功后 `+MUSICOK` 自动关 AP、失败 `+MUSICERR`
- **WiFi 存参（EEPROM 5 组）**：`AT+WEBSTART` 自动联网（无参→AP 配网；逐组尝试已存 SSID 连接，最多 4 轮；全失败→AP 配网）；`AT+CFGCLR` 清参；配网页 `/connect` 写空槽位
- **配网扫描优化**：扫描结果 <10s 复用缓存，`/scan` 返回 `{scanning, nets}`，配网页按 SSID 精确匹配

#### 修复
- **真断电重启后 WiFi 存参失效**：存参从默认 EEPROM 扇区 `0xFB`（与 SDK PHY_DATA 分区叠加，冷启动被扰动）改存专用扇区 `0xEB`，`CFG_MAGIC` 升 `QMX2` 作废旧区数据
- **音乐上传无进度/列表为空**：上传页改用 `FormData` 发送（`server.upload()` 只解析 multipart），握手成功置进度态，弹窗进度接线
- **AP SSID 大小写**：`QiMingXing` → `QIMINGXING`，与 STM32 端一致

### 2026-09-08 及更早

- WiFi 凭据由 SDK `persistent` 机制持久化（现已被 EEPROM 5 组方案替代）
- OTA 串口协议 1KB/包 + 包级 ACK + 浏览器端 CRC32 校验
- 握手 ACK 等待放宽到 10s（覆盖 STM32 擦除 App 分区耗时）
