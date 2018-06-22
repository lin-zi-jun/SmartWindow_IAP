#include "ili9325.h"
#include <stdlib.h>
#include <stdarg.h>
#include "font.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_fsmc.h"
#include "common.h"


//LCD的画笔颜色和背景色	   
u16 POINT_COLOR=0X2B12;	//画笔颜色
u16 BACK_COLOR=0X8430;  //背景色 

//管理LCD重要参数
//默认为竖屏
_lcd_dev lcddev;
	
		   
//写寄存器函数
//regval:寄存器值
void LCD_WR_REG(u16 regval)
{ 
	myLCD->LCD_REG=regval;//写入要写的寄存器序号	 
}
//写LCD数据
//data:要写入的值
void LCD_WR_DATA(u16 data)
{										    	   
	myLCD->LCD_RAM=data;		 
}
//读LCD数据
//返回值:读到的值
u16 LCD_RD_DATA(void)
{										    	   
	return myLCD->LCD_RAM;		 
}					   
//写寄存器
//LCD_Reg:寄存器地址
//LCD_RegValue:要写入的数据
void LCD_WriteReg(u8 LCD_Reg, u16 LCD_RegValue)
{	
	myLCD->LCD_REG = LCD_Reg;		//写入要写的寄存器序号	 
	myLCD->LCD_RAM = LCD_RegValue;//写入数据	    		 
}	   
//读寄存器
//LCD_Reg:寄存器地址
//返回值:读到的数据
u16 LCD_ReadReg(u8 LCD_Reg)
{										   
	LCD_WR_REG(LCD_Reg);		//写入要读的寄存器序号
	delay_us(5);		  
	return LCD_RD_DATA();		//返回读到的值
}   
//开始写GRAM
void LCD_WriteRAM_Prepare(void)
{
 	myLCD->LCD_REG=lcddev.wramcmd;	  
}	 
//LCD写GRAM
//RGB_Code:颜色值
void LCD_WriteRAM(u16 RGB_Code)
{							    
	myLCD->LCD_RAM = RGB_Code;//写十六位GRAM
}
//从ILI93xx读出的数据为GBR格式，而我们写入的时候为RGB格式。
//通过该函数转换
//c:GBR格式的颜色值
//返回值：RGB格式的颜色值
u16 LCD_BGR2RGB(u16 c)
{
	u16  r,g,b,rgb;   
	b=(c>>0)&0x1f;
	g=(c>>5)&0x3f;
	r=(c>>11)&0x1f;	 
	rgb=(b<<11)+(g<<5)+(r<<0);		 
	return(rgb);
} 
//当mdk -O1时间优化时需要设置
//延时i
void opt_delay(u8 i)
{
	while(i--);
}
//读取个某点的颜色值	 
//x,y:坐标
//返回值:此点的颜色
u16 LCD_ReadPoint(u16 x,u16 y)
{
 	u16 r=0,g=0,b=0;
	if(x>=lcddev.width||y>=lcddev.height)return 0;	//超过了范围,直接返回		   
	LCD_SetCursor(x,y);	    
	if(lcddev.id==0X9341||lcddev.id==0X6804)LCD_WR_REG(0X2E);//9341/6804 发送读GRAM指令
	else LCD_WR_REG(R34);      		 				//其他IC发送读GRAM指令
 	if(lcddev.id==0X9320)opt_delay(2);				//FOR 9320,延时2us	    
	if(myLCD->LCD_RAM)r=0;							//dummy Read	   
	opt_delay(2);	  
 	r=myLCD->LCD_RAM;  		  						//实际坐标颜色
 	if(lcddev.id==0X9341)//9341要分2次读出
 	{
		opt_delay(2);	  
		b=myLCD->LCD_RAM; 
		g=r&0XFF;//对于9341,第一次读取的是RG的值,R在前,G在后,各占8位
		g<<=8;
	}else if(lcddev.id==0X6804)r=myLCD->LCD_RAM;//6804第二次读取的才是真实值
	if(lcddev.id==0X9325||lcddev.id==0X4535||lcddev.id==0X4531||lcddev.id==0X8989||lcddev.id==0XB505)return r;//这几种IC直接返回颜色值
	else if(lcddev.id==0X9341)return (((r>>11)<<11)|((g>>10)<<5)|(b>>11));	//ILI9341需要公式转换一下
	else return LCD_BGR2RGB(r);												//其他IC
}			 
//LCD开启显示
void LCD_DisplayOn(void)
{					   
	if(lcddev.id==0X9341||lcddev.id==0X6804)LCD_WR_REG(0X29);	//开启显示
	else LCD_WriteReg(R7,0x0173); 			//开启显示
}	 
//LCD关闭显示
void LCD_DisplayOff(void)
{	   
	if(lcddev.id==0X9341||lcddev.id==0X6804)LCD_WR_REG(0X28);	//关闭显示
	else LCD_WriteReg(R7,0x0);//关闭显示 
}   
//设置光标位置
//Xpos:横坐标
//Ypos:纵坐标
void LCD_SetCursor(u16 Xpos, u16 Ypos)
{	 
 	if(lcddev.id==0X9341||lcddev.id==0X6804)
	{		    
		LCD_WR_REG(lcddev.setxcmd); 
		LCD_WR_DATA(Xpos>>8); 
		LCD_WR_DATA(Xpos&0XFF);	 
		LCD_WR_REG(lcddev.setycmd); 
		LCD_WR_DATA(Ypos>>8); 
		LCD_WR_DATA(Ypos&0XFF);
	}else
	{
		if(lcddev.dir==1)
			Xpos=lcddev.width-1-Xpos;//横屏其实就是调转x,y坐标
		LCD_WriteReg(lcddev.setxcmd, Xpos);
		LCD_WriteReg(lcddev.setycmd, Ypos);
	}	 
} 		 
//设置LCD的自动扫描方向
//注意:其他函数可能会受到此函数设置的影响(尤其是9341/6804这两个奇葩),
//所以,一般设置为L2R_U2D即可,如果设置为其他扫描方式,可能导致显示不正常.
//dir:0~7,代表8个方向(具体定义见lcd.h)
//9320/9325/9328/4531/4535/1505/b505/8989/5408/9341等IC已经实际测试	   	   
void LCD_Scan_Dir(u8 dir)
{
	u16 regval=0;
	u8 dirreg=0;
	u16 temp;  
	if(lcddev.dir==1&&lcddev.id!=0X6804)//横屏时，对6804不改变扫描方向！
	{			   
		switch(dir)//方向转换
		{
			case 0:dir=6;break;
			case 1:dir=7;break;
			case 2:dir=4;break;
			case 3:dir=5;break;
			case 4:dir=1;break;
			case 5:dir=0;break;
			case 6:dir=3;break;
			case 7:dir=2;break;	     
		}
	}
	if(lcddev.id==0x9341||lcddev.id==0X6804)//9341/6804,很特殊
	{
		switch(dir)
		{
			case L2R_U2D://从左到右,从上到下
				regval|=(0<<7)|(0<<6)|(0<<5); 
				break;
			case L2R_D2U://从左到右,从下到上
				regval|=(1<<7)|(0<<6)|(0<<5); 
				break;
			case R2L_U2D://从右到左,从上到下
				regval|=(0<<7)|(1<<6)|(0<<5); 
				break;
			case R2L_D2U://从右到左,从下到上
				regval|=(1<<7)|(1<<6)|(0<<5); 
				break;	 
			case U2D_L2R://从上到下,从左到右
				regval|=(0<<7)|(0<<6)|(1<<5); 
				break;
			case U2D_R2L://从上到下,从右到左
				regval|=(0<<7)|(1<<6)|(1<<5); 
				break;
			case D2U_L2R://从下到上,从左到右
				regval|=(1<<7)|(0<<6)|(1<<5); 
				break;
			case D2U_R2L://从下到上,从右到左
				regval|=(1<<7)|(1<<6)|(1<<5); 
				break;	 
		}
		dirreg=0X36;
 		regval|=0X08;//BGR   
		if(lcddev.id==0X6804)regval|=0x02;//6804的BIT6和9341的反了	   
		LCD_WriteReg(dirreg,regval);
 		if(regval&0X20)
		{
			if(lcddev.width<lcddev.height)//交换X,Y
			{
				temp=lcddev.width;
				lcddev.width=lcddev.height;
				lcddev.height=temp;
 			}
		}else  
		{
			if(lcddev.width>lcddev.height)//交换X,Y
			{
				temp=lcddev.width;
				lcddev.width=lcddev.height;
				lcddev.height=temp;
 			}
		}  
		LCD_WR_REG(lcddev.setxcmd); 
		LCD_WR_DATA(0);LCD_WR_DATA(0);
		LCD_WR_DATA((lcddev.width-1)>>8);LCD_WR_DATA((lcddev.width-1)&0XFF);
		LCD_WR_REG(lcddev.setycmd); 
		LCD_WR_DATA(0);LCD_WR_DATA(0);
		LCD_WR_DATA((lcddev.height-1)>>8);LCD_WR_DATA((lcddev.height-1)&0XFF);  
  	}else 
	{
		switch(dir)
		{
			case L2R_U2D://从左到右,从上到下
				regval|=(1<<5)|(1<<4)|(0<<3); 
				break;
			case L2R_D2U://从左到右,从下到上
				regval|=(0<<5)|(1<<4)|(0<<3); 
				break;
			case R2L_U2D://从右到左,从上到下
				regval|=(1<<5)|(0<<4)|(0<<3);
				break;
			case R2L_D2U://从右到左,从下到上
				regval|=(0<<5)|(0<<4)|(0<<3); 
				break;	 
			case U2D_L2R://从上到下,从左到右
				regval|=(1<<5)|(1<<4)|(1<<3); 
				break;
			case U2D_R2L://从上到下,从右到左
				regval|=(1<<5)|(0<<4)|(1<<3); 
				break;
			case D2U_L2R://从下到上,从左到右
				regval|=(0<<5)|(1<<4)|(1<<3); 
				break;
			case D2U_R2L://从下到上,从右到左
				regval|=(0<<5)|(0<<4)|(1<<3); 
				break;	 
		}
		if(lcddev.id==0x8989)//8989 IC
		{
			dirreg=0X11;
			regval|=0X6040;	//65K   
	 	}else//其他驱动IC		  
		{
			dirreg=0X03;
			regval|=1<<12;  
		}
		LCD_WriteReg(dirreg,regval);
	}
}   
//画点
//x,y:坐标
//POINT_COLOR:此点的颜色
void LCD_DrawPoint(u16 x,u16 y)
{
	LCD_SetCursor(x,y);		//设置光标位置 
	LCD_WriteRAM_Prepare();	//开始写入GRAM
	myLCD->LCD_RAM=POINT_COLOR; 
}
//快速画点
//x,y:坐标
//color:颜色
void LCD_Fast_DrawPoint(u16 x,u16 y,u16 color)
{	   
	if(lcddev.id==0X9341||lcddev.id==0X6804)
	{		    
		LCD_WR_REG(lcddev.setxcmd); 
		LCD_WR_DATA(x>>8); 
		LCD_WR_DATA(x&0XFF);	 
		LCD_WR_REG(lcddev.setycmd); 
		LCD_WR_DATA(y>>8); 
		LCD_WR_DATA(y&0XFF);
	}else
	{
 		if(lcddev.dir==1)
			x=lcddev.width-1-x;//横屏其实就是调转x,y坐标
		LCD_WriteReg(lcddev.setxcmd,x);
		LCD_WriteReg(lcddev.setycmd,y);
	}			 
	myLCD->LCD_REG=lcddev.wramcmd; 
	myLCD->LCD_RAM=color; 
}	 


