//1.5F外贸版本：没有睡眠模式，仅支持基础功能，87~108MHz收发，RSSI，电压显示，无进度条显示，无静噪，有看门狗

const int downButton = 3;  // 频率减按钮
const int upButton = 2;//频率加按钮
const int PTTbutton = 4;//PTT按钮

int Freq = 8750;//全局频率
int readFreq;//读取的频率
int RSSI;

float voltage;//检测电压值


bool downButtonState;  // 当前按键状态
bool UpButtonState;
bool PTT;



int buttonState;         // 存储PTT按钮状态
int lastButtonState = HIGH; // 存储上一次按钮状态

//与频率值有关的量

//如果有其它与电台使用相关的量可以在此设置

//汉字数据
    byte chao[] = {0x08,0x00,0x09,0xFC,0x08,0x44,0x7E,0x44,0x08,0x44,0x08,0x94,0xFF,0x08,0x08,0xFC,0x28,0x84,0x28,0x84,0x2E,0x84,0x28,0xFC,0x28,0x00,0x58,0x00,0x4F,0xFE,0x80,0x00};
    byte duan[] = {0x20,0x00,0x21,0xFE,0x3C,0x00,0x50,0x00,0x90,0xFC,0x10,0x84,0x10,0x84,0xFE,0x84,0x10,0xFC,0x10,0x00,0x10,0x84,0x28,0x44,0x24,0x48,0x44,0x00,0x41,0xFE,0x80,0x00};
    byte bo[] = {0x00,0x20,0x20,0x20,0x10,0x20,0x13,0xFE,0x82,0x22,0x42,0x24,0x4A,0x20,0x0B,0xFC,0x12,0x84,0x12,0x88,0xE2,0x48,0x22,0x50,0x22,0x20,0x24,0x50,0x24,0x88,0x09,0x06};
    byte dian[] = {0x01,0x00,0x01,0x00,0x01,0x00,0x3F,0xF8,0x21,0x08,0x21,0x08,0x21,0x08,0x3F,0xF8,0x21,0x08,0x21,0x08,0x21,0x08,0x3F,0xF8,0x21,0x0A,0x01,0x02,0x01,0x02,0x00,0xFE};
    byte tai[] = {0x02,0x00,0x02,0x00,0x04,0x00,0x08,0x20,0x10,0x10,0x20,0x08,0x7F,0xFC,0x20,0x04,0x00,0x00,0x1F,0xF0,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x1F,0xF0,0x10,0x10};
    byte wai[] = {0x10,0x40,0x10,0x40,0x10,0x40,0x10,0x40,0x3E,0x40,0x22,0x60,0x42,0x50,0x42,0x48,0xA4,0x44,0x14,0x44,0x08,0x40,0x08,0x40,0x10,0x40,0x20,0x40,0x40,0x40,0x80,0x40};
    byte mao[] = {0x06,0x00,0x78,0xFC,0x40,0x44,0x48,0x44,0x44,0x44,0x5A,0x94,0x61,0x08,0x00,0x00,0x1F,0xF0,0x10,0x10,0x11,0x10,0x11,0x10,0x11,0x10,0x02,0x60,0x0C,0x18,0x70,0x04};
    byte ban[] = {0x08,0x08,0x48,0x1C,0x49,0xE0,0x49,0x00,0x49,0x00,0x7D,0xFC,0x41,0x44,0x41,0x44,0x79,0x44,0x49,0x28,0x49,0x28,0x49,0x10,0x49,0x10,0x4A,0x28,0x4A,0x44,0x8C,0x82};
    byte ben[] = {0x01,0x00,0x01,0x00,0x01,0x00,0x01,0x00,0x7F,0xFC,0x03,0x80,0x05,0x40,0x05,0x40,0x09,0x20,0x11,0x10,0x21,0x08,0x4F,0xE4,0x81,0x02,0x01,0x00,0x01,0x00,0x01,0x00};




#include "RDA58xxTWI.h"
#include <Wire.h>
#define Wr_IIC 0x22 //rda5807的地址！
#define Rd_IIC (Wr_IIC + 1 )
//电台模块的头文件引用


#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
//显示屏的头文件引用及设置


#include <avr/wdt.h>
//看门狗的头文件


