/* Auto-generated from ESP01S/web.txt - ESP8266 dashboard page.
   HTML embedded as C++ raw string (R"=====(...)====="), keeps newlines and quotes. */
#ifndef WEB_PAGE_H
#define WEB_PAGE_H
#include <Arduino.h>

static const char WEBPAGE_0[] PROGMEM = R"=====(<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>QIMINGXING Dryer</title>
<style>
:root{
  --primary:#0A84FF; --orange:#FF9500; --red:#FF453A; --green:#30D158;
  --bg-body:#F2F3F7; --bg-card:#FFFFFF; --bg-soft:#F0F1F6;
  --text:#1C1C1E; --sub:#8E8E93; --border:#E5E5EA; --grid:#E7E8ED;
  --card-shadow:0 8px 28px rgba(30,35,50,.10),0 2px 8px rgba(30,35,50,.05);
  --pill-bg:#FFFFFF;--pill-border:#E0E2E8;
  --humi:#00B8D4;
 }
 [data-theme="dark"]{
   --bg-body:#0B0D13; --bg-card:#171A22; --bg-soft:#232733;
   --text:#F2F2F7; --sub:#98989F; --border:#2A2E3A; --grid:#2A2F3B;
   --card-shadow:0 10px 30px rgba(0,0,0,.5),0 2px 8px rgba(0,0,0,.35);
   --pill-bg:#1E222D;--pill-border:#343A48;
   --humi:#26C6DA;
 }
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent;user-select:none}
input{user-select:text}
body{margin:0;font-family:"PingFang SC","Microsoft YaHei",sans-serif;background:var(--bg-body);
  color:var(--text);height:100dvh;display:flex;flex-direction:column;overflow:hidden;
  transition:background .5s,color .5s}

#errbar{position:fixed;top:0;left:0;right:0;z-index:999;background:#c0392b;color:#fff;
  font-size:12px;padding:8px 12px;display:none;max-height:40vh;overflow:auto;white-space:pre-wrap}

header{height:58px;flex-shrink:0;display:flex;align-items:center;justify-content:space-between;
  padding:0 16px;background:var(--bg-card);border-bottom:1px solid var(--border);z-index:100}
.brand{display:flex;align-items:center;gap:10px;font-weight:800;white-space:nowrap}
.brand b{font-size:clamp(19px,4vw,25px);letter-spacing:.1em}
.brand em{font-style:normal;font-size:clamp(11px,2vw,13px);letter-spacing:.42em;color:var(--primary);font-weight:700}
.conn{display:flex;align-items:center;gap:6px;font-size:11.5px;color:var(--sub);margin-right:10px}
.conn i{width:8px;height:8px;border-radius:50%;background:var(--red)}
.conn.ok i{background:var(--green);box-shadow:0 0 8px var(--green)}

.subbar{flex-shrink:0;display:flex;align-items:center;gap:9px;padding:7px 16px;
  background:var(--bg-card);border-bottom:1px solid var(--border);z-index:90;overflow-x:auto;scrollbar-width:none}
.subbar::-webkit-scrollbar{display:none}
.pill{height:34px;display:inline-flex;align-items:center;gap:7px;padding:0 13px;
  border-radius:17px;border:1px solid var(--pill-border);background:var(--pill-bg);
  color:var(--text);font-size:13px;font-weight:600;cursor:pointer;
  box-shadow:0 2px 6px rgba(0,0,0,.06);transition:.2s;white-space:nowrap;font-family:inherit;flex-shrink:0}
.pill svg{width:15px;height:15px;stroke:var(--primary);fill:none;stroke-width:2;stroke-linecap:round;stroke-linejoin:round;flex-shrink:0}
.pill .arr{stroke:var(--sub)}
.pill.del{padding:0;width:34px;justify-content:center}
.pill.del svg{stroke:var(--sub);transition:.2s}
.pill.del:hover svg{stroke:var(--red)}
.can-sw{height:34px;display:inline-flex;align-items:center;gap:9px;padding:0 6px 0 13px;
  border-radius:17px;border:1.5px solid var(--primary);background:var(--pill-bg);
  font-size:13px;font-weight:600;cursor:pointer;white-space:nowrap;flex-shrink:0}
.can-sw input{display:none}
.can-sw .tr{width:40px;height:24px;border-radius:12px;background:#C7CDD8;position:relative;transition:.3s;flex-shrink:0}
.can-sw .tr::after{content:"";position:absolute;width:18px;height:18px;border-radius:50%;background:#fff;
  top:3px;left:3px;transition:.3s cubic-bezier(.55,-.3,.35,1.3);box-shadow:0 2px 5px rgba(0,0,0,.25)}
.can-sw input:checked + .tr{background:var(--primary)}
.can-sw input:checked + .tr::after{transform:translateX(16px)}

#toast{position:fixed;bottom:24px;left:50%;transform:translateX(-50%) translateY(20px);z-index:300;
  backgro)=====";
static const char WEBPAGE_1[] PROGMEM = R"=====(und:rgba(20,22,30,.88);color:#fff;font-size:13px;font-weight:600;
  padding:9px 18px;border-radius:20px;opacity:0;pointer-events:none;
  transition:opacity .3s,transform .3s;white-space:nowrap;max-width:92vw;overflow:hidden;text-overflow:ellipsis}
#toast.show{opacity:1;transform:translateX(-50%) translateY(0)}