//设置LCD显示方向（6804不支持横屏显示）
//dir:0,竖屏；1,横屏
void LCD_Display_Dir(u8 dir)
{
	if(dir==0)//竖屏
	{
		lcddev.dir=0;//竖屏
		lcddev.width=240;
		lcddev.height=320;
		if(lcddev.id==0X9341||lcddev.id==0X6804)
		{
			lcddev.wramcmd=0X2C;
	 		lcddev.setxcmd=0X2A;
			lcddev.setycmd=0X2B;  	 
			if(lcddev.id==0X6804)
			{
				lcddev.width=320;
				lcddev.height=480;
			}
		}else if(lcddev.id==0X8989)
		{
			lcddev.wramcmd=R34;
	 		lcddev.setxcmd=0X4E;
			lcddev.setycmd=0X4F;  
		}else
		{
			lcddev.wramcmd=R34;
	 		lcddev.setxcmd=R32;
			lcddev.setycmd=R33;  
		}
	}else if(lcddev.id!=0X6804)//6804不支持横屏显示	
	{	  
		lcddev.dir=1;//横屏
		lcddev.width=320;
		lcddev.height=240;
		if(lcddev.id==0X9341)
		{
			lcddev.wramcmd=0X2C;
	 		lcddev.setxcmd=0X2A;
			lcddev.setycmd=0X2B;  	 
		}else if(lcddev.id==0X8989)
		{
			lcddev.wramcmd=R34;
	 		lcddev.setxcmd=0X4F;
			lcddev.setycmd=0X4E;  
		}else
		{
			lcddev.wramcmd=R34;
	 		lcddev.setxcmd=R33;
			lcddev.setycmd=R32;  
		}
	} 
	LCD_Scan_Dir(DFT_SCAN_DIR);	//默认扫描方向
}
//初始化lcd
//该初始化函数可以初始化各种ILI93XX液晶,但是其他函数是基于ILI9320的!!!
//在其他型号的驱动芯片上没有测试! 
void LCD_CtrlLinesConfig(void)
{ 										  
	GPIO_InitTypeDef GPIO_InitStructure;


    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC,ENABLE);	//使能FSMC时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOD |
						   RCC_APB2Periph_GPIOE | RCC_APB2Periph_GPIOF | 
						   RCC_APB2Periph_GPIOG | RCC_APB2Periph_AFIO, ENABLE);//使能PORTB,D,E,G以及AFIO复用功能时钟


 	//PORTD复用推挽输出  
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_14|GPIO_Pin_15;				 //	//PORTD复用推挽输出  
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 		 //复用推挽输出   
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOD, &GPIO_InitStructure); 
	  
	//PORTE复用推挽输出  
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;				 //	//PORTD复用推挽输出  
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 		 //复用推挽输出   
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOE, &GPIO_InitStructure); 					 //RS->PE3(A19)

	//背光
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出   
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOG, &GPIO_InitStructure); 



	  
   	//PG.12  FSMC_NE4
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;	 
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 		
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
 	GPIO_Init(GPIOG, &GPIO_InitStructure);
}	
void LCD_FSMCConfig(void)
{
 	FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
    FSMC_NORSRAMTimingInitTypeDef  readWriteTiming; 
	FSMC_NORSRAMTimingInitTypeDef  writeTiming;
	
	readWriteTiming.FSMC_AddressSetupTime = 0x01;	 //地址建立时间（ADDSET）为2个HCLK 1/36M=27ns
    readWriteTiming.FSMC_AddressHoldTime = 0x00;	 //地址保持时间（ADDHLD）模式A未用到	
    readWriteTiming.FSMC_DataSetupTime = 0x0f;		 // 数据保存时间为16个HCLK,因为液晶驱动IC的读数据的时候，速度不能太快，尤其对1289这个IC。
    readWriteTiming.FSMC_BusTurnAroundDuration = 0x00;
    readWriteTiming.FSMC_CLKDivision = 0x00;
    readWriteTiming.FSMC_DataLatency = 0x00;
    readWriteTiming.FSMC_AccessMode = FSMC_AccessMode_A;	 //模式A 
    

	writeTiming.FSMC_AddressSetupTime = 0x00;	 //地址建立时间（ADDSET）为1个HCLK  
    writeTiming.FSMC_AddressHoldTime = 0x00;	 //地址保持时间（A		
    writeTiming.FSMC_DataSetupTime = 0x03;		 ////数据保存时间为4个HCLK	
    writeTiming.FSMC_BusTurnAroundDuration = 0x00;
    writeTiming.FSMC_CLKDivision = 0x00;
    writeTiming.FSMC_DataLatency = 0x00;
    writeTiming.FSMC_AccessMode = FSMC_AccessMode_A;	 //模式A 

 
    FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM4;//  这里我们使用NE4 ，也就对应BTCR[6],[7]。
    FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable; // 不复用数据地址
    FSMC_NORSRAMInitStructure.FSMC_MemoryType =FSMC_MemoryType_SRAM;// FSMC_MemoryType_SRAM;  //SRAM   
    FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;//存储器数据宽度为16bit   
    FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode =FSMC_BurstAccessMode_Disable;// FSMC_BurstAccessMode_Disable; 
    FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
	FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait=FSMC_AsynchronousWait_Disable; 
    FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;   
    FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;  
    FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;	//  存储器写使能
    FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;   
    FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Enable; // 读写使用不同的时序
    FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable; 
    FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &readWriteTiming; //读写时序
    FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &writeTiming;  //写时序

    FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);  //初始化FSMC配置

   	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM4, ENABLE);  // 使能BANK1 
	}
