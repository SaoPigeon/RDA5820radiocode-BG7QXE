/*1.7更新内容：接收和发射频率分为两个通道独立设置，优化了改变频率的时候频繁擦写寄存器的bug，
但是需要按下PTT按钮时才能更新模块寄存器频率，优化了电压检测，每秒读取一次电压*/

  //按钮配置
   const int downButton = 2;  // 频率减按钮
   const int upButton = 3;//频率加按钮
  const int PTTbutton = 4;//PTT按钮



  //全局变量
   int Freq = 8750;//接收频率
   int TXFreq = 8850;//发射频率
   int readFreq;//读取的频率
   int RSSI;//RSSI值
  float voltage;//检测电压值

  //按键状态
   bool downButtonState;  // 当前按键状态
   bool UpButtonState;
   bool PTT;
   int buttonState;         // 存储PTT按钮状态
  int lastButtonState = HIGH; // 存储上一次按钮状态




 //关于发射接收频率切换的状态切换的相关变量
   unsigned long pressStartTime = 0;// 记录按钮同时按下的开始时间
   const unsigned long delayTime = 2000;// 定义延时时间（毫秒）
   bool TXstate = false;// 设置发射接收频率的状态切换，为真时设置发射频率，为假时设置接收频率
   bool bothButtonsPressed = false;// 记录按钮是否同时按下


 //如果有其它与电台使用相关的量可以在此设置

  //汉字数据
    byte chao[] = {0x08,0x00,0x09,0xFC,0x08,0x44,0x7E,0x44,0x08,0x44,0x08,0x94,0xFF,0x08,0x08,0xFC,0x28,0x84,0x28,0x84,0x2E,0x84,0x28,0xFC,0x28,0x00,0x58,0x00,0x4F,0xFE,0x80,0x00};
    byte duan[] = {0x20,0x00,0x21,0xFE,0x3C,0x00,0x50,0x00,0x90,0xFC,0x10,0x84,0x10,0x84,0xFE,0x84,0x10,0xFC,0x10,0x00,0x10,0x84,0x28,0x44,0x24,0x48,0x44,0x00,0x41,0xFE,0x80,0x00};
    byte bo[] = {0x00,0x20,0x20,0x20,0x10,0x20,0x13,0xFE,0x82,0x22,0x42,0x24,0x4A,0x20,0x0B,0xFC,0x12,0x84,0x12,0x88,0xE2,0x48,0x22,0x50,0x22,0x20,0x24,0x50,0x24,0x88,0x09,0x06};
    byte dian[] = {0x01,0x00,0x01,0x00,0x01,0x00,0x3F,0xF8,0x21,0x08,0x21,0x08,0x21,0x08,0x3F,0xF8,0x21,0x08,0x21,0x08,0x21,0x08,0x3F,0xF8,0x21,0x0A,0x01,0x02,0x01,0x02,0x00,0xFE};
    byte tai[] = {0x02,0x00,0x02,0x00,0x04,0x00,0x08,0x20,0x10,0x10,0x20,0x08,0x7F,0xFC,0x20,0x04,0x00,0x00,0x1F,0xF0,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x1F,0xF0,0x10,0x10};
    byte guan[] = {0x10,0x10,0x08,0x10,0x08,0x20,0x00,0x00,0x3F,0xF8,0x01,0x00,0x01,0x00,0x01,0x00,0xFF,0xFE,0x01,0x00,0x02,0x80,0x02,0x80,0x04,0x40,0x08,0x20,0x30,0x18,0xC0,0x06};
    byte ji1[] = {0x10,0x00,0x11,0xF0,0x11,0x10,0x11,0x10,0xFD,0x10,0x11,0x10,0x31,0x10,0x39,0x10,0x55,0x10,0x55,0x10,0x91,0x10,0x11,0x12,0x11,0x12,0x12,0x12,0x12,0x0E,0x14,0x00};
   

//头文件部分
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


  #include <avr/sleep.h> 
  //睡眠模式的头文件


  #include <avr/wdt.h>
//看门狗的头文件


void setup() {

   pinMode(downButton, INPUT_PULLUP); 
   pinMode(upButton, INPUT_PULLUP); 
   pinMode(PTTbutton, INPUT_PULLUP);
   pinMode(A0, INPUT_PULLUP);
   // 设置引脚为输入上拉模式

  Serial.begin(9600); 
    //开启串口通信

  set_sleep_mode (SLEEP_MODE_PWR_DOWN); 
  // 设置“Power-down”睡眠模式


   WriteTo_58xx(0x02,0b1101001000000101);   //初始化芯片
   Rda5820BandSet(2);//解锁76-108MHz
   RX_Fm(Freq); 
    //以上是电台模块的初始化设置



 if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);//如果显示屏初始化失败就死循环
    }
  display.setTextColor(WHITE);
  display.clearDisplay();
    //以上是显示屏的初始化设置



    // 显示初始化字幕
    display.drawBitmap(17, 0, chao, 16, 16, WHITE);
    display.drawBitmap(33, 0, duan, 16, 16, WHITE);
    display.drawBitmap(49, 0, bo, 16, 16, WHITE);
    display.drawBitmap(65, 0, dian, 16, 16, WHITE);
    display.drawBitmap(81, 0, tai, 16, 16, WHITE);
    display.setCursor(17, 25);
    display.setTextSize(2);
    display.print("V1.7");  
  display.display();
  delay(3000); // 显示3秒

  // 再次清屏，准备进入正常显示流程
  display.clearDisplay();
  display.setCursor(0, 0);

  
  wdt_enable(WDTO_2S);
   //启用看门狗，超时设置2s


}

