/* V1.11更新内容：
1.加入结构体管理电台各项参数
2.从根源上优化了改变频率的时候频繁擦写寄存器的bug（1.7优化点强势回归）
通过检测按钮松开瞬间写入频率值，并且只操作一次，大大提高运行速度
*/

//第一部分：参数声明
//按钮定义
const int downButton = 2;  // 频率减按钮
const int upButton = 3;    // 频率加按钮
const int PTTbutton = 4;   // PTT按钮

//参数值
struct RadioData {
 int Freq = 8750;            // 全局频率，单位10kHz（8750 = 87.50MHz）
 int RSSI;                   // 接收信号强度指示值
 int progressValue;          // 进度条百分比
 float voltage;              // 检测电压值
};
RadioData Radio;//结构体方便管理

//布尔状态值
bool downButtonState;       // 当前频率减按钮状态
bool UpButtonState;         // 当前频率加按钮状态
bool PTT;                   // 当前PTT按钮状态
int buttonState;            // 存储PTT按钮状态（用于边沿检测）
int lastButtonState = HIGH; // 存储上一次PTT按钮状态

//进度条的位置和大小
#define PROGRESS_BAR_X 10
#define PROGRESS_BAR_Y 42
#define PROGRESS_BAR_WIDTH 100
#define PROGRESS_BAR_HEIGHT 6

// V1.10：设置模式相关变量
bool settingsMode = false;          // true: 处于设置模式，false: 主模式
int currentSetting = 0;             // 当前选中的设置项（0~3，见下方定义）
unsigned long bothPressStart = 0;   // 两个按钮同时按下的起始时间（用于长按检测）
bool bothPressed = false;           // 当前是否两个按钮同时按下
bool bothPressTriggered = false;    // 组合键长按是否已经触发（防止重复触发）
bool lastPTTStateSettings = HIGH;   // 设置模式中记录上一次PTT状态，用于检测下降沿

// 设置模式中按钮的状态机（用于处理单独的短按）
enum ButtonState { IDLE, PRESSED };
ButtonState downBtnState = IDLE;     // 频率减按钮状态
ButtonState upBtnState = IDLE;       // 频率加按钮状态
unsigned long downPressTime = 0;     // 频率减按钮按下的时刻（用于判断短按）
unsigned long upPressTime = 0;       // 频率加按钮按下的时刻

// 新增：静噪相关变量
bool squelchEnabled = false;         // 静噪开关，默认关闭
int squelchThreshold = 20;            // 静噪阈值（0~127），默认20

// 设置选项索引（用于currentSetting）
#define SETTING_SQL_ENABLE 0
#define SETTING_SQL_THRESH 1
#define SETTING_TX_PWR      2
#define SETTING_VOL         3
#define NUM_SETTINGS        4

//设置相关变量掉电记忆
#include <EEPROM.h>
#define EEPROM_ADDR_SQL_ENABLE   0   // 静噪开关，占用1字节
#define EEPROM_ADDR_SQL_THRESH    1   // 静噪阈值，占用1字节
#define EEPROM_ADDR_TX_PWR        2   // 发射功率，占用1字节
#define EEPROM_ADDR_VOL           3   // 音量，占用1字节
// 地址4~保留
//如果有其它与电台使用相关的量可以在此设置

//第二部分：头文件引用
#include "RDA58xxTWI.h"
#include <Wire.h>
#define Wr_IIC 0x22 //rda5807的地址！
#define Rd_IIC (Wr_IIC + 1 )
//电台模块的头文件引用

#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
//显示屏的头文件引用及设置

/*#include <avr/sleep.h> 
//睡眠模式的头文件（保留源码）*/

#include <avr/wdt.h>
//看门狗的头文件

