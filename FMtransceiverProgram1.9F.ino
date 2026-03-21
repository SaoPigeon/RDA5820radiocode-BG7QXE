/*1.9F外贸版
仅支持基础功能，87~108MHz收发，RSSI，电压显示，无进度条显示，无静噪，有看门狗
无“超短波电台”标题显示，发射功率上限锁定为30
去除冗余函数功能，精简运行，确保稳定
删除睡眠模式源码，无法复原睡眠模式
更新日期：2026年3月7日
*/


//第一部分：参数声明
//按钮定义
const int downButton = 2;  // 频率减按钮
const int upButton = 3;//频率加按钮
const int PTTbutton = 4;//PTT按钮

//参数值
int Freq = 8750;//全局频率
int RSSI;//RSSI值
float voltage;//检测电压值

//布尔状态值
bool downButtonState;  // 当前按键状态
bool UpButtonState;
bool PTT;
int buttonState;         // 存储PTT按钮状态
int lastButtonState = HIGH; // 存储上一次按钮状态


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

#include <avr/wdt.h>
//看门狗的头文件



//第三部分：汉字字库
//PROGMEM配置，将汉字数据放置在闪存中
#include <avr/pgmspace.h>
 const uint8_t PROGMEM signalTower[] = {0x00,0x00,0x20,0x04,0x48,0x12,0x93,0xC9,0x97,0xE9,0x93,0xC9,0x4A,0x52,0x22,0x44,0x04,0x20,0x04,0x20,0x06,0x60,0x09,0x90,0x09,0x90,0x0E,0x70,0x10,0x08,0x7F,0xFE};
 const uint8_t PROGMEM wai[] = {0x10,0x40,0x10,0x40,0x10,0x40,0x10,0x40,0x3E,0x40,0x22,0x60,0x42,0x50,0x42,0x48,0xA4,0x44,0x14,0x44,0x08,0x40,0x08,0x40,0x10,0x40,0x20,0x40,0x40,0x40,0x80,0x40};
 const uint8_t PROGMEM mao[] = {0x06,0x00,0x78,0xFC,0x40,0x44,0x48,0x44,0x44,0x44,0x5A,0x94,0x61,0x08,0x00,0x00,0x1F,0xF0,0x10,0x10,0x11,0x10,0x11,0x10,0x11,0x10,0x02,0x60,0x0C,0x18,0x70,0x04};
 const uint8_t PROGMEM ban[] = {0x08,0x08,0x48,0x1C,0x49,0xE0,0x49,0x00,0x49,0x00,0x7D,0xFC,0x41,0x44,0x41,0x44,0x79,0x44,0x49,0x28,0x49,0x28,0x49,0x10,0x49,0x10,0x4A,0x28,0x4A,0x44,0x8C,0x82};

//第四部分：功能函数实现
//电台模块函数(十分重要!!!勿删!!!)
void init_XCVR(void){
    WriteTo_58xx(0x02,0b1101001000000101);   //初始化芯片(必须有)
    Rda5820RxMode();//初始为接收模式
    Rda5820BandSet(1);//锁定76-108MHz，只允许87-108MHz
    Rda5820SpaceSet(2);//步进频率设置（100kHz）
    Rda5820FreqSet(Freq);//初始默认频率87.5MHz
    Rda5820VolSet(3);//设置音量0-15
    Rda5820TxPagSet(30);//发射功率0-63
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
   if (downButtonState == LOW && Freq >= 8700)  { 
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
    display.setCursor(17, 25);
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
    display.drawBitmap(0, 0, signalTower, 16, 16, WHITE);
    display.setCursor(18, 8);
    display.setTextSize(1);
    display.print("FM RADIO");
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

void displayInitCaption(void){
    // 显示初始化字幕
    display.drawBitmap(33, 0, wai, 16, 16, WHITE);
    display.drawBitmap(49, 0, mao, 16, 16, WHITE);
    display.drawBitmap(65, 0, ban, 16, 16, WHITE);
    display.setCursor(17, 25);
    display.setTextSize(2);
    display.print("V1.9F");  
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
 displayVoltageValue();//电压值
 displayPTT();//PTT状态
 displayRSSIValue();//RSSI值
 display.display();
 display.clearDisplay();
 //循环结束后仅清一次屏

}