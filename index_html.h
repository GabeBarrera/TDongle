#pragma once
// Signal Sniffer web page: Hunt / Settings / Database tabs, swipe gestures,
// results drawer, bloodhound logo. Separate header so the Arduino sketch
// preprocessor does not try to compile this raw string as code.

const char INDEX_HTML[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="en"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Signal Sniffer</title>
<style>
:root{
  --bg:#07070b;--panel:#0f0d13;--panel2:#15121a;--ink:#f3e7e7;--mute:#b08a8f;--dim:#6b5b60;
  --red:#ff2a2a;--red2:#ff5c5c;--deep:#7a0b12;--cyan:#2de2e6;--line:#3a1218;--glow:0 0 8px rgba(255,42,42,.55);}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--ink);
  font:15px/1.5 ui-monospace,SFMono-Regular,Menlo,Consolas,"Liberation Mono",monospace;
  background-image:radial-gradient(circle at 50% -8%,rgba(255,42,42,.10),transparent 55%);}
body::after{content:"";position:fixed;inset:0;pointer-events:none;z-index:9;
  background:repeating-linear-gradient(0deg,rgba(0,0,0,.16) 0 1px,transparent 1px 3px);opacity:.5}
header{display:flex;align-items:center;gap:14px;padding:16px;border-bottom:1px solid var(--line);
  background:linear-gradient(180deg,rgba(255,42,42,.09),transparent)}
header img{height:54px;image-rendering:pixelated;filter:drop-shadow(var(--glow))}
h1{margin:0;font-size:1.2rem;letter-spacing:.3em;color:var(--red);text-shadow:var(--glow);text-transform:uppercase}
.sub{color:var(--mute);font-size:.68rem;letter-spacing:.18em;margin-top:3px;text-transform:uppercase}
main{max-width:660px;margin:0 auto;padding:16px}
h3{font-size:.78rem;letter-spacing:.22em;text-transform:uppercase;color:var(--red2);margin:20px 0 8px;
  border-left:3px solid var(--red);padding-left:9px}
.tabs{display:flex;gap:6px;margin-bottom:16px}
.tabs button{flex:1;padding:11px 6px;border:1px solid var(--line);background:var(--panel);color:var(--mute);
  font:inherit;text-transform:uppercase;letter-spacing:.1em;font-size:.74rem;cursor:pointer;
  clip-path:polygon(8px 0,100% 0,100% calc(100% - 8px),calc(100% - 8px) 100%,0 100%,0 8px)}
.tabs button.active{background:var(--red);border-color:var(--red);color:#1a0406;font-weight:700;box-shadow:var(--glow)}
.view{display:none}.view.active{display:block}
.btn{margin-top:12px;width:100%;padding:13px;border:1px solid var(--red);
  background:linear-gradient(180deg,var(--red),var(--deep));color:#1a0406;font:inherit;font-weight:700;
  letter-spacing:.14em;text-transform:uppercase;cursor:pointer;box-shadow:var(--glow);
  clip-path:polygon(10px 0,100% 0,100% calc(100% - 10px),calc(100% - 10px) 100%,0 100%,0 10px)}
.btn:hover{filter:brightness(1.15)}
.btn.ghost{background:transparent;border:1px solid var(--line);color:var(--ink);box-shadow:none;font-weight:400}
.btn.ghost:hover{border-color:var(--red);color:var(--red2)}
.btn.danger{background:transparent;border:1px solid var(--deep);color:var(--red2);box-shadow:none}
.btn.danger:hover{background:#1a0406}
.btn.small{width:auto;margin-top:0;padding:8px 14px;font-size:.72rem;letter-spacing:.08em}
.huntbtns{display:flex;flex-direction:column;gap:10px;margin-bottom:8px}
input{width:100%;padding:11px;border:1px solid var(--line);background:#0a0810;color:var(--ink);font:inherit}
input:focus{outline:none;border-color:var(--red);box-shadow:var(--glow)}
label{display:block;margin:14px 0 4px;color:var(--mute);font-size:.68rem;letter-spacing:.12em;text-transform:uppercase}
.net{display:flex;align-items:center;justify-content:space-between;gap:10px;padding:10px 12px;
  background:var(--panel);border:1px solid var(--line);border-left:3px solid var(--red);margin:7px 0}
.net .nm{font-weight:700;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.net .meta{color:var(--mute);font-size:.7rem;white-space:nowrap}
.pick{cursor:pointer}.pick:hover{border-left-color:var(--cyan);background:var(--panel2)}
.lock{color:var(--deep);margin-left:6px}.bars{color:var(--red2)}
.dot{display:inline-block;width:9px;height:9px;border-radius:50%;margin-right:7px;vertical-align:middle;box-shadow:0 0 6px currentColor}
/* swipeable rows */
.srow,.dbrow{position:relative;background:var(--panel);border:1px solid var(--line);
  border-left:3px solid var(--red);margin:7px 0;touch-action:pan-y}
.rowwrap{position:relative;overflow:hidden}
.rowmain{display:flex;align-items:center;justify-content:space-between;gap:10px;padding:11px 12px;
  background:var(--panel);transition:transform .18s;will-change:transform}
.srow .info{flex:1;min-width:0;cursor:pointer}
.nm2,.ipn{font-weight:700;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.sub2,.meta2{color:var(--mute);font-size:.7rem}
.fname{color:var(--cyan);font-weight:400;font-size:.82rem;margin-left:8px}
.note2{color:var(--mute);font-size:.72rem;font-style:italic;margin-top:3px}
.ctl{display:flex;gap:4px}
.ctl button{min-width:30px;height:30px;border:1px solid var(--line);background:#0a0810;color:var(--ink);cursor:pointer;font:inherit}
.hintL,.hintR{position:absolute;top:0;bottom:0;display:flex;align-items:center;font-size:.68rem;
  letter-spacing:.12em;text-transform:uppercase;z-index:0}
.hintL{right:0;padding-right:16px;color:var(--red2)}
.hintR{left:0;padding-left:16px;color:var(--cyan)}
.rowmain{position:relative;z-index:1}
.editpanel{display:none;flex-direction:column;gap:6px;padding:0 12px 12px}
.editpanel.show{display:flex}
.port{display:inline-block;background:#1a0406;border:1px solid var(--deep);color:var(--red2);padding:1px 6px;margin:3px 3px 0 0;font-size:.7rem}
.noports{color:var(--dim);font-size:.72rem}
.status{margin:10px 0;padding:10px 12px;background:var(--panel);border:1px solid var(--line);border-left:3px solid var(--red);font-size:.8rem}
.muted{color:var(--mute)}.ok{color:#48e06a}.bad{color:var(--red2)}
.spin{display:inline-block;width:14px;height:14px;border:2px solid var(--line);border-top-color:var(--red);border-radius:50%;animation:s .7s linear infinite;vertical-align:middle;margin-right:6px}
@keyframes s{to{transform:rotate(360deg)}}
.hint{color:var(--mute);font-size:.72rem;margin-top:8px}
.rowbtns{display:none;gap:5px;margin-left:8px;flex:0 0 auto}
.rowbtns button{width:32px;height:32px;border:1px solid var(--line);background:#0a0810;cursor:pointer;display:flex;align-items:center;justify-content:center;padding:0}
.rowbtns .edit{color:var(--cyan);border-color:#15424a}
.rowbtns .del{color:var(--red2);border-color:var(--deep)}
.rowbtns svg{width:15px;height:15px}
.rowbtns button:hover{filter:brightness(1.3)}
@media (hover:hover) and (pointer:fine){.rowbtns{display:flex}}
.orient{display:flex;gap:8px;margin-top:8px}
.orient button{flex:1;padding:12px;border:1px solid var(--line);background:var(--panel);color:var(--ink);font:inherit;text-transform:uppercase;letter-spacing:.1em;font-size:.78rem;cursor:pointer;clip-path:polygon(8px 0,100% 0,100% calc(100% - 8px),calc(100% - 8px) 100%,0 100%,0 8px)}
.orient .obtn.on{background:var(--red);border-color:var(--red);color:#1a0406;font-weight:700;box-shadow:var(--glow)}
.orient .ibtn{color:var(--dim);border-color:var(--line)}
.orient .ibtn.on{color:var(--red2);border-color:var(--red);box-shadow:var(--glow);text-shadow:0 0 6px rgba(255,92,92,.7)}
.collap{cursor:pointer;user-select:none;display:flex;justify-content:space-between;align-items:center;gap:8px}
.caret{color:var(--red2);transition:transform .15s;font-size:.8rem}
.collap.closed .caret{transform:rotate(-90deg)}
.collapsed{display:none}
.netgroup{margin-bottom:4px}
.nethdr{font-size:.74rem;letter-spacing:.16em;text-transform:uppercase;color:var(--cyan);border-left:3px solid var(--cyan);margin:10px 0 6px;padding-left:9px}
.pchip{cursor:pointer}
.report{margin-top:8px;border-left:2px solid var(--line);padding-left:10px}
.report b{color:var(--ink)}
#dbSearch{margin-bottom:10px}
#addBox{display:none;margin-top:10px}
/* results drawer */
.sheetbg{position:fixed;inset:0;background:rgba(0,0,0,.65);opacity:0;pointer-events:none;transition:opacity .2s;z-index:20}
.sheetbg.open{opacity:1;pointer-events:auto}
.sheet{position:fixed;left:0;right:0;bottom:0;max-height:76vh;overflow:auto;z-index:21;
  background:var(--panel);border-top:2px solid var(--red);box-shadow:0 -10px 34px rgba(255,42,42,.22);
  transform:translateY(102%);transition:transform .25s;padding:0 14px 22px}
.sheet.open{transform:translateY(0)}
.sheethead{position:sticky;top:0;display:flex;justify-content:space-between;align-items:center;
  padding:14px 0 12px;background:var(--panel);border-bottom:1px solid var(--line);margin-bottom:10px}
.sheethead span{color:var(--red);letter-spacing:.2em;text-transform:uppercase;font-weight:700}
.sheethead button{background:none;border:0;color:var(--red2);font-size:1.5rem;cursor:pointer;line-height:1}
</style></head>
<body>
<header>
  <img src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAJIAAACYCAYAAAAP4LDoAAAC00lEQVR42u3dgZHDIAxEUeXm2kn/haSgXAUeGx8YCd4vwLE9m0WLAEcAQBZeqzzI9/3+pnihn89rRyH9+C+BkEBIICSAkLBZasuSwqQ8jgRCAiGBkABCwt6pbZd0tnqa40ggJBASCAkgJKyS2rKntKtp6ennyJ7iOBK68OsVmEfiSCAkEBIwL7W1ppyjGiH7/rVez9l63dk1FUeC1BYJVy30cqpqqyI4EggJhASprVaP7W6a6X2/o1Oo1AapbYde1yqrFTgSFNsgJGBuahudQu5ev1ct0fv3R/XmOBKktkqpprVXNjvNnd3X3efhSDC0gZAAM9stqexqrTR7Zvrofp+u4TgSOJKVmWokKLYBQkIk67WNHqtn9dquXr81ba3SU+NIICQQEggJICSsmtp6rQuqtg9s9ky09UgIvTZE1VNTOBI4UsUu+irfouVI4EhPpJNZaWlWD5IjgSOFE/h9CZMjgZBASNi0Rtpt7M9SE80+PaX1fjgSpLbwxUs1EjjS0qe4ruZA1iPB0AZCAggJhITVd5E8NTPae8+99MWREOaRotpMdC+nVCMBlRzpbH/V02uuq+0CuXraLkfCGo5UvRY4u/+7/+TRZxBk3Tnb+rscCYY2EBKktthqRaQ9/hwJHCnSzxfN+pLkqF7lf5+DI4EjOQVFjQRCAggJaqRcae9ovilbb86+NhjaQEiAGikG9Nau1kLZVzByJHAkKS/KfO3prHfoxDYY2kBIUCNJcd4CR8KujjTaAVp3+D59xsCs87R9QRKGNhASQEggJESxMySrzJ+0po2q80LVVhFwJBASCAmEBBASqp/8nz3NZFmpWPXMSY4EQgIhIaxHgpWaHAmbOVKvntrV6zi9liOBkEBIgNS2z1eNOBLWcqTsp2jM6sFVcRJnSMLQBkICpDa1CkcCR+Ik4EggJBASCAkgJMzb14awioAjgZBASCAkgJAAAIj4A/8fbu9CEN2jAAAAAElFTkSuQmCC" alt="bloodhound">
  <div><h1>SIGNAL SNIFFER</h1><div class="sub">T-Dongle-S3 &middot; AP: BL00DH0UND</div></div>
</header>
<main>
  <div class="status" id="status"><span class="muted">Loading status...</span></div>
  <div class="tabs">
    <button id="tHunt" class="active" onclick="show('hunt')">Hunt</button>
    <button id="tSet" onclick="show('settings')">Settings</button>
    <button id="tDb" onclick="show('database')">Database</button>
  </div>

  <section id="hunt" class="view active">
    <p class="hint">Run a tool. Results open in a panel and are saved to the Database. Authorized networks only.</p>
    <div class="huntbtns">
      <button class="btn" onclick="wapSniffer()">WAP Sniffer</button>
      <button class="btn" onclick="arpSweeper()">ARP Sweeper</button>
      <button class="btn" onclick="portScanner()">Port Scanner</button>
    </div>
    <label for="hunttarget">Port Scanner target IP</label>
    <input id="hunttarget" list="iplist" placeholder="e.g. 192.168.1.1 (type or pick a known host)">
    <datalist id="iplist"></datalist>
  </section>

  <section id="settings" class="view">
    <h3>Saved networks</h3>
    <p class="hint">Tap a row to join with its stored password. Swipe left to delete. Arrows set auto-join priority (top = first).</p>
    <div id="joinMsg" class="hint"></div>
    <div id="savedList"><p class="hint">Loading...</p></div>
    <h3>Screen orientation</h3>
    <p class="hint">Landscape toggles to portrait (USB end up, reads left-to-right). Invert flips 180&deg;; it glows red when on.</p>
    <div class="orient">
      <button class="obtn" id="btnOrient" onclick="toggleOrient()">Landscape</button>
      <button class="ibtn" id="btnInvert" onclick="toggleInvert()">Invert</button>
    </div>
    <h3 class="collap closed" onclick="toggleSec(this,'secPcfg')">Port Scanner config<span class="caret">&#9656;</span></h3>
    <div id="secPcfg" class="section collapsed">
      <p class="hint">Ports the Port Scanner checks. Tap a port to remove it.</p>
      <div id="portChips"></div>
      <label for="addPorts">Add ports (comma or space separated)</label>
      <input id="addPorts" placeholder="e.g. 8000 8443 9090">
      <div class="orient" style="margin-top:8px">
        <button class="btn small" onclick="addPortsCfg()">Add</button>
        <button class="btn ghost small" onclick="resetPortsCfg()">Reset default</button>
        <button class="btn danger small" onclick="clearPortsCfg()">Clear all</button>
      </div>
    </div>
    <h3>Saved networks actions</h3>
    <button class="btn ghost" onclick="toggleAdd()">+ Add a new network</button>
    <div id="addBox">
      <button class="btn ghost" onclick="loadFull()">Show nearby networks</button>
      <div id="fullList"></div>
      <label for="ssid">Network (SSID)</label>
      <input id="ssid" placeholder="choose above or type">
      <label for="pass">Password</label>
      <input id="pass" type="password" placeholder="network password">
      <button class="btn" onclick="connect()">Connect &amp; save</button>
      <div id="setMsg" class="hint"></div>
    </div>
  </section>

  <section id="database" class="view">
    <p class="hint">Everything the tools found. Swipe left to delete, swipe right (or desktop buttons) to edit. Tap a header to collapse.</p>
    <input id="dbSearch" oninput="filterDb()" placeholder="filter ip / ssid / mac / name / notes">
    <h3 class="collap" onclick="toggleSec(this,'secWap')">Access points<span class="caret">&#9662;</span></h3>
    <div id="secWap" class="section">
      <div id="wapDb"></div>
      <button class="btn danger small" onclick="clearDb('wap')">Clear access points</button>
    </div>
    <h3 class="collap" onclick="toggleSec(this,'secHost')">Hosts<span class="caret">&#9662;</span></h3>
    <div id="secHost" class="section">
      <div id="hostDb"></div>
      <button class="btn danger small" onclick="clearDb('host')">Clear hosts</button>
    </div>
  </section>
</main>

<div class="sheetbg" id="sheetbg" onclick="closeSheet()"></div>
<div class="sheet" id="sheet">
  <div class="sheethead"><span id="sheetTitle">Results</span><button onclick="closeSheet()">&times;</button></div>
  <div id="sheetBody"></div>
</div>
<script>
function esc(s){return (s||'').replace(/[&<>"]/g,function(c){return{'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c];});}
var SVG_EDIT='<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 20h4L18.5 9.5l-4-4L4 16z"/><path d="M13.5 6.5l4 4"/></svg>';
var SVG_DEL='<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 7h16"/><path d="M9 7V5h6v2"/><path d="M6 7l1 13h10l1-13"/></svg>';
var ROWBTN='<div class="rowbtns"><button class="noswipe edit" title="Edit" onclick="editRow(this)">'+SVG_EDIT+'</button><button class="noswipe del" title="Delete" onclick="delRow(this)">'+SVG_DEL+'</button></div>';
function editRow(b){b.closest('.dbrow').querySelector('.editpanel').classList.toggle('show');}
function delRow(b){var r=b.closest('.dbrow');delDb(r.dataset.kind,r.dataset.k1,r.dataset.k2);}
function populateIpList(){
  fetch('/hosts').then(r=>r.json()).then(list=>{
    var ips={};(window.lastArpIps||[]).forEach(function(ip){ips[ip]=1;});
    list.forEach(function(h){ips[h.ip]=1;});
    var dl=document.getElementById('iplist');
    if(dl)dl.innerHTML=Object.keys(ips).sort().map(function(ip){return '<option value="'+ip+'">';}).join('');
  }).catch(function(){});
}
function bars(r){var n=r>-60?4:r>-70?3:r>-80?2:1;var f='';for(var i=0;i<4;i++)f+=i<n?'|':'.';return f;}
function show(v){var m={hunt:'tHunt',settings:'tSet',database:'tDb'};
  for(var id in m){document.getElementById(id).classList.toggle('active',id==v);
    document.getElementById(m[id]).classList.toggle('active',id==v);}
  if(v=='settings'){loadSaved();loadPortsCfg();} if(v=='database')loadDb(); if(v=='hunt')populateIpList();}
var curPortrait=false,curInvert=false;
function applyOrientBtns(){
  var o=document.getElementById('btnOrient'),i=document.getElementById('btnInvert');
  if(o){o.textContent=curPortrait?'Portrait mode':'Landscape';o.classList.toggle('on',curPortrait);}
  if(i){i.classList.toggle('on',curInvert);}
}
function sendOrient(){
  fetch('/orient',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'portrait='+(curPortrait?1:0)+'&invert='+(curInvert?1:0)}).then(r=>r.json()).then(d=>{curPortrait=!!d.portrait;curInvert=!!d.invert;applyOrientBtns();}).catch(function(){});
}
function toggleOrient(){curPortrait=!curPortrait;applyOrientBtns();sendOrient();}
function toggleInvert(){curInvert=!curInvert;applyOrientBtns();sendOrient();}
function refreshStatus(){
  fetch('/status').then(r=>r.json()).then(d=>{var ok=d.connected;
    document.getElementById('status').innerHTML='<span class="dot" style="color:'+(ok?'#48e06a':'var(--mute)')+'"></span>'+
      (ok?'ONLINE // <b>'+esc(d.ssid)+'</b> // '+d.ip:'AP ONLY')+
      '<br><span class="muted">AP '+esc(d.ap)+' // 192.168.4.1 // '+d.clients+' client(s) // store:'+d.storage+'</span>';
    if(typeof d.portrait!=='undefined'){curPortrait=!!d.portrait;curInvert=!!d.invert;applyOrientBtns();}
  }).catch(()=>{});
}
/* ---------- results drawer ---------- */
function openSheet(title,html){document.getElementById('sheetTitle').textContent=title;
  document.getElementById('sheetBody').innerHTML=html;
  document.getElementById('sheet').classList.add('open');document.getElementById('sheetbg').classList.add('open');}
function setSheet(html){document.getElementById('sheetBody').innerHTML=html;}
function closeSheet(){document.getElementById('sheet').classList.remove('open');document.getElementById('sheetbg').classList.remove('open');}
function toDb(){closeSheet();show('database');}
var dbLink='<button class="btn ghost small" style="margin-top:14px" onclick="toDb()">View in Database &rsaquo;</button>';
/* ---------- Hunt tools ---------- */
function wapSniffer(){
  openSheet('WAP Sniffer','<p><span class="spin"></span>Scanning Wi-Fi... LED blue</p>');
  fetch('/scan').then(r=>r.json()).then(list=>{
    if(!list.length){setSheet('<p class="hint">No networks found.</p>'+dbLink);return;}
    setSheet('<p class="hint">'+list.length+' AP(s) found &middot; saved to Database.</p>'+
      list.map(n=>'<div class="net"><div style="min-width:0"><div class="nm">'+esc(n.ssid||'(hidden)')+
      '</div><div class="meta">ch '+n.ch+' // '+n.enc+'</div></div><div class="meta"><span class="bars">'+
      bars(n.rssi)+'</span> '+n.rssi+'</div></div>').join('')+dbLink);
  }).catch(()=>setSheet('<p class="bad">Scan failed.</p>'));
}
function arpSweeper(){
  openSheet('ARP Sweeper','<p><span class="spin"></span>Sweeping subnet... bloodhound filling</p>');
  fetch('/arp').then(r=>r.json()).then(d=>{
    if(d.error){setSheet('<p class="bad">'+esc(d.error)+'</p>');return;}
    var h=d.hosts||[];
    if(!h.length){setSheet('<p class="hint">No hosts answered.</p>'+dbLink);return;}
    window.lastArpIps=h.map(function(x){return x.ip;});populateIpList();
    setSheet('<p class="hint">'+h.length+' host(s) on '+esc(d.net||d.self)+' &middot; saved to Database.</p>'+
      h.map(x=>'<div class="net pick" onclick="setTarget(\''+x.ip+'\')"><div class="nm">'+x.ip+
      '</div><div class="meta">'+x.mac+'</div></div>').join('')+dbLink);
  }).catch(()=>setSheet('<p class="bad">Sweep failed.</p>'));
}
function setTarget(ip){document.getElementById('hunttarget').value=ip;closeSheet();}
function portScanner(){
  var ip=document.getElementById('hunttarget').value.trim();
  if(!ip){alert('Enter a target IP for the port scanner.');return;}
  openSheet('Port Scanner','<p><span class="spin"></span>Scanning '+esc(ip)+'...</p>');
  fetch('/portscan?ip='+encodeURIComponent(ip)).then(r=>r.json()).then(d=>{
    if(d.error){setSheet('<p class="bad">'+esc(d.error)+'</p>');return;}
    var openC=d.open.length, scanned=d.scanned||0, allp=(d.ports||[]);
    var oset={}; d.open.forEach(function(p){oset[p]=1;});
    var closed=allp.filter(function(p){return !oset[p];});
    var body='<p class="hint">'+esc(ip)+' &middot; '+openC+' open / '+scanned+' scanned &middot; saved to Database.</p>';
    body+= openC? ('<div>'+d.open.map(p=>'<span class="port">'+p+'</span>').join('')+'</div>')
                : '<p class="muted">No open ports.</p>';
    var report='<button class="btn ghost small closed" style="margin-top:14px" onclick="toggleReport(this)">Full report<span class="caret">&#9656;</span></button>'+
      '<div class="report collapsed">'+
        '<div class="meta2">Target '+esc(ip)+'</div>'+
        '<div class="meta2">Scanned '+scanned+' &middot; '+openC+' open &middot; '+closed.length+' closed/filtered</div>'+
        '<div style="margin-top:6px"><b>Open</b><br>'+(openC?d.open.map(p=>'<span class="port">'+p+'</span>').join(''):'<span class="noports">none</span>')+'</div>'+
        '<div style="margin-top:6px"><b>Closed / filtered</b><br>'+(closed.length?closed.map(p=>'<span class="port" style="color:var(--dim);border-color:var(--line)">'+p+'</span>').join(''):'<span class="noports">none</span>')+'</div>'+
      '</div>';
    setSheet(body+report+dbLink);
  }).catch(()=>setSheet('<p class="bad">Scan failed.</p>'));
}
/* ---------- swipe helper ---------- */
function attachSwipe(row,h){
  var main=row.querySelector('.rowmain')||row;
  var x0,y0,dx,dy,active=false,moved=false;
  row.addEventListener('pointerdown',function(e){
    if(e.target.closest('.noswipe')||e.target.closest('.editpanel'))return;
    x0=e.clientX;y0=e.clientY;dx=0;dy=0;active=true;moved=false;main.style.transition='none';
  });
  row.addEventListener('pointermove',function(e){
    if(!active)return;dx=e.clientX-x0;dy=e.clientY-y0;
    if(Math.abs(dx)>8)moved=true;
    if(Math.abs(dx)>Math.abs(dy))main.style.transform='translateX('+dx+'px)';
  });
  function end(){ if(!active)return;active=false;main.style.transition='transform .18s';main.style.transform='';
    if(Math.abs(dx)>60&&Math.abs(dx)>Math.abs(dy)){ if(dx<0&&h.onLeft)h.onLeft(); else if(dx>0&&h.onRight)h.onRight(); }
    else if(!moved&&h.onTap)h.onTap(); }
  row.addEventListener('pointerup',end);
  row.addEventListener('pointercancel',function(){active=false;main.style.transition='transform .18s';main.style.transform='';});
}
/* ---------- Settings: saved networks ---------- */
function loadSaved(){
  var el=document.getElementById('savedList');
  el.innerHTML='<p><span class="spin"></span>Checking saved networks...</p>';
  fetch('/saved').then(r=>r.json()).then(d=>{
    var nets=d.nets||[];
    if(!nets.length){el.innerHTML='<p class="hint">No saved networks yet. Add one below.</p>';return;}
    el.innerHTML=nets.map((n,i)=>savedRow(n,i)).join('');
    [...el.querySelectorAll('.srow')].forEach(function(row){
      var ssid=row.dataset.ssid;
      attachSwipe(row,{onTap:()=>joinNet(ssid),onLeft:()=>delNet(ssid)});
    });
  }).catch(()=>{el.innerHTML='<p class="bad">Could not load saved list.</p>';});
}
function savedRow(n,i){
  var col=n.available?'#48e06a':'var(--mute)';
  var avail=n.available?('in range // '+n.rssi+' dBm'):'not in range';
  return '<div class="srow" data-idx="'+i+'" data-ssid="'+esc(n.ssid)+'">'+
    '<div class="rowwrap"><span class="hintL">delete &raquo;</span>'+
    '<div class="rowmain"><div class="info"><span class="dot" style="color:'+col+'"></span>'+
    '<span class="nm2">'+esc(n.ssid)+'</span><div class="sub2">'+avail+' &middot; tap to join</div></div>'+
    '<div class="ctl"><button class="noswipe" onclick="move(this,-1)">&#9650;</button>'+
    '<button class="noswipe" onclick="move(this,1)">&#9660;</button></div></div></div></div>';
}
function joinNet(ssid){
  var m=document.getElementById('joinMsg');
  m.innerHTML='<span class="spin"></span>Joining '+esc(ssid)+'...';
  fetch('/joinsaved?ssid='+encodeURIComponent(ssid)).then(r=>r.json()).then(d=>{
    m.innerHTML=d.connected?'<span class="ok">Joined '+esc(ssid)+' // '+d.ip+'</span>':'<span class="bad">Could not join '+esc(ssid)+'.</span>';
    refreshStatus();loadSaved();
  }).catch(()=>{m.innerHTML='<span class="bad">Join failed.</span>';});
}
function delNet(ssid){
  if(!confirm('Delete saved network "'+ssid+'"?'))return;
  fetch('/forget',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'ssid='+encodeURIComponent(ssid)}).then(()=>{loadSaved();refreshStatus();});
}
function commitOrder(){
  var ids=[...document.querySelectorAll('#savedList .srow')].map(r=>r.dataset.idx);
  fetch('/reorder',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'order='+ids.join(',')}).then(()=>loadSaved());
}
function move(btn,dir){
  var row=btn.closest('.srow'),list=document.getElementById('savedList');
  if(dir<0&&row.previousElementSibling)list.insertBefore(row,row.previousElementSibling);
  if(dir>0&&row.nextElementSibling)list.insertBefore(row.nextElementSibling,row);
  commitOrder();
}
function toggleAdd(){var b=document.getElementById('addBox');b.style.display=b.style.display=='block'?'none':'block';}
function loadFull(){
  var el=document.getElementById('fullList');el.innerHTML='<p><span class="spin"></span>Scanning...</p>';
  fetch('/scan').then(r=>r.json()).then(list=>{
    if(!list.length){el.innerHTML='<p class="hint">No networks found.</p>';return;}
    el.innerHTML=list.map(n=>'<div class="net pick" onclick="pick(\''+(n.ssid||'').replace(/\\/g,'').replace(/'/g,"\\'")+'\')"><div class="nm">'+
      esc(n.ssid||'(hidden)')+(n.enc!='open'?'<span class="lock">#</span>':'')+'</div>'+
      '<div class="meta"><span class="bars">'+bars(n.rssi)+'</span> '+n.rssi+'</div></div>').join('');
  }).catch(()=>{el.innerHTML='<p class="bad">Scan failed.</p>';});
}
function pick(s){document.getElementById('ssid').value=s;document.getElementById('pass').focus();}
function connect(){
  var ssid=document.getElementById('ssid').value.trim(),pass=document.getElementById('pass').value,m=document.getElementById('setMsg');
  if(!ssid){m.innerHTML='<span class="bad">Enter a network name.</span>';return;}
  m.innerHTML='<span class="spin"></span>Connecting to '+esc(ssid)+'...';
  fetch('/connect',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass)})
    .then(r=>r.json()).then(d=>{
      if(d.connected){m.innerHTML='<span class="ok">Connected // '+d.ip+' // saved</span>';document.getElementById('pass').value='';loadSaved();}
      else m.innerHTML='<span class="bad">Could not connect. Check the password.</span>';
      refreshStatus();
    }).catch(()=>{m.innerHTML='<span class="bad">Request failed.</span>';});
}
/* ---------- Database ---------- */
function loadDb(){loadWapDb();loadHostDb();}
function toggleSec(h,id){document.getElementById(id).classList.toggle('collapsed');h.classList.toggle('closed');}
function toggleGroup(h){h.nextElementSibling.classList.toggle('collapsed');h.classList.toggle('closed');}
function toggleReport(b){b.nextElementSibling.classList.toggle('collapsed');b.classList.toggle('closed');}
function filterDb(){
  var box=document.getElementById('dbSearch');var t=box?box.value.toLowerCase():'';
  document.querySelectorAll('#wapDb .dbrow').forEach(function(r){
    r.style.display=(!t||r.textContent.toLowerCase().indexOf(t)>=0)?'':'none';});
  document.querySelectorAll('#hostDb .netgroup').forEach(function(g){
    var hdr=g.querySelector('.nethdr'); var hmatch=hdr&&hdr.textContent.toLowerCase().indexOf(t)>=0; var any=false;
    g.querySelectorAll('.dbrow').forEach(function(r){
      var show=!t||hmatch||r.textContent.toLowerCase().indexOf(t)>=0;
      r.style.display=show?'':'none'; if(show)any=true;});
    g.style.display=any?'':'none';});
}
function loadWapDb(){
  var el=document.getElementById('wapDb');el.innerHTML='<p><span class="spin"></span>...</p>';
  fetch('/waps').then(r=>r.json()).then(list=>{
    if(!list.length){el.innerHTML='<p class="hint">No access points saved. Run WAP Sniffer.</p>';return;}
    el.innerHTML=list.map(w=>dbRow('wap',w.ssid,'',esc(w.ssid),'ch '+w.ch+' // '+w.enc+' // '+w.rssi+' dBm // +'+w.seen+'s',w.name,w.note)).join('');
    wireDb(el,'wap'); filterDb();
  }).catch(()=>{el.innerHTML='<p class="bad">Could not read access points.</p>';});
}
function loadHostDb(){
  var el=document.getElementById('hostDb');el.innerHTML='<p><span class="spin"></span>...</p>';
  fetch('/hosts').then(r=>r.json()).then(list=>{
    if(!list.length){el.innerHTML='<p class="hint">No hosts saved. Run ARP Sweeper.</p>';return;}
    var html='',cur=null,rows='';
    function flush(){ if(cur!==null){ html+='<div class="netgroup"><h4 class="nethdr collap" onclick="toggleGroup(this)">'+esc(cur||'(unknown)')+'<span class="caret">&#9662;</span></h4><div class="netbody">'+rows+'</div></div>'; rows=''; } }
    list.forEach(function(h){
      if(h.net!==cur){ flush(); cur=h.net; }
      var ports=h.ports?h.ports.split(' ').map(p=>'<span class="port">'+esc(p)+'</span>').join(''):'<span class="noports">not scanned</span>';
      var meta=esc(h.mac||'--')+' // +'+h.seen+'s<div style="margin-top:4px">'+ports+'</div>';
      rows+=dbRow('host',h.net,h.ip,h.ip,meta,h.name,h.note);
    });
    flush();
    el.innerHTML=html; wireDb(el,'host'); filterDb();
  }).catch(()=>{el.innerHTML='<p class="bad">Could not read hosts.</p>';});
}
function dbRow(kind,k1,k2,title,meta,name,note){
  var fn=name?'<span class="fname">'+esc(name)+'</span>':'';
  var nt=note?'<div class="note2">'+esc(note)+'</div>':'';
  return '<div class="dbrow" data-kind="'+kind+'" data-k1="'+esc(k1)+'" data-k2="'+esc(k2)+'">'+
    '<div class="rowwrap"><span class="hintL">delete &raquo;</span><span class="hintR">&laquo; edit</span>'+
    '<div class="rowmain"><div style="min-width:0"><div><span class="ipn">'+title+'</span>'+fn+'</div>'+
    '<div class="meta2">'+meta+'</div>'+nt+'</div>'+ROWBTN+'</div></div>'+
    '<div class="editpanel"><input class="ename" placeholder="friendly name" value="'+esc(name)+'">'+
    '<input class="enote" placeholder="notes / description" value="'+esc(note)+'">'+
    '<button class="btn small noswipe" onclick="saveMeta(this)">Save</button></div></div>';
}
function wireDb(container,kind){
  [...container.querySelectorAll('.dbrow')].forEach(function(row){
    attachSwipe(row,{
      onLeft:function(){delDb(kind,row.dataset.k1,row.dataset.k2);},
      onRight:function(){row.querySelector('.editpanel').classList.toggle('show');}
    });
  });
}
function saveMeta(btn){
  var row=btn.closest('.dbrow'),kind=row.dataset.kind;
  var name=row.querySelector('.ename').value,note=row.querySelector('.enote').value;
  var body;
  if(kind=='wap')body='ssid='+encodeURIComponent(row.dataset.k1)+'&name='+encodeURIComponent(name)+'&note='+encodeURIComponent(note);
  else body='net='+encodeURIComponent(row.dataset.k1)+'&ip='+encodeURIComponent(row.dataset.k2)+'&name='+encodeURIComponent(name)+'&note='+encodeURIComponent(note);
  fetch(kind=='wap'?'/editwap':'/edithost',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body}).then(()=>loadDb());
}
function delDb(kind,k1,k2){
  var label=kind=='wap'?('access point "'+k1+'"'):('host '+k2);
  if(!confirm('Delete '+label+'?'))return;
  var url=kind=='wap'?'/delwap':'/delhost';
  var body=kind=='wap'?'ssid='+encodeURIComponent(k1):'net='+encodeURIComponent(k1)+'&ip='+encodeURIComponent(k2);
  fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body}).then(()=>loadDb());
}
function clearDb(kind){
  if(!confirm('Delete ALL saved '+(kind=='wap'?'access points':'hosts')+'?'))return;
  fetch(kind=='wap'?'/clearwaps':'/clearhosts',{method:'POST'}).then(()=>loadDb());
}
function renderPorts(d){var el=document.getElementById("portChips");if(!el)return;var ps=(d&&d.ports)||[];el.innerHTML=ps.length?ps.map(function(p){return '<span class="port pchip" title="remove" onclick="delPortCfg('+p+')">'+p+' &times;</span>';}).join(""):'<span class="noports">no ports set</span>';}
function loadPortsCfg(){fetch("/ports").then(r=>r.json()).then(renderPorts).catch(function(){});}
function addPortsCfg(){var v=document.getElementById("addPorts").value.trim();if(!v)return;fetch("/portsadd",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:"ports="+encodeURIComponent(v)}).then(r=>r.json()).then(function(d){document.getElementById("addPorts").value="";renderPorts(d);});}
function delPortCfg(p){fetch("/portsdel",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:"p="+p}).then(r=>r.json()).then(renderPorts);}
function clearPortsCfg(){if(!confirm("Clear ALL scan ports?"))return;fetch("/portsclear",{method:"POST"}).then(r=>r.json()).then(renderPorts);}
function resetPortsCfg(){fetch("/portsreset",{method:"POST"}).then(r=>r.json()).then(renderPorts);}
refreshStatus();populateIpList();setInterval(refreshStatus,4000);
</script>
</body></html>)HTML";
