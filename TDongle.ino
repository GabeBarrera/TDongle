/*
  LilyGO T-Dongle-S3 — AP+STA Wi-Fi Companion (SD-backed, priority list)
  ---------------------------------------------------------------------
  - Hosts access point "SAUR0N" AND joins a saved network (AP+STA).
  - Saved Wi-Fi credentials are stored on the microSD card (/wifi.conf),
    with a flash (NVS) fallback if no card is present.
  - Multiple saved networks kept in a PRIORITY ORDER. On boot the dongle
    scans and joins the highest-priority saved network that's in range.
  - Web UI:
      Scanner  : LED blue, scans, lists ALL nearby APs (browser + screen).
      Settings : shows your SAVED networks first with an availability dot;
                 a button reveals the full AP list only when you want to add
                 a new one; drag (or arrows) to reorder priority.

  Official ESP32 core libraries only (no third-party):
    WiFi.h, WebServer.h, Preferences.h, SPI.h, SD_MMC.h, FS.h

  Board settings (Tools): ESP32S3 Dev Module | USB CDC On Boot: Enabled |
    USB Mode: Hardware CDC and JTAG | Flash Size: 16MB | PSRAM: Disabled |
    Partition Scheme: 16M Flash (3MB APP/9.9MB FATFS)

  Upload: hold button while plugging in, upload, then replug to run.
  Then join Wi-Fi "SAUR0N" / "Hacktheplanet!" and open http://192.168.4.1
*/

#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <FS.h>
#include <SD_MMC.h>
#include <WiFiClient.h>
#include "lwip/etharp.h"     // ARP table access (already linked by Wi-Fi)
#include "lwip/netif.h"
#include "index_html.h"      // web page (PROGMEM raw string)

// ================= AP credentials =================
const char *AP_SSID = "SAUR0N";
const char *AP_PASS = "Hacktheplanet!";

// ================= Pin map (T-Dongle-S3) =================
#define PIN_LCD_MOSI   3
#define PIN_LCD_SCLK   5
#define PIN_LCD_CS     4
#define PIN_LCD_DC     2
#define PIN_LCD_RST    1
#define PIN_LCD_BL    38
#define PIN_LED_DATA  40
#define PIN_LED_CLK   39
#define PIN_BTN        0
// microSD (SD_MMC)
#define SD_CLK  12
#define SD_CMD  16
#define SD_D0   14
#define SD_D1   17
#define SD_D2   21
#define SD_D3   18

// ================= Screen geometry =================
#define LCD_W 160
#define LCD_H 80
#define X_OFFSET 1
#define Y_OFFSET 26
#define MADCTL_VALUE 0xA8   // 180-degree flipped landscape

// ================= Colors =================
#define C_BLACK  0x0000
#define C_WHITE  0xFFFF
#define C_RED    0xF800
#define C_GREEN  0x07E0
#define C_BLUE   0x001F
#define C_YELLOW 0xFFE0
#define C_CYAN   0x07FF
#define C_GREY   0x8410
#define C_ORANGE 0xFD20

SPISettings lcdSPI(27000000, MSBFIRST, SPI_MODE0);

