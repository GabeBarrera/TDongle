#pragma once
// SAURON companion web page. Kept in a separate header because the Arduino
// sketch preprocessor mishandles large C++ raw-string literals placed in
// the .ino itself (it tries to compile the HTML/JS as code).

const char INDEX_HTML[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="en"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>SAURON Companion</title>
<style>
  :root{--bg:#0d0b07;--panel:#1a150d;--ink:#f1e6d0;--mute:#b99b5e;--acc:#e8a33d;--line:#3a2f1a;--ok:#5fd37a;--bad:#e05a4a}
  *{box-sizing:border-box}
  body{margin:0;background:var(--bg);color:var(--ink);font:16px/1.5 -apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif}
  header{padding:18px 16px;border-bottom:1px solid var(--line);background:var(--panel)}
  h1{margin:0;font-size:1.3rem;letter-spacing:.15em;color:var(--acc)}
  .sub{color:var(--mute);font-size:.8rem;margin-top:3px}
  main{max-width:640px;margin:0 auto;padding:16px}
  .tabs{display:flex;gap:8px;margin-bottom:16px}
  .tabs button{flex:1;padding:12px;border:1px solid var(--line);background:var(--panel);color:var(--ink);border-radius:10px;font-size:1rem;cursor:pointer}
  .tabs button.active{background:var(--acc);color:#1a1205;font-weight:700;border-color:var(--acc)}
  .view{display:none}.view.active{display:block}
  h3{font-size:.95rem;color:var(--mute);margin:18px 0 8px;font-weight:600}
  .net{display:flex;align-items:center;justify-content:space-between;gap:10px;padding:11px 12px;border:1px solid var(--line);border-radius:10px;margin:8px 0;background:var(--panel)}
  .net .nm{font-weight:600;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
  .net .meta{color:var(--mute);font-size:.78rem;white-space:nowrap}
  .pick{cursor:pointer}.pick:hover{border-color:var(--acc)}
  .lock{color:var(--mute);font-size:.8rem;margin-left:6px}
  .bars{font-family:monospace}
  .dot{display:inline-block;width:9px;height:9px;border-radius:50%;margin-right:7px;vertical-align:middle}
  /* saved rows with drag + controls */
  .srow{display:flex;align-items:center;gap:8px;padding:10px 10px;border:1px solid var(--line);border-radius:10px;margin:8px 0;background:var(--panel);touch-action:none}
  .srow.drag{opacity:.6;border-color:var(--acc)}
  .srow .handle{cursor:grab;color:var(--mute);font-size:1.2rem;padding:0 4px;user-select:none}
  .srow .info{flex:1;min-width:0}
  .srow .nm2{font-weight:600;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
  .srow .sub2{color:var(--mute);font-size:.74rem}
  .srow .ctl{display:flex;gap:4px}
  .srow .ctl button{width:30px;height:30px;border:1px solid var(--line);background:#120d06;color:var(--ink);border-radius:7px;cursor:pointer;font-size:.95rem;line-height:1}
  .srow .ctl .rm{color:var(--bad);border-color:#5a2a22}
  label{display:block;margin:14px 0 4px;color:var(--mute);font-size:.85rem}
  input{width:100%;padding:11px;border:1px solid var(--line);border-radius:10px;background:#120d06;color:var(--ink);font-size:1rem}
  .btn{margin-top:14px;width:100%;padding:12px;border:0;border-radius:10px;background:var(--acc);color:#1a1205;font-weight:700;font-size:1rem;cursor:pointer}
  .btn.ghost{background:transparent;border:1px solid var(--line);color:var(--ink);font-weight:400}
  .status{margin:10px 0;padding:10px 12px;border-radius:10px;background:var(--panel);border:1px solid var(--line);font-size:.9rem}
  .muted{color:var(--mute)}.ok{color:var(--ok)}.bad{color:var(--bad)}
  .spin{display:inline-block;width:14px;height:14px;border:2px solid var(--mute);border-top-color:var(--acc);border-radius:50%;animation:s .7s linear infinite;vertical-align:middle;margin-right:6px}
  @keyframes s{to{transform:rotate(360deg)}}
  .hint{color:var(--mute);font-size:.8rem;margin-top:8px}
  #addBox{display:none;margin-top:10px}
</style></head>
<body>
<header><h1>SAUR0N</h1><div class="sub">T-Dongle-S3 Wi-Fi Companion</div></header>
<main>
  <div class="status" id="status"><span class="muted">Loading status...</span></div>
  <div class="tabs">
    <button id="tScan" class="active" onclick="show('scan')">Wi-Fi Scanner</button>
    <button id="tSet" onclick="show('settings')">Settings</button>
  </div>

  <section id="scan" class="view active">
    <button class="btn" onclick="doScan()">Scan now</button>
    <div id="scanList"><p class="hint">Tap "Scan now" to list every nearby network. The LED turns blue and results also show on the dongle screen.</p></div>
  </section>

  <section id="settings" class="view">
    <h3>Saved networks &mdash; priority order</h3>
    <p class="hint">The dongle joins the highest one that's in range. Drag the <b>&#9776;</b> handle or use the arrows to reorder. Green dot = in range now.</p>
    <div id="savedList"><p class="hint">Loading...</p></div>

    <button class="btn ghost" id="addBtn" onclick="toggleAdd()">+ Add a new network</button>
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
</main>
<script>
function esc(s){return (s||'').replace(/[&<>"]/g,function(c){return{'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c];});}
function bars(r){var n=r>-60?4:r>-70?3:r>-80?2:1;var f='';for(var i=0;i<4;i++)f+=i<n?'|':'.';return f;}
function show(v){
  document.getElementById('scan').classList.toggle('active',v=='scan');
  document.getElementById('settings').classList.toggle('active',v=='settings');
  document.getElementById('tScan').classList.toggle('active',v=='scan');
  document.getElementById('tSet').classList.toggle('active',v=='settings');
  if(v=='settings') loadSaved();
}
function refreshStatus(){
  fetch('/status').then(r=>r.json()).then(d=>{
    var ok=d.connected;
    document.getElementById('status').innerHTML=
      '<span class="dot" style="background:'+(ok?'var(--ok)':'var(--mute)')+'"></span>'+
      (ok? 'Online via <b>'+esc(d.ssid)+'</b> &middot; '+d.ip : 'Access point only')+
      '<br><span class="muted">AP: '+esc(d.ap)+' &middot; 192.168.4.1 &middot; '+d.clients+' client(s) &middot; store: '+d.storage+'</span>';
  }).catch(()=>{});
}
/* ---- Scanner tab (full list, blue LED, screen) ---- */
function doScan(){
  var el=document.getElementById('scanList');
  el.innerHTML='<p><span class="spin"></span>Scanning... LED is blue</p>';
  fetch('/scan').then(r=>r.json()).then(list=>{
    if(!list.length){el.innerHTML='<p class="hint">No networks found.</p>';return;}
    el.innerHTML=list.map(n=>'<div class="net"><div><div class="nm">'+esc(n.ssid||'(hidden)')+
      '</div><div class="meta">ch '+n.ch+' &middot; '+n.enc+'</div></div>'+
      '<div class="meta"><span class="bars">'+bars(n.rssi)+'</span> '+n.rssi+' dBm</div></div>').join('');
  }).catch(()=>{el.innerHTML='<p class="bad">Scan failed.</p>';});
}
/* ---- Settings: saved list w/ availability + reorder ---- */
function loadSaved(){
  var el=document.getElementById('savedList');
  el.innerHTML='<p><span class="spin"></span>Checking saved networks...</p>';
  fetch('/saved').then(r=>r.json()).then(d=>{
    var nets=d.nets||[];
    if(!nets.length){el.innerHTML='<p class="hint">No saved networks yet. Add one below.</p>';return;}
    el.innerHTML=nets.map((n,i)=>rowHTML(n,i)).join('');
    wireDrag();
  }).catch(()=>{el.innerHTML='<p class="bad">Could not load saved list.</p>';});
}
function rowHTML(n,i){
  var dotCol=n.available?'var(--ok)':'var(--mute)';
  var avail=n.available?('in range &middot; '+n.rssi+' dBm'):'not in range';
  return '<div class="srow" data-idx="'+i+'">'+
    '<span class="handle" aria-label="drag">&#9776;</span>'+
    '<span class="dot" style="background:'+dotCol+'"></span>'+
    '<div class="info"><div class="nm2">'+esc(n.ssid)+'</div><div class="sub2">'+avail+'</div></div>'+
    '<div class="ctl">'+
      '<button onclick="move(this,-1)" aria-label="up">&#9650;</button>'+
      '<button onclick="move(this,1)" aria-label="down">&#9660;</button>'+
      '<button class="rm" onclick="forget(\''+esc(n.ssid).replace(/'/g,"\\'")+'\')" aria-label="remove">&times;</button>'+
    '</div></div>';
}
function commitOrder(){
  var ids=[...document.querySelectorAll('#savedList .srow')].map(r=>r.getAttribute('data-idx'));
  fetch('/reorder',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'order='+ids.join(',')}).then(()=>loadSaved());
}
function move(btn,dir){
  var row=btn.closest('.srow'), list=document.getElementById('savedList');
  if(dir<0 && row.previousElementSibling) list.insertBefore(row,row.previousElementSibling);
  if(dir>0 && row.nextElementSibling) list.insertBefore(row.nextElementSibling,row);
  commitOrder();
}
/* pointer-based drag reorder (works on touch + mouse) */
var dragRow=null;
function wireDrag(){
  document.querySelectorAll('#savedList .handle').forEach(h=>{
    h.addEventListener('pointerdown',e=>{
      e.preventDefault();
      dragRow=h.closest('.srow'); dragRow.classList.add('drag');
      h.setPointerCapture(e.pointerId);
    });
  });
  document.addEventListener('pointermove',onDragMove);
  document.addEventListener('pointerup',onDragUp);
}
function onDragMove(e){
  if(!dragRow)return;
  var list=document.getElementById('savedList');
  var rows=[...list.querySelectorAll('.srow')].filter(r=>r!==dragRow);
  var after=null;
  for(var r of rows){
    var box=r.getBoundingClientRect();
    if(e.clientY < box.top+box.height/2){ after=r; break; }
  }
  if(after) list.insertBefore(dragRow,after); else list.appendChild(dragRow);
}
function onDragUp(){
  if(!dragRow)return;
  dragRow.classList.remove('drag'); dragRow=null;
  commitOrder();
}
/* ---- add a new network ---- */
function toggleAdd(){
  var b=document.getElementById('addBox');
  b.style.display = b.style.display=='block' ? 'none' : 'block';
}
function loadFull(){
  var el=document.getElementById('fullList');
  el.innerHTML='<p><span class="spin"></span>Scanning... LED is blue</p>';
  fetch('/scan').then(r=>r.json()).then(list=>{
    if(!list.length){el.innerHTML='<p class="hint">No networks found.</p>';return;}
    el.innerHTML=list.map(n=>'<div class="net pick" onclick="pick(\''+esc(n.ssid).replace(/'/g,"\\'")+'\')">'+
      '<div class="nm">'+esc(n.ssid||'(hidden)')+(n.enc!='open'?'<span class="lock">lock</span>':'')+'</div>'+
      '<div class="meta"><span class="bars">'+bars(n.rssi)+'</span> '+n.rssi+'</div></div>').join('');
  }).catch(()=>{el.innerHTML='<p class="bad">Scan failed.</p>';});
}
function pick(s){document.getElementById('ssid').value=s;document.getElementById('pass').focus();}
function connect(){
  var ssid=document.getElementById('ssid').value.trim();
  var pass=document.getElementById('pass').value;
  var m=document.getElementById('setMsg');
  if(!ssid){m.innerHTML='<span class="bad">Enter a network name.</span>';return;}
  m.innerHTML='<span class="spin"></span>Connecting to '+esc(ssid)+'...';
  fetch('/connect',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass)})
    .then(r=>r.json()).then(d=>{
      if(d.connected){m.innerHTML='<span class="ok">Connected! IP '+d.ip+' (saved)</span>';
        document.getElementById('pass').value='';loadSaved();}
      else{m.innerHTML='<span class="bad">Could not connect. Check the password.</span>';}
      refreshStatus();
    }).catch(()=>{m.innerHTML='<span class="bad">Request failed.</span>';});
}
function forget(ssid){
  fetch('/forget',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'ssid='+encodeURIComponent(ssid)}).then(()=>{loadSaved();refreshStatus();});
}
refreshStatus();setInterval(refreshStatus,4000);
</script>
</body></html>)HTML";
