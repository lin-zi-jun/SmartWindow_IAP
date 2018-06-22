/*-----------------------------------------------------------------------------
 *      RL-ARM - FlashFS
 *-----------------------------------------------------------------------------
 *      Name:    NAND_STM32F2xx.c
 *      Purpose: NAND Flash Driver for ST STM32F2xx
 *      Rev.:    V4.22
 *-----------------------------------------------------------------------------
 *      This code is part of the RealView Run-Time Library.
 *      Copyright (c) 2004-2011 KEIL - An ARM Company. All rights reserved.
 *----------------------------------------------------------------------------*/

#include <File_Config.h>
#include "stm32f10x.h"                 
#include "NAND_STM32F1xx.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_fsmc.h"

/* #define FSMC_BANK1_CS1 */            /* OneNAND Driver Template */
//#define FSMC_BANK3                      /* NAND Driver Template    */

#define FSMC_Bank_NAND     FSMC_Bank2_NAND
#define Bank_NAND_ADDR     Bank2_NAND_ADDR 
#define Bank2_NAND_ADDR    ((uint32_t)0x70000000)    

/*-----------------------------------------------------------------------------
 *      NAND driver prototypes
 *----------------------------------------------------------------------------*/
static U32 Init         (NAND_DRV_CFG *cfg);
static U32 UnInit       (NAND_DRV_CFG *cfg);
static U32 PageRead     (U32 row, U8 *buf, NAND_DRV_CFG *cfg);
static U32 PageWrite    (U32 row, U8 *buf, NAND_DRV_CFG *cfg);
static U32 BlockErase   (U32 row, NAND_DRV_CFG *cfg);

/*-----------------------------------------------------------------------------
  NAND Device Driver Control Block
 *----------------------------------------------------------------------------*/
const NAND_DRV nand0_drv = {
  Init,
  UnInit,
  PageRead,
  PageWrite,
  BlockErase,
};
/*------------------------------------------------------------------
 *Initialise Flexible Static Memory Controler (FSMC) for NAND
 *------------------------------------------------------------------*/
void NAND_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure; 
	FSMC_NAND_PCCARDTimingInitTypeDef  p;
	FSMC_NANDInitTypeDef FSMC_NANDInitStructure;
	
	/*FSMC总线使用的GPIO组时钟使能*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE, ENABLE);
	/*使能FSMC时钟*/
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC,ENABLE);
	
	/*FSMC  D0->D3, NOE, NWE, NCE2,ALE and CLE初始化，推挽复用输出*/
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_14 | GPIO_Pin_15 | GPIO_Pin_0 | GPIO_Pin_1 |
								 GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_7 | GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	
	GPIO_Init(GPIOD, &GPIO_InitStructure); 
	


	/*FSMC_D[4:7]初始化，推挽复用输出*/ 
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
	
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	
	
	/*FSMC NWAIT初始化，输入上拉*/ 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	  
	GPIO_Init(GPIOD, &GPIO_InitStructure); 
	
	
	
	/*--------------FSMC 总线 存储器参数配置------------------------------*/
	p.FSMC_SetupTime = 0x1;         //建立时间
	p.FSMC_WaitSetupTime = 0x3;     //等待时间
	p.FSMC_HoldSetupTime = 0x2;     //保持时间
	p.FSMC_HiZSetupTime = 0x1;      //高阻建立时间
	
	FSMC_NANDInitStructure.FSMC_Bank = FSMC_Bank2_NAND; //使用FSMC BANK2
	FSMC_NANDInitStructure.FSMC_Waitfeature = FSMC_Waitfeature_Enable; //使能FSMC的等待功能
	FSMC_NANDInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_8b; //NAND Flash的数据宽度为8位
	FSMC_NANDInitStructure.FSMC_ECC = FSMC_ECC_Enable;                  //使能ECC特性
	FSMC_NANDInitStructure.FSMC_ECCPageSize = FSMC_ECCPageSize_2048Bytes; //ECC页大小2048
	FSMC_NANDInitStructure.FSMC_TCLRSetupTime = 0x00;             //CLE low to RE low
	FSMC_NANDInitStructure.FSMC_TARSetupTime = 0x00;					//ALE low to RE low
	FSMC_NANDInitStructure.FSMC_CommonSpaceTimingStruct = &p;
	FSMC_NANDInitStructure.FSMC_AttributeSpaceTimingStruct = &p;
	FSMC_NANDInit(&FSMC_NANDInitStructure);
	
	/*!使能FSMC BANK2 */
	FSMC_NANDCmd(FSMC_Bank2_NAND, ENABLE);
}
void NAND_DeInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure; 
	FSMC_NAND_PCCARDTimingInitTypeDef  p;
	FSMC_NANDInitTypeDef FSMC_NANDInitStructure;
	
	/*FSMC总线使用的GPIO组时钟使能*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE, ENABLE);
	/*使能FSMC时钟*/
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC,ENABLE);
	
	/*FSMC  D0->D3, NOE, NWE , NCE2 ,ALE and CLE初始化，推挽复用输出*/
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_14 | GPIO_Pin_15 | GPIO_Pin_0 | GPIO_Pin_1 |
								 GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_7 | GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	
	GPIO_Init(GPIOD, &GPIO_InitStructure); 
	

	/*FSMC_D[4:7]初始化，推挽复用输出*/ 
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
	
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	
	
	
	/*FSMC NWAIT初始化，输入上拉*/ 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	  
	GPIO_Init(GPIOD, &GPIO_InitStructure); 
	
	
	
	/*--------------FSMC 总线 存储器参数配置------------------------------*/
	p.FSMC_SetupTime = 0x1;         //建立时间
	p.FSMC_WaitSetupTime = 0x3;     //等待时间
	p.FSMC_HoldSetupTime = 0x2;     //保持时间
	p.FSMC_HiZSetupTime = 0x1;      //高阻建立时间
	
	FSMC_NANDInitStructure.FSMC_Bank = FSMC_Bank2_NAND; //使用FSMC BANK2
	FSMC_NANDInitStructure.FSMC_Waitfeature = FSMC_Waitfeature_Enable; //使能FSMC的等待功能
	FSMC_NANDInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_8b; //NAND Flash的数据宽度为8位
	FSMC_NANDInitStructure.FSMC_ECC = FSMC_ECC_Enable;                  //使能ECC特性
	FSMC_NANDInitStructure.FSMC_ECCPageSize = FSMC_ECCPageSize_2048Bytes; //ECC页大小2048
	FSMC_NANDInitStructure.FSMC_TCLRSetupTime = 0x00;             //CLE low to RE low
	FSMC_NANDInitStructure.FSMC_TARSetupTime = 0x00;					//ALE low to RE low
	FSMC_NANDInitStructure.FSMC_CommonSpaceTimingStruct = &p;
	FSMC_NANDInitStructure.FSMC_AttributeSpaceTimingStruct = &p;
	FSMC_NANDInit(&FSMC_NANDInitStructure);
	
	/*!使能FSMC BANK2 */
	FSMC_NANDCmd(FSMC_Bank2_NAND, DISABLE);
}
/**
  * @brief  This routine reset the NAND FLASH.
  * @param  None
  * @retval NAND_READY
  */