//第三部分：汉字字库
//PROGMEM配置，将汉字数据放置在闪存中
#include <avr/pgmspace.h>
 const uint8_t PROGMEM chao[] = {0x08,0x00,0x09,0xFC,0x08,0x44,0x7E,0x44,0x08,0x44,0x08,0x94,0xFF,0x08,0x08,0xFC,0x28,0x84,0x28,0x84,0x2E,0x84,0x28,0xFC,0x28,0x00,0x58,0x00,0x4F,0xFE,0x80,0x00};
 const uint8_t PROGMEM duan[] = {0x20,0x00,0x21,0xFE,0x3C,0x00,0x50,0x00,0x90,0xFC,0x10,0x84,0x10,0x84,0xFE,0x84,0x10,0xFC,0x10,0x00,0x10,0x84,0x28,0x44,0x24,0x48,0x44,0x00,0x41,0xFE,0x80,0x00};
 const uint8_t PROGMEM bo[] = {0x00,0x20,0x20,0x20,0x10,0x20,0x13,0xFE,0x82,0x22,0x42,0x24,0x4A,0x20,0x0B,0xFC,0x12,0x84,0x12,0x88,0xE2,0x48,0x22,0x50,0x22,0x20,0x24,0x50,0x24,0x88,0x09,0x06};
 const uint8_t PROGMEM dian[] = {0x01,0x00,0x01,0x00,0x01,0x00,0x3F,0xF8,0x21,0x08,0x21,0x08,0x21,0x08,0x3F,0xF8,0x21,0x08,0x21,0x08,0x21,0x08,0x3F,0xF8,0x21,0x0A,0x01,0x02,0x01,0x02,0x00,0xFE};
 const uint8_t PROGMEM tai[] = {0x02,0x00,0x02,0x00,0x04,0x00,0x08,0x20,0x10,0x10,0x20,0x08,0x7F,0xFC,0x20,0x04,0x00,0x00,0x1F,0xF0,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x1F,0xF0,0x10,0x10};
 const uint8_t PROGMEM she[] = {0x00,0x00,0x21,0xF0,0x11,0x10,0x11,0x10,0x01,0x10,0x02,0x0E,0xF4,0x00,0x13,0xF8,0x11,0x08,0x11,0x10,0x10,0x90,0x14,0xA0,0x18,0x40,0x10,0xA0,0x03,0x18,0x0C,0x06};
const uint8_t PROGMEM zhi[] = {0x7F,0xFC,0x44,0x44,0x7F,0xFC,0x01,0x00,0x7F,0xFC,0x01,0x00,0x1F,0xF0,0x10,0x10,0x1F,0xF0,0x10,0x10,0x1F,0xF0,0x10,0x10,0x1F,0xF0,0x10,0x10,0xFF,0xFE,0x00,0x00};

//第四部分：功能函数实现
//电台模块函数（原有函数完全保留，仅添加注释）
void init_XCVR(void){
    WriteTo_58xx(0x02,0b1101001000000101);   //初始化芯片(必须有)
    Rda5820RxMode();                         //初始为接收模式
    Rda5820BandSet(2);                        //解锁76-108MHz
    Rda5820SpaceSet(2);                        //步进频率设置（100kHz）
    Rda5820FreqSet(Radio.Freq);                      //初始默认频率87.5MHz
    Rda5820VolSet(4);                          //设置音量0-15
    Rda5820TxPagSet(63);                        //发射功率0-63
    Rda5820TxAdcSet(1);                         //输入ADC增益设置0-3
    Rda5820TxPgaSet(7);                         //输入PGA增益设置0-7
    Rda5820MuteSet(1);                           //初始化取消静音
}//电台初始化

void Rda5820RxMode(void)
{
    int temp;
    temp=ReadFrom_58xx(0X40); //读取0X40的内容
    temp&=0xfff0; //RX 模式   
    WriteTo_58xx(0X40,temp) ; //FM RX模式 
}//设置RDA5820为RX模式

void Rda5820TxMode(void)
{
    int temp;
    temp=ReadFrom_58xx(0X40); //读取0X40的内容 
    temp&=0xfff0;
    temp|=0x0001;    //TX 模式
    WriteTo_58xx(0X40,temp) ; //FM TM 模式 
}//设置RDA5820为TX模式

byte Rda5820RssiGet(void)//返回值范围:0~127(原注释返回值不准，实测220+)
{
    int temp;
    temp=ReadFrom_58xx(0X0B); //读取0X0B的内容
    return temp>>9;                 //返回信号强度
}//得到信号强度

