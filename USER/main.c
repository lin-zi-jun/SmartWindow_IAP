/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include <stdio.h>
#include "common.h"
#include "ili93xx.h"
#include "sram.h"
#include "rtc.h"
#include "can.h"
#include <rtl.h>
/* Private functions ---------------------------------------------------------*/
void InitFlashFS(void) {
	if(finit("N:") == 0)
	{
		SerialPutString("NAND initialize OK.\r\n");
	} else {
		if(fformat("N:0") != 0)
		{													//NAND
			SerialPutString("NAND format failed.\r\n");
			finit("N:0");
		} else {
			SerialPutString("NAND format OK.\r\n");
		}
	}
}

int main(void)
{
	NVIC_Configuration();
	
	RTC_Init();
	USART1_Init(115200);
	FSMC_SRAM_Init();
	//InitFlashFS();
  ili93xx_Initializtion();
	LCD_Clear(WHITE);
	FLASH_Unlock();			//½âËø
	CAN_Mode_Init(CAN_SJW_1tq,CAN_BS2_8tq,CAN_BS1_7tq,5,0);
	
	while (1) {
		Main_Menu();
	}
}


