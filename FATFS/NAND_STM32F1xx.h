/*----------------------------------------------------------------------------
 *      RL-ARM - FlashFS
 *----------------------------------------------------------------------------
 *      Name:    FSMC_STM32F2xx.h 
 *      Purpose: Flexible static memory controller Driver for ST STM32F2xx
 *      Rev.:    V4.22
 *----------------------------------------------------------------------------
 *      This code is part of the RealView Run-Time Library.
 *      Copyright (c) 2004-2011 KEIL - An ARM Company. All rights reserved.
 *---------------------------------------------------------------------------*/

#ifndef __FSMC_STM32F1XX_H
#define __FSMC_STM32F1XX_H

#include <RTL.h>
#include "stm32f10x.h"


typedef struct
{
  uint8_t Maker_ID;
  uint8_t Device_ID;
  uint8_t Third_ID;
  uint8_t Fourth_ID;
}NAND_IDTypeDef;

typedef struct 
{
  uint16_t Zone;
  uint16_t Block;
  uint16_t Page;
} NAND_ADDRESS;  

/** 
  * @brief  NAND Area definition  for STM3210E-EVAL Board RevD  
  */  
#define CMD_AREA                   (uint32_t)(1<<16)  /* A20 = CLE  high */
#define ADDR_AREA                  (uint32_t)(1<<17)  /* A21 = ALE high */

#define DATA_AREA                  ((uint32_t)0x00000000) 

/* OneNAND Page Layout Definition */
#define ON_POS_LSN      2
#define ON_POS_COR      1
#define ON_POS_BBM      0
#define ON_POS_ECC      8

#define ON_SECT_INC   512
#define ON_SPARE_OFS 2048
#define ON_SPARE_INC   16

/** 
  * @brief  FSMC NAND memory command  
  */  


#define NAND_CMD_READ0			   ((uint8_t)0x00)
#define NAND_CMD_READ_TRUE1        ((uint8_t)0x30)

#define NAND_CMD_WRITE0            ((uint8_t)0x80)
#define NAND_CMD_WRITE_TRUE1       ((uint8_t)0x10)

#define NAND_CMD_ERASE0            ((uint8_t)0x60)
#define NAND_CMD_ERASE1            ((uint8_t)0xD0)

//#define NAND_CMD_READID            ((uint8_t)0x90)
//#define NAND_CMD_STATUS            ((uint8_t)0x70)
#define NAND_CMD_LOCK_STATUS       ((uint8_t)0x7A)
//#define NAND_CMD_RESET             ((uint8_t)0xFF)

#define	NAND_CMD_RDCOPYBACK         ((uint8_t)0x00)
#define	NAND_CMD_RDCOPYBACK_TRUE    ((uint8_t)0x35)

#define NAND_CMD_COPYBACKPGM        ((uint8_t)0x85)
#define NAND_CMD_COPYBACKPGM_TRUE   ((uint8_t)0x10)

#define NAND_CMD_CACHEPGM           ((uint8_t)0x80)
#define NAND_CMD_CACHEPGM_TRUE      ((uint8_t)0x15)

#define NAND_CMD_RANDOMIN           ((uint8_t)0x85)
#define NAND_CMD_RANDOMOUT          ((uint8_t)0x05)
#define NAND_CMD_RANDOMOUT_TRUE     ((uint8_t)0xE0)

#define NAND_CMD_CACHERD_START      ((uint8_t)0x00)
#define NAND_CMD_CACHERD_START2     ((uint8_t)0x31)
#define NAND_CMD_CACHERD_EXIT       ((uint8_t)0x34)


/**
  *extend command
**/
#define NAND_CMD_MOVE0				((uint8_t)0x00)
#define NAND_CMD_MOVE1				((uint8_t)0x35)
#define NAND_CMD_MOVE2				((uint8_t)0x85)
#define NAND_CMD_MOVE3				((uint8_t)0x10)
/** 
  * @brief  NAND memory status  
  */  
#define NAND_VALID_ADDRESS         ((uint32_t)0x00000100)
#define NAND_INVALID_ADDRESS       ((uint32_t)0x00000200)
#define NAND_TIMEOUT_ERROR         ((uint32_t)0x00000400)
#define NAND_BUSY                  ((uint32_t)0x00000000)
#define NAND_ERROR                 ((uint32_t)0x00000001)
#define NAND_READY                 ((uint32_t)0x00000040)


//#define NAND_SPAREWRITE_OVERFLOW   ((uint32_t)0x00000005) //添加——>页内空闲区域写入溢出——>状态值

/** 
  * @brief  FSMC NAND memory parameters  
  */  


#define NAND_PAGE_SIZE             ((u16)0x0800) /* 2K bytes per page w/o Spare Area */
#define NAND_BLOCK_SIZE            ((u16)0x0040) /* 64x2K bytes pages per block */
#define NAND_ZONE_SIZE             ((u16)0x0400) /* 1024 Block per zone */
#define NAND_SPARE_AREA_SIZE       ((u16)0x0040) /* last 64 bytes as spare area */
#define NAND_MAX_ZONE              ((u16)0x0001) /* 1 zones of 1024 block */

/** 
  * @brief  FSMC NAND memory address computation  
  */  

#define ADDR_1st_CYCLE(PADDR)       (uint8_t)(0x0)                       /* 1st addressing cycle */
#define ADDR_2nd_CYCLE(PADDR)       (uint8_t)(0x0)                       /* 2nd addressing cycle */
#define ADDR_3rd_CYCLE(PADDR)       (uint8_t)(PADDR & 0xFF)              /* 3rd addressing cycle */
#define ADDR_4th_CYCLE(PADDR)       (uint8_t)((PADDR>>8) & 0xFF)         /* 4th addressing cycle */  

/* FSMC NAND memory data computation */
#define DATA_1st_CYCLE(DATA)       (uint8_t)((DATA)& 0xFF)               /* 1st data cycle */
#define DATA_2nd_CYCLE(DATA)       (uint8_t)(((DATA)& 0xFF00) >> 8)      /* 2nd data cycle */
#define DATA_3rd_CYCLE(DATA)       (uint8_t)(((DATA)& 0xFF0000) >> 16)   /* 3rd data cycle */
#define DATA_4th_CYCLE(DATA)       (uint8_t)(((DATA)& 0xFF000000) >> 24) /* 4th data cycle */   


 
void NAND_Init(void);
void NAND_ReadID(NAND_IDTypeDef* NAND_ID);

//uint32_t NAND_Reset(void);
//uint32_t NAND_GetStatus(void);
//uint32_t NAND_ReadStatus(void);
//uint32_t NAND_ReadPage(uint32_t Address, uint8_t *pBuffer, NAND_DRV_CFG *cfg);
//uint32_t NAND_WritePage(uint32_t Address, uint8_t *pBuffer, NAND_DRV_CFG *cfg);
//uint32_t NAND_EraseBlock(uint32_t Address);



#endif





/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/