void Rda5820VolSet(byte vol)//vol:0~15
{
    int temp;
    temp=ReadFrom_58xx(0X05); //读取0X05的内容
    temp&=0XFFF0;
    temp|=vol&0X0F;    
    WriteTo_58xx(0X05,temp) ; //设置音量
}//设置音量

void Rda5820MuteSet(byte mute)//mute:0,静音;1,不静音
{
    int temp;
    temp=ReadFrom_58xx(0X02); //读取0X02的内容
    if(mute)temp|=1<<14;
    else temp&=~(1<<14);       
    WriteTo_58xx(0X02,temp) ; //设置MUTE
}//静音设置

void Rda5820RssiSet(byte rssi)//rssi:0~127
{
    int temp;
    temp=ReadFrom_58xx(0X05); //读取0X05的内容
    temp&=0X80FF;
    temp|=(u16)rssi<<8;    
    WriteTo_58xx(0X05,temp) ; //设置RSSI
}//设置灵敏度

void Rda5820TxPagSet(byte gain)//gain:0~63
{
    int temp;
    temp=ReadFrom_58xx(0X41); //读取0X41的内容
    temp&=0X01C0;              //保留bit6-9（其他位清零）
    temp|=gain;                //写入新的功率值（bit0-5）
    WriteTo_58xx(0X41,temp) ; //设置PA的功率
}//设置TX发射功率

void Rda5820TxAdcSet(byte gain)//gain:0~3
{
    int temp;
    temp=0X00F0;               //基值
    temp|=gain<<8;             //增益位于bit8-9
    WriteTo_58xx(0X68,temp) ; //设置ADC增益
}//设置TX 输入信号ADC增益 

void Rda5820TxPgaSet(byte gain)//gain:0~7
{
    int temp;
    temp=ReadFrom_58xx(0X68); //读取0X68的内容
    temp|=gain<<10;            //增益位于bit10-12
    WriteTo_58xx(0X68,temp) ; //设置PGA增益
}//设置TX 输入信号PGA增益 

void Rda5820BandSet(byte band)//band:0,87~108Mhz;1,76~91Mhz;2,76~108Mhz;3,用户自定义
{
    int temp;
    temp=ReadFrom_58xx(0X03); //读取0X03的内容
    temp&=0XFFF3;
    temp|=band<<2;     
    WriteTo_58xx(0X03,temp) ; //设置BAND
}//设置RDA5820的工作频段

void Rda5820SpaceSet(byte spc)//spc:0,100Khz;1,200Khz;3,50Khz;3,保留
{
    int temp;
    temp=ReadFrom_58xx(0X03); //读取0X03的内容
    temp&=0XFFFC;
    temp|=spc;     
    WriteTo_58xx(0X03,temp) ; //设置步进频率
}//设置RDA5820的步进频率

void Rda5820FreqSet(int freq)//freq:频率值(单位为10Khz),比如10805,表示108.05Mhz
{
    int temp;
    byte spc=0,band=0;
    int fbtm,chan;
    temp=ReadFrom_58xx(0X03); //读取0X03的内容
    temp&=0X001F;
    band=(temp>>2)&0x03; //得到频带
    spc=temp&0x03; //得到分辨率
    if(spc==0)spc=10;
    else if(spc==1)spc=20;
    else spc=5;
    if(band==0)fbtm=8700;
    else if(band==1||band==2)fbtm=7600;
    else 
    {
        fbtm=ReadFrom_58xx(0X53);//得到bottom频率
        fbtm*=10;
    }
    if(freq<fbtm)return;
    chan=(freq-fbtm)/spc;    //得到CHAN应该写入的值
    chan&=0X3FF; //取低10位 
    temp|=chan<<6;
    temp|=1<<4; //TONE ENABLE    
    WriteTo_58xx(0X03,temp) ; //设置频率
    delay(20); //等待20ms
    while((ReadFrom_58xx(0X0B)&(1<<7))==0);//等待FM_READY
}//设置RDA5820的频率

