/*
  LilyGO T-Dongle-S3 — AP+STA Wi-Fi Companion (SD-backed, priority list)
  ---------------------------------------------------------------------
  - Hosts access point "BL00DH0UND" AND joins a saved network (AP+STA).
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
  Then join Wi-Fi "BL00DH0UND" / "Hacktheplanet!" and open http://192.168.4.1
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
#include "lwip/tcpip.h"      // LOCK_TCPIP_CORE() — lwIP must be touched with the core lock held
#include "index_html.h"      // web page (PROGMEM raw string)
#include "bh_image.h"        // bloodhound boot-splash bitmap

// ================= AP credentials =================
const char *AP_SSID = "BL00DH0UND";
const char *AP_PASS = "Hacktheplanet!";
const char *DEV_NAME = "SIGNAL SNIFFER";

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

// ================= Bloodhound boot splash =================
static inline bool bhInk(int x,int by){
  uint16_t idx = by*BH_BYTES_PER_ROW + (x>>3);
  uint8_t b = pgm_read_byte(&BH_BITMAP[idx]);
  return (b >> (7-(x&7))) & 1;
}
// draw one bitmap row (ink runs) in 'color', at its screen position
void bhDrawRow(int by, uint16_t color){
  int sy = BH_Y + by, x = 0;
  while(x < BH_W){
    if(bhInk(x,by)){
      int start = x;
      while(x < BH_W && bhInk(x,by)) x++;
      fillRect(BH_X+start, sy, x-start, 1, color);
    } else x++;
  }
}
void bhDrawBase(){               // dog in white on black
  fillScreen(C_BLACK);
  for(int by=0; by<BH_H; by++) bhDrawRow(by, C_WHITE);
}
int bhWaterTop = BH_Y + BH_H;    // screen row of current red top (none yet)
int bhCur = 0;                   // current fill percent
void bhFillTo(int pct){          // animate red waterline up to pct
  if(pct>100) pct=100;
  while(bhCur < pct){
    bhCur++;
    int filled = (BH_H * bhCur) / 100;      // rows filled from bottom
    int newTop = BH_Y + BH_H - filled;
    for(int sy=newTop; sy<bhWaterTop; sy++){
      int by = sy - BH_Y;
      if(by>=0 && by<BH_H) bhDrawRow(by, C_RED);
    }
    if(newTop < bhWaterTop) bhWaterTop = newTop;
    delay(10);
  }
}
// Reset + draw the dog fresh, for reuse as a "busy/loading" splash.
void bhBegin(){ bhCur=0; bhWaterTop=BH_Y+BH_H; bhDrawBase(); }

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
  drawText(4,2,DEV_NAME,C_ORANGE,C_BLACK,1);
  fillRect(4,12,152,1,C_GREY);
  drawText(4,18,"AP:",C_GREY,C_BLACK,1);
  drawText(28,18,AP_SSID,C_RED,C_BLACK,1);   // AP name in red
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

void handleScan(){                           // WAP Sniffer: full list, blue LED, saved to DB
  int n=runScan(true);
  String j="[";
  for(int i=0;i<n;i++){ if(i)j+=",";
    String ssid=WiFi.SSID(i); String enc=encName(WiFi.encryptionType(i));
    int ch=WiFi.channel(i), rssi=WiFi.RSSI(i);
    j+="{\"ssid\":\""+jsonEsc(ssid)+"\",\"rssi\":"+rssi+
       ",\"ch\":"+ch+",\"enc\":\""+enc+"\"}";
    if(ssid.length()) upsertWap(ssid, ch, enc, rssi);   // store in the database
  }
  j+="]";
  saveWaps();
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

// Strip field separators so values are safe in a tab-delimited file.
String san(String s){ s.replace("\t"," "); s.replace("\n"," "); s.replace("\r"," "); return s; }
// split helper: nth tab-delimited field
String tsvField(const String &line, int n){
  int start=0;
  for(int k=0;k<n;k++){ int t=line.indexOf('\t',start); if(t<0) return ""; start=t+1; }
  int t=line.indexOf('\t',start); if(t<0) t=line.length();
  return line.substring(start,t);
}

// ---- Persistent HOST inventory (hosts from ARP/port scans) ----
#define HOST_MAX 128
struct HostRec { String net, ip, mac, ports, name, note; uint32_t seen; };
HostRec hosts[HOST_MAX];
int hostCount = 0;
const char *HOSTS_PATH = "/hosts.tsv";

uint32_t ipKey(const String &ip){
  IPAddress a; a.fromString(ip);
  return ((uint32_t)a[0]<<24)|((uint32_t)a[1]<<16)|((uint32_t)a[2]<<8)|a[3];
}
int findHost(const String &net, const String &ip){
  for(int i=0;i<hostCount;i++) if(hosts[i].net==net && hosts[i].ip==ip) return i;
  return -1;
}
void upsertHost(const String &net, const String &ip, const String &mac){
  int i=findHost(net,ip);
  if(i<0){ if(hostCount>=HOST_MAX) return; i=hostCount++; hosts[i].net=net; hosts[i].ip=ip; }
  if(mac.length()) hosts[i].mac=mac;
  hosts[i].seen=millis()/1000;
}
void setHostPorts(const String &net, const String &ip, const String &ports){
  int i=findHost(net,ip);
  if(i<0){ if(hostCount>=HOST_MAX) return; i=hostCount++; hosts[i].net=net; hosts[i].ip=ip; }
  hosts[i].ports=ports; hosts[i].seen=millis()/1000;
}
void setHostMeta(const String &net, const String &ip, const String &name, const String &note){
  int i=findHost(net,ip);
  if(i<0){ if(hostCount>=HOST_MAX) return; i=hostCount++; hosts[i].net=net; hosts[i].ip=ip; }
  hosts[i].name=san(name); hosts[i].note=san(note);
}
void sortHosts(){                 // by network name, then IP numeric
  for(int i=1;i<hostCount;i++){
    HostRec k=hosts[i]; int j=i-1;
    while(j>=0 && (hosts[j].net>k.net || (hosts[j].net==k.net && ipKey(hosts[j].ip)>ipKey(k.ip)))){
      hosts[j+1]=hosts[j]; j--;
    }
    hosts[j+1]=k;
  }
}
void saveHosts(){
  sortHosts();
  String blob;                    // net \t ip \t mac \t ports \t name \t note \t seen
  for(int i=0;i<hostCount;i++){
    String p=hosts[i].ports; if(p.length()==0) p="-";
    blob+=san(hosts[i].net)+"\t"+hosts[i].ip+"\t"+(hosts[i].mac.length()?hosts[i].mac:String("-"))+"\t"+
          p+"\t"+(hosts[i].name.length()?san(hosts[i].name):String("-"))+"\t"+
          (hosts[i].note.length()?san(hosts[i].note):String("-"))+"\t"+String(hosts[i].seen)+"\n";
  }
  if(usingSD){ File f=SD_MMC.open(HOSTS_PATH,FILE_WRITE); if(f){ f.print(blob); f.close(); return; } }
  prefs.begin("hosts",false); prefs.putString("tsv", blob.substring(0,3000)); prefs.end();
}
void loadHosts(){
  String blob;
  if(usingSD && SD_MMC.exists(HOSTS_PATH)){ File f=SD_MMC.open(HOSTS_PATH,FILE_READ); if(f){ blob=f.readString(); f.close(); } }
  else { prefs.begin("hosts",true); blob=prefs.getString("tsv",""); prefs.end(); }
  hostCount=0; int start=0;
  while(start<blob.length() && hostCount<HOST_MAX){
    int nl=blob.indexOf('\n',start); if(nl<0) nl=blob.length();
    String line=blob.substring(start,nl); start=nl+1;
    if(line.indexOf('\t')<0) continue;
    HostRec r;
    r.net=tsvField(line,0); r.ip=tsvField(line,1); r.mac=tsvField(line,2);
    String p=tsvField(line,3); if(p=="-")p=""; r.ports=p;
    r.name=tsvField(line,4); if(r.name=="-")r.name="";
    r.note=tsvField(line,5); if(r.note=="-")r.note="";
    r.seen=(uint32_t)tsvField(line,6).toInt();
    if(r.mac=="-") r.mac="";
    hosts[hostCount++]=r;
  }
}
void removeHostRec(int idx){
  if(idx<0||idx>=hostCount) return;
  for(int i=idx;i<hostCount-1;i++) hosts[i]=hosts[i+1];
  hostCount--;
}

// ---- Persistent WAP inventory (access points from the Wi-Fi sniffer) ----
#define WAP_MAX 64
struct WapRec { String ssid, enc, name, note; int ch; int rssi; uint32_t seen; };
WapRec waps[WAP_MAX];
int wapCount = 0;
const char *WAPS_PATH = "/waps.tsv";

int findWap(const String &ssid){
  for(int i=0;i<wapCount;i++) if(waps[i].ssid==ssid) return i;
  return -1;
}
void upsertWap(const String &ssid, int ch, const String &enc, int rssi){
  int i=findWap(ssid);
  if(i<0){ if(wapCount>=WAP_MAX) return; i=wapCount++; waps[i].ssid=ssid; }
  waps[i].ch=ch; waps[i].enc=enc; waps[i].rssi=rssi; waps[i].seen=millis()/1000;
}
void setWapMeta(const String &ssid, const String &name, const String &note){
  int i=findWap(ssid);
  if(i<0){ if(wapCount>=WAP_MAX) return; i=wapCount++; waps[i].ssid=ssid; }
  waps[i].name=san(name); waps[i].note=san(note);
}
void sortWaps(){ // by ssid
  for(int i=1;i<wapCount;i++){ WapRec k=waps[i]; int j=i-1;
    while(j>=0 && waps[j].ssid>k.ssid){ waps[j+1]=waps[j]; j--; } waps[j+1]=k; }
}
void saveWaps(){
  sortWaps();
  String blob;                    // ssid \t ch \t enc \t rssi \t name \t note \t seen
  for(int i=0;i<wapCount;i++){
    blob+=san(waps[i].ssid)+"\t"+String(waps[i].ch)+"\t"+waps[i].enc+"\t"+String(waps[i].rssi)+"\t"+
          (waps[i].name.length()?san(waps[i].name):String("-"))+"\t"+
          (waps[i].note.length()?san(waps[i].note):String("-"))+"\t"+String(waps[i].seen)+"\n";
  }
  if(usingSD){ File f=SD_MMC.open(WAPS_PATH,FILE_WRITE); if(f){ f.print(blob); f.close(); return; } }
  prefs.begin("waps",false); prefs.putString("tsv", blob.substring(0,3000)); prefs.end();
}
void loadWaps(){
  String blob;
  if(usingSD && SD_MMC.exists(WAPS_PATH)){ File f=SD_MMC.open(WAPS_PATH,FILE_READ); if(f){ blob=f.readString(); f.close(); } }
  else { prefs.begin("waps",true); blob=prefs.getString("tsv",""); prefs.end(); }
  wapCount=0; int start=0;
  while(start<blob.length() && wapCount<WAP_MAX){
    int nl=blob.indexOf('\n',start); if(nl<0) nl=blob.length();
    String line=blob.substring(start,nl); start=nl+1;
    if(line.indexOf('\t')<0) continue;
    WapRec r;
    r.ssid=tsvField(line,0); r.ch=tsvField(line,1).toInt(); r.enc=tsvField(line,2);
    r.rssi=tsvField(line,3).toInt();
    r.name=tsvField(line,4); if(r.name=="-")r.name="";
    r.note=tsvField(line,5); if(r.note=="-")r.note="";
    r.seen=(uint32_t)tsvField(line,6).toInt();
    waps[wapCount++]=r;
  }
}
void removeWapRec(int idx){
  if(idx<0||idx>=wapCount) return;
  for(int i=idx;i<wapCount-1;i++) waps[i]=waps[i+1];
  wapCount--;
}

// Find the station netif (the one holding our STA IP), not the AP netif.
struct netif* staNetif(){
  IPAddress self=WiFi.localIP();
  for(struct netif *n=netif_list; n; n=n->next){
    const ip4_addr_t *a=netif_ip4_addr(n);
    if(a && !ip4_addr_isany(a) && IPAddress(ip4_addr_get_u32(a))==self) return n;
  }
  return netif_default;
}

// Pull stable ARP-cache entries into our list. lwIP read is done under the core lock.
void drainArpTable(){
  for(size_t i=0;i<10;i++){   // default ARP_TABLE_SIZE
    ip4_addr_t *ipaddr=nullptr; struct netif *nif=nullptr; struct eth_addr *eth=nullptr;
    bool ok=false; uint32_t ipu=0; uint8_t mac[6]={0};
    LOCK_TCPIP_CORE();
    if(etharp_get_entry(i,&ipaddr,&nif,&eth)==1 && ipaddr && eth){
      ipu=ip4_addr_get_u32(ipaddr);
      for(int k=0;k<6;k++) mac[k]=eth->addr[k];
      ok=true;
    }
    UNLOCK_TCPIP_CORE();
    if(ok){
      char m[18];
      snprintf(m,sizeof(m),"%02x:%02x:%02x:%02x:%02x:%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
      discAdd(IPAddress(ipu).toString(), String(m));
    }
  }
}

// ARP-sweep the /24 of the station IP. Needs an active STA connection.
// Shows the bloodhound loading splash; saves results to the SD inventory.
void handleArp(){
  if(WiFi.status()!=WL_CONNECTED){
    server.send(200,"application/json","{\"error\":\"join a network first\"}"); return;
  }
  struct netif *nif=staNetif();
  if(!nif){ server.send(200,"application/json","{\"error\":\"no netif\"}"); return; }

  String net=WiFi.SSID();
  IPAddress self=WiFi.localIP();
  setLED(140,0,255,8);                 // purple = discovery
  screenMode=SCR_SCAN;                 // keep loop from redrawing home
  bhBegin();                           // bloodhound loading splash
  discCount=0;

  for(int host=1; host<=254; host++){
    if(host==self[3]) continue;
    ip4_addr_t t; IP4_ADDR(&t, self[0],self[1],self[2],host);
    LOCK_TCPIP_CORE(); etharp_request(nif,&t); UNLOCK_TCPIP_CORE();
    delay(3);
    if(host%16==0){ drainArpTable(); bhFillTo(host*100/254); }
  }
  delay(500);
  drainArpTable();
  bhFillTo(100);

  for(int i=0;i<discCount;i++) upsertHost(net, discIp[i], discMac[i]);
  saveHosts();

  String j="{\"self\":\""+self.toString()+"\",\"net\":\""+jsonEsc(net)+"\",\"hosts\":[";
  for(int i=0;i<discCount;i++){ if(i)j+=",";
    j+="{\"ip\":\""+discIp[i]+"\",\"mac\":\""+discMac[i]+"\"}";
  }
  j+="]}";
  server.send(200,"application/json",j);
  screenMode=SCR_HOME; screenDirty=true;
}

// TCP connect scan of common ports on one host. Saves open ports to the inventory.
void handlePortscan(){
  String ipStr=server.arg("ip");
  IPAddress ip;
  if(!ip.fromString(ipStr)){ server.send(200,"application/json","{\"error\":\"bad ip\"}"); return; }

  static const uint16_t PORTS[]={21,22,23,25,53,80,110,135,139,143,443,445,
                                 554,587,993,995,1723,3306,3389,5900,8080,8443};
  const int NP=sizeof(PORTS)/sizeof(PORTS[0]);
  setLED(140,0,255,8);
  screenMode=SCR_SCAN;
  bhBegin();                           // bloodhound loading splash

  String open="";
  String j="{\"ip\":\""+ipStr+"\",\"open\":[";
  bool first=true;
  for(int i=0;i<NP;i++){
    WiFiClient c;
    if(c.connect(ip,PORTS[i],250)){
      if(!first)j+=","; first=false;
      j+=String(PORTS[i]);
      open += (open.length()?" ":"") + String(PORTS[i]);
      c.stop();
    }
    bhFillTo((i+1)*100/NP);
  }
  j+="]}";

  String net = WiFi.status()==WL_CONNECTED ? WiFi.SSID() : String("(local)");
  setHostPorts(net, ipStr, open);
  saveHosts();

  server.send(200,"application/json",j);
  screenMode=SCR_HOME; screenDirty=true;
}

// ---- Database endpoints ----
// Host inventory JSON (sorted by network, then IP) with friendly name + note.
void handleHosts(){
  sortHosts();
  String j="[";
  for(int i=0;i<hostCount;i++){ if(i)j+=",";
    j+="{\"net\":\""+jsonEsc(hosts[i].net)+"\",\"ip\":\""+hosts[i].ip+"\",\"mac\":\""+hosts[i].mac+
       "\",\"ports\":\""+hosts[i].ports+"\",\"name\":\""+jsonEsc(hosts[i].name)+
       "\",\"note\":\""+jsonEsc(hosts[i].note)+"\",\"seen\":"+String(hosts[i].seen)+"}";
  }
  j+="]";
  server.send(200,"application/json",j);
}
// WAP inventory JSON (sorted by SSID) with friendly name + note.
void handleWaps(){
  sortWaps();
  String j="[";
  for(int i=0;i<wapCount;i++){ if(i)j+=",";
    j+="{\"ssid\":\""+jsonEsc(waps[i].ssid)+"\",\"ch\":"+String(waps[i].ch)+",\"enc\":\""+waps[i].enc+
       "\",\"rssi\":"+String(waps[i].rssi)+",\"name\":\""+jsonEsc(waps[i].name)+
       "\",\"note\":\""+jsonEsc(waps[i].note)+"\",\"seen\":"+String(waps[i].seen)+"}";
  }
  j+="]";
  server.send(200,"application/json",j);
}
void handleEditHost(){
  setHostMeta(server.arg("net"), server.arg("ip"), server.arg("name"), server.arg("note"));
  saveHosts();
  server.send(200,"application/json","{\"ok\":true}");
}
void handleEditWap(){
  setWapMeta(server.arg("ssid"), server.arg("name"), server.arg("note"));
  saveWaps();
  server.send(200,"application/json","{\"ok\":true}");
}
void handleDelHost(){
  int i=findHost(server.arg("net"), server.arg("ip"));
  if(i>=0){ removeHostRec(i); saveHosts(); }
  server.send(200,"application/json","{\"ok\":true}");
}
void handleDelWap(){
  int i=findWap(server.arg("ssid"));
  if(i>=0){ removeWapRec(i); saveWaps(); }
  server.send(200,"application/json","{\"ok\":true}");
}
void handleClearHosts(){
  hostCount=0;
  if(usingSD) SD_MMC.remove(HOSTS_PATH);
  else { prefs.begin("hosts",false); prefs.clear(); prefs.end(); }
  server.send(200,"application/json","{\"ok\":true}");
}
void handleClearWaps(){
  wapCount=0;
  if(usingSD) SD_MMC.remove(WAPS_PATH);
  else { prefs.begin("waps",false); prefs.clear(); prefs.end(); }
  server.send(200,"application/json","{\"ok\":true}");
}
// Join a saved network by SSID using its stored password (tap-to-join).
void handleJoinSaved(){
  String ssid=server.arg("ssid");
  int idx=-1; for(int i=0;i<savedCount;i++) if(savedSsid[i]==ssid){ idx=i; break; }
  if(idx<0){ server.send(200,"application/json","{\"connected\":false,\"error\":\"not saved\"}"); return; }
  bool ok=tryConnect(savedSsid[idx], savedPass[idx], 12000);
  String ip = ok ? WiFi.localIP().toString() : "";
  server.send(200,"application/json",
    String("{\"connected\":")+(ok?"true":"false")+",\"ip\":\""+ip+"\"}");
  screenMode=SCR_HOME; screenDirty=true;
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
  Serial.println("\nSignal Sniffer (T-Dongle-S3)");

  pinMode(PIN_BTN,INPUT_PULLUP);
  pinMode(PIN_LED_DATA,OUTPUT);digitalWrite(PIN_LED_DATA,LOW);
  pinMode(PIN_LED_CLK,OUTPUT);digitalWrite(PIN_LED_CLK,LOW);
  setLED(20,8,0);

  lcdInit();
  bhDrawBase();                 // bloodhound splash (white on black)
  bhFillTo(8);

  usingSD = mountSD();
  Serial.printf("Storage: %s\n", usingSD ? "SD card" : "flash (no SD)");
  bhFillTo(30);
  loadSaved();
  loadHosts();
  loadWaps();
  Serial.printf("Loaded %d saved net(s), %d host(s), %d wap(s)\n", savedCount, hostCount, wapCount);
  bhFillTo(50);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID,AP_PASS);
  Serial.printf("AP '%s' at %s\n",AP_SSID,WiFi.softAPIP().toString().c_str());
  bhFillTo(70);

  autoJoinBest();
  bhFillTo(100);
  delay(500);

  server.on("/",handleRoot);
  server.on("/scan",handleScan);
  server.on("/saved",handleSaved);
  server.on("/connect",HTTP_POST,handleConnect);
  server.on("/forget",HTTP_POST,handleForget);
  server.on("/reorder",HTTP_POST,handleReorder);
  server.on("/arp",handleArp);
  server.on("/portscan",handlePortscan);
  server.on("/hosts",handleHosts);
  server.on("/waps",handleWaps);
  server.on("/edithost",HTTP_POST,handleEditHost);
  server.on("/editwap",HTTP_POST,handleEditWap);
  server.on("/delhost",HTTP_POST,handleDelHost);
  server.on("/delwap",HTTP_POST,handleDelWap);
  server.on("/clearhosts",HTTP_POST,handleClearHosts);
  server.on("/clearwaps",HTTP_POST,handleClearWaps);
  server.on("/joinsaved",handleJoinSaved);
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
