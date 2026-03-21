/*1.9更新内容：这个版本没有加新功能，只是把代码重新整理了一下。
主要改动：
1. 把所有的功能都分开写成了单独的小函数
2. 把代码分成了四个部分：参数声明、头文件、字库、功能函数
3. 把显示的东西也都分开写了，每个显示内容一个函数
4. 现在loop()里面只有几行，看起来很清爽
5. 注释也重新写了，解释每个函数是干什么的*/


//第一部分：参数声明
//按钮定义
const int downButton = 2;  // 频率减按钮
const int upButton = 3;//频率加按钮
const int PTTbutton = 4;//PTT按钮

//参数值
int Freq = 8750;//全局频率
int RSSI;//RSSI值
int progressValue;//进度条百分比
float voltage;//检测电压值

//布尔状态值
bool downButtonState;  // 当前按键状态
bool UpButtonState;
bool PTT;
int buttonState;         // 存储PTT按钮状态
int lastButtonState = HIGH; // 存储上一次按钮状态

//进度条的位置和大小
#define PROGRESS_BAR_X 10
#define PROGRESS_BAR_Y 42
#define PROGRESS_BAR_WIDTH 100
#define PROGRESS_BAR_HEIGHT 6

//如果有其它与电台使用相关的量可以在此设置



//第二部分：头文件引用
#include "RDA58xxTWI.h"
#include <Wire.h>
#define Wr_IIC 0x22 //rda5807的地址！
#define Rd_IIC (Wr_IIC + 1 )
//电台模块的头文件引用

#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h> //新增图形显示库显示场强进度条
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
//显示屏的头文件引用及设置

/*#include <avr/sleep.h> 
//睡眠模式的头文件*/

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
 const uint8_t PROGMEM kai[] = {0x00,0x00,0x7F,0xFC,0x08,0x20,0x08,0x20,0x08,0x20,0x08,0x20,0x08,0x20,0xFF,0xFE,0x08,0x20,0x08,0x20,0x08,0x20,0x08,0x20,0x10,0x20,0x10,0x20,0x20,0x20,0x40,0x20};
 const uint8_t PROGMEM guan[] = {0x10,0x10,0x08,0x10,0x08,0x20,0x00,0x00,0x3F,0xF8,0x01,0x00,0x01,0x00,0x01,0x00,0xFF,0xFE,0x01,0x00,0x02,0x80,0x02,0x80,0x04,0x40,0x08,0x20,0x30,0x18,0xC0,0x06};
 const uint8_t PROGMEM ya[] = {0x00,0x00,0x3F,0xFE,0x20,0x00,0x20,0x80,0x20,0x80,0x20,0x80,0x20,0x80,0x2F,0xFC,0x20,0x80,0x20,0x80,0x20,0x90,0x20,0x88,0x20,0x88,0x40,0x80,0x5F,0xFE,0x80,0x00};
 const uint8_t PROGMEM di[] = {0x08,0x08,0x08,0x3C,0x0B,0xE0,0x12,0x20,0x12,0x20,0x32,0x20,0x32,0x20,0x53,0xFE,0x92,0x20,0x12,0x10,0x12,0x10,0x12,0x12,0x12,0x0A,0x12,0x8A,0x13,0x26,0x12,0x12};
 const uint8_t PROGMEM ji2[] = {0x00,0x00,0x7E,0x7C,0x42,0x44,0x42,0x44,0x7E,0x44,0x42,0x44,0x42,0x44,0x7E,0x44,0x40,0x44,0x48,0x44,0x44,0x54,0x4A,0x48,0x52,0x40,0x60,0x40,0x00,0x40,0x00,0x40};
 const uint8_t PROGMEM jiang[] = {0x08,0x80,0x08,0xF8,0x09,0x08,0x4A,0x10,0x28,0xA0,0x28,0x40,0x08,0x90,0x0B,0x10,0x18,0x10,0x2B,0xFE,0xC8,0x10,0x09,0x10,0x08,0x90,0x08,0x10,0x08,0x50,0x08,0x20};
const uint8_t PROGMEM ji1[] = {0x10,0x00,0x11,0xF0,0x11,0x10,0x11,0x10,0xFD,0x10,0x11,0x10,0x31,0x10,0x39,0x10,0x55,0x10,0x55,0x10,0x91,0x10,0x11,0x12,0x11,0x12,0x12,0x12,0x12,0x0E,0x14,0x00};