int Rda5820FreqGet(void)//返回值:频率值(单位10Khz)
{
    int temp;
    byte spc=0,band=0;
    int fbtm,chan;
    temp=ReadFrom_58xx(0X03); //读取0X03的内容
    chan=temp>>6;   
    band=(temp>>2)&0x03; //得到频带
    spc=temp&0x03; //得到分辨率
    if(spc==0)spc=10;
    else if(spc==1)spc=20;
    else spc=5;
    if(band==0)fbtm=8700;
    else if(band==1||band==2)fbtm=7600;
    else 
    {
        fbtm=ReadFrom_58xx(0X53);//得到bottom频率
        fbtm*=10;
    }
    temp=fbtm+chan*spc;  
    return temp;//返回频率值
}//得到当前频率

void RdaSeek(void)
{
 int temp;
 int count = 0;
    temp=ReadFrom_58xx(0x02); //读取0x02的内容
    temp|=0X0100;     
    WriteTo_58xx(0x02,temp) ; //开始搜台
 do
 {
 delay(20);
 temp = ReadFrom_58xx(0x02);//读取搜台结果
 temp&= 0x0100;
 count++;
 if(count > 5000)return;

 }while(temp == 0x0100);
}//自动搜台

void RX_Fm(int Freq)
{
   Rda5820RxMode();//接收模式
   Rda5820FreqSet(Freq);//设置FM频率
}//FM接收模式（最终在设置时用此函数）

void TX_Fm(int Freq)
{
   Rda5820TxMode();//发射模式
   Rda5820FreqSet(Freq);//设置FM频率
}//FM发射模式（最终在设置时用此函数）

//控制方面函数
void readState(void){
   downButtonState = digitalRead(downButton);  
   UpButtonState = digitalRead(upButton);
   PTT = digitalRead(PTTbutton);
   buttonState = digitalRead(PTTbutton);
}// 读取当前按键状态

void voltageRead(void){
  static unsigned long lastVoltRead;
  if (millis() - lastVoltRead > 1000) {  // 每秒读一次电压
    Radio.voltage = analogRead(A0) * (5.0 / 1023.0);  // 直接浮点计算
    lastVoltRead = millis();
  }
}// 电压读取（优化计算）

void PTTcontrol(void){
  if (buttonState != lastButtonState) { // 检测PTT状态变化（下降沿或上升沿）
    if (buttonState == LOW) {
      TX_Fm(Radio.Freq);   // 按下PTT，切换到发射模式
    } else {
      RX_Fm(Radio.Freq);   // 松开PTT，切换回接收模式
    }
    lastButtonState = buttonState; // 更新上一次状态，防止重复执行
  }
}//PTT按钮控制发射状态

void init_screen(void){
 if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;); // 如果显示屏初始化失败就死循环
    }
  display.setTextColor(WHITE);
  display.clearDisplay();
}//显示屏的初始化设置

//显示方面函数
void displayPTT(void){
  if (PTT == LOW){
     display.setCursor(115, 55);
     display.setTextSize(1);
     display.print("TX");
   }else{
     display.setCursor(115, 55);
     display.setTextSize(1);
     display.print("RX");
  }
}//显示发射状态

void displayFreqValue(void){
    display.setCursor(12, 22);
    display.setTextSize(2);
    display.print((float)Radio.Freq/100);  
    //显示频率
    display.setCursor(90, 30);
    display.setTextSize(1);
    display.print("MHz");
    //显示单位
}//频率值及单位显示

void displayTopTitle(void){
    display.setCursor(110, 0);
    display.setTextSize(1);
    display.print("VFO");
    //上标
    display.drawBitmap(0, 0, chao, 16, 16, WHITE);
    display.drawBitmap(17, 0, duan, 16, 16, WHITE);
    display.drawBitmap(33, 0, bo, 16, 16, WHITE);
    display.drawBitmap(49, 0, dian, 16, 16, WHITE);
    display.drawBitmap(65, 0, tai, 16, 16, WHITE);
    //显示“超短波电台”
}//顶部标题显示

void displayVoltageValue(void){
    display.setCursor(0, 55);
    display.print(Radio.voltage);
    display.println("V");
} //显示电压

