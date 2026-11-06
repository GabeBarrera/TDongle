#pragma once
// Signal Sniffer web page (cyberpunk theme, bloodhound logo embedded as a
// data URI). Kept in a separate header because the Arduino sketch
// preprocessor mishandles large C++ raw-string literals in the .ino.

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
  border-left:3px solid var(--red);padding-left:9px;text-shadow:0 0 6px rgba(255,92,92,.4)}
.tabs{display:flex;gap:6px;margin-bottom:16px}
.tabs button{flex:1;padding:11px 6px;border:1px solid var(--line);background:var(--panel);color:var(--mute);
  font:inherit;text-transform:uppercase;letter-spacing:.1em;font-size:.72rem;cursor:pointer;
  clip-path:polygon(8px 0,100% 0,100% calc(100% - 8px),calc(100% - 8px) 100%,0 100%,0 8px)}
.tabs button.active{background:var(--red);border-color:var(--red);color:#1a0406;font-weight:700;box-shadow:var(--glow)}
.view{display:none}.view.active{display:block}
.btn{margin-top:14px;width:100%;padding:12px;border:1px solid var(--red);
  background:linear-gradient(180deg,var(--red),var(--deep));color:#1a0406;font:inherit;font-weight:700;
  letter-spacing:.12em;text-transform:uppercase;cursor:pointer;box-shadow:var(--glow);
  clip-path:polygon(10px 0,100% 0,100% calc(100% - 10px),calc(100% - 10px) 100%,0 100%,0 10px)}
.btn:hover{filter:brightness(1.15)}
.btn.ghost{background:transparent;border:1px solid var(--line);color:var(--ink);box-shadow:none;font-weight:400}
.btn.ghost:hover{border-color:var(--red);color:var(--red2)}
.btn.danger{background:transparent;border:1px solid var(--deep);color:var(--red2);box-shadow:none}
.btn.danger:hover{background:#1a0406}
input{width:100%;padding:11px;border:1px solid var(--line);background:#0a0810;color:var(--ink);font:inherit}
input:focus{outline:none;border-color:var(--red);box-shadow:var(--glow)}
label{display:block;margin:14px 0 4px;color:var(--mute);font-size:.68rem;letter-spacing:.12em;text-transform:uppercase}
.net,.srow,.hostrow{display:flex;align-items:center;justify-content:space-between;gap:10px;padding:10px 12px;
  background:var(--panel);border:1px solid var(--line);border-left:3px solid var(--red);margin:7px 0}
.hostrow{align-items:flex-start}
.net .nm,.srow .nm2,.hostrow .nm{font-weight:700;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;color:var(--ink)}
.net .meta,.srow .sub2,.hostrow .meta{color:var(--mute);font-size:.7rem}
.net .meta,.srow .sub2{white-space:nowrap}
.pick{cursor:pointer}.pick:hover{border-left-color:var(--cyan);background:var(--panel2)}
.lock{color:var(--deep);margin-left:6px}
.bars{color:var(--red2)}
.dot{display:inline-block;width:9px;height:9px;border-radius:50%;margin-right:7px;vertical-align:middle;box-shadow:0 0 6px currentColor}
.srow{touch-action:none}.srow.drag{opacity:.6;border-left-color:var(--cyan)}
.srow .handle{cursor:grab;color:var(--red);font-size:1.1rem;padding:0 4px;user-select:none}
.srow .info{flex:1;min-width:0}.srow .ctl{display:flex;gap:4px}
.srow .ctl button,.hostdel{min-width:30px;height:30px;border:1px solid var(--line);background:#0a0810;color:var(--ink);cursor:pointer;font:inherit}
.srow .ctl .rm,.hostdel{color:var(--red2);border-color:var(--deep)}
.status{margin:10px 0;padding:10px 12px;background:var(--panel);border:1px solid var(--line);border-left:3px solid var(--red);font-size:.8rem}
.muted{color:var(--mute)}.ok{color:#48e06a}.bad{color:var(--red2)}
.spin{display:inline-block;width:14px;height:14px;border:2px solid var(--line);border-top-color:var(--red);border-radius:50%;animation:s .7s linear infinite;vertical-align:middle;margin-right:6px}
@keyframes s{to{transform:rotate(360deg)}}
.hint{color:var(--mute);font-size:.72rem;margin-top:8px}
.port{display:inline-block;background:#1a0406;border:1px solid var(--deep);color:var(--red2);padding:1px 6px;margin:3px 3px 0 0;font-size:.7rem}
.noports{color:var(--dim);font-size:.72rem}
#addBox{display:none;margin-top:10px}
</style></head>
<body>
<header>
  <img src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAJIAAACYCAYAAAAP4LDoAAAC00lEQVR42u3dgZHDIAxEUeXm2kn/haSgXAUeGx8YCd4vwLE9m0WLAEcAQBZeqzzI9/3+pnihn89rRyH9+C+BkEBIICSAkLBZasuSwqQ8jgRCAiGBkABCwt6pbZd0tnqa40ggJBASCAkgJKyS2rKntKtp6ennyJ7iOBK68OsVmEfiSCAkEBIwL7W1ppyjGiH7/rVez9l63dk1FUeC1BYJVy30cqpqqyI4EggJhASprVaP7W6a6X2/o1Oo1AapbYde1yqrFTgSFNsgJGBuahudQu5ev1ct0fv3R/XmOBKktkqpprVXNjvNnd3X3efhSDC0gZAAM9stqexqrTR7Zvrofp+u4TgSOJKVmWokKLYBQkIk67WNHqtn9dquXr81ba3SU+NIICQQEggJICSsmtp6rQuqtg9s9ky09UgIvTZE1VNTOBI4UsUu+irfouVI4EhPpJNZaWlWD5IjgSOFE/h9CZMjgZBASNi0Rtpt7M9SE80+PaX1fjgSpLbwxUs1EjjS0qe4ruZA1iPB0AZCAggJhITVd5E8NTPae8+99MWREOaRotpMdC+nVCMBlRzpbH/V02uuq+0CuXraLkfCGo5UvRY4u/+7/+TRZxBk3Tnb+rscCYY2EBKktthqRaQ9/hwJHCnSzxfN+pLkqF7lf5+DI4EjOQVFjQRCAggJaqRcae9ovilbb86+NhjaQEiAGikG9Nau1kLZVzByJHAkKS/KfO3prHfoxDYY2kBIUCNJcd4CR8KujjTaAVp3+D59xsCs87R9QRKGNhASQEggJESxMySrzJ+0po2q80LVVhFwJBASCAmEBBASqp/8nz3NZFmpWPXMSY4EQgIhIaxHgpWaHAmbOVKvntrV6zi9liOBkEBIgNS2z1eNOBLWcqTsp2jM6sFVcRJnSMLQBkICpDa1CkcCR+Ik4EggJBASCAkgJMzb14awioAjgZBASCAkgJAAAIj4A/8fbu9CEN2jAAAAAElFTkSuQmCC" alt="bloodhound">
  <div><h1>SIGNAL SNIFFER</h1><div class="sub">T-Dongle-S3 &middot; AP: BL00DH0UND</div></div>
</header>
<main>
  <div class="status" id="status"><span class="muted">Loading status...</span></div>
  <div class="tabs">
    <button id="tScan" class="active" onclick="show('scan')">Scanner</button>
    <button id="tSet" onclick="show('settings')">Settings</button>
    <button id="tNet" onclick="show('network')">Network</button>
  </div>

  <section id="scan" class="view active">
    <button class="btn" onclick="doScan()">Scan now</button>
    <div id="scanList"><p class="hint">Tap "Scan now" to list every nearby network. The LED turns blue and results also show on the device screen.</p></div>
  </section>

  <section id="settings" class="view">
    <h3>Saved networks &mdash; priority order</h3>
    <p class="hint">The dongle joins the highest one in range. Drag the handle or use arrows to reorder. Green dot = in range now.</p>
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

  <section id="network" class="view">
    <h3>Host discovery &mdash; ARP sweep</h3>
    <p class="hint">Sweeps the /24 of the joined network for live hosts. Authorized networks only. The bloodhound fills red while it runs.</p>
    <button class="btn" onclick="arpSweep()">ARP sweep</button>
    <div id="hostList"></div>
    <h3>Port scan</h3>
    <label for="pip">Target IP</label>
    <input id="pip" placeholder="tap a host, or type an IP">
    <button class="btn" onclick="portScan()">Scan ports</button>
    <div id="portResult" class="hint"></div>
    <h3>Saved inventory (SD)</h3>
    <p class="hint">Every discovered host, grouped by network, sorted by IP, with any open ports. Stored on the card.</p>
    <button class="btn ghost" onclick="loadInventory()">Refresh inventory</button>
    <button class="btn danger" onclick="clearHosts()">Delete all saved hosts</button>
    <div id="inv"></div>
  </section>
</main>
<script>
function esc(s){return (s||'').replace(/[&<>"]/g,function(c){return{'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c];});}
function q(s){return (s||'').replace(/\\/g,'').replace(/'/g,"\\'");}
function bars(r){var n=r>-60?4:r>-70?3:r>-80?2:1;var f='';for(var i=0;i<4;i++)f+=i<n?'|':'.';return f;}
function show(v){var m={scan:'tScan',settings:'tSet',network:'tNet'};
  for(var id in m){document.getElementById(id).classList.toggle('active',id==v);
    document.getElementById(m[id]).classList.toggle('active',id==v);}
  if(v=='settings')loadSaved(); if(v=='network')loadInventory();}
function refreshStatus(){
  fetch('/status').then(r=>r.json()).then(d=>{
    var ok=d.connected;
    document.getElementById('status').innerHTML=
      '<span class="dot" style="color:'+(ok?'#48e06a':'var(--mute)')+'"></span>'+
      (ok?'ONLINE // <b>'+esc(d.ssid)+'</b> // '+d.ip:'AP ONLY')+
      '<br><span class="muted">AP '+esc(d.ap)+' // 192.168.4.1 // '+d.clients+' client(s) // store:'+d.storage+'</span>';
  }).catch(()=>{});
}
function doScan(){
  var el=document.getElementById('scanList');
  el.innerHTML='<p><span class="spin"></span>Scanning... LED is blue</p>';
  fetch('/scan').then(r=>r.json()).then(list=>{
    if(!list.length){el.innerHTML='<p class="hint">No networks found.</p>';return;}
    el.innerHTML=list.map(n=>'<div class="net"><div><div class="nm">'+esc(n.ssid||'(hidden)')+
      '</div><div class="meta">ch '+n.ch+' // '+n.enc+'</div></div>'+
      '<div class="meta"><span class="bars">'+bars(n.rssi)+'</span> '+n.rssi+' dBm</div></div>').join('');
  }).catch(()=>{el.innerHTML='<p class="bad">Scan failed.</p>';});
}
function loadSaved(){
  var el=document.getElementById('savedList');
  el.innerHTML='<p><span class="spin"></span>Checking saved networks...</p>';
  fetch('/saved').then(r=>r.json()).then(d=>{
    var nets=d.nets||[];
    if(!nets.length){el.innerHTML='<p class="hint">No saved networks yet. Add one below.</p>';return;}
    el.innerHTML=nets.map((n,i)=>rowHTML(n,i)).join(''); wireDrag();
  }).catch(()=>{el.innerHTML='<p class="bad">Could not load saved list.</p>';});
}
function rowHTML(n,i){
  var col=n.available?'#48e06a':'var(--mute)';
  var avail=n.available?('in range // '+n.rssi+' dBm'):'not in range';
  return '<div class="srow" data-idx="'+i+'">'+
    '<span class="handle">&#9776;</span>'+
    '<span class="dot" style="color:'+col+'"></span>'+
    '<div class="info"><div class="nm2">'+esc(n.ssid)+'</div><div class="sub2">'+avail+'</div></div>'+
    '<div class="ctl"><button onclick="move(this,-1)">&#9650;</button>'+
    '<button onclick="move(this,1)">&#9660;</button>'+
    '<button class="rm" onclick="forget(\''+q(n.ssid)+'\')">&times;</button></div></div>';
}
function commitOrder(){
  var ids=[...document.querySelectorAll('#savedList .srow')].map(r=>r.getAttribute('data-idx'));
  fetch('/reorder',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'order='+ids.join(',')}).then(()=>loadSaved());
}
function move(btn,dir){
  var row=btn.closest('.srow'),list=document.getElementById('savedList');
  if(dir<0&&row.previousElementSibling)list.insertBefore(row,row.previousElementSibling);
  if(dir>0&&row.nextElementSibling)list.insertBefore(row.nextElementSibling,row);
  commitOrder();
}
var dragRow=null;
function wireDrag(){
  document.querySelectorAll('#savedList .handle').forEach(h=>{
    h.addEventListener('pointerdown',e=>{e.preventDefault();dragRow=h.closest('.srow');dragRow.classList.add('drag');h.setPointerCapture(e.pointerId);});
  });
  document.addEventListener('pointermove',onDragMove);
  document.addEventListener('pointerup',onDragUp);
}
function onDragMove(e){
  if(!dragRow)return;
  var list=document.getElementById('savedList');
  var rows=[...list.querySelectorAll('.srow')].filter(r=>r!==dragRow);
  var after=null;
  for(var r of rows){var b=r.getBoundingClientRect();if(e.clientY<b.top+b.height/2){after=r;break;}}
  if(after)list.insertBefore(dragRow,after);else list.appendChild(dragRow);
}
function onDragUp(){if(!dragRow)return;dragRow.classList.remove('drag');dragRow=null;commitOrder();}
function toggleAdd(){var b=document.getElementById('addBox');b.style.display=b.style.display=='block'?'none':'block';}
function loadFull(){
  var el=document.getElementById('fullList');
  el.innerHTML='<p><span class="spin"></span>Scanning... LED is blue</p>';
  fetch('/scan').then(r=>r.json()).then(list=>{
    if(!list.length){el.innerHTML='<p class="hint">No networks found.</p>';return;}
    el.innerHTML=list.map(n=>'<div class="net pick" onclick="pick(\''+q(n.ssid)+'\')"><div class="nm">'+
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
      if(d.connected){m.innerHTML='<span class="ok">Connected // IP '+d.ip+' // saved</span>';document.getElementById('pass').value='';loadSaved();}
      else m.innerHTML='<span class="bad">Could not connect. Check the password.</span>';
      refreshStatus();
    }).catch(()=>{m.innerHTML='<span class="bad">Request failed.</span>';});
}
function forget(ssid){
  fetch('/forget',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'ssid='+encodeURIComponent(ssid)}).then(()=>{loadSaved();refreshStatus();});
}
function arpSweep(){
  var el=document.getElementById('hostList');
  el.innerHTML='<p><span class="spin"></span>Sweeping subnet... bloodhound filling</p>';
  fetch('/arp').then(r=>r.json()).then(d=>{
    if(d.error){el.innerHTML='<p class="bad">'+esc(d.error)+'</p>';return;}
    var h=d.hosts||[];
    if(!h.length){el.innerHTML='<p class="hint">No hosts answered. Some devices ignore ARP from unknown hosts.</p>';loadInventory();return;}
    el.innerHTML='<p class="hint">'+h.length+' host(s) on '+esc(d.net||d.self)+' ('+esc(d.self)+'). Tap one to port-scan. Saved to SD.</p>'+
      h.map(x=>'<div class="net pick" onclick="pip(\''+x.ip+'\')"><div class="nm">'+x.ip+'</div><div class="meta">'+x.mac+'</div></div>').join('');
    loadInventory();
  }).catch(()=>{el.innerHTML='<p class="bad">Sweep failed.</p>';});
}
function pip(ip){document.getElementById('pip').value=ip;}
function portScan(){
  var ip=document.getElementById('pip').value.trim(),el=document.getElementById('portResult');
  if(!ip){el.innerHTML='<span class="bad">Enter an IP.</span>';return;}
  el.innerHTML='<span class="spin"></span>Scanning '+esc(ip)+'...';
  fetch('/portscan?ip='+encodeURIComponent(ip)).then(r=>r.json()).then(d=>{
    if(d.error){el.innerHTML='<span class="bad">'+esc(d.error)+'</span>';return;}
    if(!d.open.length){el.innerHTML='<span class="muted">No common ports open on '+esc(ip)+'.</span>';}
    else el.innerHTML='<b>'+esc(ip)+'</b> open: '+d.open.map(p=>'<span class="port">'+p+'</span>').join('');
    loadInventory();
  }).catch(()=>{el.innerHTML='<span class="bad">Scan failed.</span>';});
}
function loadInventory(){
  var el=document.getElementById('inv');
  el.innerHTML='<p><span class="spin"></span>Reading inventory...</p>';
  fetch('/hosts').then(r=>r.json()).then(list=>{
    if(!list.length){el.innerHTML='<p class="hint">No hosts saved yet. Run an ARP sweep.</p>';return;}
    var html='',cur=null;
    list.forEach(function(h){
      if(h.net!==cur){cur=h.net;html+='<h3>'+esc(cur||'(unknown)')+'</h3>';}
      var ports=h.ports?h.ports.split(' ').map(p=>'<span class="port">'+esc(p)+'</span>').join(''):'<span class="noports">not scanned</span>';
      html+='<div class="hostrow"><div style="min-width:0"><div class="nm">'+h.ip+
        '</div><div class="meta">'+esc(h.mac||'--')+' // seen +'+h.seen+'s</div>'+
        '<div style="margin-top:4px">'+ports+'</div></div>'+
        '<button class="hostdel" onclick="delHost(\''+q(h.net)+'\',\''+h.ip+'\')">&times;</button></div>';
    });
    el.innerHTML=html;
  }).catch(()=>{el.innerHTML='<p class="bad">Could not read inventory.</p>';});
}
function delHost(net,ip){
  fetch('/delhost',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'net='+encodeURIComponent(net)+'&ip='+encodeURIComponent(ip)}).then(()=>loadInventory());
}
function clearHosts(){
  if(!confirm('Delete ALL saved hosts from the SD card?'))return;
  fetch('/clearhosts',{method:'POST'}).then(()=>loadInventory());
}
refreshStatus();setInterval(refreshStatus,4000);
</script>
</body></html>)HTML";
