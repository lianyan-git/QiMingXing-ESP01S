/*************************************************************
 * QiMingXing ESP-01S 自定义 AT 固件 (Arduino / ESP8266 core)
 *
 * 设计目标：本机由 STM32 下发 WEBSTART 指令后自助联网/配网，
 * 网页端(移植自 web.txt) 通过 WebSocket 与 STM32 双向透传：
 *   - 浏览器 WS 文本帧 <-> UART 行协议（JSON 原样透传）
 *   - STM32 的 '{' 行 -> 广播给所有 WS 客户端
 *   - STM32 的 "AT+..." 行 -> 本机指令
 *
 * WiFi 存参：本机内部 Flash(EEPROM) 可存 5 组 {SSID, PASSWORD}。
 * 打开 WiFi(AT+WEBSTART) 时：
 *   ① 无任何存参 -> 开 AP 配网(QiMingXing/12345678)
 *   ② 依次尝试每组合法存参：
 *      - 同步扫描附近网络，按 SSID 精确匹配
 *      - 未匹配则每 2s 重新扫描，同一组扫 3 次未命中 -> 换下一组
 *      - 匹配则 begin() 等待连接(≤10s)，成功 -> "+IP:x.x.x.x"
 *   ③ 5 组全部失败 -> 开 AP 配网
 * 配网页 /connect 连接成功后把 {SSID,PASS} 写入一个空槽位(满则替换槽0)。
 *
 * WebSocket: 端口 81（网页 /ws 已适配为 :81/ws）。
 * HTTP: 端口 80，根路径按模式返回 配网页 / 仪表盘(web.txt)/ OTA页。
 *
 * OTA 串口协议(1KB/包, 包级 ACK)与 README 一致，原样保留。
 *************************************************************/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <EEPROM.h>
#include "web_page.h"

#define UART_BAUD   115200
#define AP_SSID     "QIMINGXING"
#define AP_PASS     "12345678"
#define OTA_PKT_MAX 1024
#define WS_PORT     81

#define CFG_MAX     5
#define CFG_MAGIC   0x514D5832UL       /* "QMX2"：v2 数据（布局修复 + 存参改存 0xEB 扇区），作废旧 0xFB 区数据 */
#define CFG_SLOT_SZ (33 + 65)          /* 每槽：ssid 33B + pass 65B（均含结尾 0） */
#define CFG_DATA_OFF 10                /* 数据区起点 = magic4 + count1 + valid5 */

ESP8266WebServer server(80);
WebSocketsServer ws(WS_PORT);

enum WebMode { WEB_OTA, WEB_CONFIG, WEB_DASH, WEB_MUSIC };
WebMode webMode = WEB_OTA;
bool    webActive = false;

String lastData   = "";
String stationIP  = "";

/* 配网成功后保留 AP 的宽限期(ms)终点；到点由 loop() 自动关闭 AP，手机已跳转完成 */
unsigned long ap_shutdown_at = 0;

/* ---- 配网 WiFi 扫描状态 ---- */
String  lastScanJson = "[]";
int     scanState    = 0;     /* 1=扫描中 */
uint32_t lastScanDoneMs = 0;  /* 最近一次扫描完成时间戳(毫秒)，用于缓存复用 */
bool    wifiConnecting = false; /* 自动连网进行中：重复 WEBSTART 忽略 */

/* ---- 5 组存参 ---- */
struct WifiCfg {
    char ssid[33];
    char pass[65];
};
static WifiCfg s_cfg[CFG_MAX];
static uint8_t s_cfg_valid[CFG_MAX];
static uint8_t s_cfg_count = 0;

/* 存参存储：专用扇区 0xEB（0x402EB000，1M64 布局的 SPIFFS 保留区，本固件从不挂载 SPIFFS）。
 * 旧版默认扇区 0xFB 是 SDK 经典 EEPROM 区，且 SDK 分区表把 PHY_DATA 分区叠加在其上——
 * 真实断电冷启动会完整走 SDK 分区/PHY 初始化，可能扰动该扇区（不断电时 ESP 未真正冷启动则表现为正常）。
 * 0xFC=RF校准 / 0xFD-=系统参数 均同样远离。 */
static EEPROMClass cfg_ee(0xEB);

/* ---- OTA 转发状态 ---- */
bool     otaInProgress = false;
uint32_t otaTotal      = 0;
uint32_t otaRecv       = 0;
uint16_t otaSeq        = 0;
uint32_t otaCrc        = 0xFFFFFFFF;
uint32_t otaExpCrc     = 0;
uint8_t  otaBuf[2048];
uint16_t otaBufLen     = 0;

/* ---- 音乐上传状态 ---- */
uint32_t musicTotal    = 0;

String cmdLine = "";

/* ===================== CRC ===================== */
static uint16_t ota_crc16(uint16_t seq, const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    uint8_t  sb[2] = { (uint8_t)(seq >> 8), (uint8_t)(seq & 0xFF) };
    for (int i = 0; i < 2; i++) {
        crc ^= sb[i];
        for (int b = 0; b < 8; b++) crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
    }
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
    }
    return crc;
}

static uint32_t ota_crc32_upd(uint32_t crc, uint8_t b)
{
    crc ^= b;
    for (int i = 0; i < 8; i++) crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320UL : (crc >> 1);
    return crc;
}

/* ===================== UART 辅助 ===================== */
static int uart_wait_ack(int timeoutMs)
{
    unsigned long t = millis();
    while ((int)(millis() - t) < timeoutMs) {
        if (Serial.available()) {
            int b = Serial.read();
            if (b == 0x06) return 1;
            if (b == 0x15) return -1;
        }
        yield();
    }
    return 0;
}

static void uart_send_handshake(uint32_t size)
{
    uint8_t hdr[7] = { 0xAA, 0x55, 0x01,
                       (uint8_t)(size >> 24), (uint8_t)(size >> 16),
                       (uint8_t)(size >> 8),  (uint8_t)size };
    Serial.write(hdr, 7);
    Serial.flush();
    uart_wait_ack(10000);
}