// V1.10修改：RSSI显示函数，使用全局RSSI变量，并附加SQL状态显示
void displayRSSIValue(void){
    display.setCursor(35, 55);
    display.print("RSSI ");
    display.print(Radio.RSSI);
    // 如果静噪开启且当前静音（RSSI低于阈值），显示SQL提示
    if (squelchEnabled && (Radio.RSSI < squelchThreshold)) {
        display.print(" SQL");
    }
} //显示RSSI值及静噪状态

void displayProgressBar(void){
  Radio.progressValue = map(Radio.RSSI, 0, 220, 0, 100);
  // 绘制进度条边框
   display.drawRect(PROGRESS_BAR_X, PROGRESS_BAR_Y, PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEIGHT, SSD1306_WHITE);
   // 计算进度条的填充长度
   int fillWidth = (Radio.progressValue * PROGRESS_BAR_WIDTH) / 100;
   // 绘制进度条填充部分
   display.fillRect(PROGRESS_BAR_X, PROGRESS_BAR_Y, fillWidth, PROGRESS_BAR_HEIGHT, SSD1306_WHITE);
} //显示进度条

void displayInitCaption(void){
    display.drawBitmap(17, 0, chao, 16, 16, WHITE);
    display.drawBitmap(33, 0, duan, 16, 16, WHITE);
    display.drawBitmap(49, 0, bo, 16, 16, WHITE);
    display.drawBitmap(65, 0, dian, 16, 16, WHITE);
    display.drawBitmap(81, 0, tai, 16, 16, WHITE);
    display.setCursor(17, 25);
    display.setTextSize(2);
    display.print("V1.11");  
    display.display();
    delay(3000); // 显示3秒
}// 显示初始化字幕

// V1.10修改：显示设置菜单，包含四个选项，顶部预留汉字位，无底部提示
void displaySettingsMenu(void) {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  // 顶部预留两个16x16汉字位置："设"和"置"
  display.drawBitmap(0, 0, she, 16, 16, WHITE);
  display.drawBitmap(17, 0, zhi, 16, 16, WHITE);
  // 选项起始Y坐标从18开始，避开顶部字模区域
  int yStart = 18;
  int lineSpacing = 12;
  
  // 选项1: 静噪开关
  display.setCursor(0, yStart);
  if (currentSetting == SETTING_SQL_ENABLE) display.print(">");
  else display.print(" ");
  display.print("SQL Enable: ");
  display.println(squelchEnabled ? "ON " : "OFF");
  
  // 选项2: 静噪阈值
  display.setCursor(0, yStart + lineSpacing);
  if (currentSetting == SETTING_SQL_THRESH) display.print(">");
  else display.print(" ");
  display.print("SQL Thresh: ");
  display.println(squelchThreshold);
  
  // 选项3: 发射功率
  display.setCursor(0, yStart + 2*lineSpacing);
  if (currentSetting == SETTING_TX_PWR) display.print(">");
  else display.print(" ");
  display.print("TX Pwr: ");
  int power = ReadFrom_58xx(0x41) & 0x3F;
  display.println(power);
  
  // 选项4: 音量
  display.setCursor(0, yStart + 3*lineSpacing);
  if (currentSetting == SETTING_VOL) display.print(">");
  else display.print(" ");
  display.print("Vol:    ");
  int vol = ReadFrom_58xx(0x05) & 0x0F;
  display.println(vol);
  
  // 底部不再显示退出提示
}

// V1.10新增：检测两个按钮同时长按2秒（用于进入/退出设置模式）
bool checkBothLongPress(void) {
  if (downButtonState == LOW && UpButtonState == LOW) {
    if (!bothPressed) {
      bothPressed = true;
      bothPressStart = millis();
      bothPressTriggered = false;
    } else {
      if (!bothPressTriggered && (millis() - bothPressStart >= 2000)) {
        bothPressTriggered = true;
        return true;
      }
    }
  } else {
    bothPressed = false;
    bothPressTriggered = false;
  }
  return false;
}