void LCD_X_Init(void)
{
 /* Configure the LCD Control pins --------------------------------------------*/
  LCD_CtrlLinesConfig();

/* Configure the FSMC Parallel interface -------------------------------------*/
  LCD_FSMCConfig();
}
void ili9325_Initializtion(void)
{
	LCD_X_Init();
	delay_ms(50); // delay 50 ms 
 	LCD_WriteReg(0x0000,0x0001);
	delay_ms(50); // delay 50 ms 
  	lcddev.id = 0x9325;	//不能用自动读取ID
//  LCD_ReadReg(0x0000); //自动读取ID  

//	LCD_WriteReg(0x00E5,0x78F0); 
	LCD_WriteReg(0x0001,0x0100); 
	LCD_WriteReg(0x0002,0x0700); 
	LCD_WriteReg(0x0003,0x1030); 
	LCD_WriteReg(0x0004,0x0000); 
	LCD_WriteReg(0x0008,0x0202);  
	LCD_WriteReg(0x0009,0x0000);
	LCD_WriteReg(0x000A,0x0000); 
	LCD_WriteReg(0x000C,0x0000); 
	LCD_WriteReg(0x000D,0x0000);
	LCD_WriteReg(0x000F,0x0000);
	//power on sequence VGHVGL
	LCD_WriteReg(0x0010,0x0000);   
	LCD_WriteReg(0x0011,0x0007);  
	LCD_WriteReg(0x0012,0x0000);  
	LCD_WriteReg(0x0013,0x0000); 
	LCD_WriteReg(0x0007,0x0000); 
	//vgh 
	LCD_WriteReg(0x0010,0x1690);   
	LCD_WriteReg(0x0011,0x0227);
	//delayms(100);
	//vregiout 
	LCD_WriteReg(0x0012,0x009D); //0x001b
	//delayms(100); 
	//vom amplitude
	LCD_WriteReg(0x0013,0x1900);
	//delayms(100); 
	//vom H
	LCD_WriteReg(0x0029,0x0025); 
	LCD_WriteReg(0x002B,0x000D); 
	//gamma
	LCD_WriteReg(0x0030,0x0007);
	LCD_WriteReg(0x0031,0x0303);
	LCD_WriteReg(0x0032,0x0003);// 0006
	LCD_WriteReg(0x0035,0x0206);
	LCD_WriteReg(0x0036,0x0008);
	LCD_WriteReg(0x0037,0x0406); 
	LCD_WriteReg(0x0038,0x0304);//0200
	LCD_WriteReg(0x0039,0x0007); 
	LCD_WriteReg(0x003C,0x0602);// 0504
	LCD_WriteReg(0x003D,0x0008); 
	//ram
	LCD_WriteReg(0x0050,0x0000); 
	LCD_WriteReg(0x0051,0x00EF);
	LCD_WriteReg(0x0052,0x0000); 
	LCD_WriteReg(0x0053,0x013F);  
	LCD_WriteReg(0x0060,0xA700); 
	LCD_WriteReg(0x0061,0x0001); 
	LCD_WriteReg(0x006A,0x0000); 
	//
	LCD_WriteReg(0x0080,0x0000); 
	LCD_WriteReg(0x0081,0x0000); 
	LCD_WriteReg(0x0082,0x0000); 
	LCD_WriteReg(0x0083,0x0000); 
	LCD_WriteReg(0x0084,0x0000); 
	LCD_WriteReg(0x0085,0x0000); 
	//
	LCD_WriteReg(0x0090,0x0010); 
	LCD_WriteReg(0x0092,0x0600); 
	
	LCD_WriteReg(0x0007,0x0133);
//	LCD_WriteReg(0x00,0x0022);//
	LCD_Display_Dir(1);		 	//默认为0=竖屏
//	LCD_LED=1;					//点亮背光  直接上电点亮的
	GPIO_SetBits(GPIOG,GPIO_Pin_11);
	LCD_Clear(WHITE);
}


