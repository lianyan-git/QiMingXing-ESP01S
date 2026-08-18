/*************************************************************
 * QiMingXing ESP-01S 自定义 AT 固件 (Arduino / ESP8266 core)
 *
 * 设计目标：把原本由 STM32 承担的 HTTP 解析 / 网页服务 / 多包
 * 重组等工作全部转移到 ESP 端，STM32 只需发一条 AT 指令并在 UART
 * 上按简单二进制协议收固件，从而减轻 STM32 负担、加快网页打开速度、
 * 降低丢包率。
 *
 * 串口 (UART, 连接 STM32 USART1)：GPIO1(TX) / GPIO3(RX) @ 115200
 *
 * 自定义指令：
 *   AT                -> 回复 OK（供 STM32 探测 ESP 是否就绪）
 *   AT+OTAAP          -> 开启 SoftAP(QiMingXing/12345678) + 网页上传固件，
 *                        上传完成后自动按二进制协议经 UART 转发给 STM32
 *   AT+CFGAP          -> 开 AP 配网：网页选择 WiFi，连接后关 AP 切 STA，
 *                        凭据自动存入 ESP Flash（下次重启自动重连）
 *   AT+STARTWEB       -> 在 STA 模式下启动 Web 服务器（数据展示页），
 *                        输出 "+IP:xxx.xxx.xxx.xxx"
 *   AT+PUSHDATA=<str> -> 缓存数据，数据展示网页每 2 秒轮询显示最新内容
 *   AT+OTACLOSE       -> 关闭 Web Server，ESP 进入 Modem-Sleep 低功耗
 *
 * WiFi 凭据由 ESP8266 SDK 自动持久化（WiFi.persistent），
 * 无需 EEPROM，重启后 WiFi.begin() 自动使用上次的 SSID/密码。
 *
 * OTA 串口协议（1KB/包，包级 ACK）见 README 与下方常量说明。
 *************************************************************/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define UART_BAUD   115200
#define AP_SSID     "QiMingXing"
#define AP_PASS     "12345678"
#define OTA_PKT_MAX 1024

ESP8266WebServer server(80);

enum WebMode { WEB_OTA, WEB_CONFIG, WEB_DASH };
WebMode webMode = WEB_OTA;
bool    webActive = false;

String lastData   = "";
String stationIP  = "";

/* ---- 配网 WiFi 扫描状态（ESP 后台异步扫描，网页轮询取缓存结果）---- */
String  lastScanJson = "[]";
int     scanState    = 0;   /* 0=空闲可启动, 1=扫描中 */

/* ---- OTA 转发状态（ESP -> STM32 二进制协议）---- */
bool     otaInProgress = false;
uint32_t otaTotal      = 0;
uint32_t otaRecv       = 0;
uint16_t otaSeq        = 0;
uint32_t otaCrc        = 0xFFFFFFFF;   /* 运行中的 CRC32（最终取反） */
uint32_t otaExpCrc     = 0;           /* 浏览器上报的原始文件 CRC32（用于拦住 WiFi 丢包污染） */
uint8_t  otaBuf[2048];
uint16_t otaBufLen     = 0;
uint32_t otaPktCount   = 0;

String cmdLine = "";

/* ===================== CRC ===================== */
/* Modbus CRC16：多项式 0x8005(反射 0xA001)，初值 0xFFFF，无最终异或。
 * 覆盖 包序号(2B) + 数据(NB)。 */
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

/* 标准 IEEE 802.3 CRC32（zlib/以太网），与 STM32 端 CRC32_Calculate 一致。
 * 初值 0xFFFFFFFF，反射，最终取反。 */
static uint32_t ota_crc32_upd(uint32_t crc, uint8_t b)
{
    crc ^= b;
    for (int i = 0; i < 8; i++) crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320UL : (crc >> 1);
    return crc;
}

/* ===================== UART 辅助 ===================== */
/* 等待 STM32 回复：0x06=ACK 继续，0x15=NAK 重传当前包，0=超时 */
static int uart_wait_ack(int timeoutMs)
{
    unsigned long t = millis();
    while ((int)(millis() - t) < timeoutMs) {
        if (Serial.available()) {
            int b = Serial.read();
            if (b == 0x06) return 1;   /* ACK */
            if (b == 0x15) return -1;  /* NAK */
        }
        yield();
    }
    return 0;
}

/* 阶段1 握手：ESP -> STM32: [0xAA 0x55 0x01] + [4字节固件大小 大端] */
static void uart_send_handshake(uint32_t size)
{
    uint8_t hdr[7] = { 0xAA, 0x55, 0x01,
                       (uint8_t)(size >> 24), (uint8_t)(size >> 16),
                       (uint8_t)(size >> 8),  (uint8_t)size };
    Serial.write(hdr, 7);
    Serial.flush();
}

/* 阶段2 数据包：ESP -> STM32: [0xAA] + [2字节序号大端] + [≤1024数据] + [2字节CRC16] + [0x55] */
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
        if (r == 1) return;            /* ACK */
        /* NAK / 超时：重传当前包 */
    }
}