//主模式处理函数，增加静噪控制逻辑
void handleMainMode(void) {
  // 检测组合键长按，触发则进入设置模式
  if (checkBothLongPress()) {
    settingsMode = true;
    currentSetting = 0;
    downBtnState = IDLE;
    upBtnState = IDLE;
    lastPTTStateSettings = HIGH;
    lastButtonState = PTT;
    return;
  }

  // 频率加减短按处理（改进）
 static int lastDown = HIGH, lastUp = HIGH;   //按钮上拉，未按下为 HIGH，记录上次变化状态

 // 频率加减短按处理（只改变变量，不写入寄存器）
 if (downButtonState == LOW && Radio.Freq >= 7600) Radio.Freq -= 10;
 if (UpButtonState == LOW && Radio.Freq < 10800) Radio.Freq += 10;

 // 松开按钮的瞬间执行且只执行一次写入（检测上升沿）
 if (lastDown == LOW && downButtonState == HIGH) Rda5820FreqSet(Radio.Freq);
 if (lastUp == LOW && UpButtonState == HIGH) Rda5820FreqSet(Radio.Freq);

 // 更新状态
 lastDown = downButtonState;
 lastUp = UpButtonState;


  // PTT控制（切换收发模式）
  PTTcontrol();

  // 读取最新RSSI值（用于静噪判断和显示）
  Radio.RSSI = Rda5820RssiGet();

  //静噪处理（仅在接收模式下生效）
    if (squelchEnabled && (Radio.RSSI < squelchThreshold)){
      Rda5820MuteSet(0);                 // 静音
    } else {
      Rda5820MuteSet(1);                 // 取消静音
    }
 
  // 电压读取（每秒一次）
  voltageRead();

  // 刷新主界面显示
  display.clearDisplay();
  displayTopTitle();
  displayFreqValue();
  displayProgressBar();
  displayVoltageValue();
  displayPTT();
  displayRSSIValue();  // 此处使用更新后的RSSI值
  display.display();
}

//设置模式处理函数，支持四个选项的调节
void handleSettingsMode(void) {
  // 检测组合键长按，触发则退出设置模式
if (checkBothLongPress()) {
    saveSettingsToEEPROM();//退出前保存所有设置到EEPROM
    settingsMode = false;
    downBtnState = IDLE;
    upBtnState = IDLE;
    bothPressed = false;
    bothPressTriggered = false;
    lastButtonState = PTT;
    return;
  }
  // 处理PTT短按切换光标（仅在非组合键按下时）
  if (!bothPressed) {
    if (PTT == LOW && lastPTTStateSettings == HIGH) {
      currentSetting = (currentSetting + 1) % NUM_SETTINGS;
    }
    lastPTTStateSettings = PTT;
  } else {
    lastPTTStateSettings = PTT;
  }

  // 处理频率+和频率-的短按（调整当前选中项的值）
  if (!bothPressed) {
    // 频率减按钮处理
    if (downButtonState == LOW && downBtnState == IDLE) {
      downBtnState = PRESSED;
      downPressTime = millis();
    }
    if (downButtonState == HIGH && downBtnState == PRESSED) {
      if (millis() - downPressTime < 2000) { // 短按
        switch (currentSetting) {
          case SETTING_SQL_ENABLE:
            squelchEnabled = !squelchEnabled;
            break;
          case SETTING_SQL_THRESH:
            if (squelchThreshold > 0) squelchThreshold--;
            break;
          case SETTING_TX_PWR: {
            int power = ReadFrom_58xx(0x41) & 0x3F;
            if (power > 0) power--;
            Rda5820TxPagSet(power);
            break;
          }
          case SETTING_VOL: {
            int vol = ReadFrom_58xx(0x05) & 0x0F;
            if (vol > 0) vol--;
            Rda5820VolSet(vol);
            break;
          }
        }
      }
      downBtnState = IDLE;
    }

    // 频率加按钮处理
    if (UpButtonState == LOW && upBtnState == IDLE) {
      upBtnState = PRESSED;
      upPressTime = millis();
    }
    if (UpButtonState == HIGH && upBtnState == PRESSED) {
      if (millis() - upPressTime < 2000) { // 短按
        switch (currentSetting) {
          case SETTING_SQL_ENABLE:
            squelchEnabled = !squelchEnabled;
            break;
          case SETTING_SQL_THRESH:
            if (squelchThreshold < 127) squelchThreshold++;
            break;
          case SETTING_TX_PWR: {
            int power = ReadFrom_58xx(0x41) & 0x3F;
            if (power < 63) power++;
            Rda5820TxPagSet(power);
            break;
          }
          case SETTING_VOL: {
            int vol = ReadFrom_58xx(0x05) & 0x0F;
            if (vol < 15) vol++;
            Rda5820VolSet(vol);
            break;
          }
        }
      }
      upBtnState = IDLE;
    }
  } else {
    // 组合键按下时，重置短按状态机
    downBtnState = IDLE;
    upBtnState = IDLE;
  }

  // 电压读取（保持更新）
  voltageRead();

  // 刷新设置界面
  display.clearDisplay();
  displaySettingsMenu();
  display.display();
}

