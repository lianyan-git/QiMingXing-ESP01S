# QiMingXing ESP-01S 自定义 AT 固件

把原本由 STM32（F103C8T6）承担的 **HTTP 解析 / 网页服务 / 多包重组** 工作全部转移到 ESP-01S，STM32 只需发一条 AT 指令、并在 UART 上按简单二进制协议收固件，从而：

- 减轻 STM32（仅 20KB RAM）负担
- 网页直接由 ESP 托管，打开更快
- 固件经 ESP 重组为 1KB 定长包、带 CRC16 与包级 ACK，显著降低丢包率

串口连接：ESP `GPIO1(TX)` / `GPIO3(RX)` → STM32 `USART1`（`PA10`/`PA9`），**115200 8N1**。

---

## 自定义 AT 指令

| 指令 | 说明 | STM32 用法 |
|------|------|------------|
| `AT` | 探测 ESP 是否就绪，回 `OK` | Bootloader 上电先发 `AT` 等 `OK` |
| `AT+OTAAP` | 开 SoftAP（`QiMingXing`/`12345678`）+ 网页上传固件；上传完成后自动按二进制协议经 UART 转发给 STM32 | Bootloader 发 `AT+OTAAP\r\n`，等 `OK\r\n` 后进入 UART 接收状态 |
| `AT+CFGAP` | 开 AP 配网：网页选周边 WiFi、输密码；连接成功后关 AP 切 STA 模式，凭据自动存入 Flash | 需配网时发 `AT+CFGAP\r\n`；收到 `+IP:` 表示配网成功 |
| `AT+STARTWEB` | 在 STA 模式下启动 Web 服务器（数据展示页），输出 `+IP:xxx.xxx.xxx.xxx` | 配网完成后发 `AT+STARTWEB\r\n` 开启网页 |
| `AT+PUSHDATA=<string>` | 把要显示的数据缓存到内存，供数据展示网页每 2 秒轮询显示 | APP 运行时定时发送，如 `AT+PUSHDATA=TEMP:25.3C\r\n` |
| `AT+OTACLOSE` | 关闭所有 Web Server，ESP 进入 Modem-Sleep 低功耗 | 固件升级完成或不需要网络时由 STM32 发送（**休眠时机由 STM32 控制，ESP 不自决**） |

> 所有指令以 `\r\n` 结尾；除 `AT+PUSHDATA` 外均回 `OK\r\n` 或 `ERROR\r\n`。

网页地址（配网模式连 `QiMingXing` 后访问）：
- `http://192.168.4.1/` — 随当前模式自动展示：固件上传页 / WiFi 配网页
- `http://192.168.4.1/data` — 返回最新 `PUSHDATA` 的 JSON（数据页轮询用）
- `http://192.168.4.1/startscan` — 触发一次 WiFi 扫描（空闲时才发起），返回 `{"ok":true}`
- `http://192.168.4.1/scan` — 返回扫描状态与结果：`{"scanning":<bool>,"nets":[{"ssid":"..."},...]}`；配网页进入时自动扫描一次，下拉只显示 SSID（不显示信号强度），需刷新时点页面 ⟳ 按钮触发新扫描，不再周期自动重扫

配网成功后 AP 自动关闭，ESP 仅以 STA 模式接入路由器。之后通过 `AT+STARTWEB` 启动的数据展示页通过路由器分配的 IP 访问（`http://<路由器分配的IP>/`）。

---

## OTA 串口协议（1KB/包，包级 ACK）

```
阶段1 握手： ESP → STM32: [0xAA 0x55 0x01] + [4字节固件大小 大端]
             STM32 → ESP:   0x06 (ACK)

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

网页在上传前用 JS 计算文件 CRC32 并随 `?crc=` 上传。ESP 收到文件后**再次计算 CRC32**，若与浏览器上报值不一致，判定为 WiFi 上传链路污染，拒绝转发（返回 `FIRMWARE CRC MISMATCH`），避免把坏固件传给 STM32 导致其"下载成功"却黑屏。浏览器会显示失败，用户重试即可（TCP 节流保证重试通常能成功）。

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

> ESP-01S 仅 1MB Flash / ~40KB 空闲 RAM：网页以 `PROGMEM` 存放，固件采用边收边转发（不缓存整个文件），可稳定处理 ≤~100KB 的 App 固件。

---

## 与 STM32 固件的配合

- **Bootloader**：`bootloader/bl_esp01s.c` 已改为上述二进制协议接收端，上电发 `AT+OTAAP` 后按协议写入外部 Flash，再拷贝到 App 分区。
- **App（待迁移）**：当前 `module/mod_wifi_manager.c`、`module/http_server.c` 仍使用官方 stock AT 指令（CWMODE / CIPSTART / …）。烧录本自定义固件后这些指令不再存在，**App 端需改为使用 `AT+CFGAP` / `AT+PUSHDATA` 等自定义指令**（后续任务）。在 App 迁移完成前，仅 Bootloader 的 OTA 升级链路可用。

### WiFi 凭据持久化

WiFi 凭据由 ESP8266 SDK 的 `WiFi.persistent(true)` 机制自动存入 Flash。配网成功后，`WiFi.begin(ssid, pass)` 会将 SSID 和密码写入 Flash 末扇区。重启后 `WiFi.begin()` 无参数自动使用上次的凭据连接，无需重新配网。

如需重新配网，可通过数据展示页的「重新配网」按钮清除凭据并重启 ESP，或由 STM32 发送 `AT+CFGAP` 进入配网模式。