/* 阶段3 结束：ESP -> STM32: [0xAA 0x55 0x02] + [4字节总CRC32大端] */
static void uart_send_end(uint32_t crc)
{
    uint8_t hdr[7] = { 0xAA, 0x55, 0x02,
                       (uint8_t)(crc >> 24), (uint8_t)(crc >> 16),
                       (uint8_t)(crc >> 8),  (uint8_t)crc };
    Serial.write(hdr, 7);
    Serial.flush();
    uart_wait_ack(3000);
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
 button.go{width:100%;background:linear-gradient(90deg,#3b6cff,#7c4dff);color:#fff;border:0;border-radius:12px;padding:14px;font-size:16px;cursor:pointer}
 .msg{margin-top:14px;font-size:13px;color:#9fb0e0;min-height:18px}
</style></head>
<body><div class=card>
 <h2>WiFi 配网</h2>
 <div class=row>
  <select id=ssid><option value="">正在扫描附近网络…</option></select>
  <button class=rf id=rf onclick=doScan() title="刷新">⟳</button>
 </div>
 <input id=pass type=password placeholder="WiFi 密码（开放网络留空）">
 <button class=go onclick=conn()>连接</button>
 <div class=msg id=m>进入页面自动扫描一次，可点 ⟳ 手动刷新</div>
</div>
<script>
var polling=null;
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
  .then(function(r){return r.json();}).then(function(j){ if(j.ok){ m.innerHTML='✅ 已连接<span id=ip>'+j.ip+'</span><br>AP 即将关闭，请刷新页面使用新 IP 访问'; } else m.textContent='❌ 连接失败，请检查密码'; })
  .catch(function(){ m.textContent='❌ 连接失败'; });
}
window.onload=doScan;
</script></body></html>
)=====";

static const char DASH_PAGE[] PROGMEM = R"=====(
<!DOCTYPE html><html lang=zh><head><meta charset=UTF-8>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>启明星状态</title>
<style>
 body{font-family:-apple-system,Segoe UI,Arial,sans-serif;margin:0;background:#0f1830;color:#eaf0ff;padding:18px}
 h2{color:#7cc4ff;margin:0 0 16px}
 .grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(140px,1fr));gap:14px}
 .c{background:#172347;border:1px solid #2a3a66;border-radius:14px;padding:16px}
 .c .k{color:#8a97c0;font-size:12px;margin-bottom:6px}
 .c .v{font-size:22px;font-weight:600;color:#7cffb0}
 .txt{margin-top:18px;white-space:pre-wrap;background:#101a36;border:1px solid #2a3a66;border-radius:12px;padding:14px;font-size:13px;color:#cfe0ff}
</style></head>
<body><h2>烘干箱实时状态</h2>
<div class=grid id=grid></div>
<div class=txt id=txt>等待数据…</div>
<script>
function esc(s){return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');}
function render(s){ var grid=document.getElementById('grid'), txt=document.getElementById('txt');
 if(!s){txt.textContent='等待数据…';return;}
 var lines=s.split(/\r?\n/), cards='';
 lines.forEach(function(ln){ var i=ln.indexOf(':'); if(i>0){ var k=ln.slice(0,i).trim(), v=ln.slice(i+1).trim();
   cards+='<div class=c><div class=k>'+esc(k)+'</div><div class=v>'+esc(v)+'</div></div>'; } });
 grid.innerHTML=cards; txt.textContent=s; }
function reconfig(){ if(confirm('确认重新配网？')){ fetch('/reconfig',{method:'POST'}).then(function(){ alert('已清除WiFi设置，设备将重启进入配网模式'); }); } }
setInterval(function(){ fetch('/data').then(r=>r.json()).then(j=>render(j.data)).catch(function(){}); },2000);
fetch('/data').then(r=>r.json()).then(j=>render(j.data)).catch(function(){});
</script>
<div style="text-align:center;margin-top:22px"><button onclick="reconfig()" style="background:#3b6cff;color:#fff;border:0;border-radius:10px;padding:10px 20px;font-size:14px;cursor:pointer">重新配网</button></div>
</body></html>
)=====";

/* ===================== 网页处理 ===================== */
static void handle_root()
{
    String page;
    if (webMode == WEB_OTA)        page = FPSTR(OTA_PAGE);
    else if (webMode == WEB_CONFIG) page = FPSTR(CONFIG_PAGE);
    else                           page = FPSTR(DASH_PAGE);
    server.send(200, "text/html", page);
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

static void handle_scan()
{
    String body = "{\"scanning\":" + String(scanState == 1 ? "true" : "false") + ",\"nets\":" + lastScanJson + "}";
    server.send(200, "application/json", body);
}

static void handle_startscan()
{
    if (scanState == 0) start_wifi_scan();
    server.send(200, "application/json", "{\"ok\":true}");
}

static void handle_connect()
{
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    WiFi.persistent(true);
    WiFi.begin(ssid.c_str(), pass.c_str());
    bool ok = false;
    for (int t = 0; t < 30; t++) {
        if (WiFi.status() == WL_CONNECTED) { ok = true; break; }
        delay(500); yield();
    }
    if (ok) {
        stationIP = WiFi.localIP().toString();
        Serial.print("+IP:"); Serial.print(stationIP); Serial.print("\r\n");
        /* 关闭 AP 模式，ESP 仅作为 STA 设备接入路由器 */
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        webMode = WEB_DASH;
        server.send(200, "application/json", "{\"ok\":true,\"ip\":\"" + stationIP + "\"}");
    } else {
        server.send(200, "application/json", "{\"ok\":false}");
    }
}

static void handle_reconfig()
{
    WiFi.persistent(true);
    WiFi.disconnect(true);
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

/* 文件上传：浏览器以 multipart 上传，size 通过查询参数 ?size= 传入。
 * 数据流：握手 -> 每满 1KB 发一包(等待 ACK) -> 末尾补齐 -> CRC32 -> 0xDD */
static void handle_upload()
{
    HTTPUpload &up = server.upload();

    if (up.status == UPLOAD_FILE_START) {
        String s = server.arg("size");
        otaTotal  = s.toInt();
        otaExpCrc = strtoul(server.arg("crc").c_str(), NULL, 16);
        otaPktCount = (otaTotal + (OTA_PKT_MAX - 1)) / OTA_PKT_MAX;
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

/* ===================== AT 指令处理 ===================== */
static void start_ota()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    webMode = WEB_OTA;
    if (!webActive) { server.begin(); webActive = true; }
}

static void start_cfg()
{
    /* 先尝试用 SDK 自动存储的凭据连接（WiFi.persistent 自动保存） */
    WiFi.persistent(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin();
    for (int t = 0; t < 20; t++) {
        if (WiFi.status() == WL_CONNECTED) {
            stationIP = WiFi.localIP().toString();
            Serial.print("+IP:"); Serial.print(stationIP); Serial.print("\r\n");
            webMode = WEB_DASH;
            if (!webActive) { server.begin(); webActive = true; }
            return;
        }
        delay(500); yield();
    }

    /* 自动连接失败 → 开 AP 配网 */
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASS);
    webMode = WEB_CONFIG;
    lastScanJson = "[]";
    start_wifi_scan();
    if (!webActive) { server.begin(); webActive = true; }
}

static void start_web()
{
    /* 在 STA 模式下启动 Web 服务器，输出 IP 供 STM32 使用 */
    WiFi.mode(WIFI_STA);
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.persistent(true);
        WiFi.begin();
        for (int t = 0; t < 20; t++) {
            if (WiFi.status() == WL_CONNECTED) break;
            delay(500); yield();
        }
    }
    stationIP = WiFi.localIP().toString();
    webMode = WEB_DASH;
    if (!webActive) { server.begin(); webActive = true; }
    Serial.print("+IP:"); Serial.print(stationIP); Serial.print("\r\n");
}

static void stop_all()
{
    webActive = false;
    server.stop();
    otaInProgress = false;
    scanState = 0;
    WiFi.scanDelete();
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
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
        start_cfg();
        Serial.println("OK");
    }
    else if (cmd == "AT+STARTWEB") {
        start_web();
        Serial.println("OK");
    }
    else if (cmd.startsWith("AT+PUSHDATA=")) {
        lastData = cmd.substring(12);
        Serial.println("OK");
    }
    else if (cmd == "AT+OTACLOSE") {
        stop_all();
        Serial.println("OK");
    }
    else {
        Serial.println("ERROR");
    }
}

static void process_uart()
{
    while (Serial.available()) {
        int c = Serial.read();
        if (c == '\r' || c == '\n') {
            if (cmdLine.length() > 0) { handle_at(cmdLine); cmdLine = ""; }
        }
        else if (c >= 32 && c < 127) {
            cmdLine += (char)c;
        }
    }
}

/* ===================== 入口 ===================== */
void setup()
{
    Serial.begin(UART_BAUD);
    Serial.println("QiMingXing ESP AT Ready");

    server.on("/", HTTP_GET, handle_root);
    server.on("/scan",      HTTP_GET,  handle_scan);
    server.on("/startscan", HTTP_GET,  handle_startscan);
    server.on("/connect",   HTTP_POST, handle_connect);
    server.on("/data",     HTTP_GET,  handle_data);
    server.on("/reconfig", HTTP_POST, handle_reconfig);
    server.on("/upload",   HTTP_POST, []() { server.send(200, "text/plain", "OK"); }, handle_upload);
}

void loop()
{
    if (!otaInProgress) process_uart();

    if (webActive && webMode == WEB_CONFIG && scanState == 1) {
        int n = WiFi.scanComplete();
        if (n >= 0)      { lastScanJson = build_scan_json(); scanState = 0; }
        else if (n == -2){ lastScanJson = "[]";               scanState = 0; }
    }

    if (webActive) server.handleClient();
    yield();
}