//第四部分：功能函数实现
//电台模块函数(十分重要!!!勿删!!!)
void init_XCVR(void){
    WriteTo_58xx(0x02,0b1101001000000101);   //初始化芯片(必须有)
    Rda5820RxMode();//初始为接收模式
    Rda5820BandSet(2);//解锁76-108MHz
    Rda5820SpaceSet(2);//步进频率设置（100kHz）
    Rda5820FreqSet(Freq);//初始默认频率87.5MHz
    Rda5820VolSet(4);//设置音量0-15
    Rda5820TxPagSet(63);//发射功率0-63
    Rda5820TxAdcSet(1);//输入ADC增益设置0-3
    Rda5820TxPgaSet(7);//输入PGA增益设置0-7
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
}//设置音量ok

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
    temp=ReadFrom_58xx(0X41); //读取0X42的内容
    temp&=0X01C0;
    temp|=gain;   //GAIN
    WriteTo_58xx(0X41,temp) ; //设置PA的功率
}//设置TX发射功率

void Rda5820TxAdcSet(byte gain)//gain:0~3
{
    int temp;
  //  temp=ReadFrom_58xx(0X68); //读取0X42的内容
    temp=0X00F0;
    temp|=gain<<8;    //GAIN
    WriteTo_58xx(0X68,temp) ; //设置PGA
}//设置TX 输入信号ADC增益 

void Rda5820TxPgaSet(byte gain)//gain:0~7
{
    int temp;
    temp=ReadFrom_58xx(0X68); //读取0X42的内容
    temp|=gain<<10;  //GAIN
    WriteTo_58xx(0X68,temp) ; //设置PGA
}//设置TX 输入信号PGA增益 

void Rda5820BandSet(byte band)//band:0,87~108Mhz;1,76~91Mhz;2,76~108Mhz;3,用户自定义(53H~54H(实测无用))
{
    int temp;
    temp=ReadFrom_58xx(0X03); //读取0X03的内容
    temp&=0XFFF3;
    temp|=band<<2;     
    WriteTo_58xx(0X03,temp) ; //设置BAND
}//设置RDA5820的工作频段

void Rda5820SpaceSet(byte spc)//band:0,100Khz;1,200Khz;3,50Khz;3,保留
{
    int temp;
    temp=ReadFrom_58xx(0X03); //读取0X03的内容
    temp&=0XFFFC;
    temp|=spc;     
    WriteTo_58xx(0X03,temp) ; //设置BAND
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
    temp=ReadFrom_58xx(0x02); //读取0X03的内容
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
}//FM接收模式(最终在设置的时候要用这个!!!不要用上面那个)

void TX_Fm(int Freq)
{
   Rda5820TxMode();//发射模式
   Rda5820FreqSet(Freq);//设置FM频率
}//FM发射模式(最终在设置的时候要用这个!!!不要用上面那个)



//控制方面函数
void readState(void){
   downButtonState = digitalRead(downButton);  
   UpButtonState = digitalRead(upButton);
   PTT = digitalRead(PTTbutton);
   buttonState = digitalRead(PTTbutton);
}// 读取当前按键状态

void handleFreqChange(void){
   if (downButtonState == LOW && Freq >= 7600)  { 
     Freq-=10;//短按频率减按钮频率减0.1MHz（10kHz）
     Rda5820FreqSet(Freq);//变化后立即设置频率
   }
   //频率加按键按下时
   if (UpButtonState == LOW && Freq < 10800) {
     Freq+=10;//短按频率加按钮频率加0.1MHz（10kHz）
     Rda5820FreqSet(Freq);//变化后立即设置频率
   }
    
}//处理频率加减

void voltageRead(void){
  static unsigned long lastVoltRead;
  if (millis() - lastVoltRead > 1000) {  // 每秒读一次电压
    voltage = analogRead(A0) * (5.0 / 1023.0);  // 直接浮点计算
    lastVoltRead = millis();
  }
}// 电压读取（优化计算）1.7改进

void PTTcontrol(void){
  if (buttonState!= lastButtonState) {// 如果按钮状态发生变化
    if (buttonState == LOW) {
      TX_Fm(Freq);
    } else {
      RX_Fm(Freq);
    }
    lastButtonState = buttonState;//重新复位last防止重复执行
  }
}//PTT按钮控制发射状态

void init_screen(void){
 if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);//如果显示屏初始化失败就死循环
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
    display.print((float)Freq/100);  
	  //显示频率
    display.setCursor(90, 30);
    display.setTextSize(1);
    display.print("MHz");
    //显示单位兆赫兹
}//频率值及单位显示