void loop() {

  //读取按键状态
   downButtonState = digitalRead(downButton);  // 读取当前按键状态
   UpButtonState = digitalRead(upButton);
   PTT = digitalRead(PTTbutton);
   buttonState = digitalRead(PTTbutton); // 读取PTT按钮状态

   
  //切换发射接收频率状态
  if (downButtonState == LOW && UpButtonState == LOW) {
    if (!bothButtonsPressed) {
      // 记录按钮同时按下的开始时间
      pressStartTime = millis();
      bothButtonsPressed = true;
     } else if (millis() - pressStartTime >= delayTime) {
      // 2秒后切换状态
      TXstate = !TXstate;
      bothButtonsPressed = false;
     }
     } else {
     // 按钮未同时按下，重置状态
     bothButtonsPressed = false;
    }



   // 根据当前状态执行的操作（非常重要！！！）
   if (TXstate) {
        // 状态为真时的操作
       // 频率减按键松开时
        if (downButtonState == HIGH  && TXFreq >= 7600)  { 
        TXFreq-=10;//短按频率减按钮频率减0.1MHz（10kHz）
        }
 
      //频率加按键松开时
      if (UpButtonState == HIGH && TXFreq < 10800) {
      TXFreq+=10;//短按频率加按钮频率加0.1MHz（10kHz）
      } 
    } else {
      // 状态为假时的操作
  
       // 频率减按键松开时
       if (downButtonState == HIGH  && Freq >= 7600)  { 
        Freq-=10;//短按频率减按钮频率减0.1MHz（10kHz）
        }
 
      //频率加按键松开时
       if (UpButtonState == HIGH && Freq < 10800) {
       Freq+=10;//短按频率加按钮频率加0.1MHz（10kHz）
       } 
  }

  

  //PTT按钮控制发射状态（同时把设置的频率写入模块寄存器）
  if (buttonState != lastButtonState) {
    if (buttonState == LOW) {
      TX_Fm(TXFreq);
    } else {
      RX_Fm(Freq);
    }
    lastButtonState = buttonState;//重新复位last防止重复执行
  }



 // 电压读取（优化计算）
  static unsigned long lastVoltRead;
  if (millis() - lastVoltRead > 1000) {  // 每秒读一次电压
    voltage = analogRead(A0) * (5.0 / 1023.0);  // 直接浮点计算
    lastVoltRead = millis();
  }



  //显示部分程序

  if (PTT == LOW){
     display.setCursor(114, 55);
     display.setTextSize(1);
     display.print("TX");
     
   }else{
    display.setCursor(114, 55);
     display.setTextSize(1);
    display.print("RX");
  }//显示发射状态




 //显示参数的程序
 float displayFreq = (float)Freq/100;
  display.setCursor(20, 10);
    display.setTextSize(2);
    display.print(displayFreq);  
	  //显示接收频率
 float displayTXFreq = (float)TXFreq/100;
  display.setCursor(20, 33);
    display.setTextSize(2);
    display.print(displayTXFreq);
    //显示发射频率
    
 //显示频率状态
 if(TXstate){
    display.setCursor(2, 33);
    display.setTextSize(2);
    display.print(">");
  }else{
    display.setCursor(2, 10);
    display.setTextSize(2);
    display.print(">");
  }

  display.setCursor(95, 20);
    display.setTextSize(1);
    display.print("RXMHz");
  display.setCursor(95, 40);
    display.print("TXMHz");
    //显示单位兆赫兹

  display.setCursor(110, 0);
    display.setTextSize(1);
    display.print("VFO");
  //上标(测试版使用VFO“频率振荡器”，正式版则用CLG标)
  
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



    //启用睡眠模式（低电压自动关机）
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
  }

 

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
//mute:0,不静音;1,静音
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


void RX_Fm(int F)//FM接收模式
{
   Rda5820RxMode();//接收模式
   Rda5820VolSet(4);//设置音量0-15
   Rda5820FreqSet(F);//设置FM频率
}

void TX_Fm(int F)//FM发送模式
{
   Rda5820TxMode();//发射模式
   Rda5820TxPagSet(63);//发射功率0-63
   Rda5820TxAdcSet(1);//输入ADC增益设置0-3
   Rda5820TxPgaSet(7);//输入PGA增益设置0-7
   Rda5820FreqSet(F);//设置FM频率
}