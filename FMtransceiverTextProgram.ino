//本程序为CLG—VHF数字电台的测试程序，由串口发送命令完成配置设置，PTT按钮控制发射
/*在上传本程序前，请务必仔细阅读本说明！！！
最新发现：
波段76~108MHz实测最高频率可达127.1MHz（因模块不同有差异）
可能有部分模块读取频率有问题，超过100MHz显示不可信
实测自定义步进频率最小50kHz，再小就无法设置，寄存器返回数据正常但是接收不正常
发射后需要重新设置频率否则无法发射*/
//更新日期：2025年6月17日
//更新日期：2025年6月30日


/*命令集（请在串口监视器输入以下命令，串口波特率设置9600）
F+频率：设置电台的频率值，单位10kHz，如F8750 设置频率为87.5MHz
R：读取当前的电台频率
S: 读取当前的信号强度（0~220）
K；自动向上搜索电台，并显示搜索到的频点值
V+音量：设置音量值（0~15），如V4
P+功率：设置发射功率（0~63），如P35
A+增益：设置音频输入的ADC增益值（0~3），如A3
G+增益：设置音频输入的PGA增益值（0~7），如G3
L+灵敏度：设置接收灵敏度（虽然我感觉没什么用）（0~127），如L120
Q：静噪（静音）
T；串口控制发射（按PTT可以解除）
*/


#include <RDA58xxTWI.h>
const int PTTbutton = 4;//PTT按钮
int buttonState;         // 存储PTT按钮状态
int lastButtonState = HIGH; // 存储上一次按钮状态
bool Squelch = false;//静噪状态

void setup(){
  Serial.begin(9600);     
  pinMode(PTTbutton, INPUT_PULLUP);//串口,按钮配置
 

 //电台初始化设置
    WriteTo_58xx(0x02,0b1101001000000101);   //初始化芯片
    //                  1101001000000101
    Rda5820BandSet(2);//解锁76-108MHz
    Rda5820SpaceSet(2);//步进频率
    Rda5820RxMode();//初始为接收模式
    Rda5820FreqSet(8750);//初始默认频率87.5MHz
    Rda5820VolSet(4);//设置音量0-15
    Rda5820TxPagSet(63);//发射功率0-63
    Rda5820TxAdcSet(1);//输入ADC增益设置0-3
    Rda5820TxPgaSet(7);//输入PGA增益设置0-7
  //电台模块的初始化设置

 
  Serial.println("初始化已完成");
  
}