void setup() {

   pinMode(downButton, INPUT_PULLUP); 
   pinMode(upButton, INPUT_PULLUP); 
   pinMode(PTTbutton, INPUT_PULLUP);
   pinMode(A0, INPUT_PULLUP);
   // 设置引脚为输入上拉模式

 if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);//如果显示屏初始化失败就死循环
    }
  display.setTextColor(WHITE);
  display.clearDisplay();
    //以上是显示屏的初始化设置



    
  Serial.begin(9600); 
    //开启串口通信


    // 显示初始化字幕
    display.drawBitmap(33, 0, wai, 16, 16, WHITE);
    display.drawBitmap(49, 0, mao, 16, 16, WHITE);
    display.drawBitmap(65, 0, ban, 16, 16, WHITE);
    
    display.setCursor(17, 25);
    display.setTextSize(2);
    display.print("V1.5F");
    
  display.display();
  delay(3000); // 显示3秒

  // 再次清屏，准备进入正常显示流程
  display.clearDisplay();
  display.setCursor(0, 0);



 

   WriteTo_58xx(0x02,0b1101001000000101);   //初始化芯片
   Rda5820BandSet(2);//解锁76-108MHz
   RX_Fm(); 
    //以上是电台模块的初始化设置
    
   wdt_enable(WDTO_2S);
   //启用看门狗，超时设置2s

}

void loop() {

  if (Freq != readFreq){
   Rda5820FreqSet(Freq);  //当按动按钮改变频率时重新给电台设置频率
  }  
 

  //以下是控制频率加减的程序
  downButtonState = digitalRead(downButton);  // 读取当前按键状态
  UpButtonState = digitalRead(upButton);
  PTT = digitalRead(PTTbutton);
  buttonState = digitalRead(PTTbutton); // 读取PTT按钮状态




  //以下是读取电压的程序
  int volt = analogRead(A0);//从A0引脚读取电压
  int volta = map(volt, 0, 1023, 0, 500);
  voltage = (float)volta/100;


  
  //PTT按钮控制发射状态
  // 如果按钮状态发生变化
  if (buttonState!= lastButtonState) {
    if (buttonState == LOW) {
      TX_Fm();
    } else {
      RX_Fm();
    }
    lastButtonState = buttonState;//重新复位last防止重复执行
  }



  // 频率减按键松开时
  if (downButtonState == HIGH  && Freq >= 8700)  { 
    Freq-=10;//短按频率减按钮频率减0.1MHz（10kHz）
  }
 
  //频率加按键松开时
  if (UpButtonState == HIGH && Freq < 10800) {
    Freq+=10;//短按频率加按钮频率加0.1MHz（10kHz）
  } 





  
 //从模块读取频率并保存在随机存储器的程序
 readFreq = Rda5820FreqGet();
  
  //显示部分程序

 if (PTT == LOW){
    display.setCursor(115, 55);
    display.setTextSize(1);
    display.print("TX");
    
    
  }else{
    display.setCursor(115, 55);
    display.setTextSize(1);
    display.print("RX");
  }//显示发射状态




 //显示参数的程序
 float displayFreq = (float)readFreq/100;
  display.setCursor(17, 25);
    display.setTextSize(2);
    display.print(displayFreq);  
	  //显示频率
  display.setCursor(90, 31);
    display.setTextSize(1);
    display.print("MHz");
    //显示单位兆赫兹

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

  display.setCursor(0, 55);
    display.print(voltage);
    display.println("V");
    //显示电压
  
   RSSI = Rda5820RssiGet();
  display.setCursor(35, 55);
    display.print("RSSI ");
    display.println(RSSI);
    //显示RSSI值


  
 //启用显示
  display.display();

 //循环结束后仅清一次屏
  display.clearDisplay();
  wdt_reset(); // 喂狗
}


//以下是电台模块的函数库（非常重要！）

//设置RDA5820为RX模式
void Rda5820RxMode(void)
{
    int temp;
    temp=ReadFrom_58xx(0X40); //读取0X40的内容
    temp&=0xfff0; //RX 模式   
    WriteTo_58xx(0X40,temp) ; //FM RX模式 
}

