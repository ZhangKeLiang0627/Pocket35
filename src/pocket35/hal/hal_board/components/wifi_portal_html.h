#pragma once
#include <pgmspace.h>

const char kWifiPortalHtml[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, viewport-fit=cover">
<title>Pocket35 · Wi-Fi 配网</title>
<style>
  :root {
    --accent: #4f46e5;
    --accent-2: #2563eb;
    --ink: #0f172a;
    --muted: #64748b;
    --line: #e5eaf2;
    --field: #f7f9fc;
    --ok: #10b981;
  }
  * { margin: 0; padding: 0; box-sizing: border-box; -webkit-tap-highlight-color: transparent; }
  body {
    min-height: 100vh;
    display: flex; align-items: center; justify-content: center;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", "PingFang SC", "Microsoft YaHei", Roboto, sans-serif;
    color: rgba(67, 67, 67, 0.9);
    padding: 22px;
    background-image: linear-gradient(
      145deg,
      rgba(255, 150, 205, 0.5),
      #EF96C5,
      #EAD6EE,
      #EEBD89,
      #F9957F
    );
    background-size: 800%;
    animation: bganimation 15s infinite;
  }
  @keyframes bganimation {
    0% { background-position: 0% 50%; }
    50% { background-position: 100% 50%; }
    100% { background-position: 0% 50%; }
  }
  .card {
    width: 100%; max-width: 392px;
    background: rgba(255, 255, 255, 0.86);
    backdrop-filter: blur(14px);
    -webkit-backdrop-filter: blur(14px);
    border: 1px solid rgba(255, 255, 255, 0.7);
    border-radius: 26px;
    padding: 32px 28px 26px;
    box-shadow: 0 30px 80px -18px rgba(37, 52, 110, 0.35), 0 2px 6px rgba(37, 52, 110, 0.06);
    animation: rise 0.55s cubic-bezier(0.16, 1, 0.3, 1);
  }
  @keyframes rise { from { opacity: 0; transform: translateY(20px) scale(0.98); } to { opacity: 1; transform: none; } }
  .brand { display: flex; flex-direction: column; align-items: center; margin-bottom: 26px; }
  .logo {
    width: 60px; height: 60px; border-radius: 18px;
    background: linear-gradient(135deg, var(--accent), var(--accent-2));
    display: flex; align-items: center; justify-content: center;
    box-shadow: 0 12px 26px -6px rgba(79, 70, 229, 0.5), inset 0 1px 0 rgba(255, 255, 255, 0.25);
    position: relative;
  }
  .logo::after {
    content: ""; position: absolute; inset: 0; border-radius: 18px;
    background: radial-gradient(120% 120% at 20% 0%, rgba(255,255,255,0.45), transparent 45%);
  }
  .logo svg { width: 30px; height: 30px; position: relative; z-index: 1; }
  h1 { font-size: 22px; font-weight: 800; letter-spacing: 0.2px; margin-top: 14px; }
  .sub { font-size: 13px; color: var(--muted); margin-top: 4px; }
  .field { margin-bottom: 16px; }
  label { display: block; font-size: 12px; font-weight: 600; color: #475569; margin-bottom: 7px; letter-spacing: 0.3px; }
  .input-wrap { position: relative; }
  .input-wrap .lead {
    position: absolute; left: 14px; top: 50%; transform: translateY(-50%);
    color: #94a3b8; display: flex; pointer-events: none;
  }
  .input-wrap .lead svg { width: 17px; height: 17px; }
  input[type="text"], input[type="password"] {
    width: 100%; padding: 13px 14px 13px 40px;
    border: 1.5px solid var(--line); border-radius: 14px;
    font-size: 15px; color: var(--ink); background: var(--field);
    transition: border-color 0.2s, box-shadow 0.2s, background 0.2s;
    -webkit-appearance: none;
  }
  input:focus {
    outline: none; border-color: var(--accent); background: #fff;
    box-shadow: 0 0 0 4px rgba(79, 70, 229, 0.12);
  }
  .pw-wrap .toggle {
    position: absolute; right: 8px; top: 50%; transform: translateY(-50%);
    border: 0; background: none; cursor: pointer; color: #94a3b8;
    padding: 8px; border-radius: 10px; display: flex;
    transition: color 0.2s, background 0.2s;
  }
  .pw-wrap .toggle:hover { color: var(--accent); background: #eef2ff; }
  .pw-wrap .toggle svg { width: 18px; height: 18px; }
  .pw-wrap input { padding-right: 46px; }
  .section { display: flex; align-items: center; justify-content: space-between; margin: 20px 0 10px; }
  .section .title { font-size: 12px; font-weight: 700; color: #475569; letter-spacing: 0.4px; }
  .scan-btn {
    border: 0; background: none; cursor: pointer;
    font-size: 13px; font-weight: 600; color: var(--accent);
    display: flex; align-items: center; gap: 5px; padding: 4px 8px; border-radius: 8px;
    transition: background 0.15s;
  }
  .scan-btn:hover { background: #eef2ff; }
  .scan-btn .spin { display: none; width: 13px; height: 13px; }
  .scan-btn.busy .spin { display: block; animation: rot 0.8s linear infinite; }
  .scan-btn.busy .ico { display: none; }
  @keyframes rot { to { transform: rotate(360deg); } }
  .net-list { max-height: 150px; overflow-y: auto; margin-bottom: 20px; -webkit-overflow-scrolling: touch; }
  .net-list::-webkit-scrollbar { width: 0; }
  .net {
    display: flex; align-items: center; gap: 12px;
    padding: 11px 14px; margin-bottom: 7px;
    background: #fff; border: 1.5px solid var(--line); border-radius: 14px;
    cursor: pointer; transition: border-color 0.15s, background 0.15s, box-shadow 0.15s;
  }
  .net:hover { border-color: #c7d2fe; background: #fbfcff; }
  .net .name { font-size: 14px; font-weight: 600; color: #1e293b; flex: 1; min-width: 0; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
  .net .lock { color: #94a3b8; display: flex; flex: none; }
  .net .lock svg { width: 14px; height: 14px; }
  .net .bars { display: flex; align-items: flex-end; gap: 2px; flex: none; height: 16px; }
  .net .bars i { width: 3px; border-radius: 2px; background: #dbe3ef; display: block; }
  .net .bars i:nth-child(1) { height: 5px; }
  .net .bars i:nth-child(2) { height: 8px; }
  .net .bars i:nth-child(3) { height: 11px; }
  .net .bars i:nth-child(4) { height: 15px; }
  .net .bars i.on { background: linear-gradient(180deg, var(--accent), var(--accent-2)); }
  .net .check { width: 20px; height: 20px; border-radius: 50%; border: 1.5px solid #d3dbe8; flex: none; display: flex; align-items: center; justify-content: center; transition: all 0.15s; }
  .net .check svg { width: 11px; height: 11px; opacity: 0; }
  .net.selected { border-color: var(--accent); background: #f5f7ff; box-shadow: 0 6px 18px -8px rgba(79,70,229,0.4); }
  .net.selected .check { background: var(--accent); border-color: var(--accent); }
  .net.selected .check svg { opacity: 1; }
  .empty { text-align: center; color: #94a3b8; font-size: 13px; padding: 14px 0; }
  .submit {
    width: 100%; padding: 15px; border: 0; border-radius: 15px;
    background: linear-gradient(135deg, var(--accent), var(--accent-2));
    color: #fff; font-size: 16px; font-weight: 700; letter-spacing: 1px;
    cursor: pointer; display: flex; align-items: center; justify-content: center; gap: 8px;
    box-shadow: 0 14px 30px -8px rgba(79, 70, 229, 0.55), inset 0 1px 0 rgba(255,255,255,0.2);
    transition: transform 0.1s, box-shadow 0.2s, opacity 0.2s;
  }
  .submit:active { transform: scale(0.985); box-shadow: 0 6px 16px -8px rgba(79,70,229,0.55); }
  .submit:disabled { opacity: 0.7; cursor: default; }
  .submit .spin { display: none; width: 16px; height: 16px; }
  .submit.busy .spin { display: block; animation: rot 0.8s linear infinite; }
  @keyframes rot2 { to { transform: rotate(360deg); } }
  .foot { text-align: center; font-size: 11px; color: #a7b1c2; margin-top: 20px; letter-spacing: 0.3px; }
</style>
</head>
<body>
  <form class="card" method="POST" action="/save" autocomplete="off">
    <div class="brand">
      <div class="logo">
        <svg viewBox="0 0 24 24" fill="none" stroke="#ffffff" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
          <path d="M5 12.55a11 11 0 0 1 14.08 0"></path>
          <path d="M8.53 16.11a6 6 0 0 1 6.95 0"></path>
          <circle cx="12" cy="19.5" r="1" fill="#ffffff" stroke="none"></circle>
        </svg>
      </div>
      <h1>Pocket35</h1>
      <p class="sub">连接 Wi-Fi 以继续</p>
    </div>

    <div class="field">
      <label>Wi-Fi 名称</label>
      <div class="input-wrap">
        <span class="lead">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M5 12.55a11 11 0 0 1 14.08 0"></path><path d="M8.53 16.11a6 6 0 0 1 6.95 0"></path><path d="M12 20h.01"></path></svg>
        </span>
        <input name="ssid" id="ssid" type="text" placeholder="输入或从下方选择" required>
      </div>
    </div>

    <div class="field">
      <label>密码</label>
      <div class="input-wrap pw-wrap">
        <span class="lead">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="4" y="11" width="16" height="10" rx="2.5"></rect><path d="M8 11V7a4 4 0 0 1 8 0v4"></path></svg>
        </span>
        <input name="pass" id="pass" type="password" placeholder="输入 Wi-Fi 密码">
        <button type="button" class="toggle" id="eye" aria-label="toggle">
          <svg id="eye-open" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle></svg>
          <svg id="eye-closed" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="display:none"><path d="M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94"></path><path d="M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19"></path><path d="M14.12 14.12a3 3 0 1 1-4.24-4.24"></path><path d="M1 1l22 22"></path></svg>
        </button>
      </div>
    </div>

    <div class="section">
      <span class="title">附近网络</span>
      <button type="button" class="scan-btn" id="scan">
        <svg class="ico" viewBox="0 0 24 24" width="14" height="14" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="11" cy="11" r="8"></circle><path d="M21 21l-4.35-4.35"></path></svg>
        <svg class="spin" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round"><path d="M21 12a9 9 0 1 1-6.2-8.56"></path></svg>
        <span id="scan-label">扫描</span>
      </button>
    </div>
    <div class="net-list" id="netlist"></div>

    <button type="submit" class="submit" id="submit">
      <svg class="spin" viewBox="0 0 24 24" fill="none" stroke="#fff" stroke-width="2.5" stroke-linecap="round"><path d="M21 12a9 9 0 1 1-6.2-8.56"></path></svg>
      <span id="submit-label">连 接</span>
    </button>
    <p class="foot">Pocket35 · ESP32-S3</p>
  </form>

  <script>
    var eye = document.getElementById('eye');
    var pass = document.getElementById('pass');
    var eyeOpen = document.getElementById('eye-open');
    var eyeClosed = document.getElementById('eye-closed');
    eye.addEventListener('click', function () {
      var show = pass.type === 'password';
      pass.type = show ? 'text' : 'password';
      eyeOpen.style.display = show ? 'none' : 'block';
      eyeClosed.style.display = show ? 'block' : 'none';
    });

    var form = document.querySelector('.card');
    var submit = document.getElementById('submit');
    var submitLabel = document.getElementById('submit-label');
    form.addEventListener('submit', function () {
      submit.disabled = true;
      submit.classList.add('busy');
      submitLabel.textContent = '连接中…';
    });

    var ssid = document.getElementById('ssid');
    var netlist = document.getElementById('netlist');
    var scanBtn = document.getElementById('scan');
    var scanLabel = document.getElementById('scan-label');
    var scanning = false;

    function esc(s) {
      return String(s).replace(/[&<>"']/g, function (c) {
        return { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c];
      });
    }

    function bars(rssi) {
      var n = rssi > -50 ? 4 : rssi > -65 ? 3 : rssi > -80 ? 2 : 1;
      var html = '';
      for (var i = 0; i < 4; i++) html += '<i class="' + (i < n ? 'on' : '') + '"></i>';
      return html;
    }

    function loadNetworks() {
      if (scanning) return;
      scanning = true;
      scanBtn.classList.add('busy');
      scanLabel.textContent = '扫描中';
      fetch('/scan').then(function (r) { return r.json(); }).then(function (list) {
        netlist.innerHTML = '';
        if (!list.length) {
          netlist.innerHTML = '<div class="empty">未发现网络</div>';
        }
        list.forEach(function (n) {
          var d = document.createElement('div');
          d.className = 'net';
          var lock = n.open ? '' : '<span class="lock"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="4" y="11" width="16" height="10" rx="2.5"></rect><path d="M8 11V7a4 4 0 0 1 8 0v4"></path></svg></span>';
          var check = '<span class="check"><svg viewBox="0 0 24 24" fill="none" stroke="#fff" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"><path d="M20 6 9 17l-5-5"></path></svg></span>';
          d.innerHTML = lock + '<span class="name">' + esc(n.ssid) + '</span><span class="bars">' + bars(n.rssi) + '</span>' + check;
          d.addEventListener('click', function () {
            ssid.value = n.ssid;
            var cur = netlist.querySelector('.selected');
            if (cur) cur.classList.remove('selected');
            d.classList.add('selected');
          });
          netlist.appendChild(d);
        });
        scanLabel.textContent = '重新扫描';
        scanning = false;
        scanBtn.classList.remove('busy');
      }).catch(function () {
        netlist.innerHTML = '<div class="empty">扫描失败</div>';
        scanLabel.textContent = '扫描';
        scanning = false;
        scanBtn.classList.remove('busy');
      });
    }

    scanBtn.addEventListener('click', loadNetworks);
    loadNetworks();
  </script>
</body>
</html>
)rawliteral";