static void uart_send_packet(uint16_t seq, const uint8_t *data, uint16_t len)
{
    for (int attempt = 0; attempt < 5; attempt++) {
        uint8_t hdr[3] = { 0xAA, (uint8_t)(seq >> 8), (uint8_t)(seq & 0xFF) };
        uint16_t crc = ota_crc16(seq, data, len);
        uint8_t crcb[2] = { (uint8_t)(crc >> 8), (uint8_t)(crc & 0xFF) };
        Serial.write(hdr, 3);
        Serial.write(data, len);
        Serial.write(crcb, 2);
        Serial.write(0x55);
        Serial.flush();
        int r = uart_wait_ack(3000);
        if (r == 1) return;
    }
}

static void uart_send_end(uint32_t crc)
{
    uint8_t hdr[7] = { 0xAA, 0x55, 0x02,
                       (uint8_t)(crc >> 24), (uint8_t)(crc >> 16),
                       (uint8_t)(crc >> 8),  (uint8_t)crc };
    Serial.write(hdr, 7);
    Serial.flush();
    uart_wait_ack(3000);
}

/* 音乐帧：握手 0x11 / 结束 0x12（与 OTA 帧 0x01/0x02 区分，App 端音乐接收器识别） */
static void uart_music_handshake(uint32_t size)
{
    uint8_t hdr[7] = { 0xAA, 0x55, 0x11,
                       (uint8_t)(size >> 24), (uint8_t)(size >> 16),
                       (uint8_t)(size >> 8),  (uint8_t)size };
    for (int a = 0; a < 5; a++) {
        Serial.write(hdr, 7);
        Serial.flush();
        if (uart_wait_ack(3000) == 1) return;
    }
}

static void uart_music_end(uint32_t crc)
{
    uint8_t hdr[7] = { 0xAA, 0x55, 0x12,
                       (uint8_t)(crc >> 24), (uint8_t)(crc >> 16),
                       (uint8_t)(crc >> 8),  (uint8_t)crc };
    for (int a = 0; a < 5; a++) {
        Serial.write(hdr, 7);
        Serial.flush();
        if (uart_wait_ack(3000) == 1) return;
    }
}

/* ===================== 5 组存参读写 =====================
 * 布局: [magic4 0-3][count1 4][valid5 5-9][数据区 10..] 每槽 33+65=98B x5
 * 注：数据区必须从 10 起（跳过 valid 的 5-9），旧版从 9 起导致 valid[4] 与槽0 SSID 首字节同址互相覆盖。 */
static void load_cfg()
{
    uint32_t magic = 0;
    uint8_t i;
    s_cfg_count = 0;
    for (i = 0; i < CFG_MAX; i++) s_cfg_valid[i] = 0;
    cfg_ee.begin(512);
    magic = (uint32_t)cfg_ee.read(0) | ((uint32_t)cfg_ee.read(1) << 8) |
            ((uint32_t)cfg_ee.read(2) << 16) | ((uint32_t)cfg_ee.read(3) << 24);
    if (magic != CFG_MAGIC) return;
    s_cfg_count = cfg_ee.read(4);
    if (s_cfg_count > CFG_MAX) s_cfg_count = CFG_MAX;
    for (i = 0; i < CFG_MAX; i++) {
        uint16_t off = CFG_DATA_OFF + i * CFG_SLOT_SZ;
        uint8_t v = cfg_ee.read(5 + i);
        s_cfg_valid[i] = (v == 1) ? 1 : 0;
        if (!s_cfg_valid[i]) continue;
        for (uint16_t k = 0; k < 33; k++) s_cfg[i].ssid[k] = (char)cfg_ee.read(off + k);
        s_cfg[i].ssid[32] = 0;
        for (uint16_t k = 0; k < 65; k++) s_cfg[i].pass[k] = (char)cfg_ee.read(off + 33 + k);
        s_cfg[i].pass[64] = 0;
    }
}

static void save_cfg()
{
    uint8_t i;
    cfg_ee.begin(512);
    cfg_ee.write(0, (uint8_t)(CFG_MAGIC & 0xFF));
    cfg_ee.write(1, (uint8_t)((CFG_MAGIC >> 8) & 0xFF));
    cfg_ee.write(2, (uint8_t)((CFG_MAGIC >> 16) & 0xFF));
    cfg_ee.write(3, (uint8_t)((CFG_MAGIC >> 24) & 0xFF));
    cfg_ee.write(4, s_cfg_count);
    for (i = 0; i < CFG_MAX; i++) {
        uint16_t off = CFG_DATA_OFF + i * CFG_SLOT_SZ;
        cfg_ee.write(5 + i, s_cfg_valid[i] ? 1 : 0);
        if (!s_cfg_valid[i]) continue;
        for (uint16_t k = 0; k < 32; k++) cfg_ee.write(off + k, (uint8_t)s_cfg[i].ssid[k]);
        cfg_ee.write(off + 32, 0);
        for (uint16_t k = 0; k < 64; k++) cfg_ee.write(off + 33 + k, (uint8_t)s_cfg[i].pass[k]);
        cfg_ee.write(off + 33 + 64, 0);
    }
    if (!cfg_ee.commit()) { Serial.println("+CFGEW"); return; }   /* 写 Flash 失败 */

    /* 回读校验：重新从 Flash 读回，确认本次写入确实落盘（防止地址越界/被 SDK 扰动/写保护） */
    cfg_ee.begin(512);
    uint32_t m2 = (uint32_t)cfg_ee.read(0) | ((uint32_t)cfg_ee.read(1) << 8) |
                  ((uint32_t)cfg_ee.read(2) << 16) | ((uint32_t)cfg_ee.read(3) << 24);
    if (m2 != CFG_MAGIC || cfg_ee.read(4) != s_cfg_count) { Serial.println("+CFGVR:FAIL"); return; }
    for (i = 0; i < CFG_MAX; i++) {
        uint16_t off = CFG_DATA_OFF + i * CFG_SLOT_SZ;
        uint8_t v = cfg_ee.read(5 + i);
        if ((v == 1) != (s_cfg_valid[i] == 1)) { Serial.println("+CFGVR:FAIL"); return; }
        if (s_cfg_valid[i] && (uint8_t)cfg_ee.read(off) != (uint8_t)s_cfg[i].ssid[0]) { Serial.println("+CFGVR:FAIL"); return; }
    }
    Serial.println("+CFGVR:OK");
}