//设置RDA5820为TX模式
void Rda5820TxMode(void)
{
    int temp;
    temp=ReadFrom_58xx(0X40); //读取0X40的内容 
    temp&=0xfff0;
    temp|=0x0001;    //TX 模式
    WriteTo_58xx(0X40,temp) ; //FM TM 模式 
}


//得到信号强度
//返回值范围:0~127
byte Rda5820RssiGet(void)
{
    int temp;
    temp=ReadFrom_58xx(0X0B); //读取0X0B的内容
    return temp>>9;                 //返回信号强度
}

//设置音量ok
//vol:0~15;
void Rda5820VolSet(byte vol)
{
    int temp;
    temp=ReadFrom_58xx(0X05); //读取0X05的内容
    temp&=0XFFF0;
    temp|=vol&0X0F;    
    WriteTo_58xx(0X05,temp) ; //设置音量
}

//静音设置
//mute:0,静音;1,不静音
void Rda5820MuteSet(byte mute)
{
    int temp;
    temp=ReadFrom_58xx(0X02); //读取0X02的内容
    if(mute)temp|=1<<14;
    else temp&=~(1<<14);       
    WriteTo_58xx(0X02,temp) ; //设置MUTE
}

//设置灵敏度
//rssi:0~127;
void Rda5820RssiSet(byte rssi)
{
    int temp;
    temp=ReadFrom_58xx(0X05); //读取0X05的内容
    temp&=0X80FF;
    temp|=(u16)rssi<<8;    
    WriteTo_58xx(0X05,temp) ; //设置RSSI
}
//设置TX发送功率() 
//gain:0~63
void Rda5820TxPagSet(byte gain)
{
    int temp;
    temp=ReadFrom_58xx(0X41); //读取0X42的内容
    temp&=0X01C0;
    temp|=gain;   //GAIN
    WriteTo_58xx(0X41,temp) ; //设置PA的功率
}

//设置TX 输入信号ADC增益 
//gain:0~3
void Rda5820TxAdcSet(byte gain)
{
    int temp;
  //  temp=ReadFrom_58xx(0X68); //读取0X42的内容
    temp=0X00F0;
    temp|=gain<<8;    //GAIN
    WriteTo_58xx(0X68,temp) ; //设置PGA
}

//设置TX 输入信号PGA增益 
//gain:0~7
void Rda5820TxPgaSet(byte gain)
{
    int temp;
    temp=ReadFrom_58xx(0X68); //读取0X42的内容
    temp|=gain<<10;  //GAIN
    WriteTo_58xx(0X68,temp) ; //设置PGA
}

//设置RDA5820的工作频段
//band:0,87~108Mhz;1,76~91Mhz;2,76~108Mhz;3,用户自定义(53H~54H)
void Rda5820BandSet(byte band)
{
    int temp;
    temp=ReadFrom_58xx(0X03); //读取0X03的内容
    temp&=0XFFF3;
    temp|=band<<2;     
    WriteTo_58xx(0X03,temp) ; //设置BAND
}


//设置RDA5820的步进频率
//band:0,100Khz;1,200Khz;3,50Khz;3,保留
void Rda5820SpaceSet(byte spc)
{
    int temp;
    temp=ReadFrom_58xx(0X03); //读取0X03的内容
    temp&=0XFFFC;
    temp|=spc;     
    WriteTo_58xx(0X03,temp) ; //设置BAND
}


//设置RDA5820的频率
//freq:频率值(单位为10Khz),比如10805,表示108.05Mhz
void Rda5820FreqSet(int freq)
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
}
//得到当前频率
//返回值:频率值(单位10Khz)
int Rda5820FreqGet(void)
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
}

void RdaSeek(void)//自动搜台
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
}


void RX_Fm()//FM接收模式
{
   Rda5820RxMode();//接收模式
   Rda5820VolSet(4);//设置音量0-15
   Rda5820FreqSet(Freq);//设置FM频率
}

void TX_Fm()//FM发送模式
{
   Rda5820TxMode();//发射模式
   Rda5820TxPagSet(63);//发射功率0-63
   Rda5820TxAdcSet(1);//输入ADC增益设置0-3
   Rda5820TxPgaSet(7);//输入PGA增益设置0-7
   Rda5820FreqSet(Freq);//设置FM频率
}