// ================= Low-level LCD =================
void writeCmd(uint8_t c){digitalWrite(PIN_LCD_DC,LOW);digitalWrite(PIN_LCD_CS,LOW);SPI.beginTransaction(lcdSPI);SPI.transfer(c);SPI.endTransaction();digitalWrite(PIN_LCD_CS,HIGH);digitalWrite(PIN_LCD_DC,HIGH);}
void writeData(const uint8_t*d,size_t n){digitalWrite(PIN_LCD_CS,LOW);SPI.beginTransaction(lcdSPI);while(n--)SPI.transfer(*d++);SPI.endTransaction();digitalWrite(PIN_LCD_CS,HIGH);}
void writeData1(uint8_t d){writeData(&d,1);}
void setWindow(uint16_t x0,uint16_t y0,uint16_t x1,uint16_t y1){
  x0+=X_OFFSET;x1+=X_OFFSET;y0+=Y_OFFSET;y1+=Y_OFFSET;
  uint8_t ca[4]={(uint8_t)(x0>>8),(uint8_t)x0,(uint8_t)(x1>>8),(uint8_t)x1};
  uint8_t ra[4]={(uint8_t)(y0>>8),(uint8_t)y0,(uint8_t)(y1>>8),(uint8_t)y1};
  writeCmd(0x2A);writeData(ca,4);writeCmd(0x2B);writeData(ra,4);writeCmd(0x2C);
}
void fillRect(int x,int y,int w,int h,uint16_t color){
  if(x>=LCD_W||y>=LCD_H||w<=0||h<=0)return;
  if(x<0){w+=x;x=0;}if(y<0){h+=y;y=0;}
  if(x+w>LCD_W)w=LCD_W-x;if(y+h>LCD_H)h=LCD_H-y;
  setWindow(x,y,x+w-1,y+h-1);
  uint32_t n=(uint32_t)w*h;
  digitalWrite(PIN_LCD_CS,LOW);SPI.beginTransaction(lcdSPI);
  while(n--)SPI.transfer16(color);
  SPI.endTransaction();digitalWrite(PIN_LCD_CS,HIGH);
}
void fillScreen(uint16_t c){fillRect(0,0,LCD_W,LCD_H,c);}
void lcdInit(){
  pinMode(PIN_LCD_CS,OUTPUT);digitalWrite(PIN_LCD_CS,HIGH);
  pinMode(PIN_LCD_DC,OUTPUT);digitalWrite(PIN_LCD_DC,HIGH);
  pinMode(PIN_LCD_RST,OUTPUT);
  pinMode(PIN_LCD_BL,OUTPUT);digitalWrite(PIN_LCD_BL,HIGH);
  SPI.begin(PIN_LCD_SCLK,-1,PIN_LCD_MOSI,PIN_LCD_CS);
  digitalWrite(PIN_LCD_RST,HIGH);delay(10);
  digitalWrite(PIN_LCD_RST,LOW);delay(20);
  digitalWrite(PIN_LCD_RST,HIGH);delay(120);
  writeCmd(0x01);delay(150);writeCmd(0x11);delay(255);
  writeCmd(0x3A);writeData1(0x05);writeCmd(0x36);writeData1(MADCTL_VALUE);
  writeCmd(0x21);writeCmd(0x13);fillScreen(C_BLACK);writeCmd(0x29);delay(20);
  digitalWrite(PIN_LCD_BL,LOW);
}

// ================= APA102 LED =================
void ledByte(uint8_t b){for(int i=7;i>=0;i--){digitalWrite(PIN_LED_DATA,(b>>i)&1);digitalWrite(PIN_LED_CLK,HIGH);digitalWrite(PIN_LED_CLK,LOW);}}
void setLED(uint8_t r,uint8_t g,uint8_t b,uint8_t bright=4){
  for(int i=0;i<4;i++)ledByte(0x00);
  ledByte(0xE0|(bright&0x1F));ledByte(b);ledByte(g);ledByte(r);
  for(int i=0;i<4;i++)ledByte(0xFF);
}

