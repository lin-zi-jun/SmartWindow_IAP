#ifndef __SRAM_H
#define __SRAM_H															    
#include "stm32f10x.h"

#define Bank1_SRAM3_ADDR    ((u32)0x68000000)
#define SRAM_LEN			0x80000
											  
void FSMC_SRAM_Init(void);
void FSMC_SRAM_WriteBuffer(u16* pBuffer, u32 WriteAddr, u32 NumHalfwordToWrite);
void FSMC_SRAM_ReadBuffer(u16* pBuffer, u32 ReadAddr, u32 NumHalfwordToRead);


#endif