uint32_t NAND_Reset(void)
{
  *(__IO uint8_t *)(Bank_NAND_ADDR | CMD_AREA) = NAND_CMD_RESET;

  return (NAND_READY);
}
/**
  * @brief  Reads NAND memory's ID.
  * @param  NAND_ID: pointer to a NAND_IDTypeDef structure which will hold
  *         the Manufacturer and Device ID.  
  * @retval None
  */
void NAND_ReadID(NAND_IDTypeDef* NAND_ID)
{					  
	uint32_t data = 0;
	
	/*!< Send Command to the command area */
	*(__IO uint8_t *)(Bank_NAND_ADDR | CMD_AREA) = 0x90;
	*(__IO uint8_t *)(Bank_NAND_ADDR | ADDR_AREA) = 0x00;
	
	/*!< Sequence to read ID from NAND flash */
	data = *(__IO uint32_t *)(Bank_NAND_ADDR | DATA_AREA);
	
	NAND_ID->Maker_ID   = DATA_1st_CYCLE (data);
	NAND_ID->Device_ID  = DATA_2nd_CYCLE (data);
	NAND_ID->Third_ID   = DATA_3rd_CYCLE (data);
	NAND_ID->Fourth_ID  = DATA_4th_CYCLE (data);
}

/**
  * @brief  Reads the NAND memory status using the Read status command. 
  * @param  None
  * @retval The status of the NAND memory. This parameter can be:
  *              - NAND_BUSY: when memory is busy
  *              - NAND_READY: when memory is ready for the next operation
  *              - NAND_ERROR: when the previous operation gererates error
  */