//画上框架
 void Display_Frame(void)						
{ 
  unsigned short i,j,b;
  for(i=0; i<320; ++i) 
	{         //画上背景框
    for(j=0; j<50; ++j)
   {
   LCD_SetCursor(i,j); 
	 LCD_WriteRAM_Prepare();
   LCD_WR_DATA(BLUE);		 
   }
  }
	LCD_Printfwhite(103, 20, "SmartLab v1.2");
	LCD_Printf(85, 80, "2016-09-05 16:11:23");
//   for(i=0; i<320; ++i) 
// 	{         //画下背景框
//     for(j=210; j<240; ++j) 
//    {
// 	 LCD_SetCursor(i,j); 
// 	 LCD_WriteRAM_Prepare();
//    LCD_WR_DATA(BLUE);
//    } 
//   }
// 	 for(j=0; j<240; ++j) 
// 	{         //画上背景框
//     for(i=0; i<30; ++i)
//    {
//    LCD_SetCursor(i,j); 
// 	 LCD_WriteRAM_Prepare();
//    LCD_WR_DATA(BLUE);		 
//    }
//   }
// 	 for(j=0; j<240; ++j) 
// 	{         //画上背景框
//     for(i=290; i<320; ++i)
//    {
//    LCD_SetCursor(i,j); 
// 	 LCD_WriteRAM_Prepare();
//    LCD_WR_DATA(BLUE);		 
//    }
//   }
}
//清屏函数
//color:要清屏的填充色