// ================= 5x7 font =================
static const uint8_t FONT5x7[][5]={
  {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00},{0x00,0x07,0x00,0x07,0x00},
  {0x14,0x7F,0x14,0x7F,0x14},{0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,0x08,0x64,0x62},
  {0x36,0x49,0x55,0x22,0x50},{0x00,0x05,0x03,0x00,0x00},{0x00,0x1C,0x22,0x41,0x00},
  {0x00,0x41,0x22,0x1C,0x00},{0x08,0x2A,0x1C,0x2A,0x08},{0x08,0x08,0x3E,0x08,0x08},
  {0x00,0x50,0x30,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},{0x00,0x60,0x60,0x00,0x00},
  {0x20,0x10,0x08,0x04,0x02},{0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},
  {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},{0x18,0x14,0x12,0x7F,0x10},
  {0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
  {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},{0x00,0x36,0x36,0x00,0x00},
  {0x00,0x56,0x36,0x00,0x00},{0x08,0x14,0x22,0x41,0x00},{0x14,0x14,0x14,0x14,0x14},
  {0x00,0x41,0x22,0x14,0x08},{0x02,0x01,0x51,0x09,0x06},{0x32,0x49,0x79,0x41,0x3E},
  {0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
  {0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x01,0x01},
  {0x3E,0x41,0x41,0x51,0x32},{0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},
  {0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},
  {0x7F,0x02,0x04,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
  {0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},
  {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},
  {0x1F,0x20,0x40,0x20,0x1F},{0x7F,0x20,0x18,0x20,0x7F},{0x63,0x14,0x08,0x14,0x63},
  {0x03,0x04,0x78,0x04,0x03},{0x61,0x51,0x49,0x45,0x43}
};
void drawChar(int x,int y,char c,uint16_t fg,uint16_t bg,uint8_t s){
  if(c>='a'&&c<='z')c-=32;if(c<32||c>90)c='?';
  const uint8_t*g=FONT5x7[c-32];
  for(int col=0;col<6;col++){uint8_t bits=(col<5)?g[col]:0;
    for(int row=0;row<8;row++)fillRect(x+col*s,y+row*s,s,s,(bits>>row)&1?fg:bg);}
}
void drawText(int x,int y,const char*str,uint16_t fg,uint16_t bg,uint8_t s){
  while(*str){drawChar(x,y,*str++,fg,bg,s);x+=6*s;}
}

// ================= Saved-network store =================
#define MAX_SAVED 12
String savedSsid[MAX_SAVED];
String savedPass[MAX_SAVED];
int    savedCount = 0;
bool   usingSD = false;          // true = SD card, false = flash fallback
const char *CONF_PATH = "/wifi.conf";

WebServer server(80);
Preferences prefs;

enum ScreenMode { SCR_HOME, SCR_SCAN };
ScreenMode screenMode = SCR_HOME;
bool screenDirty = true;
bool scanLedActive = false;
uint32_t scanLedUntil = 0;

// ---- storage helpers ----
void persistSaved() {
  // Format: one "ssid\tpass" per line, order = priority.
  String blob;
  for (int i = 0; i < savedCount; i++)
    blob += savedSsid[i] + "\t" + savedPass[i] + "\n";

  if (usingSD) {
    File f = SD_MMC.open(CONF_PATH, FILE_WRITE);
    if (f) { f.print(blob); f.close(); return; }
    usingSD = false;   // card vanished; fall through to flash
  }
  prefs.begin("wifi", false);
  prefs.putString("nets", blob);
  prefs.end();
}

void parseBlob(const String &blob) {
  savedCount = 0;
  int start = 0;
  while (start < blob.length() && savedCount < MAX_SAVED) {
    int nl = blob.indexOf('\n', start);
    if (nl < 0) nl = blob.length();
    String line = blob.substring(start, nl);
    int tab = line.indexOf('\t');
    if (tab > 0) {
      savedSsid[savedCount] = line.substring(0, tab);
      savedPass[savedCount] = line.substring(tab + 1);
      savedCount++;
    }
    start = nl + 1;
  }
}

void loadSaved() {
  if (usingSD && SD_MMC.exists(CONF_PATH)) {
    File f = SD_MMC.open(CONF_PATH, FILE_READ);
    if (f) { String blob = f.readString(); f.close(); parseBlob(blob); return; }
  }
  prefs.begin("wifi", true);
  String blob = prefs.getString("nets", "");
  prefs.end();
  parseBlob(blob);
}

int findSaved(const String &ssid) {
  for (int i = 0; i < savedCount; i++) if (savedSsid[i] == ssid) return i;
  return -1;
}
void addOrUpdateSaved(const String &ssid, const String &pass) {
  int idx = findSaved(ssid);
  if (idx >= 0) { savedPass[idx] = pass; return; }
  if (savedCount >= MAX_SAVED) return;
  savedSsid[savedCount] = ssid; savedPass[savedCount] = pass; savedCount++;
}
void removeSaved(int idx) {
  if (idx < 0 || idx >= savedCount) return;
  for (int i = idx; i < savedCount - 1; i++) {
    savedSsid[i] = savedSsid[i + 1]; savedPass[i] = savedPass[i + 1];
  }
  savedCount--;
}

// ================= Screen =================
// Home screen is drawn in two parts: static labels once (drawHomeStatic),
// then dynamic fields repainted ONLY when their value changes (updateHome).
struct HomeState { bool valid; bool connected; String ssid; String ip; int saved; bool sd; };
HomeState lastHome = { false, false, "", "", -1, false };

void drawHomeStatic(){
  fillScreen(C_BLACK);
  drawText(4,2,"SAUR0N COMPANION",C_ORANGE,C_BLACK,1);
  fillRect(4,12,152,1,C_GREY);
  drawText(4,18,"AP:",C_GREY,C_BLACK,1);
  drawText(28,18,AP_SSID,C_CYAN,C_BLACK,1);
  drawText(4,30,"IP:",C_GREY,C_BLACK,1);
  drawText(28,30,"192.168.4.1",C_WHITE,C_BLACK,1);
  drawText(4,46,"WIFI:",C_GREY,C_BLACK,1);
  drawText(4,58,"STA:",C_GREY,C_BLACK,1);
  lastHome.valid=false;   // force a full dynamic repaint next
}

// Repaint only the fields that changed. force=true repaints everything.
void updateHome(bool force){
  bool connected = WiFi.status()==WL_CONNECTED;
  String ssid = connected ? WiFi.SSID() : String("");
  String ip   = connected ? WiFi.localIP().toString() : String("");

  bool changed = force || !lastHome.valid ||
                 connected!=lastHome.connected || ssid!=lastHome.ssid ||
                 ip!=lastHome.ip || savedCount!=lastHome.saved || usingSD!=lastHome.sd;
  if(!changed) return;

  // WIFI value (row 46)
  fillRect(44,46,116,8,C_BLACK);
  if(connected) drawText(44,46,ssid.c_str(),C_GREEN,C_BLACK,1);
  else          drawText(44,46,savedCount?"searching":"no saved",C_YELLOW,C_BLACK,1);

  // STA IP (row 58)
  fillRect(40,58,120,8,C_BLACK);
  if(connected) drawText(40,58,ip.c_str(),C_WHITE,C_BLACK,1);
  else          drawText(40,58,"--",C_GREY,C_BLACK,1);

  // store + saved count (row 70)
  fillRect(4,70,152,8,C_BLACK);
  char st[28]; snprintf(st,sizeof(st),"store:%s  saved:%d",usingSD?"SD":"flash",savedCount);
  drawText(4,70,st,C_GREY,C_BLACK,1);

  // LED only when the connection state itself flips
  if(force || connected!=lastHome.connected)
    setLED(connected?0:20, connected?20:8, 0);

  lastHome = { true, connected, ssid, ip, savedCount, usingSD };
}
void drawScanScreen(int n){
  fillScreen(C_BLACK);
  drawText(4,2,"WIFI SCAN",C_CYAN,C_BLACK,1);
  char hdr[20]; snprintf(hdr,sizeof(hdr),"%d found",n);
  drawText(100,2,hdr,C_WHITE,C_BLACK,1);
  fillRect(4,12,152,1,C_GREY);
  int rows=n<7?n:7;
  for(int i=0;i<rows;i++){
    int y=16+i*9; String s=WiFi.SSID(i); if(!s.length())s="(hidden)";
    if(s.length()>18)s=s.substring(0,18);
    int rssi=WiFi.RSSI(i);
    uint16_t col=rssi>-60?C_GREEN:(rssi>-75?C_YELLOW:C_RED);
    drawText(4,y,s.c_str(),C_WHITE,C_BLACK,1);
    char r[8]; snprintf(r,sizeof(r),"%d",rssi);
    drawText(132,y,r,col,C_BLACK,1);
  }
}

// ================= JSON helpers =================
String jsonEsc(String s){ s.replace("\\","\\\\"); s.replace("\"","\\\""); return s; }
String encName(wifi_auth_mode_t m){
  switch(m){case WIFI_AUTH_OPEN:return"open";case WIFI_AUTH_WEP:return"WEP";
    case WIFI_AUTH_WPA_PSK:return"WPA";case WIFI_AUTH_WPA2_PSK:return"WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:return"WPA/2";case WIFI_AUTH_WPA3_PSK:return"WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:return"WPA2/3";default:return"sec";}
}

// run a scan; returns count. 'blue' lights the LED + draws to screen.
int runScan(bool blue){
  if(blue){scanLedActive=true;scanLedUntil=millis()+8000;setLED(0,0,255,8);}
  int n=WiFi.scanNetworks(false,true);
  if(blue){screenMode=SCR_SCAN;drawScanScreen(n);}
  return n;
}
int rssiFor(const String &ssid){            // from the most recent scan buffer
  int n=WiFi.scanComplete();
  if(n<=0)return 0;
  for(int i=0;i<n;i++) if(WiFi.SSID(i)==ssid) return WiFi.RSSI(i);
  return 0;
}

// ================= Route handlers =================
void handleRoot(){ server.send_P(200,"text/html",INDEX_HTML); }

void handleScan(){                           // full list, blue LED, on screen
  int n=runScan(true);
  String j="[";
  for(int i=0;i<n;i++){ if(i)j+=",";
    j+="{\"ssid\":\""+jsonEsc(WiFi.SSID(i))+"\",\"rssi\":"+WiFi.RSSI(i)+
       ",\"ch\":"+WiFi.channel(i)+",\"enc\":\""+encName(WiFi.encryptionType(i))+"\"}";
  }
  j+="]";
  server.send(200,"application/json",j);
  WiFi.scanDelete();
}

void handleSaved(){                          // saved list + availability (quiet scan)
  int n=runScan(false);                      // no blue, no screen takeover
  String j="{\"storage\":\""+String(usingSD?"SD":"flash")+"\",\"nets\":[";
  for(int i=0;i<savedCount;i++){ if(i)j+=",";
    int r=rssiFor(savedSsid[i]);
    bool avail=(r!=0);
    j+="{\"ssid\":\""+jsonEsc(savedSsid[i])+"\",\"available\":"+(avail?"true":"false")+
       ",\"rssi\":"+r+"}";
  }
  j+="]}";
  server.send(200,"application/json",j);
  WiFi.scanDelete();
}

bool tryConnect(const String &ssid,const String &pass,uint32_t timeoutMs){
  WiFi.begin(ssid.c_str(),pass.c_str());
  uint32_t start=millis();
  while(WiFi.status()!=WL_CONNECTED && millis()-start<timeoutMs) delay(200);
  return WiFi.status()==WL_CONNECTED;
}

void handleConnect(){
  String ssid=server.arg("ssid"), pass=server.arg("pass");
  bool ok=false;
  if(ssid.length()){
    ok=tryConnect(ssid,pass,12000);
    if(ok){ addOrUpdateSaved(ssid,pass); persistSaved(); }
  }
  String ip=ok?WiFi.localIP().toString():"";
  server.send(200,"application/json",
    String("{\"connected\":")+(ok?"true":"false")+",\"ip\":\""+ip+"\"}");
  screenMode=SCR_HOME; screenDirty=true;
}

void handleForget(){
  String ssid=server.arg("ssid");
  int idx=findSaved(ssid);
  if(idx>=0){ removeSaved(idx); persistSaved(); }
  server.send(200,"application/json","{\"ok\":true}");
  screenMode=SCR_HOME; screenDirty=true;
}

void handleReorder(){
  // body: order=2,0,1  (indexes into the CURRENT saved array, new priority order)
  String order=server.arg("order");
  String ns[MAX_SAVED], np[MAX_SAVED]; int c=0;
  int start=0;
  while(start<order.length() && c<MAX_SAVED){
    int comma=order.indexOf(',',start); if(comma<0)comma=order.length();
    int idx=order.substring(start,comma).toInt();
    if(idx>=0 && idx<savedCount){ ns[c]=savedSsid[idx]; np[c]=savedPass[idx]; c++; }
    start=comma+1;
  }
  if(c==savedCount){                         // only accept a full, valid permutation
    for(int i=0;i<c;i++){ savedSsid[i]=ns[i]; savedPass[i]=np[i]; }
    persistSaved();
    server.send(200,"application/json","{\"ok\":true}");
  }else{
    server.send(400,"application/json","{\"ok\":false}");
  }
}

void handleStatus(){
  bool ok=WiFi.status()==WL_CONNECTED;
  String s="{\"connected\":"+String(ok?"true":"false");
  s+=",\"ssid\":\""+(ok?jsonEsc(WiFi.SSID()):String(""))+"\"";
  s+=",\"ip\":\""+(ok?WiFi.localIP().toString():String(""))+"\"";
  s+=",\"ap\":\""+String(AP_SSID)+"\"";
  s+=",\"storage\":\""+String(usingSD?"SD":"flash")+"\"";
  s+=",\"clients\":"+String(WiFi.softAPgetStationNum());
  s+="}";
  server.send(200,"application/json",s);
}

// ================= Network discovery (ARP sweep + TCP port scan) =================
// For authorized use on networks you own or are permitted to assess.
#define DISC_MAX 64
String discIp[DISC_MAX];
String discMac[DISC_MAX];
int    discCount = 0;

void discAdd(const String &ip, const String &mac){
  for(int i=0;i<discCount;i++) if(discIp[i]==ip) return;   // dedup
  if(discCount>=DISC_MAX) return;
  discIp[discCount]=ip; discMac[discCount]=mac; discCount++;
}
// Pull current stable entries out of the lwIP ARP cache into our list.
void drainArpTable(){
  for(size_t i=0;i<10;i++){   // default ARP_TABLE_SIZE
    ip4_addr_t *ipaddr=nullptr; struct netif *nif=nullptr; struct eth_addr *eth=nullptr;
    if(etharp_get_entry(i,&ipaddr,&nif,&eth)==1 && ipaddr && eth){
      IPAddress ip(ip4_addr_get_u32(ipaddr));
      char m[18];
      snprintf(m,sizeof(m),"%02x:%02x:%02x:%02x:%02x:%02x",
               eth->addr[0],eth->addr[1],eth->addr[2],eth->addr[3],eth->addr[4],eth->addr[5]);
      discAdd(ip.toString(), String(m));
    }
  }
}
void drawDiscScreen(const char *title, int count){
  fillScreen(C_BLACK);
  drawText(4,2,title,C_CYAN,C_BLACK,1);
  char h[16]; snprintf(h,sizeof(h),"%d up",count);
  drawText(120,2,h,C_WHITE,C_BLACK,1);
  fillRect(4,12,152,1,C_GREY);
  int rows=count<7?count:7;
  for(int i=0;i<rows;i++) drawText(4,16+i*9,discIp[i].c_str(),C_GREEN,C_BLACK,1);
}

// ARP-sweep the /24 of the station IP. Needs an active STA connection.
void handleArp(){
  if(WiFi.status()!=WL_CONNECTED){
    server.send(200,"application/json","{\"error\":\"join a network first\"}"); return;
  }
  struct netif *nif=netif_default;
  if(!nif){ server.send(200,"application/json","{\"error\":\"no netif\"}"); return; }

  setLED(140,0,255,8);                 // purple = discovery
  screenMode=SCR_SCAN;                 // borrow scan mode so loop won't redraw home
  discCount=0;
  IPAddress self=WiFi.localIP();
  drawDiscScreen("ARP SWEEP", 0);

  for(int host=1; host<=254; host++){
    if(host==self[3]) continue;
    ip4_addr_t t; IP4_ADDR(&t, self[0],self[1],self[2],host);
    etharp_request(nif,&t);
    delay(3);
    if(host%16==0){ drainArpTable(); drawDiscScreen("ARP SWEEP", discCount); }
  }
  delay(500);
  drainArpTable();
  drawDiscScreen("ARP SWEEP", discCount);

  String j="{\"self\":\""+self.toString()+"\",\"hosts\":[";
  for(int i=0;i<discCount;i++){ if(i)j+=",";
    j+="{\"ip\":\""+discIp[i]+"\",\"mac\":\""+discMac[i]+"\"}";
  }
  j+="]}";
  server.send(200,"application/json",j);
  screenMode=SCR_HOME; screenDirty=true;
}

// TCP connect scan of common ports on one host.
void handlePortscan(){
  String ipStr=server.arg("ip");
  IPAddress ip;
  if(!ip.fromString(ipStr)){ server.send(200,"application/json","{\"error\":\"bad ip\"}"); return; }

  static const uint16_t PORTS[]={21,22,23,25,53,80,110,135,139,143,443,445,
                                 554,587,993,995,1723,3306,3389,5900,8080,8443};
  const int NP=sizeof(PORTS)/sizeof(PORTS[0]);
  setLED(140,0,255,8);

  String j="{\"ip\":\""+ipStr+"\",\"open\":[";
  bool first=true;
  for(int i=0;i<NP;i++){
    WiFiClient c;
    if(c.connect(ip,PORTS[i],250)){
      if(!first)j+=","; first=false;
      j+=String(PORTS[i]);
      c.stop();
    }
  }
  j+="]}";
  server.send(200,"application/json",j);
  screenDirty=true;
}

// Auto-join best saved network that's currently in range (priority order).
void autoJoinBest(){
  if(savedCount==0)return;
  int n=runScan(false);
  for(int i=0;i<savedCount;i++){                 // priority order
    for(int j=0;j<n;j++){
      if(WiFi.SSID(j)==savedSsid[i]){
        Serial.printf("Auto-join #%d '%s'\n",i,savedSsid[i].c_str());
        if(tryConnect(savedSsid[i],savedPass[i],10000)){ WiFi.scanDelete(); return; }
      }
    }
  }
  WiFi.scanDelete();
}

// ================= SD mount =================
bool mountSD(){
  SD_MMC.setPins(SD_CLK,SD_CMD,SD_D0,SD_D1,SD_D2,SD_D3);
  if(SD_MMC.begin("/sdcard",false)) return true;   // 4-bit
  if(SD_MMC.begin("/sdcard",true))  return true;   // 1-bit fallback
  return false;
}

// ================= Setup / loop =================
void setup(){
  Serial.begin(115200); delay(300);
  Serial.println("\nT-Dongle-S3 SAURON companion");

  pinMode(PIN_BTN,INPUT_PULLUP);
  pinMode(PIN_LED_DATA,OUTPUT);digitalWrite(PIN_LED_DATA,LOW);
  pinMode(PIN_LED_CLK,OUTPUT);digitalWrite(PIN_LED_CLK,LOW);
  setLED(20,8,0);

  lcdInit(); fillScreen(C_BLACK);
  drawText(4,30,"STARTING...",C_ORANGE,C_BLACK,2);

  usingSD = mountSD();
  Serial.printf("Storage: %s\n", usingSD ? "SD card" : "flash (no SD)");
  loadSaved();
  Serial.printf("Loaded %d saved network(s)\n", savedCount);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID,AP_PASS);
  Serial.printf("AP '%s' at %s\n",AP_SSID,WiFi.softAPIP().toString().c_str());

  autoJoinBest();

  server.on("/",handleRoot);
  server.on("/scan",handleScan);
  server.on("/saved",handleSaved);
  server.on("/connect",HTTP_POST,handleConnect);
  server.on("/forget",HTTP_POST,handleForget);
  server.on("/reorder",HTTP_POST,handleReorder);
  server.on("/arp",handleArp);
  server.on("/portscan",handlePortscan);
  server.on("/status",handleStatus);
  server.onNotFound(handleRoot);
  server.begin();

  screenMode=SCR_HOME; screenDirty=true;
}

void loop(){
  server.handleClient();

  static bool lastBtn=HIGH;
  bool b=digitalRead(PIN_BTN);
  if(lastBtn==HIGH && b==LOW){ screenMode=SCR_HOME; screenDirty=true; }
  lastBtn=b;

  if(scanLedActive && millis()>scanLedUntil){
    scanLedActive=false; screenMode=SCR_HOME; screenDirty=true;
  }

  // Home screen: redraw labels only when (re)entering; otherwise poll state
  // twice a second and repaint a field only if it actually changed.
  static uint32_t lastCheck=0;
  if(screenMode==SCR_HOME){
    if(screenDirty){ screenDirty=false; drawHomeStatic(); updateHome(true); lastCheck=millis(); }
    else if(millis()-lastCheck>500){ lastCheck=millis(); updateHome(false); }
  }
  delay(5);
}