/* 新配网连接成功后写入：先覆盖同名，其次空槽；全满则替换槽0并前移 */
static void store_cfg(const char *ssid, const char *pass)
{
    uint8_t i;
    int idx = -1;
    for (i = 0; i < CFG_MAX; i++) {            /* 1) 同名覆盖 */
        if (s_cfg_valid[i] && strcmp(s_cfg[i].ssid, ssid) == 0) { idx = (int)i; break; }
    }
    if (idx < 0) {                              /* 2) 空槽 */
        for (i = 0; i < CFG_MAX; i++)
            if (!s_cfg_valid[i]) { idx = (int)i; break; }
    }
    if (idx < 0) {                              /* 3) 满：整体前移，新值进槽0 */
        for (i = CFG_MAX - 1; i > 0; i--) {
            s_cfg_valid[i] = s_cfg_valid[i - 1];
            if (s_cfg_valid[i]) {
                strncpy(s_cfg[i].ssid, s_cfg[i - 1].ssid, 32);
                strncpy(s_cfg[i].pass, s_cfg[i - 1].pass, 64);
            }
        }
        idx = 0;
    }
    i = (uint8_t)idx;
    if (!s_cfg_valid[i] && s_cfg_count < CFG_MAX) s_cfg_count++;   /* 仅新增槽才计数 */
    s_cfg_valid[i] = 1;
    strncpy(s_cfg[i].ssid, ssid, 32); s_cfg[i].ssid[32] = 0;
    strncpy(s_cfg[i].pass, pass, 64); s_cfg[i].pass[64] = 0;
    save_cfg();
}

static void clear_cfg()
{
    uint8_t i;
    for (i = 0; i < CFG_MAX; i++) s_cfg_valid[i] = 0;
    s_cfg_count = 0;
    save_cfg();
}