.daynight{--w:64px;--h:32px;width:var(--w);height:var(--h);border-radius:calc(var(--h)/2);
  position:relative;cursor:pointer;border:none;padding:0;flex-shrink:0;overflow:hidden;
  background:linear-gradient(90deg,#26304F 0%,#3A466B 46%,#7D8CBB 60%,#AFC6E8 78%,#D8E9FB 100%);
  box-shadow:inset 0 3px 8px rgba(0,0,0,.30),inset 0 -2px 6px rgba(255,255,255,.18);transition:background .6s}
[data-theme="dark"] .daynight{background:linear-gradient(90deg,#1A2140 0%,#232C52 55%,#39456F 75%,#4A588A 100%)}
.daynight .st{position:absolute;opacity:0;transition:opacity .5s}
.daynight .st svg{display:block}
.daynight .p1{left:6px;top:4px}.daynight .p2{left:19px;top:17px}
.daynight .p3{left:30px;top:5px}.daynight .p4{left:12px;top:21px}
[data-theme="dark"] .daynight .st{opacity:1;animation:tw 2.4s infinite ease-in-out}
.daynight .p2{animation-delay:.7s}.daynight .p3{animation-delay:1.3s}.daynight .p4{animation-delay:1.8s}
@keyframes tw{50%{opacity:.25;transform:scale(.55)}}
.daynight .cloud{position:absolute;right:4px;top:18px;width:25px;height:9px;background:#fff;
  border-radius:10px;opacity:.95;transition:.5s;box-shadow:11px -5px 0 -2px #fff}
[data-theme="dark"] .daynight .cloud{opacity:0;transform:translateX(14px)}
.daynight .knob{position:absolute;top:3px;left:3px;width:26px;height:26px;border-radius:50%;
  background:radial-gradient(circle at 34% 30%,#FFFBE8,#FFD84D 62%,#F5A623);
  box-shadow:0 3px 8px rgba(0,0,0,.35),inset -3px -3px 6px rgba(200,120,0,.25);
  transition:left .5s cubic-bezier(.5,-.3,.3,1.35),background .5s;z-index:2}
[data-theme="dark"] .daynight .knob{left:calc(var(--w) - 29px);
  background:radial-gradient(circle at 36% 32%,#F7F9FD,#CDD4E2 66%,#98A2B8);
  box-shadow:0 3px 8px rgba(0,0,0,.45),inset -3px -3px 6px rgba(70,80,110,.35)}
.daynight .cr{position:absolute;border-radius:50%;background:rgba(105,115,142,.45);opacity:0;transition:.45s}
.daynight .k1{width:8px;height:8px;left:5px;top:5px}
.daynight .k2{width:6px;height:6px;left:14px;top:13px}
.daynight .k3{width:4px;height:4px;left:6px;top:16px}
[data-theme="dark"] .daynight .cr{opacity:1}

.modal-mask{position:fixed;inset:0;z-index:200;display:flex;align-items:center;justify-content:center;
  background:rgba(20,22,30,.35);backdrop-filter:blur(8px);-webkit-backdrop-filter:blur(8px);
  opacity:0;pointer-events:none;transition:opacity .3s}
.modal-mask.show{opacity:1;pointer-events:auto}
.modal{width:min(480px,92vw);max-height:82vh;background:var(--bg-card);border-radius:22px;
  box-shadow:var(--card-shadow);padding:20px;display:flex;flex-direction:column;
  transform:translateY(18px) scale(.96);transition:transform .35s cubic-bezier(.5,-.2,.3,1.3)}
.modal-mask.show .modal{transform:none}
.modal h3{margin:0 0 4px;font-size:17px;display:flex;align-items:center;gap:8px}
.modal h3 svg{width:18px;height:18px;stroke:var(--primary);fill:none;stroke-width:2;stroke-linecap:round;stroke-linejoin:round}
.modal h3 .badge{margin-left:auto;font-size:11px;font-weight:700;color:var(--red);
  background:rgba(255,69,58,.1);padding:3px 10px;border-radius:10px}
.modal .msub{margin:0 0 12px;font-size:12.5px;color:var(--sub)}
.pscroll{overflow-y:auto;flex:1;min-height:0;display:flex;flex-direction:column;gap:9px;
  scrollbar-width:thin;padding-right:4px}
.pcard{border:1.5px solid var(--border);border-radius:14px;background:var(--bg-soft);
  padding:12px 14px;cursor:pointer;transition:.15s;font-family:inherit;color:var(--text);text-align:l)=====";
static const char WEBPAGE_2[] PROGMEM = R"=====(eft;
  display:flex;align-items:center;gap:12px;flex-shrink:0}
.pcard.sel{border-color:var(--primary);background:rgba(10,132,255,.08)}
.pcard.del-sel{border-color:var(--red);background:rgba(255,69,58,.08)}
.pcard .pc-main{flex:1;min-width:0}
.pcard .pn{font-weight:700;font-size:14.5px}
.pcard .pd{font-size:12px;color:var(--sub);margin-top:2px}
.pcard .tick{width:22px;height:22px;border-radius:50%;background:var(--primary);
  display:none;align-items:center;justify-content:center;flex-shrink:0}
.pcard.sel .tick,.pcard.del-sel .tick{display:flex}
.pcard.del-sel .tick{background:var(--red)}
.pcard .tick svg{width:12px;height:12px;stroke:#fff;stroke-width:3;fill:none;stroke-linecap:round;stroke-linejoin:round}
.modal .loading{padding:30px 0;text-align:center;color:var(--sub);font-size:13px}
.modal .mfoot{margin-top:14px;display:flex;justify-content:flex-end;gap:10px;flex-shrink:0;align-items:center}
.mfoot .left{margin-right:auto}

.form{display:flex;flex-direction:column;gap:14px;margin-top:8px}
.frow{display:flex;flex-direction:column;gap:6px}
.frow label{font-size:12.5px;font-weight:600;color:var(--sub)}
.frow input[type=text]{height:40px;padding:0 12px;border:1.5px solid var(--border);border-radius:12px;
  background:var(--bg-soft);color:var(--text);font-size:15px;outline:none;font-family:inherit}
.frow input[type=text]:focus{border-color:var(--primary)}
.fhms{display:flex;align-items:center;gap:8px}
.fhms input{flex:1;height:44px;min-width:0;text-align:center;border:1.5px solid var(--border);border-radius:12px;
  background:var(--bg-soft);color:var(--text);font-size:18px;font-weight:700;outline:none;
  font-variant-numeric:tabular-nums;font-family:inherit;-moz-appearance:textfield}
.fhms input::-webkit-outer-spin-button,.fhms input::-webkit-inner-spin-button{-webkit-appearance:none}
.fhms input:focus{border-color:var(--primary)}
.fhms span{font-size:12px;color:var(--sub);flex-shrink:0}
.fhms .sp{font-size:18px;color:var(--sub);font-weight:600}

main{flex:1;min-height:0;position:relative;overflow:hidden;touch-action:pan-y;margin-top:0}
#stage{height:100%;display:flex;touch-action:pan-y;
  transition:transform .5s cubic-bezier(.3,1.2,.35,1)}
#stage.drag{transition:none!important}
.pane{flex:0 0 100%;height:100%;display:flex;align-items:flex-start;justify-content:center;
  padding:4px 60px 8px}
.edge{position:absolute;top:0;bottom:0;width:52px;z-index:50;cursor:pointer}
.edge.l{left:0}.edge.r{right:0}
.arrow{position:absolute;top:calc(50% - 20px);transform:translateY(-50%);z-index:60;
  width:36px;height:36px;border-radius:50%;border:none;background:var(--bg-card);color:var(--text);
  box-shadow:var(--card-shadow);cursor:pointer;font-size:13px;transition:.2s;opacity:0;pointer-events:none}
body.multi .arrow{opacity:.92;pointer-events:auto}
.arrow:hover{transform:translateY(-50%) scale(1.12)}
.arrow.l{left:11px}.arrow.r{right:11px}
.arrow:disabled{opacity:0!important;pointer-events:none}
.dots{flex-shrink:0;height:22px;display:none;justify-content:center;align-items:center;gap:7px}
body.multi .dots{display:flex}
.dots i{width:7px;height:7px;border-radius:50%;background:var(--border);cursor:pointer;
  transition:all .35s cubic-bezier(.55,-.3,.35,1.4);flex-shrink:0}
.dots i.on{background:var(--primary);width:20px;border-radius:5px;box-shadow:0 0 8px var(--primary)}

.device-card{width:100%;max-width:760px;height:100%;background:var(--bg-card);
  border-radius:20px;box-shadow:var(--card-shadow);
  display:flex;flex-direction:column;overflow:hidden;min-height:0}
.card-top{padding:11px 20px 7px;display:flex;justify-content:space-between;align-items:center}
.dev-name{font-size:16px;font-weight:700;display:flex;align-items:center;gap:8px}
.dot{width:8px;height:8px;border-radius:50%;bac)=====";
static const char WEBPAGE_3[] PROGMEM = R"=====(kground:var(--sub)}
.dot.on{background:var(--green);box-shadow:0 0 10px var(--green);animation:pu 1.6s infinite}
@keyframes pu{50%{opacity:.5}}
.metrics{display:grid;grid-template-columns:repeat(5,1fr);gap:9px;padding:0 20px 9px}
.mc{background:var(--bg-soft);border-radius:13px;padding:8px 4px;
  display:flex;flex-direction:column;align-items:center;gap:2px}
.mc svg{width:18px;height:18px;stroke:var(--sub);fill:none;stroke-width:1.8;
  stroke-linecap:round;stroke-linejoin:round}
.mc.blue svg{stroke:var(--primary)} .mc.or svg{stroke:var(--orange)}
.mc .v{font-size:clamp(13px,1.5vw,18px);font-weight:700;font-variant-numeric:tabular-nums}
.mc.blue .v{color:var(--primary)} .mc.or .v{color:var(--orange)}
.mc .l{font-size:10px;color:var(--sub);letter-spacing:.05em}
.chart-wrap{flex:1;min-height:0;position:relative;padding:2px 16px}
.chart-wrap canvas{width:100%;height:100%;display:block;cursor:crosshair;touch-action:none}
.ctip{position:absolute;background:rgba(20,22,30,.88);color:#fff;padding:6px 10px;border-radius:8px;
  font-size:11.5px;pointer-events:none;opacity:0;transition:opacity .12s;white-space:nowrap;z-index:30;
  transform:translate(-50%,0);line-height:1.6}
.controls{padding:9px 20px 13px;display:grid;grid-template-columns:1fr 1fr 1.25fr;gap:22px;border-top:1px solid var(--border)}
.cg{display:flex;flex-direction:column;gap:8px;min-width:0}
.ch{display:flex;justify-content:space-between;align-items:center;font-size:12.5px;font-weight:600}
.numin{width:54px;height:26px;padding:0 7px;border:1px solid var(--border);border-radius:8px;
  background:var(--bg-soft);color:var(--text);text-align:center;font-size:13px;outline:none}
.numin:focus{border-color:var(--primary)}
.rail input[type=range]{-webkit-appearance:none;appearance:none;width:100%;height:8px;margin:0;
  border-radius:8px;background:transparent;outline:none;cursor:pointer;touch-action:none}
.rail input[type=range]::-webkit-slider-runnable-track{height:8px;border-radius:8px;
  background:linear-gradient(to right,var(--primary) var(--p,50%),var(--border) var(--p,50%))}
.rail input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:20px;height:20px;
  margin-top:-6px;border-radius:50%;background:#fff;border:4px solid var(--primary);
  box-shadow:0 2px 8px rgba(0,0,0,.28)}
.rail input[type=range]::-moz-range-track{height:8px;border-radius:8px;
  background:linear-gradient(to right,var(--primary) var(--p,50%),var(--border) var(--p,50%))}
.rail input[type=range]::-moz-range-thumb{width:14px;height:14px;border-radius:50%;background:#fff;
  border:4px solid var(--primary);box-shadow:0 2px 8px rgba(0,0,0,.28)}
.timebox{display:flex;align-items:center;justify-content:space-between;gap:6px}
.tc{flex:1;display:flex;flex-direction:column;align-items:center;gap:2px;
  background:var(--bg-soft);border-radius:12px;padding:6px 0 4px;min-width:0;
  border:1.5px solid transparent;transition:.2s}
.tc:focus-within{border-color:var(--primary);background:var(--bg-card)}
.tc input{width:90%;text-align:center;font-size:clamp(17px,2vw,22px);font-weight:700;
  border:none;background:transparent;color:var(--text);
  outline:none;font-variant-numeric:tabular-nums;cursor:ns-resize;touch-action:none;-moz-appearance:textfield}
.tc input::-webkit-outer-spin-button,.tc input::-webkit-inner-spin-button{-webkit-appearance:none}
.tc span{font-size:9.5px;color:var(--sub);letter-spacing:.2em}
.tsp{font-size:18px;color:var(--sub);font-weight:600}
.btn{border:none;border-radius:10px;padding:7px 16px;font-size:13px;font-weight:700;cursor:pointer;transition:.15s;font-family:inherit}
.btn:active{transform:scale(.94)}
.btn-p{background:var(--primary);color:#fff;box-shadow:0 4px 12px rgba(10,132,255,.35)}
.btn-d{background:transparent;color:var(--red);bor)=====";
static const char WEBPAGE_4[] PROGMEM = R"=====(der:1.5px solid var(--red)}
.btn-ghost{background:var(--bg-soft);color:var(--text)}
.gfooter{flex-shrink:0;height:52px;display:none;align-items:center;justify-content:space-between;
  padding:0 clamp(16px,4vw,44px);border-top:1px solid var(--border);background:var(--bg-card)}
body.multi .gfooter{display:flex}
.gfooter .gt{font-weight:700;font-size:13.5px}
.gfooter .gc{display:flex;gap:11px}

@media (min-width:1200px){
  .device-card{max-width:1100px}
  .pane{padding:10px 80px 12px}
  .mc .v{font-size:clamp(16px,1.4vw,24px)}
  .mc .l{font-size:12px}
  .mc svg{width:22px;height:22px}
  .dev-name{font-size:19px}
  .ch{font-size:14.5px}
  .numin{width:64px;height:30px;font-size:14px}
  .tc input{font-size:26px}
  .legend{font-size:13px!important}
  .brand b{font-size:28px}
}
@media (max-width:860px){
  .controls{grid-template-columns:1fr;gap:10px}
  .pane{padding:4px 8px 8px}
  .arrow{top:auto;bottom:14px;transform:none}
  .conn span{display:none}
}
</style>
</head>
<body>

<div id="errbar"></div>

<header>
  <div class="brand"><b>QIMINGXING</b><em>DRYER</em></div>
  <div style="display:flex;align-items:center">
    <div class="conn" id="conn"><i></i><span>未连接</span></div>
    <button class="daynight" onclick="toggleTheme()">
      <span class="st p1"><svg width="10" height="10" viewBox="0 0 10 10"><path d="M5 0L6 4L10 5L6 6L5 10L4 6L0 5L4 4Z" fill="#fff"/></svg></span>
      <span class="st p2"><svg width="7" height="7" viewBox="0 0 10 10"><path d="M5 0L6 4L10 5L6 6L5 10L4 6L0 5L4 4Z" fill="#fff"/></svg></span>
      <span class="st p3"><svg width="8" height="8" viewBox="0 0 10 10"><path d="M5 0L6 4L10 5L6 6L5 10L4 6L0 5L4 4Z" fill="#fff"/></svg></span>
      <span class="st p4"><svg width="6" height="6" viewBox="0 0 10 10"><path d="M5 0L6 4L10 5L6 6L5 10L4 6L0 5L4 4Z" fill="#fff"/></svg></span>
      <span class="cloud"></span>
      <span class="knob"><span class="cr k1"></span><span class="cr k2"></span><span class="cr k3"></span></span>
    </button>
  </div>
</header>

<div class="subbar">
  <button class="pill" onclick="openPreset()">
    <svg viewBox="0 0 24 24"><path d="M12 2l2.4 5.4L20 8l-4 4 1 5.8L12 15l-5 2.8L8 12 4 8l5.6-.6z"/></svg>
    <span id="presetName">PLA</span>
    <svg class="arr" viewBox="0 0 24 24"><path d="M6 9l6 6 6-6"/></svg>
  </button>
  <button class="pill" onclick="openAddPreset()">
    <svg viewBox="0 0 24 24"><path d="M12 5v14M5 12h14"/></svg>
    <span>新增</span>
  </button>
  <button class="pill del" id="delBtn" onclick="delFromBar()" title="删除预设">
    <svg viewBox="0 0 24 24"><path d="M3 6h18M8 6V4a1 1 0 0 1 1-1h6a1 1 0 0 1 1 1v2m3 0v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6M10 11v6M14 11v6"/></svg>
  </button>
  <label class="can-sw">
    <span>CAN 设备级联</span>
    <input type="checkbox" id="cascadeToggle" onchange="toggleCascade(this.checked)">
    <span class="tr"></span>
  </label>
</div>

<main id="main">
  <div id="stage"></div>
  <div class="edge l" id="edgeL"></div>
  <div class="edge r" id="edgeR"></div>
  <button class="arrow l" id="arrL" onclick="nav(-1)">❮</button>
  <button class="arrow r" id="arrR" onclick="nav(1)">❯</button>
</main>

<div class="dots" id="dots"></div>

<div class="gfooter">
  <div class="gt">级联全局控制</div>
  <div class="gc">
    <button class="btn btn-p" onclick="globalCtrl(1)">全部启动</button>
    <button class="btn btn-d" onclick="globalCtrl(0)">全部停止</button>
  </div>
</div>

<div class="modal-mask" id="presetModal">
  <div class="modal">
    <h3>
      <svg viewBox="0 0 24 24"><path d="M12 2l2.4 5.4L20 8l-4 4 1 5.8L12 15l-5 2.8L8 12 4 8l5.6-.6z"/></svg>
      选择耗材预设
      <span class="badge" id="delBadge" style="display)=====";
static const char WEBPAGE_5[] PROGMEM = R"=====(:none">删除模式</span>
    </h3>
    <div class="msub" id="msub"></div>
    <div class="pscroll" id="pscroll"></div>
    <div class="mfoot">
      <span class="left" id="delCount" style="font-size:12.5px;color:var(--red);font-weight:600;visibility:hidden"></span>
      <button class="btn btn-ghost" onclick="closePreset()">取消</button>
      <button class="btn btn-p" id="okBtn" onclick="presetOK()">应用</button>
    </div>
  </div>
</div>

<div class="modal-mask" id="addModal">
  <div class="modal" style="width:min(400px,92vw)">
    <h3><svg viewBox="0 0 24 24"><path d="M12 5v14M5 12h14"/></svg>新增预设</h3>
    <div class="form">
      <div class="frow">
        <label>耗材名称（仅大写字母和空格，最多10字符）</label>
        <input type="text" id="fName" maxlength="10" placeholder="例如：PLA">
      </div>
      <div class="frow">
        <label>烘干温度（°C）</label>
        <div class="fhms">
          <input type="number" id="fTemp" min="1" max="80" placeholder="45">
          <span style="flex:0 0 auto;font-size:13px">°C</span>
        </div>
      </div>
      <div class="frow">
        <label>烘干时间（最长 47:59:59）</label>
        <div class="fhms">
          <input type="number" id="fH" min="0" max="47" value="12"><span>时</span>
          <span class="sp">:</span>
          <input type="number" id="fM" min="0" max="59" value="0"><span>分</span>
          <span class="sp">:</span>
          <input type="number" id="fS" min="0" max="59" value="0"><span>秒</span>
        </div>
      </div>
    </div>
    <div class="mfoot">
      <button class="btn btn-ghost" onclick="closeAddPreset()">退出</button>
      <button class="btn btn-p" onclick="savePreset()">保存</button>
    </div>
  </div>
</div>

<div id="toast"></div>

<script>
window.onerror=function(msg,src,line){
  var b=document.getElementById('errbar');
  b.style.display='block';
  b.textContent='JS 错误: '+msg+' @第'+line+'行';
};

/* =====================================================================
   通信：ESP01S 只做透传。网页 WS 文本帧 <-> UART 字节流。
   STM32 每秒推一条 DATA，本页收到只改文本节点和 canvas，不重建 DOM。
   ===================================================================== */

var MAXT={temp:80,ptc:160},MINT={temp:30,ptc:40},MAXTIME={H:47,M:59,S:59},MAXDEV=16;
var S={dark:false,multi:false,cur:0,preset:0,connected:MAXDEV,dev:{}};
var charts={};
/* 默认值对齐 STM32 固件默认(TEMP_DEFAULT=50℃, PTC=70℃, 时长2:00:00) */
S.dev.master={name:'主设备',run:true,temp:42.5,ptc:65.2,humi:55,wtG:1200,
  setTemp:50,setPtc:70,tH:2,tM:0,tS:0,rem:7200,hist:{t:[],p:[],h:[],x:[]}};
for(var i=1;i<MAXDEV;i++){
  S.dev['slave_'+i]={name:'设备 0x'+(16+i).toString(16).toUpperCase(),run:(i%3===0),
    temp:25+i%8,ptc:30+i%8,humi:50+i%6,wtG:400+i*35,setTemp:50,setPtc:70,
    tH:0,tM:45,tS:0,rem:2700,hist:{t:[],p:[],h:[],x:[]}};
}


/* ---------- WS：收发与 ACK 重发 ---------- */
var ws=null,wsReady=false,ackWait={},ACK_TIMEOUT=6000,ACK_RETRY=3;
function setConn(ok){
  var el=document.getElementById('conn');
  el.classList.toggle('ok',ok);
  el.querySelector('span').textContent=ok?'已连接':'未连接';
}
function wsConnect(){
  try{ws=new WebSocket((location.protocol==='https:'?'wss://':'ws://')+(location.hostname||'192.168.4.1')+':81/');}
  catch(e){return;}
  ws.onopen=function(){wsReady=true;setConn(true);rawSend('{"t":"HELLO"}');};
  ws.onclose=function(){wsReady=false;setConn(false);setTimeout(wsConnect,2000);};
  ws.onerror=function(){try{ws.close();}catch(e){}};
  ws.onmessage=function(ev){
    var m;try{m=JSON.parse(ev.data);}catch(e){return;}
 )=====";
static const char WEBPAGE_6[] PROGMEM = R"=====(   if(m.t==='ACK'&&m.d&&ackWait[m.d.cmd]){clearTimeout(ackWait[m.d.cmd].timer);delete ackWait[m.d.cmd];return;}
    handleMsg(m.t,m.d);
    if(m.id)rawSend('{"t":"ACK","d":{"cmd":"'+m.id+'"}}');
  };
}
function rawSend(str){if(wsReady&&ws.readyState===1)ws.send(str);else console.log('[离线模拟>>]',str);}
function send(t,d){
  var id=t+'_'+Date.now();
  var json=JSON.stringify({id:id,t:t,d:d});
  ackWait[id]={retries:0,timer:setTimeout(function(){retry(id,json);},ACK_TIMEOUT)};
  rawSend(json);
}
function retry(id,json){
  var en=ackWait[id];if(!en)return;
  if(++en.retries>ACK_RETRY){delete ackWait[id];toast('设备未响应');return;}
  rawSend(json);
  en.timer=setTimeout(function(){retry(id,json);},ACK_TIMEOUT);
}

/* ---------- STM32 → 网页 分发 ---------- */
function handleMsg(t,d){
  switch(t){
    case 'DEVS': onDevs(d);break;
    case 'PRESET_LIST':
      if(d&&d.list)mcuPresets=d.list;
      if(d&&d.current!=null)S.preset=d.current;
      if(d&&d.connected!=null)S.connected=d.connected;
      updatePresetName();
      if(document.getElementById('presetModal').classList.contains('show'))renderPresetList();
      break;
    case 'PRESET_SAVED': toast('预设已写入设备');break;
    case 'PRESET_DELETED': toast('预设已删除');break;
    case 'DATA': onData(d);break;
  }
}

/* ---------- 设备列表：主机 + 实际已连接的从机(名称=设备+序列号) ---------- */
function onDevs(d){
  if(!d||!d.list||!d.list.length)return;
  var cur=keys()[S.cur];
  S.dev={};
  d.list.forEach(function(x){
    var nm=(x.id==='master')?'主设备':('设备'+(x.sn||x.id));
    S.dev[x.id]={
      name:nm,run:false,temp:0,ptc:0,humi:0,wtG:0,
      setTemp:50,setPtc:70,tH:2,tM:0,tS:0,rem:7200,hist:{t:[],p:[],h:[],x:[]}
    };
  });
  S.connected=d.list.length-1;
  S.cur=S.dev[cur]?keys().indexOf(cur):0;
  rerenderAll();
}

/* ---------- 1Hz 传感数据：只改文本，不重建卡片 ---------- */
function onData(d){
  if(!d||!d.id)return;
  var k=d.id,dev=S.dev[k];if(!dev)return;
  var changed=false;
  ['temp','ptc','humi','wtG','tH','tM','tS','setTemp','setPtc','rem'].forEach(function(f){
    if(_prmTimers[k+'_'+f])return;   /* 正在调节：该值以本地为准，不被回推覆盖 */
    if(d[f]!=null&&d[f]!==dev[f]){dev[f]=d[f];changed=true;}
  });
  if(d.run!=null&&d.run!==dev.run){dev.run=d.run;updateRunUI(k,dev);changed=true;}
  if(k==='master'&&d.can!=null&&(!!d.can)!==S.multi){S.multi=!!d.can;document.body.classList.toggle('multi',S.multi);var ct=document.getElementById('cascadeToggle');if(ct)ct.checked=S.multi;changed=true;}
  if(k===keys()[S.cur]){
    /* 网页端预设/参数变化：同步烘干温度、PTC 温度、时长控制件 */
    var syncs={setTemp:[dev.setTemp,dev.setTemp],setPtc:[dev.setPtc,dev.setPtc],tH:[dev.tH,dev.tH],tM:[dev.tM,dev.tM],tS:[dev.tS,dev.tS]};
    Object.keys(syncs).forEach(function(p){
      if(_prmTimers[k+'_'+p])return;   /* 正在拖动/防抖期间：值以本地为准，不被回推覆盖 */
      var inp=document.querySelector('input[data-key="'+k+'"][data-p="'+p+'"]');
      var rng=document.querySelector('input[type=range][data-key="'+k+'"][data-p="'+p+'"]');
      var nv=syncs[p][1];
      if(inp&&inp.value!=String(nv)){inp.value=nv;}
      if(rng){ if(rng.value!=String(nv))rng.value=nv; paintRange(rng); }
    });
  }
  if(!changed)return;
  pushHist(dev);
  if(k===keys()[S.cur]){
    var V={};
    V.temp=document.getElementById('v-temp-'+k);
    V.humi=document.getElementById('v-humi-'+k);
    V.wt=document.getElementById('v-wt-'+k);
    V.ptc=document.getElementById('v-ptc-'+k);
    V.time=document.getElementById('v-time-'+k);
    if(V.temp){
      V.temp.textContent=dev.temp.t)=====";
static const char WEBPAGE_7[] PROGMEM = R"=====(oFixed(1)+'°C';
      V.humi.textContent=dev.humi.toFixed(0)+'%';
      V.wt.textContent=fmtWt(dev.wtG);
      V.ptc.textContent=dev.ptc.toFixed(1)+'°C';
      V.time.textContent=fmtRem(dev);
    }
    if(charts[k])drawChart(k);
  }
}
function updateRunUI(k,dev){
  var dot=document.getElementById('dot-'+k),btn=document.getElementById('btn-'+k);
  if(dot)dot.classList.toggle('on',dev.run);
  if(btn){
    btn.textContent=dev.run?'停止':'启动';
    btn.classList.toggle('btn-d',dev.run);
    btn.classList.toggle('btn-p',!dev.run);
    btn.setAttribute('onclick',"toggleRun('"+k+"')");
  }
}

function fmt(d){return [d.tH,d.tM,d.tS].map(function(v){return String(v).padStart(2,'0');}).join(':');}
/* 卡片“剩余”显示剩余时长(rem秒)；无 rem 时退回设定时长 */
function fmtRem(d){
  if(d.rem!=null&&d.rem>=0){
    var s=Math.round(d.rem);
    return [Math.floor(s/3600),Math.floor((s%3600)/60),s%60].map(function(v){return String(v).padStart(2,'0');}).join(':');
  }
  return fmt(d);
}
function fmtWt(g){return g>=1000?(g/1000).toFixed(2)+'kg':Math.round(g)+'g';}
function keys(){return Object.keys(S.dev);}
function pushHist(d){if(!d.hist)return;
  d.hist.t.push(d.temp);d.hist.p.push(d.ptc);d.hist.h.push(d.humi);
  d.hist.x.push(new Date().toLocaleTimeString('zh-CN',{hour12:false}));
  if(d.hist.t.length>21600){d.hist.t.shift();d.hist.h.shift();d.hist.p.shift();d.hist.x.shift();}
}
Object.keys(S.dev).forEach(function(k){var d=S.dev[k];
  for(var j=30;j>=0;j--){
    d.hist.t.push(d.temp-j*.15);d.hist.p.push(d.ptc-j*.3);d.hist.h.push(d.humi);
    d.hist.x.push(new Date(Date.now()-j*2000).toLocaleTimeString('zh-CN',{hour12:false}));
  }});

/* ================= 预设 ================= */
var mcuPresets=[
  {name:'PLA',temp:45,h:12,m:0,s:0},{name:'PETG',temp:55,h:6,m:0,s:0},
  {name:'ABS',temp:70,h:4,m:0,s:0},{name:'TPU',temp:50,h:8,m:30,s:0},
  {name:'ASA',temp:70,h:4,m:0,s:0},{name:'PA12',temp:80,h:10,m:0,s:0},
  {name:'PC',temp:80,h:12,m:0,s:0},{name:'Nylon',temp:70,h:8,m:0,s:0}
];
var pendingPreset=0,delMode=false,delSel={};
function updatePresetName(){
  var p=mcuPresets[S.preset];
  document.getElementById('presetName').textContent=p?p.name:'未加载';
}
function renderPresetList(){
  var box=document.getElementById('pscroll'),html='';
  for(var i=0;i<mcuPresets.length;i++){
    var p=mcuPresets[i];
    var cls=delMode?(delSel[i]?' del-sel':''):(pendingPreset===i?' sel':'');
    var ts=String(p.h).padStart(2,'0')+'h'+String(p.m).padStart(2,'0')+'m'+String(p.s).padStart(2,'0')+'s';
    html+='<button class="pcard'+cls+'" onclick="pickPreset('+i+')">'
      +'<div class="pc-main"><div class="pn">'+p.name+'</div>'
      +'<div class="pd">空气温度 '+p.temp+'°C · '+ts+'</div></div>'
      +'<span class="tick"><svg viewBox="0 0 24 24"><path d="M4 12l5 5L20 7"/></svg></span></button>';
  }
  box.innerHTML=html||'<div class="loading">暂无预设，点击工具栏「新增」创建</div>';
}
function refreshDelUI(){
  var n=Object.keys(delSel).length;
  var dc=document.getElementById('delCount');
  dc.textContent='已选 '+n+' 项';
  dc.style.visibility=(delMode&&n>0)?'visible':'hidden';
  var ok=document.getElementById('okBtn');
  ok.textContent=delMode?'删除':'应用';
  ok.classList.toggle('btn-d',delMode);
  ok.classList.toggle('btn-p',!delMode);
}
function pickPreset(i){
  if(i<0)return;
  if(delMode){if(delSel[i])delete delSel[i];else delSel[i]=true;}
  else pendingPreset=i;
  renderPresetList();
  refreshDelUI();
}
function openPreset(){
  pendingPreset=S.preset;delMode=false;delSel={};
  document.getElementById('delBadge').style.display='none';
  document.getElementById('msub').textContent=S.multi?('选用后将发送到全)=====";
static const char WEBPAGE_8[] PROGMEM = R"=====(部 '+S.connected+' 台设备'):'';
  pickPreset(pendingPreset);
  refreshDelUI();
  document.getElementById('presetModal').classList.add('show');
  send('PRESET_GET',null);
}
function closePreset(){
  document.getElementById('presetModal').classList.remove('show');
  delMode=false;delSel={};
}
function delFromBar(){
  delMode=true;delSel={};
  pendingPreset=S.preset;
  document.getElementById('delBadge').style.display='inline-block';
  document.getElementById('msub').textContent='点选要删除的预设（可多选）';
  renderPresetList();
  refreshDelUI();
  document.getElementById('presetModal').classList.add('show');
}
function presetOK(){
  if(delMode){
    var ids=Object.keys(delSel);
    if(ids.length===0){toast('请先点选要删除的预设');return;}
    var remain=[];
    for(var i=0;i<mcuPresets.length;i++)if(!delSel[i])remain.push(mcuPresets[i]);
    send('PRESET_DELETE',{remain:remain});
    mcuPresets=remain;
    if(S.preset>=remain.length)S.preset=Math.max(0,remain.length-1);
    pendingPreset=S.preset;
    delMode=false;delSel={};
    updatePresetName();closePreset();
    toast('已删除所选预设');
  }else{
    var p=mcuPresets[pendingPreset];if(!p)return closePreset();
    S.preset=pendingPreset;
    var targets=S.multi?keys():['master'];
    targets.forEach(function(k){var d=S.dev[k];
      d.setTemp=p.temp;d.tH=p.h;d.tM=p.m;d.tS=p.s;});
    updatePresetName();
    toast('已应用「'+p.name+'」'+(S.multi?('到 '+S.connected+' 台设备'):''));
    send('PRESET_APPLY',{index:pendingPreset,name:p.name,temp:p.temp,h:p.h,m:p.m,s:p.s,targets:targets});
    closePreset();rerenderAll();
  }
}
function openAddPreset(){
  document.getElementById('fName').value='';
  document.getElementById('fTemp').value='';
  document.getElementById('fH').value=12;
  document.getElementById('fM').value=0;
  document.getElementById('fS').value=0;
  document.getElementById('addModal').classList.add('show');
  setTimeout(function(){document.getElementById('fName').focus();},350);
}
function closeAddPreset(){document.getElementById('addModal').classList.remove('show');}
function savePreset(){
  var name=document.getElementById('fName').value.trim();
  var temp=parseInt(document.getElementById('fTemp').value);
  var h=Math.min(47,Math.max(0,parseInt(document.getElementById('fH').value)||0));
  var m=Math.min(59,Math.max(0,parseInt(document.getElementById('fM').value)||0));
  var s=Math.min(59,Math.max(0,parseInt(document.getElementById('fS').value)||0));
  if(!name){document.getElementById('fName').focus();return;}
  if(!/^[A-Z ]{1,10}$/.test(name)){toast('名称只能包含大写字母和空格，最多10字符');document.getElementById('fName').focus();return;}
  if(isNaN(temp)||temp<MINT.temp||temp>MAXT.temp){toast('烘干温度需在 '+MINT.temp+'~'+MAXT.temp+'°C');return;}
  var item={name:name,temp:temp,h:h,m:m,s:s};
  var idx=-1;
  for(var i=0;i<mcuPresets.length;i++)if(mcuPresets[i].name===name)idx=i;
  if(idx>=0)mcuPresets[idx]=item;else mcuPresets.push(item);
  send('PRESET_SAVE',item);
  pendingPreset=mcuPresets.indexOf(item);S.preset=pendingPreset;
  updatePresetName();closeAddPreset();
  toast('预设「'+name+'」已保存');
}

/* ================= 卡片 ================= */
var IC={
  temp:'<svg viewBox="0 0 24 24"><path d="M14 14.76V5a2 2 0 0 0-4 0v9.76a4 4 0 1 0 4 0z"/></svg>',
  humi:'<svg viewBox="0 0 24 24"><path d="M12 2.7s6.5 7 6.5 11.8a6.5 6.5 0 0 1-13 0C5.5 9.7 12 2.7 12 2.7z"/></svg>',
  wt:'<svg viewBox="0 0 24 24"><rect x="4" y="7" width="16" height="13" rx="2"/><path d="M9 7a3 3 0 0 1 6 0"/><path d="M12 11v4"/></svg>',
  ptc:'<svg viewBox="0 0 24 24"><path d="M12 22c4 0 7-2.9 7-7 0-3-2-5.5-3.5-7C15 9.5 14 10 14 8c0-2-1-4.5)=====";
static const char WEBPAGE_9[] PROGMEM = R"=====(-2-6-1 2.5-4 5-4 9 0 0-1.5-1-2-2.5C4.7 10 5 15 5 15c0 4.1 3 7 7 7z"/></svg>',
  time:'<svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="9"/><path d="M12 7v5l3.5 2"/></svg>'
};
function cardHTML(k){
  var d=S.dev[k];
  function mc(id,cls,ic,v,l){
    return '<div class="mc '+cls+'">'+ic+'<div class="v" id="v-'+id+'">'+v+'</div><div class="l">'+l+'</div></div>';
  }
  return '<div class="card-top">'
  +'<div class="dev-name"><span class="dot '+(d.run?'on':'')+'" id="dot-'+k+'"></span>'+d.name+'</div>'
  +'<button class="btn '+(d.run?'btn-d':'btn-p')+'" id="btn-'+k+'" onclick="toggleRun(\''+k+'\')">'+(d.run?'停止':'启动')+'</button>'
  +'</div>'
  +'<div class="metrics">'
  +mc('temp-'+k,'blue',IC.temp,d.temp.toFixed(1)+'°C','温度')
  +mc('humi-'+k,'',IC.humi,d.humi.toFixed(0)+'%','湿度')
  +mc('wt-'+k,'',IC.wt,fmtWt(d.wtG),'重量')
  +mc('ptc-'+k,'or',IC.ptc,d.ptc.toFixed(1)+'°C','PTC')
  +mc('time-'+k,'',IC.time,fmtRem(d),'剩余')
  +'</div>'
  +'<div class="chart-wrap"><canvas id="cv-'+k+'"></canvas><div class="ctip" id="tip-'+k+'"></div></div>'
  +'<div class="legend" style="display:flex;gap:16px;font-size:11px;color:var(--sub);padding:0 22px 2px">'
  +'<span><i style="display:inline-block;width:14px;height:3px;border-radius:2px;margin-right:5px;background:var(--primary);vertical-align:middle"></i>箱内温度</span>'
  +'<span><i style="display:inline-block;width:14px;height:3px;border-radius:2px;margin-right:5px;background:var(--humi);vertical-align:middle"></i>湿度</span>'
  +'<span><i style="display:inline-block;width:14px;height:3px;border-radius:2px;margin-right:5px;background:var(--orange);vertical-align:middle"></i>PTC 温度</span></div>'
  +'<div class="controls">'
  +'<div class="cg"><div class="ch"><span>烘干温度</span>'
  +'<input class="numin" type="number" onfocus="this.select();focusInp(this)" onblur="focusOut(this)" min="'+MINT.temp+'" max="'+MAXT.temp+'" value="'+d.setTemp+'" data-key="'+k+'" data-p="setTemp" onchange="numSet(this)"></div>'
  +'<div class="rail"><input type="range" min="'+MINT.temp+'" max="'+MAXT.temp+'" value="'+d.setTemp+'" data-key="'+k+'" data-p="setTemp" oninput="rangeSet(this)"></div></div>'
  +'<div class="cg"><div class="ch"><span>PTC 工作温度</span>'
  +'<input class="numin" type="number" onfocus="this.select();focusInp(this)" onblur="focusOut(this)" min="'+MINT.ptc+'" max="'+MAXT.ptc+'" value="'+d.setPtc+'" data-key="'+k+'" data-p="setPtc" onchange="numSet(this)"></div>'
  +'<div class="rail"><input type="range" min="'+MINT.ptc+'" max="'+MAXT.ptc+'" value="'+d.setPtc+'" data-key="'+k+'" data-p="setPtc" oninput="rangeSet(this)"></div></div>'
  +'<div class="cg"><div class="ch"><span>烘干时长</span></div><div class="timebox">'
  +'<div class="tc"><input type="number" onfocus="this.select();focusInp(this)" onblur="focusOut(this)" max="47" value="'+d.tH+'" data-key="'+k+'" data-p="tH" onchange="timeSet(this)"><span>时</span></div>'
  +'<span class="tsp">:</span>'
  +'<div class="tc"><input type="number" onfocus="this.select();focusInp(this)" onblur="focusOut(this)" max="59" value="'+d.tM+'" data-key="'+k+'" data-p="tM" onchange="timeSet(this)"><span>分</span></div>'
  +'<span class="tsp">:</span>'
  +'<div class="tc"><input type="number" onfocus="this.select();focusInp(this)" onblur="focusOut(this)" max="59" value="'+d.tS+'" data-key="'+k+'" data-p="tS" onchange="timeSet(this)"><span>秒</span></div>'
  +'</div></div></div>';
}

/* ================= 参数 ================= */
function paintRange(el){
  var mn=+el.min||0,mx=+el.max||100,v=+el.value;
  var pct=(mx===mn)?0:(v-mn)/(mx-mn);
  /* 按 20px 滑块半径补偿进度条填充终点，使球心与填充前沿对齐 */
  var tw=el.clientWidth||220, th=20;
  v)=====";
static const char WEBPAGE_10[] PROGMEM = R"=====(ar f=pct*(tw-th)/tw*100 + th/2/tw*100;
  el.style.setProperty('--p',f+'%');
}
function pushParamChange(k,p){
  var d=S.dev[k];
  var pn=mcuPresets[S.preset]?mcuPresets[S.preset].name:'';
  send('PARAM_SET',{id:k,param:p,value:d[p],
    preset:{name:pn,temp:d.setTemp,h:d.tH,m:d.tM,s:d.tS}});
}
var _prmTimers={};

function focusInp(el){_prmTimers[el.dataset.key+'_'+el.dataset.p]='focus';}
function focusOut(el){var id=el.dataset.key+'_'+el.dataset.p;if(_prmTimers[id]==='focus')delete _prmTimers[id];}
function paramMin(p){return p==='setTemp'?MINT.temp:(p==='setPtc'?MINT.ptc:0);}
function rangeSet(el){
  var k=el.dataset.key,p=el.dataset.p;
  S.dev[k][p]=+el.value;paintRange(el);
  document.querySelectorAll('input[data-key="'+k+'"][data-p="'+p+'"]').forEach(function(x){
    if(x!==el){x.value=el.value;paintRange(x);}});
  /* 拖动/输入中不刷屏：停止 200ms 后才把最终值发主机 */
  var key=k+'_'+p;
  clearTimeout(_prmTimers[key]);
  _prmTimers[key]=setTimeout(function(){delete _prmTimers[key];pushParamChange(k,p);},200);
}
function numSet(el){
  var k=el.dataset.key,p=el.dataset.p;
  el.value=Math.min(+el.max,Math.max(paramMin(p),(+el.value||0)));
  S.dev[k][p]=+el.value;
  document.querySelectorAll('input[data-key="'+k+'"][data-p="'+p+'"]').forEach(function(x){
    if(x!==el){x.value=el.value;paintRange(x);}});
  pushParamChange(k,p);   /* change = discrete commit: push now */
  _prmTimers[k+'_'+p]=setTimeout(function(){delete _prmTimers[k+'_'+p];},300);
}
function timeSet(el,f){
  var k=el.dataset.key,p=el.dataset.p;
  var v=(f!==undefined)?f:(+el.value||0);
  v=Math.max(0,Math.min(+el.max,v));
  el.value=v;S.dev[k][p]=v;
  clearTimeout(_prmTimers[k+'_'+p]);
  pushParamChange(k,p);   /* duration input: push now */
  _prmTimers[k+'_'+p]=setTimeout(function(){delete _prmTimers[k+'_'+p];},300);
}

/* ================= 舞台 ================= */
function buildStage(){
  var st=document.getElementById('stage'),html='';
  keys().forEach(function(k){
    html+='<div class="pane"><div class="device-card">'+cardHTML(k)+'</div></div>';
  });
  st.innerHTML=html;
  go(S.cur,true);
  st.querySelectorAll('input[type=range]').forEach(paintRange);
}
var dragDX=0;
function go(idx,instant){
  var st=document.getElementById('stage'),n=st.children.length;
  if(n===0)return;
  S.cur=Math.max(0,Math.min(n-1,idx));
  if(instant){
    st.classList.add('drag');
    st.style.transform='translateX(-'+(S.cur*100)+'%)';
    void st.offsetWidth;
    st.classList.remove('drag');
  }else{
    st.style.transform='translateX(-'+(S.cur*100)+'%)';
  }
  dragDX=0;renderDots();updateArrows();
  var k=keys()[S.cur];
  if(k&&!charts[k]){initChart(k);bindChart(k);}
}
function nav(d){go(S.cur+d);}
function updateArrows(){
  document.getElementById('arrL').disabled=S.cur===0;
  document.getElementById('arrR').disabled=S.cur===keys().length-1;
}
function renderDots(){
  var html='';
  keys().forEach(function(k,i){
    html+='<i class="'+(i===S.cur?'on':'')+'" onclick="go('+i+')"></i>';
  });
  document.getElementById('dots').innerHTML=html;
}
(function(){
  var st=document.getElementById('stage'),main=document.getElementById('main');
  var sx=0,sy=0,active=false,lock=null;
  main.addEventListener('pointerdown',function(e){
    if(!S.multi)return;
    if(e.target.closest('input,select,button,canvas,.tc,.rail,.numin'))return;
    active=true;lock=null;sx=e.clientX;sy=e.clientY;
  });
  main.addEventListener('pointermove',function(e){
    if(!active)return;
    var dx=e.clientX-sx,dy=e.clientY-sy;
    if(lock===null){
      if(Math.abs(dx)>12||Math.abs(dy)>12)lock=Math.abs(dx)>Math.abs(dy)?'h':'v';
      else return;
    }
    if(lock!=='h')return;
    var atEdge=(S.c)=====";
static const char WEBPAGE_11[] PROGMEM = R"=====(ur===0&&dx>0)||(S.cur===keys().length-1&&dx<0);
    dragDX=atEdge?dx*0.25:dx;
    st.classList.add('drag');
    st.style.transform='translateX(calc(-'+(S.cur*100)+'% + '+dragDX+'px))';
  });
  function end(){
    if(!active)return;active=false;
    st.classList.remove('drag');
    if(lock==='h'){
      var th=st.clientWidth*0.18;
      if(dragDX<-th)go(S.cur+1);
      else if(dragDX>th)go(S.cur-1);
      else go(S.cur);
    }
    dragDX=0;
  }
  main.addEventListener('pointerup',end);
  main.addEventListener('pointercancel',end);
})();
function bindEdges(){
  document.getElementById('edgeL').addEventListener('click',function(){nav(-1);});
  document.getElementById('edgeR').addEventListener('click',function(){nav(1);});
}

/* ================= 模式 ================= */
function toggleCascade(on){
  S.multi=on;S.cur=0;dragDX=0;
  document.body.classList.toggle('multi',on);
  rerenderAll();
  send('CAN_MODE',{on:!!on});
}
function rerenderAll(){
  buildStage();charts={};
  requestAnimationFrame(function(){
    keys().forEach(initChart);keys().forEach(bindChart);
  });
}
function toggleRun(k){S.dev[k].run=!S.dev[k].run;updateRunUI(k,S.dev[k]);send('RUN',{id:k,run:S.dev[k].run});}
function globalCtrl(on){
  keys().forEach(function(k){S.dev[k].run=!!on;updateRunUI(k,S.dev[k]);});
  send('GLOBAL',{on:on});
}

/* ================= 图表 ================= */
function initChart(k){
  var cv=document.getElementById('cv-'+k);if(!cv)return;
  var dpr=window.devicePixelRatio||1,r=cv.getBoundingClientRect();
  if(r.width===0)return;
  cv.width=r.width*dpr;cv.height=r.height*dpr;
  charts[k]={cv:cv,ctx:cv.getContext('2d'),w:r.width,h:r.height,d:S.dev[k].hist};
  drawChart(k);
}
function drawChart(k,hi){
  var c=charts[k];if(!c)return;hi=hi===undefined?-1:hi;
  var ctx=c.ctx,w=c.w,h=c.h,d=c.d;
  function css(v){return getComputedStyle(document.body).getPropertyValue(v).trim();}
  var dpr=window.devicePixelRatio||1;
  ctx.setTransform(dpr,0,0,dpr,0,0);
  ctx.clearRect(0,0,w,h);
  var PL=36,PR=10,PT=8,PB=20,cw=w-PL-PR,ch=h-PT-PB;
  var n=d.t.length;if(n<2)return;
  var mx=50,i;
  for(i=0;i<n;i++)if(d.t[i]>mx)mx=d.t[i];
  for(i=0;i<n;i++)if(d.p[i]>mx)mx=d.p[i];
  if(d.h)for(i=0;i<n;i++)if(d.h[i]>mx)mx=d.h[i];
  mx=Math.ceil(mx*1.15/20)*20;
  function X(j){return PL+j/(n-1)*cw;}
  function Y(v){return PT+ch-v/mx*ch;}
  ctx.font='10px sans-serif';ctx.textAlign='right';
  for(var g=0;g<=4;g++){
    var gv=mx/4*g,gy=Y(gv);
    ctx.strokeStyle=css('--grid');ctx.beginPath();ctx.moveTo(PL,gy);ctx.lineTo(w-PR,gy);ctx.stroke();
    ctx.fillStyle=css('--sub');ctx.fillText(gv+'°',PL-4,gy+3);
  }
  ctx.textAlign='center';
  var steps=Math.min(5,n);
  for(var s2=0;s2<steps;s2++){
    var xi=Math.round(s2/(steps-1)*(n-1));
    ctx.fillStyle=css('--sub');ctx.fillText(d.x[xi].slice(-5),X(xi),h-5);
  }
  ctx.strokeStyle=css('--grid');ctx.beginPath();ctx.moveTo(PL,PT);ctx.lineTo(PL,PT+ch);ctx.lineTo(w-PR,PT+ch);ctx.stroke();
  function line(a,col){
    ctx.beginPath();ctx.strokeStyle=col;ctx.lineWidth=2;ctx.lineJoin='round';
    for(var j=0;j<n;j++){if(j)ctx.lineTo(X(j),Y(a[j]));else ctx.moveTo(X(j),Y(a[j]));}
    ctx.stroke();
  }
  line(d.p,css('--orange'));line(d.t,css('--primary'));if(d.h)line(d.h,css('--humi'));
  if(hi>=0){
    var hx=X(hi);
    ctx.setLineDash([4,4]);ctx.strokeStyle='rgba(128,128,128,.6)';
    ctx.beginPath();ctx.moveTo(hx,PT);ctx.lineTo(hx,PT+ch);ctx.stroke();ctx.setLineDash([]);
    [d.t,d.p].forEach(function(a,ai){
      ctx.beginPath();ctx.fillStyle=ai?css('--orange'):css('--primary');
      ctx.arc(hx,Y(a[hi]),3.5,0,7);ctx.fill();});
    if(d.h){ctx.beginPath();ctx.fillStyle=css('--humi');ctx.arc(hx,Y(d.h[hi]),3.5,0,7);ctx.fill();}
  }
  c.p)=====";
static const char WEBPAGE_12[] PROGMEM = R"=====(l=PL;c.cw=cw;c.n=n;
}
function chartHover(k,clientX){
  var c=charts[k];if(!c||!c.cw)return;
  var r=c.cv.getBoundingClientRect(),x=clientX-r.left;
  var i2=Math.max(0,Math.min(c.n-1,Math.round((x-c.pl)/c.cw*(c.n-1))));
  drawChart(k,i2);
  var tip=document.getElementById('tip-'+k);if(!tip)return;
  tip.style.left=(c.pl+i2/(c.n-1)*c.cw)+'px';tip.style.top='8px';tip.style.opacity=1;
  var hum=(c.d.h&&c.d.h[i2]!=null)?' · <span style="color:#35D0CE">湿度 '+c.d.h[i2].toFixed(0)+'%</span>':'';
  tip.innerHTML=c.d.x[i2]+'<br><span style="color:#6EB4FF">箱内 '+c.d.t[i2].toFixed(1)+'°C</span>'+hum+' · <span style="color:#FFB340">PTC '+c.d.p[i2].toFixed(1)+'°C</span>';
}
function bindChart(k){
  var cv=document.getElementById('cv-'+k);if(!cv)return;
  function move(e){
    var pt=e.touches?e.touches[0]:e;
    chartHover(k,pt.clientX);
    if(e.touches)e.preventDefault();
  }
  cv.onmousemove=move;cv.ontouchstart=move;cv.ontouchmove=move;
  function hide(){drawChart(k);var t=document.getElementById('tip-'+k);if(t)t.style.opacity=0;}
  cv.onmouseleave=hide;cv.ontouchend=hide;
}

/* ================= 历史曲线已由主机 CURVE/CURVE_PART 推送（见 curveStart/mergeCurve） ================= */

function toggleTheme(){
  S.dark=!S.dark;
  document.body.setAttribute('data-theme',S.dark?'dark':'light');
}
function toast(msg){
  var t=document.getElementById('toast');
  t.textContent=msg;t.classList.add('show');
  clearTimeout(t._tm);
  t._tm=setTimeout(function(){t.classList.remove('show');},2500);
}

/* ================= 初始化 ================= */
function init(){
  try{
    document.getElementById('fName').addEventListener('input',function(){
      var v=this.value.toUpperCase().replace(/[^A-Z ]/g,'');
      if(v!==this.value)this.value=v;
    });
    updatePresetName();
    buildStage();bindEdges();
    requestAnimationFrame(function(){
      keys().forEach(initChart);keys().forEach(bindChart);
    });
    if(location.protocol==='http:'||location.protocol==='https:')wsConnect();
  }catch(err){
    var b=document.getElementById('errbar');
    b.style.display='block';b.textContent='初始化失败: '+err.message;
  }
}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',init);
else init();
window.addEventListener('resize',function(){
  charts={};
  keys().forEach(initChart);keys().forEach(bindChart);
});
</script>
</body>
</html>
)=====";

/* chunk table: no null separator, send per WEBPAGE_LEN */
static const char * const WEBPAGE_CHUNKS[] = {
WEBPAGE_0, WEBPAGE_1, WEBPAGE_2, WEBPAGE_3, WEBPAGE_4, WEBPAGE_5, WEBPAGE_6, WEBPAGE_7, WEBPAGE_8, WEBPAGE_9, WEBPAGE_10, WEBPAGE_11, WEBPAGE_12
};
static const uint16_t WEBPAGE_LEN[] = {
3800, 3800, 3800, 3800, 3800, 3800, 3800, 3800, 3800, 3800, 3800, 3800, 2469
};
static const uint8_t WEBPAGE_NCHUNKS = 13;

#endif /* WEB_PAGE_H */
