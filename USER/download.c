/**
  ******************************************************************************
  * @file    IAP/src/download.c 
  * @author  MCD Application Team
  * @version V3.3.0
  * @date    10/15/2010
  * @brief   This file provides the software which allows to download an image 
  *          to internal Flash.
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  */ 

/** @addtogroup IAP
  * @{
  */
/* Includes ------------------------------------------------------------------*/
#include "common.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
u32 CanSize;
s32 FilePos;
u32 PkgFlag = 0;
u32 loop_time=10000000;
u32 errorLOOP = 0;
u32 gCanStartFlag=0;
extern u16 PkgPos;
extern uint32_t CanMode;
extern u8 FileRcvBuf[1024];
extern uint8_t file_name[FILE_NAME_LENGTH];
extern u32 FlashDestination;
uint8_t tab_1024[1024] =
  {
    0
  };

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Download a file via serial port
  * @param  None
  * @retval None
  */
void SerialDownload(void)
{
  uint8_t Number[10] = "          ";
  int32_t Size = 0;

  SerialPutString("Waiting for the file to be sent ... (press 'a' to abort)\r\n");
  Size = Ymodem_Receive(&tab_1024[0]);
  if (Size > 0)
  {
    SerialPutString("\r\n Programming Completed Successfully!\r\n--------------------------------\r\n Name: ");
    SerialPutString(file_name);
    Int2Str(Number, Size);
    SerialPutString("\r\n Size: ");
    SerialPutString(Number);
    SerialPutString(" Bytes\r\n");
    SerialPutString("-------------------\r\n");
  }
  else if (Size == -1)
  {
    SerialPutString("\r\nThe image size is higher than the allowed space memory!\r\n");
  }
  else if (Size == -2)
  {
    SerialPutString("\r\nVerification failed!\r\n");
  }
  else if (Size == -3)
  {
    SerialPutString("\r\nAborted by user.\r\n");
  }
  else
  {
    SerialPutString("\r\nFailed to receive the file!\r\n");
  }
}

/**
  * @}
  */
/**
  * @brief  Download a file via can
  * @param  None
  * @retval 0 Success
  */
int32_t CanDownload(void)
{
	   u16 i,j;
		 u8* FRec;
	   u16 temp;
	   u16 Changebuf[512];
	   u32 canbegin=0;
	   FLASH_Status status = FLASH_COMPLETE;
     for (FRec=FileRcvBuf; ;)
    {
				if (FilePos<0) 
        {
					LCD_Printf(85, 70, "    Can Mode");
					LCD_Printf(80,140, " FileSize:%d", CanSize);
					FlashErase(CanSize);
					LCD_Printf(80, 100, " Update System...");
					FilePos= 0;
					canbegin=1;
				} 
			  else {
				  if (CanSize>(FilePos + 1024))
  				{
						if(PkgFlag==1)
						{
					FRec=FileRcvBuf;
				  for(i=0;i<512;i++)
				  {						    
					temp=(u16)FRec[1]<<8;
					temp+=(u16)FRec[0];	  
					FRec+=2;                  	   
					Changebuf[i]=temp;	     
				  }
						errorLOOP=0;
						PkgPos = 0;
					  FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
						for (j=0;(j<512)&&(status == FLASH_COMPLETE);j++)
					  {
						status=FLASH_ProgramHalfWord((FilePos+ApplicationAddress+2*j), Changebuf[j]);
					  }
						LCD_Printf(80, 180, " Transmit:%d",FilePos);
						FilePos += 1024;
						PkgFlag=0;	
					  } 
				  }
					else 
					{
						if(PkgFlag==1)
						{
					FRec=FileRcvBuf;
				  for(i=0;i<512;i++)
				  {						    
					temp=(u16)FRec[1]<<8;
					temp+=(u16)FRec[0];	  
					FRec+=2;                  	   
					Changebuf[i]=temp;	     
				  }
						errorLOOP=0;
					  FLASH_ClearFlag(FLASH_FLAG_BSY | FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
						for (j=0;(j<512)&&(status = FLASH_COMPLETE);j++)
					  {
						FLASH_ProgramHalfWord((FilePos+ApplicationAddress+2*j), Changebuf[j]);
					  }
						//STMFLASH_Write((FilePos+FLASH_BASE),Changebuf,512);
						LCD_Printf(80,100," Receive OK       ");
						PkgFlag=0;
						gCanStartFlag= 0;
						return 0;
					  }	
					}
				}
			if (canbegin==1)
			{
		  if (errorLOOP++>loop_time)
			 {
				gCanStartFlag= 0;
				errorLOOP=0;
				canbegin=0;
				LCD_Printf(80,100," Restart            "); 
       }
		  }
		}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void FlashErase(uint32_t size)
{
	u16 NbrOfPage, EraseCounter;
	FLASH_Status Status = FLASH_COMPLETE;
	NbrOfPage = FLASH_PagesMask(size);
	FLASH_Unlock();

	/* Erase the FLASH pages */
	for (EraseCounter = 0; (EraseCounter < NbrOfPage) && (Status == FLASH_COMPLETE); EraseCounter++)
	{
	  Status = FLASH_ErasePage(ApplicationAddress + (PAGE_SIZE * EraseCounter));
	}
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/*******************(C)COPYRIGHT 2010 STMicroelectronics *****END OF FILE******/