uint32_t NAND_ReadStatus(void)
{
	uint32_t data = 0x00, status = NAND_BUSY;
	
	/*!< Read status operation ------------------------------------ */
	*(__IO uint8_t *)(Bank_NAND_ADDR | CMD_AREA) = NAND_CMD_STATUS;
	data = *(__IO uint8_t *)(Bank_NAND_ADDR);
	
	if((data & NAND_ERROR) == NAND_ERROR)
	{
		status = NAND_ERROR;
	} 
	else if((data & NAND_READY) == NAND_READY)
	{
		status = NAND_READY;
	}
		else
		{
			status = NAND_BUSY; 
		}
	
	return (status);
}
/**
  * @brief  Get the NAND operation status.
  * @param  None
  * @retval New status of the NAND operation. This parameter can be:
  *              - NAND_TIMEOUT_ERROR: when the previous operation generate
  *                a Timeout error
  *              - NAND_READY: when memory is ready for the next operation
  */
uint32_t NAND_GetStatus(void)
{
	uint32_t timeout = 0x1000000, status = NAND_READY;
	
	status = NAND_ReadStatus();
	
	/*!< Wait for a NAND operation to complete or a TIMEOUT to occur */
	while ((status != NAND_READY) &&( timeout != 0x00))
	{
		status = NAND_ReadStatus();
		timeout --;
	}
	
	if(timeout == 0x00)
	{
		status =  NAND_TIMEOUT_ERROR;
	}
	
	/*!< Return the operation status */
	return (status);
}

/**
  * @brief  This routine is for sequential read from one 2112 Bytes Page size.
  * @param  pBuffer: pointer on the Buffer to fill
  * @param  Address: First page address
  * @retval New status of the NAND operation. This parameter can be:
  *              - NAND_TIMEOUT_ERROR: when the previous operation generate 
  *                a Timeout error
  *              - NAND_READY: when memory is ready for the next operation 
  *                And the new status of the increment address operation. It can be:
  *              - NAND_VALID_ADDRESS: When the new address is valid address
  *              - NAND_INVALID_ADDRESS: When the new address is invalid address
  */
uint32_t NAND_ReadPage(uint32_t Address, uint8_t *pBuffer)
{
	uint32_t index = 0x00,  addressstatus = NAND_VALID_ADDRESS;
	uint32_t status = NAND_READY, size = 0x00;

	if(Address++ > (NAND_MAX_ZONE*NAND_ZONE_SIZE*NAND_BLOCK_SIZE))  
	{ 
		addressstatus = NAND_INVALID_ADDRESS;
		return (addressstatus);
	}
	

	/*!< Page Read command and page address */ 
	*(__IO uint8_t *)(Bank_NAND_ADDR | CMD_AREA) = NAND_CMD_READ0;

    *(__IO uint8_t *)(Bank_NAND_ADDR | ADDR_AREA) = ADDR_1st_CYCLE(Address);
    *(__IO uint8_t *)(Bank_NAND_ADDR | ADDR_AREA) = ADDR_2nd_CYCLE(Address);
    *(__IO uint8_t *)(Bank_NAND_ADDR | ADDR_AREA) = ADDR_3rd_CYCLE(Address);
    *(__IO uint8_t *)(Bank_NAND_ADDR | ADDR_AREA) = ADDR_4th_CYCLE(Address);
    
    *(__IO uint8_t *)(Bank_NAND_ADDR | CMD_AREA) = NAND_CMD_READ_TRUE1;

    /**wait for ready**/
//    while( GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_6) == 0 );

    /*!< Calculate the size */  
    size = 2112;
    
    /*!< Get Data into Buffer */    
    for(index = 0; index < size; index++)
    {
      pBuffer[index]= *(__IO uint8_t *)(Bank_NAND_ADDR | DATA_AREA);
    }

	
  	status = NAND_GetStatus();
  
  	return (status | addressstatus);
}
/**
  * @brief  This routine is for writing one 2112 Bytes Page size.
  * @param  pBuffer: pointer on the Buffer containing data to be written 
  * @param  Address: First page address
  * @param  NumPageToWrite: Number of page to write  
  * @retval New status of the NAND operation. This parameter can be:
  *              - NAND_TIMEOUT_ERROR: when the previous operation generate 
  *                a Timeout error
  *              - NAND_READY: when memory is ready for the next operation 
  *                And the new status of the increment address operation. It can be:
  *              - NAND_VALID_ADDRESS: When the new address is valid address
  *              - NAND_INVALID_ADDRESS: When the new address is invalid address  
  */
