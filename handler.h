#pragma once
/*
  handler.h
  Halaman HTML yang disimpan di dalam firmware.
  PAGE_CONFIG : halaman konfigurasi (/)
  PAGE_VIEW   : halaman video (/view)
*/

// ===================== Halaman konfigurasi =====================
const char PAGE_CONFIG[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32-CAM Konfigurasi</title>
<style>
  :root { --bg:#0f172a; --card:#1e293b; --accent:#06b6d4; --text:#f8fafc; }
  * { box-sizing:border-box; margin:0; padding:0; font-family:'Segoe UI',Tahoma,sans-serif; }
  body { background:var(--bg); color:var(--text); min-height:100vh; display:flex; align-items:center; justify-content:center; padding:12px; }
  .card { background:var(--card); border:1px solid #334155; border-radius:12px; padding:18px 22px; width:100%; max-width:720px; }
  h1 { font-size:18px; margin-bottom:14px; }
  .grid { display:grid; grid-template-columns:1fr 1fr; gap:14px 22px; }
  .row { display:flex; flex-direction:column; gap:6px; font-size:13px; }
  .row label { display:flex; justify-content:space-between; }
  select, input[type=range] { width:100%; }
  select { background:#0f172a; color:var(--text); border:1px solid #475569; border-radius:6px; padding:8px; font-size:14px; }
  input[type=range] { accent-color:var(--accent); }
  .btn { display:block; margin-top:18px; text-align:center; text-decoration:none; background:var(--accent); color:#fff; font-weight:bold; padding:12px; border-radius:8px; }
  .hint { font-size:11px; color:#94a3b8; }
  @media (max-width:520px) { .grid { grid-template-columns:1fr; } }
</style>
</head>
<body>
<div class="card">
  <h1>Konfigurasi Kamera</h1>
  <div class="grid">
    <div class="row">
      <label for="framesize">Resolusi</label>
      <select id="framesize">
        <option value="4">QVGA (320x240)</option>
        <option value="5">CIF (400x296)</option>
        <option value="6">VGA (640x480)</option>
        <option value="7">SVGA (800x600)</option>
        <option value="8">XGA (1024x768)</option>
      </select>
      <span class="hint">Resolusi lebih kecil, latensi lebih rendah</span>
    </div>
    <div class="row">
      <label for="quality">Kualitas JPEG <span id="v-quality">63</span></label>
      <input type="range" id="quality" min="10" max="63" step="1" value="63">
      <span class="hint">10 terbaik, 63 terendah (lebih ringan)</span>
    </div>
    <div class="row">
      <label for="brightness">Brightness <span id="v-brightness">0</span></label>
      <input type="range" id="brightness" min="-2" max="2" step="1" value="0">
    </div>
    <div class="row">
      <label for="contrast">Contrast <span id="v-contrast">0</span></label>
      <input type="range" id="contrast" min="-2" max="2" step="1" value="0">
    </div>
  </div>
  <a class="btn" href="/view">Mulai</a>
</div>

<script>
  const ids = ['framesize','quality','brightness','contrast'];

  function send(name, value) {
    const lbl = document.getElementById('v-' + name);
    if (lbl) lbl.textContent = value;
    fetch('/control?var=' + name + '&val=' + value).catch(() => {});
  }

  ids.forEach(id => {
    const el = document.getElementById(id);
    el.addEventListener('change', () => send(id, el.value));
    if (el.type === 'range') {
      el.addEventListener('input', () => {
        document.getElementById('v-' + id).textContent = el.value;
      });
    }
  });

  fetch('/status').then(r => r.json()).then(s => {
    ids.forEach(id => {
      const el = document.getElementById(id);
      el.value = s[id];
      const lbl = document.getElementById('v-' + id);
      if (lbl) lbl.textContent = s[id];
    });
  }).catch(() => {});
</script>
</body>
</html>
)rawliteral";

// ===================== Halaman video =====================
const char PAGE_VIEW[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, viewport-fit=cover">
<title>ESP32-CAM Live</title>
<style>
  * { box-sizing:border-box; margin:0; padding:0; font-family:'Segoe UI',Tahoma,sans-serif; }
  html, body { height:100%; background:#000; overflow:hidden; }
  body { display:flex; align-items:center; justify-content:center; }
  .frame {
    position:relative;
    width:min(100vw, calc(100dvh * 4 / 3));
    aspect-ratio:4 / 3;
    background:#000;
    overflow:hidden;
  }
  .frame img { width:100%; height:100%; object-fit:contain; display:block; }

  .tag {
    position:absolute; z-index:10;
    background:rgba(15,23,42,0.6);
    color:#f8fafc;
    font-size:10px; font-weight:600; letter-spacing:0.4px;
    padding:3px 7px; border-radius:4px;
    text-decoration:none;
  }

  .back { left:8px; top:8px; }
  .rssi { right:8px; top:8px; }
  .lat  { right:8px; bottom:8px; color:#4ade80; }

  .res { left:8px; bottom:8px; display:flex; align-items:center; gap:5px; }
  .res select {
    background:#0f172a; color:#f8fafc;
    border:1px solid #475569; border-radius:4px;
    font-size:10px; padding:2px 4px; outline:none;
  }

  .vslider {
    top:50%; transform:translateY(-50%);
    display:flex; flex-direction:column; align-items:center; gap:4px;
    padding:6px 5px;
  }
  .vslider input[type=range] {
    writing-mode:vertical-lr;
    direction:rtl;
    width:14px; height:70px;
    accent-color:#06b6d4;
    cursor:pointer;
  }
  .bright { left:8px; }
  .contr  { right:8px; }
</style>
</head>
<body>
  <div class="frame">
    <img id="view" alt="">

    <a class="tag back" href="/">&#9881; Config</a>
    <div class="tag rssi">RSSI: <span id="rssi">-</span> dBm</div>

    <div class="tag vslider bright">
      <span>&#9728;</span>
      <input type="range" id="brightness" min="-2" max="2" step="1" value="0">
      <span id="v-brightness">0</span>
    </div>

    <div class="tag vslider contr">
      <span>&#9680;</span>
      <input type="range" id="contrast" min="-2" max="2" step="1" value="0">
      <span id="v-contrast">0</span>
    </div>

    <div class="tag res">
      <label for="framesize">RES</label>
      <select id="framesize">
        <option value="4">QVGA 320x240</option>
        <option value="5">CIF 400x296</option>
        <option value="6">VGA 640x480</option>
        <option value="7">SVGA 800x600</option>
        <option value="8">XGA 1024x768</option>
      </select>
    </div>

    <div class="tag lat">LATENCY: <span id="lat">-</span> ms</div>
  </div>

<script>
  const FETCH_TIMEOUT_MS = 3000;
  const img = document.getElementById('view');
  const latEl = document.getElementById('lat');
  const rssiEl = document.getElementById('rssi');
  const capUrl = 'http://' + location.hostname + ':81/capture';
  let fps = 10;
  let prevUrl = null;

  const sleep = ms => new Promise(r => setTimeout(r, ms));

  function show(blob) {
    const u = URL.createObjectURL(blob);
    img.onload = () => {
      if (prevUrl) URL.revokeObjectURL(prevUrl);
      prevUrl = u;
    };
    img.src = u;
  }

  // Model tarik: minta satu foto, tampilkan, tunggu sisa interval, ulangi.
  // Jika WiFi macet, permintaan dibatalkan dan gambar terakhir tetap tampil.
  async function loop() {
    while (true) {
      const t0 = performance.now();
      try {
        const ctrl = new AbortController();
        const timer = setTimeout(() => ctrl.abort(), FETCH_TIMEOUT_MS);
        const r = await fetch(capUrl + '?t=' + Date.now(), { signal: ctrl.signal });
        const blob = await r.blob();
        clearTimeout(timer);
        if (!r.ok) throw new Error('http ' + r.status);
        show(blob);
        latEl.textContent = Math.round(performance.now() - t0);
      } catch (e) {
        latEl.textContent = 'N/A';
        await sleep(300);
        continue;
      }
      const wait = 1000 / fps - (performance.now() - t0);
      if (wait > 0) await sleep(wait);
    }
  }

  function send(name, value) {
    fetch('/control?var=' + name + '&val=' + value).catch(() => {});
  }

  const fs = document.getElementById('framesize');
  fs.addEventListener('change', () => send('framesize', fs.value));

  ['brightness', 'contrast'].forEach(id => {
    const el = document.getElementById(id);
    const lbl = document.getElementById('v-' + id);
    el.addEventListener('input', () => { lbl.textContent = el.value; });
    el.addEventListener('change', () => send(id, el.value));
  });

  function applyRssi(v) {
    rssiEl.textContent = v || '-';
    rssiEl.parentElement.style.color =
      !v ? '#f8fafc' : v > -65 ? '#4ade80' : v > -75 ? '#facc15' : '#f87171';
  }

  function loadStatus() {
    return fetch('/status', { cache:'no-store' }).then(r => r.json()).then(s => {
      fps = s.fps || fps;
      fs.value = s.framesize;
      ['brightness', 'contrast'].forEach(id => {
        document.getElementById(id).value = s[id];
        document.getElementById('v-' + id).textContent = s[id];
      });
      applyRssi(s.rssi);
    });
  }

  loadStatus().catch(() => {}).finally(loop);

  // Sinkronkan fps dan RSSI tiap 2 detik
  setInterval(() => {
    fetch('/status', { cache:'no-store' }).then(r => r.json()).then(s => {
      fps = s.fps || fps;
      applyRssi(s.rssi);
    }).catch(() => { rssiEl.textContent = 'N/A'; });
  }, 2000);
</script>
</body>
</html>
)rawliteral";