void displayTopTitle(void){
    display.setCursor(110, 0);
    display.setTextSize(1);
    display.print("VFO");
  //上标(测试版使用VFO“频率振荡器”，正式版则用CLG标)
    display.drawBitmap(0, 0, chao, 16, 16, WHITE);
    display.drawBitmap(17, 0, duan, 16, 16, WHITE);
    display.drawBitmap(33, 0, bo, 16, 16, WHITE);
    display.drawBitmap(49, 0, dian, 16, 16, WHITE);
    display.drawBitmap(65, 0, tai, 16, 16, WHITE);
  //显示“超短波电台”
}//顶部标题显示

void displayVoltageValue(void){
    display.setCursor(0, 55);
    display.print(voltage);
    display.println("V");
} //显示电压

void displayRSSIValue(void){
  RSSI = Rda5820RssiGet();
  display.setCursor(35, 55);
    display.print("RSSI ");
    display.println(RSSI);
} //显示RSSI值

void displayProgressBar(void){
  progressValue = map(RSSI, 0, 220, 0, 100);
  // 绘制进度条边框
   display.drawRect(PROGRESS_BAR_X, PROGRESS_BAR_Y, PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEIGHT, SSD1306_WHITE);
   // 计算进度条的填充长度
   int fillWidth = (progressValue * PROGRESS_BAR_WIDTH) / 100;
   // 绘制进度条填充部分
   display.fillRect(PROGRESS_BAR_X, PROGRESS_BAR_Y, fillWidth, PROGRESS_BAR_HEIGHT, SSD1306_WHITE);
   // 显示当前进度值
   display.setCursor(PROGRESS_BAR_X, PROGRESS_BAR_Y + PROGRESS_BAR_HEIGHT + 5);
} //显示进度条

void displayInitCaption(void){
    display.drawBitmap(17, 0, chao, 16, 16, WHITE);
    display.drawBitmap(33, 0, duan, 16, 16, WHITE);
    display.drawBitmap(49, 0, bo, 16, 16, WHITE);
    display.drawBitmap(65, 0, dian, 16, 16, WHITE);
    display.drawBitmap(81, 0, tai, 16, 16, WHITE);
    display.setCursor(17, 25);
    display.setTextSize(2);
    display.print("V1.9");  
    display.display();
    delay(3000); // 显示3秒
}// 显示初始化字幕

void setup()
{
  Serial.begin(9600); 
  //开启串口通信
  pinMode(downButton, INPUT_PULLUP); 
   pinMode(upButton, INPUT_PULLUP); 
   pinMode(PTTbutton, INPUT_PULLUP);
   pinMode(A0, INPUT_PULLUP);
  // 设置引脚为输入上拉模式
  init_XCVR();
  //电台初始化设置
  init_screen();
  //屏幕初始化设置
  displayInitCaption();
  //初始化信息显示
  display.clearDisplay();
  display.setCursor(0, 0);
  //再次清屏，准备进入正常显示流程
  wdt_enable(WDTO_2S);
  //启用看门狗，超时设置2s

  /*set_sleep_mode (SLEEP_MODE_PWR_DOWN); 
  // 设置“Power-down”睡眠模式*/
  //源码保留
}

void loop()
{
 wdt_reset();//喂狗
 readState();
 handleFreqChange();
 PTTcontrol();
 voltageRead();

 //启用显示
 displayTopTitle();//顶标题
 displayFreqValue();//频率显示 
 displayProgressBar();//进度条
 displayVoltageValue();//电压值
 displayPTT();//PTT状态
 displayRSSIValue();//RSSI值
 display.display();
 display.clearDisplay();
 //循环结束后仅清一次屏

 /*//启用睡眠模式（低电压自动关机）（保留源码）
    
  if(voltage <= 3.50){
    display.clearDisplay();//清屏
   
    display.drawBitmap(40, 17, guan, 16, 16, WHITE);
    display.drawBitmap(80, 17, ji1, 16, 16, WHITE);
    display.display();
    //显示“关机”
    delay(2000);
    display.ssd1306_command(SSD1306_DISPLAYOFF);//关闭显示屏
    Rda5820MuteSet(0);//关闭电台模块声音输出，以节省电能
    wdt_disable();//关闭看门狗
    delay(500);
    sleep_enable();// 启动睡眠模式 
    sleep_cpu ();  // 进入睡眠模式
  }*/
 
}