uint32_t NAND_WritePage(uint32_t Address, uint8_t *pBuffer)
{
	uint32_t index = 0x00, addressstatus = NAND_VALID_ADDRESS;
	uint32_t status = NAND_READY, size = 0x00;
	 
	if(Address++ > (NAND_MAX_ZONE*NAND_ZONE_SIZE*NAND_BLOCK_SIZE))  
	{ 
		addressstatus = NAND_INVALID_ADDRESS;
		return (addressstatus);
	}

	    /*!< Page write command and address */

	    *(__IO uint8_t *)(Bank_NAND_ADDR | CMD_AREA) = NAND_CMD_WRITE0;
	
	    *(__IO uint8_t *)(Bank_NAND_ADDR | ADDR_AREA) = ADDR_1st_CYCLE(Address);
	    *(__IO uint8_t *)(Bank_NAND_ADDR | ADDR_AREA) = ADDR_2nd_CYCLE(Address);
	    *(__IO uint8_t *)(Bank_NAND_ADDR | ADDR_AREA) = ADDR_3rd_CYCLE(Address);
	    *(__IO uint8_t *)(Bank_NAND_ADDR | ADDR_AREA) = ADDR_4th_CYCLE(Address); 
	
	    /*!< Calculate the size */	  
	    size = 2112;	 	  
	
	    /*!< Write data */
	    for(index = 0; index < size; index++)
	    {
	      *(__IO uint8_t *)(Bank_NAND_ADDR | DATA_AREA) = pBuffer[index];
	    }
	    
	    *(__IO uint8_t *)(Bank_NAND_ADDR | CMD_AREA) = NAND_CMD_WRITE_TRUE1;
	   
	    /**wait for ready singal**/
//	    while( GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_6) == 0 );
	
	    /*!< Check status for successful operation */
	    status = NAND_GetStatus();
      		   

	return (status | addressstatus);
}
/**
  * @brief  This routine erase complete block from NAND FLASH
  * @param  Address: Any address into block to be erased
  * @retval New status of the NAND operation. This parameter can be:
  *              - NAND_TIMEOUT_ERROR: when the previous operation generate 
  *                a Timeout error
  *              - NAND_READY: when memory is ready for the next operation 
  */
uint32_t NAND_EraseBlock(uint32_t Address)
{

	*(__IO uint8_t *)(Bank_NAND_ADDR | CMD_AREA) = NAND_CMD_ERASE0;
	
	
    *(__IO uint8_t *)(Bank_NAND_ADDR | ADDR_AREA) = ADDR_3rd_CYCLE(Address);
    *(__IO uint8_t *)(Bank_NAND_ADDR | ADDR_AREA) = ADDR_4th_CYCLE(Address);
	
	*(__IO uint8_t *)(Bank_NAND_ADDR | CMD_AREA) = NAND_CMD_ERASE1; 
	 
	/* Read status operation ------------------------------------ */ 	   
    return(NAND_GetStatus());
  
}


static U32 Init         (NAND_DRV_CFG *cfg)
{


	uint32_t status = NAND_READY;
	
	if(cfg->DrvInst == 0)
	{
		NAND_Init();  
		
		status = NAND_Reset();
		if(status != NAND_READY)
		{
			return ERR_NAND_HW_TOUT;
		}
		
		return RTV_NOERR;			
	}
	else
	{
		return ERR_INVALID_PARAM;	
	}
	
}
static U32 UnInit       (NAND_DRV_CFG *cfg)
{
	NAND_DeInit();
	return RTV_NOERR;
}
/******************************************
 *The function ReadPage reads data from a NAND Flash device. 
 *The parameter row specifies the address of the starting page. 
 *The parameter buf is a pointer to the buffer that receives the data. 
 ***************************************************************/
static U32 PageRead     (U32 row, U8 *buf, NAND_DRV_CFG *cfg)
{
	uint32_t status = NAND_READY;
	
  /* Determine Chip Select from instance number */
	if(cfg->DrvInst == 0)
	{
		status = NAND_ReadPage(row, buf);
	
		if(!(status & NAND_READY))
		{
			return  ERR_NAND_HW_TOUT;
		}
		return  RTV_NOERR;
	}
	else
	{
		return  ERR_INVALID_PARAM;	
	}	
}
static U32 PageWrite    (U32 row, U8 *buf, NAND_DRV_CFG *cfg)
{
		uint32_t status = NAND_READY;
	
  /* Determine Chip Select from instance number */
	if(cfg->DrvInst == 0)
	{
		status = NAND_WritePage(row, buf);
		
		if(!(status&NAND_READY))
		{
			return  ERR_NAND_PROG;
		}
		return  RTV_NOERR;
	}
	else
	{
		return  ERR_INVALID_PARAM;	
	}
}
static U32 BlockErase   (U32 row, NAND_DRV_CFG *cfg)
{
	uint32_t status = NAND_READY;
	
  /* Determine Chip Select from instance number */
	if(cfg->DrvInst == 0)
	{
		status = NAND_EraseBlock(row);
		
		if(status !=NAND_READY)
		{
			return  ERR_NAND_ERASE;
		}
		return  RTV_NOERR;
	}
	else
	{
		return  ERR_INVALID_PARAM;
	}	
}