//将当前所有设置保存到EEPROM
void saveSettingsToEEPROM(void) {
  // 静噪开关（布尔值转为0/1）
  EEPROM.update(EEPROM_ADDR_SQL_ENABLE, squelchEnabled ? 1 : 0);
  
  // 静噪阈值（0~127）
  EEPROM.update(EEPROM_ADDR_SQL_THRESH, squelchThreshold);
  
  // 发射功率（从芯片读取当前值，确保最新）
  int power = ReadFrom_58xx(0x41) & 0x3F;
  EEPROM.update(EEPROM_ADDR_TX_PWR, power);
  
  // 音量（从芯片读取当前值）
  int vol = ReadFrom_58xx(0x05) & 0x0F;
  EEPROM.update(EEPROM_ADDR_VOL, vol);
  
  // 使用update()可避免重复写入相同值，延长EEPROM寿命
}

void setup()
{
  Serial.begin(9600); 
  //开启串口通信
  pinMode(downButton, INPUT_PULLUP); 
  pinMode(upButton, INPUT_PULLUP); 
  pinMode(PTTbutton, INPUT_PULLUP);
  pinMode(A0, INPUT_PULLUP);
  // 设置引脚为输入上拉模式

  init_XCVR();          // 电台初始化
  init_screen();        // 屏幕初始化
  displayInitCaption(); // 显示启动画面（V1.9字样）
  display.clearDisplay();
  display.setCursor(0, 0);
  
  //从EEPROM读取上次保存的设置参数
  byte value;

  // 读取静噪开关（存储为0或1）
  value = EEPROM.read(EEPROM_ADDR_SQL_ENABLE);
  if (value == 0 || value == 1) {
    squelchEnabled = value;
  } else {
    squelchEnabled = false;        // 首次上电或无效值，默认关闭
  }

  // 读取静噪阈值（0~127）
  value = EEPROM.read(EEPROM_ADDR_SQL_THRESH);
  if (value >= 0 && value <= 127) {
    squelchThreshold = value;
  } else {
    squelchThreshold = 20;          // 默认20
  }

  // 读取发射功率（0~63）
  value = EEPROM.read(EEPROM_ADDR_TX_PWR);
  if (value >= 0 && value <= 63) {
    Rda5820TxPagSet(value);         // 写入芯片
  } else {
    Rda5820TxPagSet(63);            // 默认63
  }

  // 读取音量（0~15）
  value = EEPROM.read(EEPROM_ADDR_VOL);
  if (value >= 0 && value <= 15) {
    Rda5820VolSet(value);            // 写入芯片
  } else {
    Rda5820VolSet(4);                // 默认4
  }

  wdt_enable(WDTO_2S);  // 启用看门狗，超时2秒
  
  /* 睡眠模式源码保留
  set_sleep_mode (SLEEP_MODE_PWR_DOWN); 
  */
}

void loop()
{
  wdt_reset();          // 喂狗，防止复位
  readState();          // 读取所有按键当前状态

  if (settingsMode) {
    handleSettingsMode(); // 当前处于设置模式
  } else {
    handleMainMode();     // 当前处于主模式
  }
}