/* ===================== 网页 (HTML) ===================== */
static const char OTA_PAGE[] PROGMEM = R"=====(
<!DOCTYPE html><html lang=zh><head><meta charset=UTF-8>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>启明星固件升级</title>
<style>
 body{font-family:-apple-system,Segoe UI,Arial,sans-serif;margin:0;background:#0f1830;color:#eaf0ff;display:flex;min-height:100vh;align-items:center;justify-content:center}
 .card{background:#172347;border:1px solid #2a3a66;border-radius:18px;padding:32px;width:340px;box-shadow:0 10px 40px rgba(0,0,0,.4)}
 h2{margin:0 0 6px;color:#7cc4ff}
 p.sub{margin:0 0 22px;color:#8a97c0;font-size:13px}
 .box{border:2px dashed #3b6cff;border-radius:12px;padding:18px;text-align:center;background:#101a36;margin-bottom:18px}
 input[type=file]{width:100%;color:#cfe0ff;font-size:13px}
 button{width:100%;background:linear-gradient(90deg,#3b6cff,#7c4dff);color:#fff;border:0;border-radius:12px;padding:14px;font-size:16px;cursor:pointer}
 button:disabled{opacity:.5}
 .bar{height:10px;background:#101a36;border-radius:6px;overflow:hidden;margin-top:18px;display:none}
 .bar>i{display:block;height:100%;width:0;background:linear-gradient(90deg,#3bff9e,#3b6cff);transition:width .2s}
 .msg{margin-top:14px;font-size:13px;color:#9fb0e0;min-height:18px}
</style></head>
<body><div class=card>
 <h2>固件升级</h2><p class=sub>选择 .bin 固件，点击上传。ESP 接收后将自动转发给主控。</p>
 <div class=box><input type=file id=f accept=.bin></div>
 <button id=b onclick=up()>上传并更新</button>
 <div class=bar id=bar><i></i></div>
 <div class=msg id=m></div>
</div>
<script>
function crc32(arr){
 var crc=0xFFFFFFFF;
 for(var i=0;i<arr.length;i++){
  crc^=arr[i];
  for(var j=0;j<8;j++) crc=(crc&1)?(crc>>>1^0xEDB88320):(crc>>>1);
 }
 return (crc^0xFFFFFFFF)>>>0;
}
function up(){
 var f=document.getElementById('f'); if(!f.files[0]){alert('请先选择固件');return;}
 var file=f.files[0], btn=document.getElementById('b'), bar=document.getElementById('bar'), fill=bar.firstChild, m=document.getElementById('m');
 btn.disabled=true; bar.style.display='block'; m.textContent='校验文件中…';
 var reader=new FileReader();
 reader.onload=function(){
  var buf=new Uint8Array(reader.result);
  var crc=crc32(buf);
  m.textContent='正在上传…';
  var fd=new FormData(); fd.append('file',file);
  var x=new XMLHttpRequest();
  x.upload.onprogress=function(e){ if(e.total){ fill.style.width=(e.loaded*100/e.total)+'%'; } };
  x.onload=function(){ if(x.status==200){ m.textContent='✅ 上传完成，已转发给主控'; } else { m.textContent='❌ 校验失败('+x.status+')，请重试'; } btn.disabled=false; };
  x.onerror=function(){ m.textContent='❌ 上传失败，请重试'; btn.disabled=false; };
  x.open('POST','/upload?size='+file.size+'&crc='+crc.toString(16),true); x.send(fd);
 };
 reader.onerror=function(){ m.textContent='❌ 读取文件失败'; btn.disabled=false; };
 reader.readAsArrayBuffer(file);
}
</script></body></html>
)=====";

static const char CONFIG_PAGE[] PROGMEM = R"=====(
<!DOCTYPE html><html lang=zh><head><meta charset=UTF-8>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>启明星配网</title>
<style>
 body{font-family:-apple-system,Segoe UI,Arial,sans-serif;margin:0;background:#0f1830;color:#eaf0ff;display:flex;min-height:100vh;align-items:center;justify-content:center}
 .card{background:#172347;border:1px solid #2a3a66;border-radius:18px;padding:32px;width:340px;box-shadow:0 10px 40px rgba(0,0,0,.4)}
 h2{margin:0 0 22px;color:#7cc4ff}
 .row{display:flex;gap:10px;margin-bottom:16px}
 select{flex:1;min-width:0;padding:12px;background:#101a36;border:1px solid #2a3a66;border-radius:10px;color:#eaf0ff;box-sizing:border-box}
 .rf{flex:0 0 46px;background:#101a36;border:1px solid #2a3a66;border-radius:10px;color:#7cc4ff;font-size:20px;cursor:pointer;padding:0}
 .rf:active{background:#1c2a52}
 input{width:100%;padding:12px;margin-bottom:16px;background:#101a36;border:1px solid #2a3a66;border-radius:10px;color:#eaf0ff;box-sizing:border-box}
 .passrow{position:relative;margin-bottom:16px}
 .passrow input{padding-right:46px;margin-bottom:0;box-sizing:border-box}
 .eye{position:absolute;right:4px;top:0;bottom:0;margin:auto 0;width:38px;height:38px;background:none;border:0;color:#7cc4ff;cursor:pointer;opacity:.9;text-align:center;padding:0;display:flex;align-items:center;justify-content:center}
 .eye svg{width:22px;height:22px}
 .eye:active{opacity:1}
 button.go{width:100%;background:linear-gradient(90deg,#3b6cff,#7c4dff);color:#fff;border:0;border-radius:12px;padding:14px;font-size:16px;cursor:pointer}
 .msg{margin-top:14px;font-size:13px;color:#9fb0e0;min-height:18px}
 .saved{color:#7cffb0;font-size:12px;margin-bottom:10px}
</style></head>
<body><div class=card>
 <h2>WiFi 配网</h2>
 <div class=row>
  <select id=ssid><option value="">正在扫描附近网络…</option></select>
  <button class=rf id=rf onclick=doScan() title="刷新">⟳</button>
 </div>
 <div class=passrow>
  <input id=pass type=password placeholder="WiFi 密码（开放网络留空）">
  <button class=eye id=eye onclick=togglePass() type=button title="显示/隐藏密码" aria-label="显示/隐藏密码">
   <svg id=eyeSvg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/><circle cx="12" cy="12" r="3"/></svg>
  </button>
 </div>
 <button class=go onclick=conn()>连接</button>
 <div class=msg id=m>进入页面自动扫描一次，可点 ⟳ 手动刷新</div>
 <div class=saved id=sv></div>
</div>
<script>
var polling=null;
function togglePass(){
 var p=document.getElementById('pass');
 var show=(p.type==='password');
 p.type=show?'text':'password';
 document.getElementById('eyeSvg').innerHTML = show
  ? '<path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/><circle cx="12" cy="12" r="3"/>'
  : '<path d="M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19m-6.72-1.07a3 3 0 1 1-4.24-4.24"/><line x1="1" y1="1" x2="23" y2="23"/>';
 p.focus();
}
function render(a){
 var s=document.getElementById('ssid'), sel=s.value, m=document.getElementById('m');
 s.innerHTML='';
 if(!a.length){ s.innerHTML='<option value="">未找到网络</option>'; m.textContent='未找到网络，请点击 ⟳ 手动刷新'; return; }
 a.forEach(function(n){ var o=document.createElement('option'); o.value=n.ssid; o.textContent=n.ssid; if(n.ssid===sel)o.selected=true; s.appendChild(o); });
 m.textContent='共 '+a.length+' 个网络';
}
function poll(){
 fetch('/scan').then(function(r){return r.json();}).then(function(j){
  var rf=document.getElementById('rf');
  if(j.scanning){ document.getElementById('m').textContent='扫描中…'; rf.style.opacity=.5; polling=setTimeout(poll,700); return; }
  rf.style.opacity=1; render(j.nets);
 }).catch(function(){ document.getElementById('m').textContent='扫描失败，请点击 ⟳ 手动刷新'; document.getElementById('rf').style.opacity=1; });
}
function doScan(){ clearTimeout(polling); fetch('/startscan').then(poll).catch(poll); }
function conn(){ var ssid=document.getElementById('ssid').value, pass=document.getElementById('pass').value, m=document.getElementById('m');
 if(!ssid){ m.textContent='请先选择一个网络'; return; }
 m.textContent='连接中…';
 fetch('/connect',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass)})
  .then(function(r){return r.json();}).then(function(j){
   if(j.ok){
    m.innerHTML='✅ 已连接 <b>'+j.ip+'</b><br>正在跳转仪表盘…';
    goDash(j.ip);
   } else m.textContent='❌ 连接失败，请检查密码';
  })
  .catch(function(){ m.textContent='❌ 连接失败'; });
}
/* 配网完成后自动跳转：优先跳新 IP(手机已切回该 WiFi 时可达)；
 * 若 4s 内探测不通(手机仍挂在设备 AP 上)，自动退回设备 AP 的仪表盘地址。 */
function goDash(ip){
 var moved=false, t=setTimeout(function(){
  if(!moved){ moved=true; try{ location.href='http://192.168.4.1/'; }catch(e){} }
 },4000);
 var img=new Image();
 img.onload=function(){ if(!moved){ moved=true; clearTimeout(t); try{ location.href='http://'+ip+'/'; }catch(e){} } };
 img.onerror=function(){};
 img.src='http://'+ip+'/data?_='+Date.now();
}
function showSaved(){
 fetch('/cfgcount').then(function(r){return r.json();}).then(function(j){
  document.getElementById('sv').textContent='已保存 '+j.count+'/5 组 WiFi';
 }).catch(function(){});
}
/* 载入先读缓存：AP 开启时已提前扫描并缓存，直接显示；空/过期才触发新扫描 */
window.onload=function(){
 showSaved();
 clearTimeout(polling);
 fetch('/scan').then(function(r){return r.json();}).then(function(j){
  var rf=document.getElementById('rf');
  if(j.scanning){ document.getElementById('m').textContent='扫描中…'; rf.style.opacity=.5; polling=setTimeout(poll,700); return; }
  rf.style.opacity=1;
  if(j.nets && j.nets.length){ render(j.nets); }
  else if(j.nets && !j.nets.length){ doScan(); }
  else { doScan(); }
 }).catch(doScan);
};
</script></body></html>
)=====";

/* 音乐上传页（AP 模式下服务，内联资源，无外网依赖） */
static const char MUSIC_PAGE[] PROGMEM = R"=====(
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>音乐固件上传</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{
  min-height:100vh;display:flex;align-items:center;justify-content:center;
  font-family:"PingFang SC","Microsoft YaHei",-apple-system,Helvetica,Arial,sans-serif;
  background:linear-gradient(160deg,#12141f 0%,#1c1f35 45%,#241b3a 100%);
  padding:16px;color:#eef1ff;
}
.card{
  width:100%;max-width:420px;background:rgba(30,34,58,.72);
  border:1px solid rgba(255,255,255,.10);border-radius:20px;
  padding:28px 24px 24px;backdrop-filter:blur(6px);
  box-shadow:0 18px 50px rgba(0,0,0,.45);
}
.top{display:flex;align-items:center;gap:12px;margin-bottom:6px}
.note{width:46px;height:46px;border-radius:14px;flex:none;
  background:linear-gradient(135deg,#8b5cf6,#d946ef);display:flex;align-items:center;justify-content:center;
  font-size:22px;color:#fff;box-shadow:0 6px 16px rgba(139,92,246,.4)}
h1{font-size:19px;font-weight:600}
.sub{color:#9aa0c6;font-size:12px;line-height:1.6;margin:2px 0 18px}
.drop{
  border:2px dashed rgba(255,255,255,.22);border-radius:14px;padding:26px 14px;
  text-align:center;cursor:pointer;transition:.18s;background:rgba(255,255,255,.03);
}
.drop:hover,.drop.over{border-color:#a78bfa;background:rgba(167,139,250,.08)}
.drop .big{font-size:13px;color:#dfe3ff}
.drop .small{font-size:11px;color:#7c82a8;margin-top:6px}
.file{display:none;margin-top:14px;padding:12px 14px;border-radius:12px;
  background:rgba(255,255,255,.06);font-size:12px;color:#dfe3ff;word-break:break-all}
.file b{color:#c4b5fd}
.ctrl{display:none;margin-top:18px}
button{
  width:100%;padding:14px;border:0;border-radius:12px;font-size:15px;font-weight:600;
  color:#fff;cursor:pointer;background:linear-gradient(135deg,#7c3aed,#db2777);
  box-shadow:0 8px 20px rgba(124,58,237,.35);transition:.15s;
}
button:disabled{opacity:.45;cursor:not-allowed;box-shadow:none}
.barbox{margin-top:16px;display:none}
.bar{height:10px;border-radius:6px;background:rgba(255,255,255,.10);overflow:hidden}
.bar>i{display:block;height:100%;width:0%;border-radius:6px;
  background:linear-gradient(90deg,#7c3aed,#22d3ee);transition:width .25s}
.pct{text-align:right;font-size:11px;color:#9aa0c6;margin-top:6px}
#st{margin-top:16px;font-size:13px;text-align:center;min-height:18px;letter-spacing:.3px}
#st.ok{color:#34d399}#st.err{color:#fb7185}#st.wait{color:#fbbf24}
</style></head><body>
<div class="card">
  <div class="top"><div class="note">&#9835;</div><div>
    <h1>音乐固件上传</h1>
    <div class="sub">将编好的音乐固件上传到设备，完成后自动关闭热点</div>
  </div></div>
  <div class="drop" id="dz">
    <div class="big">点击选择 或 拖入音乐固件文件</div>
    <div class="small">支持 .mub 音乐固件 · 最大约 1.7 MB</div>
  </div>
  <input type="file" id="f" accept=".mub,.bin" hidden>
  <div class="file" id="fd"></div>
  <div class="ctrl" id="cw"><button id="b">开始上传</button></div>
  <div class="barbox" id="bb"><div class="bar"><i id="pb"></i></div><div class="pct" id="pc">0%</div></div>
  <div id="st"></div>
</div>
<script>
var dz=document.getElementById('dz'),f=document.getElementById('f'),
    fd=document.getElementById('fd'),cw=document.getElementById('cw'),
    b=document.getElementById('b'),bb=document.getElementById('bb'),
    pb=document.getElementById('pb'),pc=document.getElementById('pc'),st=document.getElementById('st'),sel=null;
dz.onclick=function(){f.click()};
dz.ondragover=function(e){e.preventDefault();dz.classList.add('over')};
dz.ondragleave=function(){dz.classList.remove('over')};
dz.ondrop=function(e){e.preventDefault();dz.classList.remove('over');if(e.dataTransfer.files.length)f.files=e.dataTransfer.files;show()};
f.onchange=show;
function show(){
  sel=f.files[0];if(!sel)return;
  var n=sel.name.toLowerCase();
  if(n.indexOf('.mub')<0&&n.indexOf('.bin')<0){st.className='err';st.textContent='请选择 .mub 音乐固件文件';sel=null;return;}
  fd.textContent='已选择　'+sel.name+'　·　'+(sel.size/1024).toFixed(1)+' KB';
  fd.style.display='block';cw.style.display='block';st.textContent='';st.className='';
}
b.onclick=function(){
  if(!sel)return;
  b.disabled=true;st.className='wait';st.textContent='正在上传，请保持本页打开…';
  bb.style.display='block';pb.style.width='0%';pc.textContent='0%';
  var x=new XMLHttpRequest();
  var fmd=new FormData();fmd.append('file',sel);   /* multipart: server.upload() 必需 */
  x.open('POST','/music?size='+sel.size,true);
  x.upload.onprogress=function(e){if(e.lengthComputable){var p=Math.round(e.loaded*100/e.total);pb.style.width=p+'%';pc.textContent=p+'%';}};
  x.onload=function(){
    pb.style.width='100%';pc.textContent='100%';
    if(x.status==200){st.className='ok';st.textContent='上传完成，正在写入并关闭热点…';}
    else{st.className='err';st.textContent='上传失败（'+x.status+'），请重试';b.disabled=false;}
  };
  x.onerror=function(){st.className='err';st.textContent='网络错误，请重试';b.disabled=false;};
  x.send(fmd);
};
</script></body></html>
)=====";

/* ===================== 网页处理 ===================== */
static void handle_root()
{
    if (webMode == WEB_OTA) {
        server.send(200, "text/html", FPSTR(OTA_PAGE));
        return;
    }
    if (webMode == WEB_CONFIG) {
        server.send(200, "text/html", FPSTR(CONFIG_PAGE));
        return;
    }
    if (webMode == WEB_MUSIC) {
        server.send(200, "text/html", FPSTR(MUSIC_PAGE));
        return;
    }
    /* DASH：分块发送 43KB 仪表盘（PROGMEM，避免整串拷贝 RAM） */
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");
    for (uint8_t i = 0; i < WEBPAGE_NCHUNKS; i++) {
        server.sendContent_P(WEBPAGE_CHUNKS[i]);
        yield();
    }
}

static void start_wifi_scan()
{
    WiFi.scanNetworks(true /*async*/, true /*show_hidden*/);
    scanState = 1;
}

static String build_scan_json()
{
    int n = WiFi.scanComplete();
    int idx[32]; int cnt = 0;
    for (int i = 0; i < n && cnt < 32; i++) {
        if (WiFi.SSID(i).length() > 0) idx[cnt++] = i;
    }
    for (int a = 0; a < cnt - 1; a++)
        for (int b = a + 1; b < cnt; b++)
            if (WiFi.RSSI(idx[b]) > WiFi.RSSI(idx[a])) { int t = idx[a]; idx[a] = idx[b]; idx[b] = t; }
    String json = "[";
    for (int k = 0; k < cnt; k++) {
        if (k) json += ",";
        String ssid = WiFi.SSID(idx[k]);
        ssid.replace("\\", "\\\\");
        ssid.replace("\"", "\\\"");
        json += "{\"ssid\":\"" + ssid + "\"}";
    }
    json += "]";
    WiFi.scanDelete();
    return json;
}

static String handle_scan_json()
{
    return build_scan_json();
}

static void handle_scan()
{
    String body = "{\"scanning\":" + String(scanState == 1 ? "true" : "false") + ",\"nets\":" + lastScanJson + "}";
    server.send(200, "application/json", body);
}

static void handle_startscan()
{
    /* 已缓存结果较新(<10s)则不重扫，页面直接展示缓存，避免每次打开都等待 */
    if (scanState == 0 && lastScanDoneMs != 0 && (long)(millis() - lastScanDoneMs) < 10000L) {
        server.send(200, "application/json", "{\"ok\":true,\"cached\":true}");
        return;
    }
    if (scanState == 0) start_wifi_scan();
    server.send(200, "application/json", "{\"ok\":true}");
}

/* 同步扫描并按要求返回网络(可复用给自动连网逻辑)。
 * ESP8266 上电初期无线电未就绪时首次扫描常返回 -1/0，
 * 这里最多重试 3 次直到拿到有效结果，避免“明明有存参却走配网”。 */
static int sync_scan()
{
    int n = -1;
    for (int r = 0; r < 3 && n < 0; r++) {
        n = WiFi.scanNetworks(false, true);
        if (n < 0) { delay(500); yield(); }
    }
    WiFi.scanDelete();
    return (n < 0) ? 0 : n;
}

/* 长阻塞操作期间排空 UART，避免 FIFO 溢出后命令粘连 */
static void drain_uart()
{
    while (Serial.available()) {
        int c = Serial.read();
        (void)c;
    }
    cmdLine = "";
}

/* 尝试用指定网络连接，成功返回 1。
 * 注意：不自行切 WiFi 模式——调用方按场景设置(自动连网用 WIFI_STA，
 * 配网页连接用 WIFI_AP_STA 保留 AP 供手机随后跳转)。 */
static bool try_connect(const char *ssid, const char *pass)
{
    WiFi.begin(ssid, pass);
    for (int t = 0; t < 40; t++) {          /* 最长 20s */
        drain_uart();
        if (WiFi.status() == WL_CONNECTED) return true;
        delay(500); yield();
    }
    WiFi.disconnect(true);
    return false;
}

static void handle_connect()
{
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    bool ok;
    /* 配网场景保留 AP(AP_STA)，手机配置完随即由页面跳转到 STA IP 的仪表盘 */
    WiFi.mode(WIFI_AP_STA);
    ok = try_connect(ssid.c_str(), pass.c_str());
    if (ok) {
        store_cfg(ssid.c_str(), pass.c_str());
        stationIP = WiFi.localIP().toString();
        Serial.print("+IP:"); Serial.print(stationIP); Serial.print("\r\n");
        /* 保留 AP 宽限期(25s)：让手机收到响应并跳转到新 IP 的仪表盘，
         * 到点后由 loop() 自动关闭 AP，仅保留 STA。 */
        ap_shutdown_at = millis() + 25000UL;
        webMode = WEB_DASH;
        server.send(200, "application/json", "{\"ok\":true,\"ip\":\"" + stationIP + "\"}");
    } else {
        server.send(200, "application/json", "{\"ok\":false}");
    }
}

static void handle_cfgcount()
{
    char b[16];
    sprintf(b, "{\"count\":%d}", (int)s_cfg_count);
    server.send(200, "application/json", b);
}

static void handle_reconfig()
{
    clear_cfg();
    server.send(200, "application/json", "{\"ok\":true}");
    delay(500);
    ESP.restart();
}

static void handle_data()
{
    String s = lastData;
    s.replace("\\", "\\\\");
    s.replace("\"", "\\\"");
    s.replace("\r", "");
    s.replace("\n", "\\n");
    server.send(200, "application/json", "{\"data\":\"" + s + "\"}");
}

/* ===================== OTA 上传 ===================== */
static void handle_upload()
{
    HTTPUpload &up = server.upload();

    if (up.status == UPLOAD_FILE_START) {
        String s = server.arg("size");
        otaTotal  = s.toInt();
        otaExpCrc = strtoul(server.arg("crc").c_str(), NULL, 16);
        otaRecv   = 0;
        otaSeq    = 0;
        otaCrc    = 0xFFFFFFFF;
        otaBufLen = 0;
        otaInProgress = true;
        uart_send_handshake(otaTotal);
    }
    else if (up.status == UPLOAD_FILE_WRITE) {
        const uint8_t *d = up.buf;
        size_t n = up.currentSize;
        for (size_t i = 0; i < n; i++) {
            otaBuf[otaBufLen++] = d[i];
            otaCrc = ota_crc32_upd(otaCrc, d[i]);
            otaRecv++;
            if (otaBufLen >= OTA_PKT_MAX) {
                uart_send_packet(otaSeq++, otaBuf, OTA_PKT_MAX);
                otaBufLen = 0;
            }
        }
    }
    else if (up.status == UPLOAD_FILE_END) {
        if (otaBufLen > 0) {
            uart_send_packet(otaSeq++, otaBuf, otaBufLen);
            otaBufLen = 0;
        }
        uint32_t finalCrc = ~otaCrc;
        if (server.arg("crc").length() > 0 && finalCrc != otaExpCrc) {
            otaInProgress = false;
            webMode = WEB_OTA;
            server.send(400, "text/plain", "FIRMWARE CRC MISMATCH");
            return;
        }
        uart_send_end(finalCrc);
        Serial.write(0xDD);
        Serial.flush();
        otaInProgress = false;
        webMode = WEB_DASH;
        server.send(200, "text/plain", "OK");
    }
}

/* ===================== 音乐上传 =====================
 * AT+MUSICAP：开启音乐上传 AP（已连 STA 时用 APSTA 共存）；服务 MUSIC_PAGE。
 * 上传走与 OTA 相同的 0xAA 帧（复用 uart_send_handshake/packet/end），
 * STM32 在“音乐接收态”下将其写入外部 Flash，完成后回 ACK 帧结束。
 * AT+MUSICCLOSE：关闭音乐 AP（不关 STA）。 */

bool    musicInProgress = false;

/* 音乐上传 AP：采用原厂 AT+CWMODE=3 的 AP+STA 共存模式。
 * ESP8266 单射频时分复用：softAP 不指定信道，SDK 自动跟随 STA 当前信道，
 * STA 持续在线（保留路由器 IP），AP 同时广播供上传。 */
bool music_sta_was_up = false;

static void start_music_ap()
{
    music_sta_was_up = (WiFi.status() == WL_CONNECTED);
    WiFi.mode(WIFI_AP_STA);               /* CWMODE=3：AP+STA 共存 */
    WiFi.softAP(AP_SSID, AP_PASS);        /* 信道自动随 STA，不抢占路由器信道 */
    if (music_sta_was_up) {
        /* 模式切换期间 STA 可能短暂重连；若掉线则用已存参数恢复（AP 不受影响） */
        if (WiFi.status() != WL_CONNECTED) {
            for (int i = 0; i < CFG_MAX; i++) {
                if (s_cfg_valid[i] && s_cfg[i].ssid[0]) {
                    WiFi.begin(s_cfg[i].ssid, s_cfg[i].pass);
                    break;
                }
            }
        }
    }
    webMode = WEB_MUSIC;
    if (!webActive) { server.begin(); webActive = true; }
    Serial.print("+MUSICAP\r\n");
}

/* 关闭音乐 AP：只关 AP，STA 若在线保持（回仪表盘）；否则整机关 WiFi 待主机断电。
 * 若 STA 在音乐会话中被断过，按已存参数重连。 */
static void close_music_ap()
{
    bool need_sta = music_sta_was_up;
    WiFi.softAPdisconnect(true);          /* 只关 AP（STATION 模式位保留） */
    if (need_sta) {
        if (WiFi.status() != WL_CONNECTED) {
            WiFi.mode(WIFI_STA);
            WiFi.disconnect(false);
            for (int i = 0; i < CFG_MAX; i++) {
                if (s_cfg_valid[i] && s_cfg[i].ssid[0]) {
                    WiFi.begin(s_cfg[i].ssid, s_cfg[i].pass);
                    break;
                }
            }
        }
        music_sta_was_up = false;
        webMode = WEB_DASH;
        if (!webActive) { server.begin(); webActive = true; }
    } else {
        WiFi.mode(WIFI_OFF);
        WiFi.forceSleepBegin();
        webActive = false;
        server.stop();
    }
    Serial.print("+MUSICCLOSED\r\n");
}

static void handle_music_upload()
{
    HTTPUpload &up = server.upload();

    if (up.status == UPLOAD_FILE_START) {
        String s = server.arg("size");
        musicTotal = s.toInt();
        otaRecv   = 0;
        otaSeq    = 0;
        otaCrc    = 0xFFFFFFFF;
        otaBufLen = 0;
        musicInProgress = true;
        uart_music_handshake(musicTotal);
    }
    else if (up.status == UPLOAD_FILE_WRITE) {
        const uint8_t *d = up.buf;
        size_t n = up.currentSize;
        for (size_t i = 0; i < n; i++) {
            otaBuf[otaBufLen++] = d[i];
            otaCrc = ota_crc32_upd(otaCrc, d[i]);
            otaRecv++;
            if (otaBufLen >= OTA_PKT_MAX) {
                uart_send_packet(otaSeq++, otaBuf, OTA_PKT_MAX);
                otaBufLen = 0;
            }
        }
    }
    else if (up.status == UPLOAD_FILE_END) {
        if (otaBufLen > 0) {
            uart_send_packet(otaSeq++, otaBuf, otaBufLen);
            otaBufLen = 0;
        }
        uint32_t finalCrc = ~otaCrc;
        uart_music_end(finalCrc);
        musicInProgress = false;
        if (finalCrc != 0 && otaRecv == musicTotal) {   /* 校验由 App 端完成，这里仅表示已转发 */
            Serial.print("+MUSICOK\r\n");
            server.send(200, "text/plain", "OK");   /* 先回页面，再做 AP 关闭/STA 重连（重连可能耗时） */
            close_music_ap();
        } else {
            Serial.print("+MUSICERR\r\n");
            server.send(400, "text/plain", "MUSIC CRC FAIL");
        }
    }
}

/* ===================== AT 指令 ===================== */
static void start_ota()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    webMode = WEB_OTA;
    if (!webActive) { server.begin(); webActive = true; }
}

/* 立即打开配网 AP */
static void start_cfg_now()
{
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASS);
    webMode = WEB_CONFIG;
    lastScanJson = "[]";
    start_wifi_scan();
    if (!webActive) { server.begin(); webActive = true; }
    Serial.print("+AP\r\n");
}

/* AT+WEBSTART：直接用已存的 WiFi 存参逐个连接（不再先扫描匹配，
 * 已配过网络则直接连；否则也无牌扫描失败导致误判进配网）。 */
static void start_wifi_connect()
{
    uint8_t i, attempt;
    if (wifiConnecting) {               /* 重复 WEBSTART 忽略，避免打断进行中的连接 */
        Serial.println("BUSY");
        return;
    }
    if (WiFi.status() == WL_CONNECTED) {  /* 已联网直接回报 +IP */
        Serial.print("+IP:"); Serial.print(stationIP); Serial.print("\r\n");
        webMode = WEB_DASH;
        if (!webActive) { server.begin(); webActive = true; }
        return;
    }

    wifiConnecting = true;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    delay(200);

    if (s_cfg_count == 0) { wifiConnecting = false; start_cfg_now(); return; }

    /* 跳过扫描检测，直接按已存 WiFi 逐个 WiFi.begin（存参跨断电持久） */
    Serial.printf("+CFG:%u\r\n", (unsigned)s_cfg_count);   /* 诊断：实际存参数（上电后由 load_cfg 载入） */
    for (attempt = 0; attempt < 4; attempt++) {   /* 全组走 4 轮（最多 ~80s）：给路由器上电/重启动留时间 */
        for (i = 0; i < CFG_MAX; i++) {
            if (!s_cfg_valid[i]) continue;
            if (s_cfg[i].ssid[0] == 0) continue;   /* 防御：空 SSID 不浪费 20s */
            Serial.printf("+TRY:%u:%s\r\n", (unsigned)i, s_cfg[i].ssid);
            if (try_connect(s_cfg[i].ssid, s_cfg[i].pass)) {
                stationIP = WiFi.localIP().toString();
                Serial.print("+IP:"); Serial.print(stationIP); Serial.print("\r\n");
                webMode = WEB_DASH;
                if (!webActive) { server.begin(); webActive = true; }
                drain_uart();   /* 去掉积压的重复 WEBSTART */
                WiFi.mode(WIFI_STA);
                wifiConnecting = false;
                return;
            }
        }
    }

    wifiConnecting = false;
    Serial.printf("+CFGFAIL:%u\r\n", (unsigned)s_cfg_count);  /* 有存参但全部连接失败 → 配网 */
    start_cfg_now();                       /* 全部失败 → 配网 */
}

static void stop_all()
{
    webActive = false;
    server.stop();
    for (int i = 0; i < WEBSOCKETS_SERVER_CLIENT_MAX; i++) ws.disconnect(i);
    otaInProgress = false;
    scanState = 0;
    WiFi.scanDelete();
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    Serial.print("OK\r\n");
}

static void handle_at(const String &cmd)
{
    if (cmd == "AT") {
        Serial.println("OK");
    }
    else if (cmd == "AT+OTAAP") {
        start_ota();
        Serial.println("OK");
    }
    else if (cmd == "AT+CFGAP") {
        start_cfg_now();
        Serial.println("OK");
    }
    else if (cmd == "AT+CFGCLR") {
        clear_cfg();
        Serial.println("OK");
    }
    else if (cmd == "AT+STARTWEB" || cmd == "AT+WEBSTART") {
        start_wifi_connect();
        Serial.println("OK");
    }
    else if (cmd == "AT+MUSICAP") {
        start_music_ap();
        Serial.println("OK");
    }
    else if (cmd == "AT+MUSICCLOSE") {
        close_music_ap();
    }
    else if (cmd == "AT+WEBCLOSE") {
        stop_all();
    }
    else if (cmd.startsWith("AT+PUSHDATA=")) {
        lastData = cmd.substring(12);
        Serial.println("OK");
    }
    else {
        Serial.println("ERROR");
    }
}

/* ===================== 串口/WS 双向透传 ===================== */
static void process_uart()
{
    while (Serial.available()) {
        int c = Serial.read();
        if (c == '\r' || c == '\n') {
            if (cmdLine.length() > 0) {
                if (cmdLine[0] == '{') {
                    /* STM32 -> 网页：广播 JSON */
                    ws.broadcastTXT(cmdLine);
                } else {
                    handle_at(cmdLine);
                }
                cmdLine = "";
            }
        }
        else if (c >= 32 && c < 127) {
            cmdLine += (char)c;
        }
    }
}

static void wsEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    if (type == WStype_TEXT) {
        for (size_t i = 0; i < length; i++) Serial.write(payload[i]);
        Serial.write('\r'); Serial.write('\n');
        Serial.flush();
    }
}

/* ===================== 入口 ===================== */
void setup()
{
    Serial.begin(UART_BAUD);
    load_cfg();
    Serial.printf("+CFGLOAD:%u\r\n", (unsigned)s_cfg_count);   /* 诊断：上电实际从 Flash 载回的存参数量 */

    server.on("/", HTTP_GET, handle_root);
    server.on("/scan",      HTTP_GET,  handle_scan);
    server.on("/startscan", HTTP_GET,  handle_startscan);
    server.on("/connect",   HTTP_POST, handle_connect);
    server.on("/cfgcount", HTTP_GET,   handle_cfgcount);
    server.on("/data",     HTTP_GET,  handle_data);
    server.on("/reconfig", HTTP_POST, handle_reconfig);
    server.on("/upload",   HTTP_POST, []() { server.send(200, "text/plain", "OK"); }, handle_upload);
    server.on("/music",    HTTP_POST, []() { server.send(200, "text/plain", "OK"); }, handle_music_upload);

    ws.begin();
    ws.onEvent(wsEvent);

    Serial.println("QiMingXing ESP AT Ready");
}

void loop()
{
    if (!otaInProgress && !musicInProgress) process_uart();

    if (webActive && webMode == WEB_CONFIG && scanState == 1) {
        int n = WiFi.scanComplete();
        if (n >= 0)      { lastScanJson = build_scan_json(); scanState = 0; lastScanDoneMs = millis(); }
        else if (n == -2){ lastScanJson = "[]";               scanState = 0; lastScanDoneMs = millis(); }
    }

    if (webActive) server.handleClient();
    ws.loop();

    /* 配网宽限期结束且 STA 已连：关闭 AP，仅保留 STA，手机已完成跳转 */
    if (ap_shutdown_at != 0 && (long)(millis() - ap_shutdown_at) >= 0 &&
        WiFi.status() == WL_CONNECTED) {
        ap_shutdown_at = 0;
        WiFi.softAPdisconnect(true);
        Serial.print("+APOFF\r\n");
    }
    yield();
}