void LCD_Clear(u16 color)
{
	u32 index=0;      
	u32 totalpoint=lcddev.width;
	totalpoint*=lcddev.height; 	//得到总点数
	LCD_SetCursor(0x00,0x0000);	//设置光标位置 
	LCD_WriteRAM_Prepare();     //开始写入GRAM	 	  
	for(index=0;index<totalpoint;index++)
	{
		myLCD->LCD_RAM=color;	   
	}
}  
//在指定区域内填充单个颜色
//(sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex-sx+1)*(ey-sy+1)   
//color:要填充的颜色
 void LCD_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 color)
 {          
 	u16 i,j;
 	u16 xlen=0;
 	xlen=ex-sx+1;	   
 	for(i=sy;i<=ey;i++)
 	{
 	 	LCD_SetCursor(sx,i);      				//设置光标位置 
 		LCD_WriteRAM_Prepare();     			//开始写入GRAM	  
 		for(j=0;j<xlen;j++)LCD_WR_DATA(color);	//设置光标位置 	    
 	}
 }  
 //在指定区域内填充指定颜色块			 
 //(sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex-sx+1)*(ey-sy+1)   
 //color:要填充的颜色
 void LCD_Color_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 *color)
 {  
 	u16 height,width;
 	u16 i,j;
 	width=ex-sx+1; 		//得到填充的宽度
 	height=ey-sy+1;		//高度
  	for(i=0;i<height;i++)
 	{
  		LCD_SetCursor(sx,sy+i);   	//设置光标位置 
 		LCD_WriteRAM_Prepare();     //开始写入GRAM
 		for(j=0;j<width;j++)myLCD->LCD_RAM=color[i*height+j];//写入数据 
 	}	  
 }  
 //画线
 //x1,y1:起点坐标
 //x2,y2:终点坐标  
 void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
 {
 	u16 t; 
 	int xerr=0,yerr=0,delta_x,delta_y,distance; 
 	int incx,incy,uRow,uCol; 
 	delta_x=x2-x1; //计算坐标增量 
 	delta_y=y2-y1; 
 	uRow=x1; 
 	uCol=y1; 
 	if(delta_x>0)incx=1; //设置单步方向 
 	else if(delta_x==0)incx=0;//垂直线 
 	else {incx=-1;delta_x=-delta_x;} 
 	if(delta_y>0)incy=1; 
 	else if(delta_y==0)incy=0;//水平线 
 	else{incy=-1;delta_y=-delta_y;} 
 	if( delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
 	else distance=delta_y; 
 	for(t=0;t<=distance+1;t++ )//画线输出 
 	{  
 		LCD_DrawPoint(uRow,uCol);//画点 
 		xerr+=delta_x ; 
 		yerr+=delta_y ; 
 		if(xerr>distance) 
 		{ 
 			xerr-=distance; 
 			uRow+=incx; 
 		} 
 		if(yerr>distance) 
 		{ 
 			yerr-=distance; 
 			uCol+=incy; 
 		} 
 	}  
 }    
 //画矩形	  
 //(x1,y1),(x2,y2):矩形的对角坐标
 void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2)
 {
 	LCD_DrawLine(x1,y1,x2,y1);
 	LCD_DrawLine(x1,y1,x1,y2);
 	LCD_DrawLine(x1,y2,x2,y2);
 	LCD_DrawLine(x2,y1,x2,y2);
 }
 //在指定位置画一个指定大小的圆
 //(x,y):中心点
 //r    :半径
 void Draw_Circle(u16 x0,u16 y0,u8 r)
 {
 	int a,b;
 	int di;
 	a=0;b=r;	  
 	di=3-(r<<1);             //判断下个点位置的标志
 	while(a<=b)
 	{
 		LCD_DrawPoint(x0+a,y0-b);             //5
  		LCD_DrawPoint(x0+b,y0-a);             //0           
 		LCD_DrawPoint(x0+b,y0+a);             //4               
 		LCD_DrawPoint(x0+a,y0+b);             //6 
 		LCD_DrawPoint(x0-a,y0+b);             //1       
  		LCD_DrawPoint(x0-b,y0+a);             
 		LCD_DrawPoint(x0-a,y0-b);             //2             
   	LCD_DrawPoint(x0-b,y0-a);             //7     	         
 		a++;
 		//使用Bresenham算法画圆     
 		if(di<0)di +=4*a+6;	  
 		else
 		{
 			di+=10+4*(a-b);   
 			b--;
 		} 						    
 	}
 } 									  
 //在指定位置显示一个字符
 //x,y:起始坐标
 //num:要显示的字符:" "--->"~"
 //size:字体大小 12/16
 //mode:叠加方式(1)还是非叠加方式(0)
 void LCD_ShowChar(u16 x,u16 y,u8 num,u8 size,u8 mode)
 {  							  
    u8 temp,t1,t;
 	u16 y0=y;
 	u16 colortemp=POINT_COLOR;      			     
 	//设置窗口		   
 	num=num-' ';//得到偏移后的值
 	if(!mode) //非叠加方式
 	{
 	    for(t=0;t<size;t++)
 	    {   
 			if(size==12)temp=asc2_1206[num][t];  //调用1206字体
 			else temp=asc2_1608[num][t];		 //调用1608字体 	                          
 	        for(t1=0;t1<8;t1++)
 			{			    
 		        if(temp&0x80)POINT_COLOR=colortemp;
 				else POINT_COLOR=WHITE;
 				LCD_DrawPoint(x,y);	
 				temp<<=1;
 				y++;
 				if(x>=lcddev.width){POINT_COLOR=colortemp;return;}//超区域了
 				if((y-y0)==size)
 				{
 					y=y0;
 					x++;
 					if(x>=lcddev.width){POINT_COLOR=colortemp;return;}//超区域了
 					break;
 				}
 			}  	 
 	    }    
 	}else//叠加方式
 	{
 	    for(t=0;t<size;t++)
 	    {   
 			if(size==12)temp=asc2_1206[num][t];  //调用1206字体
 			else temp=asc2_1608[num][t];		 //调用1608字体 	                          
 	        for(t1=0;t1<8;t1++)
 			{			    
 		        if(temp&0x80)LCD_DrawPoint(x,y); 
 				temp<<=1;
 				y++;
 				if(x>=lcddev.height){POINT_COLOR=colortemp;return;}//超区域了
 				if((y-y0)==size)
 				{
 					y=y0;
 					x++;
 					if(x>=lcddev.width){POINT_COLOR=colortemp;return;}//超区域了
 					break;
 				}
 			}  	 
 	    }     
 	}
 	POINT_COLOR=colortemp;	    	   	 	  
 }   
  //在指定位置显示一个字符
 //x,y:起始坐标
 //num:要显示的字符:" "--->"~"
 //size:字体大小 12/16
 //mode:叠加方式(1)还是非叠加方式(0)
 void LCD_ShowCharwhite(u16 x,u16 y,u8 num,u8 size,u8 mode)
 {  							  
    u8 temp,t1,t;
 	u16 y0=y;
 	u16 colortemp=POINT_COLOR;      			     
 	//设置窗口		   
 	num=num-' ';//得到偏移后的值
 	if(!mode) //非叠加方式
 	{
 	    for(t=0;t<size;t++)
 	    {   
 			if(size==12)temp=asc2_1206[num][t];  //调用1206字体
 			else temp=asc2_1608[num][t];		 //调用1608字体 	                          
 	        for(t1=0;t1<8;t1++)
 			{			    
 		        if(temp&0x80)POINT_COLOR=WHITE;
 				else POINT_COLOR=BLUE;
 				LCD_DrawPoint(x,y);	
 				temp<<=1;
 				y++;
 				if(x>=lcddev.width){POINT_COLOR=colortemp;return;}//超区域了
 				if((y-y0)==size)
 				{
 					y=y0;
 					x++;
 					if(x>=lcddev.width){POINT_COLOR=colortemp;return;}//超区域了
 					break;
 				}
 			}  	 
 	    }    
 	}else//叠加方式
 	{
 	    for(t=0;t<size;t++)
 	    {   
 			if(size==12)temp=asc2_1206[num][t];  //调用1206字体
 			else temp=asc2_1608[num][t];		 //调用1608字体 	                          
 	        for(t1=0;t1<8;t1++)
 			{			    
 		        if(temp&0x80)LCD_DrawPoint(x,y); 
 				temp<<=1;
 				y++;
 				if(x>=lcddev.height){POINT_COLOR=colortemp;return;}//超区域了
 				if((y-y0)==size)
 				{
 					y=y0;
 					x++;
 					if(x>=lcddev.width){POINT_COLOR=colortemp;return;}//超区域了
 					break;
 				}
 			}  	 
 	    }     
 	}
 	POINT_COLOR=colortemp;	    	   	 	  
 }   
 //m^n函数
 //返回值:m^n次方.
 u32 LCD_Pow(u8 m,u8 n)
 {
 	u32 result=1;	 
 	while(n--)result*=m;    
 	return result;
 }			 
 //显示数字,高位为0,则不显示
 //x,y :起点坐标	 
 //len :数字的位数
 //size:字体大小
 //color:颜色 
 //num:数值(0~4294967295);	 
 void LCD_ShowNum(u16 x,u16 y,u32 num,u8 len,u8 size)
 {         	
 	u8 t,temp;
 	u8 enshow=0;						   
 	for(t=0;t<len;t++)
 	{
 		temp=(num/LCD_Pow(10,len-t-1))%10;
 		if(enshow==0&&t<(len-1))
 		{
 			if(temp==0)
 			{
 				LCD_ShowChar(x+(size/2)*t,y,' ',size,0);
 				continue;
 			}else enshow=1; 
 		 	 
 		}
 	 	LCD_ShowChar(x+(size/2)*t,y,temp+'0',size,0); 
 	}
 } 
 //显示数字,高位为0,还是显示
 //x,y:起点坐标
 //num:数值(0~999999999);	 
 //len:长度(即要显示的位数)
 //size:字体大小
 //mode:
 //[7]:0,不填充;1,填充0.
 //[6:1]:保留
 //[0]:0,非叠加显示;1,叠加显示.
 void LCD_ShowxNum(u16 x,u16 y,u32 num,u8 len,u8 size,u8 mode)
 {  
 	u8 t,temp;
 	u8 enshow=0;						   
 	for(t=0;t<len;t++)
 	{
 		temp=(num/LCD_Pow(10,len-t-1))%10;
 		if(enshow==0&&t<(len-1))
 		{
 			if(temp==0)
 			{
 				if(mode&0X80)LCD_ShowChar(x+(size/2)*t,y,'0',size,mode&0X01);  
 				else LCD_ShowChar(x+(size/2)*t,y,' ',size,mode&0X01);  
  				continue;
 			}else enshow=1; 
 		 	 
 		}
 	 	LCD_ShowChar(x+(size/2)*t,y,temp+'0',size,mode&0X01); 
 	}
 } 
 //显示字符串
 //x,y:起点坐标
 //width,height:区域大小  
 //size:字体大小
 //*p:字符串起始地址		  
 void LCD_ShowString(u16 x,u16 y,u16 width,u16 height,u8 size,u8 *p)
 {         
 	u8 x0=x;
 	width+=x;
 	height+=y;
     while((*p<='~')&&(*p>=' '))//判断是不是非法字符!
     {       
         //if(x>=width){x=x0;y+=size;}
         //if(y>=height)break;//退出
         LCD_ShowChar(x,y,*p,size,0);
         x+=size/2;
         p++;
     }  
 }
  //显示字符串
 //x,y:起点坐标
 //width,height:区域大小  
 //size:字体大小
 //*p:字符串起始地址		  
 void LCD_ShowStringwhite(u16 x,u16 y,u16 width,u16 height,u8 size,u8 *p)
 {         
 	u8 x0=x;
 	width+=x;
 	height+=y;
     while((*p<='~')&&(*p>=' '))//判断是不是非法字符!
     {       
         //if(x>=width){x=x0;y+=size;}
         //if(y>=height)break;//退出
         LCD_ShowCharwhite(x,y,*p,size,0);
         x+=size/2;
         p++;
     }  
 }

uint8_t LCDBuf[128];
void LCD_Printf(u16 x,u16 y, char *fmt,...)
{
    va_list ap;
	va_start(ap,fmt);
	memset(LCDBuf, 0x20, 40);
	LCDBuf[48] = 0;
	LCD_ShowString(x, y, 200, 20, 16, LCDBuf);
    vsprintf((char *)LCDBuf, fmt, ap);
	LCD_ShowString(x, y, 200, 240, 16, LCDBuf);
    va_end(ap);
	//OS_CRITICAL_EXIT();
}


uint8_t LCDBuftwo[128];
void LCD_Printfwhite(u16 x,u16 y, char *fmt,...)
{
    va_list ap;
	va_start(ap,fmt);
	memset(LCDBuf, 0x20, 40);
	LCDBuf[48] = 0;
	LCD_ShowStringwhite(x, y, 200, 20, 16, LCDBuftwo);
    vsprintf((char *)LCDBuftwo, fmt, ap);
	LCD_ShowStringwhite(x, y, 200, 240, 16, LCDBuftwo);
    va_end(ap);
	//OS_CRITICAL_EXIT();
}