void loop(){
  int receivedNumber;//频率
  buttonState = digitalRead(PTTbutton); // 读取PTT按钮状态

 // PTT控制收发状态
  if (buttonState!= lastButtonState) {
    if (buttonState == LOW) {
      Rda5820TxMode();
      Serial.println("TX");
    } else {
      Rda5820RxMode();
      Serial.println("RX");
    }
    lastButtonState = buttonState;//重新复位last防止重复执行
  }
 

 //串口输入命令
 // 检查串口是否有数据可读
  if (Serial.available() > 0) {
    // 读取命令类型（首字符）
    char commandType = Serial.read();
    
    // 根据命令类型执行不同的处理逻辑
    switch (commandType) {
      case 'F':  // 设置频率命令
        // 解析后面的数字部分
        
        receivedNumber = Serial.parseInt();
        // 清空缓冲区残留字符
        while (Serial.available()) Serial.read();

        // 校验数据范围（0~14999）
        if (receivedNumber >= 0 && receivedNumber < 15000) {
          Serial.print("检验输入频率 ");
          Serial.println(receivedNumber);
          delay(100);
          Rda5820FreqSet(receivedNumber);
          Serial.print("已成功设置频率: ");
          Serial.println(Rda5820FreqGet());
        } else {
          Serial.println("频率值超出范围 (0~14999)");
        }
        break;
        
      
      case 'R':  // 读取当前频率
        Serial.print("当前频率: ");
        Serial.println(Rda5820FreqGet());
        // 清空缓冲区
        while (Serial.available()) Serial.read();
        break;
        
      case 'S':  // 读取信号强度
        Serial.print("信号强度: ");
        Serial.println(Rda5820RssiGet());  // 假设存在此函数
        // 清空缓冲区
        while (Serial.available()) Serial.read();
        break;
        
      case 'K':  // 自动搜台
        Serial.println("启动自动搜台");
        RdaSeek();
        Serial.print("当前频率: ");
        Serial.println(Rda5820FreqGet());
        Serial.print("信号强度: ");
        Serial.println(Rda5820RssiGet());
        while (Serial.available()) Serial.read();// 清空缓冲区
        break;
      case 'V':  // 设置音量命令
        // 解析后面的数字部分
        int volume;//音量
        volume = Serial.parseInt();
        // 清空缓冲区残留字符
        while (Serial.available()) Serial.read();

        // 校验数据范围（0~15）
        if (volume >= 0 && volume < 16) {
          Serial.print("检验输入音量 ");
          Serial.println(volume);
          delay(100);
          Rda5820VolSet(volume);
          Serial.println("已成功设置音量 ");
        } else {
          Serial.println("音量值超出范围 (0~15)");
        }
        break;

      case 'P':  // 设置功率命令
        // 解析后面的数字部分
        int Power;//功率
        Power = Serial.parseInt();
        // 清空缓冲区残留字符
        while (Serial.available()) Serial.read();

        // 校验数据范围（0~63）
        if (Power >= 0 && Power < 64) {
          Serial.print("检验输入功率 ");
          Serial.println(Power);
          delay(100);
          Rda5820TxPagSet(Power);
          Serial.println("已成功设置功率 ");
          
        } else {
          Serial.println("功率值超出范围 (0~63)");
        }
        break;

      case 'A':  // 设置输入信号ADC增益命令
        // 解析后面的数字部分
        int ADCGain;//ADC增益
        ADCGain = Serial.parseInt();
        // 清空缓冲区残留字符
        while (Serial.available()) Serial.read();

        // 校验数据范围（0~3）
        if (ADCGain >= 0 && ADCGain < 4) {
          Serial.print("检验输入ADC ");
          Serial.println(ADCGain);
          delay(100);
          Rda5820TxAdcSet(ADCGain);
          Serial.println("已成功设置增益 ");
        } else {
          Serial.println("增益值超出范围 (0~3)");
        }
        break;

      case 'G':  // 设置输入信号PGA增益命令
        // 解析后面的数字部分
        int PGAGain;//pga增益
        PGAGain = Serial.parseInt();
        // 清空缓冲区残留字符
        while (Serial.available()) Serial.read();

        // 校验数据范围（0~7）
        if (PGAGain >= 0 && PGAGain < 8) {
          Serial.print("检验输入PGA ");
          Serial.println(PGAGain);
          delay(100);
          Rda5820TxPgaSet(PGAGain);
          Serial.println("已成功设置增益 ");
        } else {
          Serial.println("增益值超出范围 (0~7)");
        }
        break;

      case 'L':  // 设置输入信号灵敏度命令
        // 解析后面的数字部分
        int LMD;//灵敏度值
        LMD = Serial.parseInt();
        // 清空缓冲区残留字符
        while (Serial.available()) Serial.read();

        // 校验数据范围（0~127）
        if (LMD >= 0 && LMD < 128) {
          Serial.print("检验输入灵敏度值 ");
          Serial.println(LMD);
          delay(100);
          Rda5820RssiSet(LMD);
          Serial.println("已成功设置灵敏度 ");
        } else {
          Serial.println("灵敏度超出范围 (0~127)");
        }
        break;
      
      case 'Q':  // 设置静噪
        Squelch = !Squelch;
        if(Squelch) Rda5820MuteSet(0);//静噪
         else Rda5820MuteSet(1);//取消静噪
         Serial.print("静噪状态：");
         Serial.println(Squelch);
        while (Serial.available()) Serial.read();
        break;

      case 'T':  // 串口发射
       Rda5820TxMode();//发射模式
       Serial.println("TX");

        // 清空缓冲区残留字符
       while (Serial.available()) Serial.read();
       break;

      default:
        // 未知命令
        Serial.print("未知命令: ");
        Serial.println(commandType);
        // 清空缓冲区
        while (Serial.available()) Serial.read();
    }
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
//设置TX发送功率
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




