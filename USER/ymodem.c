/**
  ******************************************************************************
  * @file    IAP/src/ymodem.c 
  * @author  MCD Application Team
  * @version V3.3.0
  * @date    10/15/2010
  * @brief   This file provides all the software functions related to the ymodem 
  *          protocol.
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
#include "rtc.h"
#include "can.h"
#include "common.h"
#include "ili9325.h"
#include "sram.h"
#include "stm32f10x_flash.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
uint32_t wait_time = 5;
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t file_name[FILE_NAME_LENGTH];
uint32_t FlashDestination = ApplicationAddress; /* Flash user program offset */
uint32_t extSram = Bank1_SRAM3_ADDR;
uint16_t PageSize = PAGE_SIZE;
uint32_t EraseCounter = 0x0;
uint32_t NbrOfPage = 0;
FLASH_Status FLASHStatus = FLASH_COMPLETE;
uint32_t RamSource;
uint32_t WaitTime = 0;
extern uint32_t CanMode;
extern uint8_t tab_1024[1024];
  u8 canbuf[8]="$MR,1";
	u8 sizebuf[16];

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Receive byte from sender
  * @param  c: Character
  * @param  timeout: Timeout
  * @retval 0: Byte received
  *         -1: Timeout
  */
static  int32_t Receive_Byte (uint8_t *c, uint32_t timeout)
{
  while (timeout-- > 0)
  {
    if (SerialKeyPressed(c) == 1)
    {
      return 0;
    }
  }
  return -1;
}

/**
  * @brief  Send a byte
  * @param  c: Character
  * @retval 0: Byte sent
  */
static uint32_t Send_Byte (uint8_t c)
{
  SerialPutChar(c);
  return 0;
}

/**
  * @brief  Receive a packet from sender
  * @param  data
  * @param  length
  * @param  timeout
  *     0: end of transmission
  *    -1: abort by sender
  *    >0: packet length
  * @retval 0: normally return
  *        -1: timeout or packet error
  *         1: abort by user
  */
static int32_t Receive_Packet (uint8_t *data, int32_t *length, uint32_t timeout)
{
  uint16_t i, packet_size;
  uint8_t c;
  *length = 0;
  if (Receive_Byte(&c, timeout) != 0)
  {
    return -1;
  }
  switch (c)
  {
    case SOH:               //标题起始
      packet_size = PACKET_SIZE;
      break;
    case STX:               //正文起始
      packet_size = PACKET_1K_SIZE;
      break;
    case EOT:               //结束接收
      return 0;
    case CA:                //发送端停止
      if ((Receive_Byte(&c, timeout) == 0) && (c == CA))
      {
        *length = -1;
        return 0;
      }
      else
      {
        return -1;
      }
    case ABORT1:
    case ABORT2:
      return 1;
    default:
      return -1;
  }
  *data = c;
  for (i = 1; i < (packet_size + PACKET_OVERHEAD); i ++)
  {
    if (Receive_Byte(data + i, timeout) != 0)
    {
      return -1;
    }
  }
  if (data[PACKET_SEQNO_INDEX] != ((data[PACKET_SEQNO_COMP_INDEX] ^ 0xff) & 0xff))
  {
    return -1;
  }
  *length = packet_size;
  return 0;
}

/**
  * @brief  Receive a file using the ymodem protocol
  * @param  buf: Address of the first byte
  * @retval The size of the file
  */
int32_t Ymodem_Receive (uint8_t *buf)
{
	uint32_t   Wtime=wait_time;
  uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD], file_size[FILE_SIZE_LENGTH], *file_ptr, *buf_ptr;
  int32_t i, j, d, packet_length, session_done, file_done, packets_received, errors, session_begin, size = 0;
  uint16_t crc1, crc2;
  u8 res,b='1';
  /* Initialize FlashDestination variable */
  FlashDestination = ApplicationAddress;
  WaitTime = 0;
  extSram = Bank1_SRAM3_ADDR;
	//FLASH_Unlock();			//解锁
  for (session_done = 0, errors = 0, session_begin = 0; ;)
  {
    for (packets_received = 0, file_done = 0, buf_ptr = buf; ;)
    {
      switch (Receive_Packet(packet_data, &packet_length, NAK_TIMEOUT))
      {
        case 0:////////////////////////////////////////////////////////////////
          errors = 0;
          switch (packet_length)
          {
            /* Abort by sender */
            case - 1:	//switch (packet_length)
              Send_Byte(ACK);
              return 0;
            /* End of transmission */
            case 0:		//switch (packet_length)
              Send_Byte(ACK);
              file_done = 1;
              break;
            /* Normal packet */
            default:	//switch (packet_length)
              if ((packet_data[PACKET_SEQNO_INDEX] & 0xff) != (packets_received & 0xff))
              {
                Send_Byte(NAK);
              }
              else
              {
                if (packets_received == 0)
                {
                  /* Filename packet */
                  if (packet_data[PACKET_HEADER] != 0)
                  {
                    /* Filename packet has valid data */
                    for (i = 0, file_ptr = packet_data + PACKET_HEADER; (*file_ptr != 0) && (i < FILE_NAME_LENGTH);)
                    {
                      file_name[i++] = *file_ptr++;
                    }
                    file_name[i++] = '\0';
					//LCD_Printf(60,30, "FileName:%s", file_name);
                    for (i = 0, file_ptr ++; (*file_ptr != '\0') && (i < FILE_SIZE_LENGTH);)
                    {
                      file_size[i++] = *file_ptr++;
                    }
                    file_size[i++] = '\0';
                    //Str2Int(file_size, &size);
					sscanf((char*)file_size, "%d", &size);
					LCD_Printf(80,140, " FileSize:%d", size);

                    /* Test the size of the image to be sent */
                    /* Image size is greater than Flash size */
                    if (size > (FLASH_SIZE - 1))
                    {
                      /* End session */
                      Send_Byte(CA);
                      Send_Byte(CA);
                      return -1;
                    }
					SerialPutString("format\r\n");
					WaitTime = 0;	
										if(CanMode==1)
										{
										sprintf((char*)sizebuf,"$SIZE,%d",size);
										Can_Send_Msg(sizebuf,8);
										Can_Send_Msg(&sizebuf[8],8);
										delay_ms(5000);
										}
                    /* Erase the needed pages where the user application will be loaded */
                    /* Define the number of page to be erased */
										else
										{
                    NbrOfPage = FLASH_PagesMask(size);

                    /* Erase the FLASH pages */
                    for (EraseCounter = 0; (EraseCounter < NbrOfPage) && (FLASHStatus == FLASH_COMPLETE); EraseCounter++)
                    {
                      FLASHStatus = FLASH_ErasePage(FlashDestination + (PageSize * EraseCounter));
                    }
									  }
					LCD_Printf(80, 100, " Update System...");
                    Send_Byte(ACK);
                    Send_Byte(CRC16);
                  }
                  /* Filename packet is empty, end session */
                  else
                  {
                    Send_Byte(ACK);
                    file_done = 1;
                    session_done = 1;
					LCD_Printf(80, 100, " Trnasmit OK.");
					SerialPutString("Trnasmit OK.");
                    break;
                  }
                }
                /* Data packet */
                else
                {
                  memcpy(buf_ptr, packet_data + PACKET_HEADER, packet_length);
				  crc1 = crc_ccitt(buf_ptr, packet_length);
				  memcpy(&crc2, packet_data + PACKET_HEADER + packet_length, 2);
				  if (crc1 == crc2) {
					  RamSource = (uint32_t)buf;
					  memcpy((uint8_t *)extSram, buf_ptr, packet_length);
					  extSram+=packet_length;
					  LCD_Printf(80, 180, " Transmit:%d", (FlashDestination-ApplicationAddress));
					  WaitTime = 0;
					  if(CanMode==1)
						{
						for (d = 0;d < 1024;d += 8)
						{
						Can_Send_Msg(&buf_ptr[d],8);
						delay_ms(2);
						}
						FlashDestination+=1024;
					  }else
            { 						
					  for (j = 0;(j < packet_length) && (FlashDestination <  ApplicationAddress + size);j += 4)
					  {
						//Program the data received into STM32F10x Flash 
						FLASH_ProgramWord(FlashDestination, *(uint32_t*)RamSource);

						if (*(uint32_t*)FlashDestination != *(uint32_t*)RamSource)
						{
						  //End session 
						  Send_Byte(CA);
						  Send_Byte(CA);
						  LCD_Printf(80, 140, " Write FLASH ERROR!");
						  return -2;
						}
						FlashDestination += 4;
						RamSource += 4;
					  }
					  }
					  showTime();
					  Send_Byte(ACK);
				  
					  /*
					  if ((extSram>=(Bank1_SRAM3_ADDR+size)) || (extSram>=(Bank1_SRAM3_ADDR+SRAM_LEN))) {
						  file_done = 1;
						  session_begin = 1;
						  RamSource = (uint32_t)Bank1_SRAM3_ADDR;
						  for (j = 0;(j < size) && (RamSource <  extSram) && (FlashDestination <  ApplicationAddress + size);j += 4)
						  {
							//Program the data received into STM32F10x Flash 
							FLASH_ProgramWord(FlashDestination, *(uint32_t*)RamSource);

							if (*(uint32_t*)FlashDestination != *(uint32_t*)RamSource)
							{
							  //End session 
							  LCD_Printf(0, 200, "Write FLASH ERROR!");
							  return -2;
							}
							FlashDestination += 4;
							RamSource += 4;
						  }
						  
						  break;
					  }
					  */
				  } else {
					  // crc1 != crc2
					  //Send_Byte(NCK);
					  LCD_Printf(80, 140, " CRC check error.");
				  }
                }
				if (FlashDestination <  ApplicationAddress + size) {
					packets_received ++;
					session_begin = 1;
					Send_Byte(CRC16);
				} else {
// 					file_done = 1;
// 					session_begin = 1;
					LCD_Printf(80, 140, " Transmit:%d", (FlashDestination-ApplicationAddress));
					LCD_Printf(80, 120, " Trnasmit OK.");
					SerialPutString("Trnasmit OK.");
					return size;
					break;
				}
              }
          }
          break;
        case 1:////////////////////////////////////////////////////////////////
          Send_Byte(CA);
          Send_Byte(CA);
          return -3;
        default:///////////////////////////////////////////////////////////////
          if (session_begin > 0)
          {
            errors ++;
          }
          if (errors > MAX_ERRORS)
          {
            Send_Byte(CA);
            Send_Byte(CA);
            return 0;
          }
		  if (WaitTime++>wait_time)
			  return 0;
			    showTime();
					LCD_Printf(65, 140, "  Booting System in %d.",Wtime--);
          Send_Byte(CRC16);
      }
      if (file_done != 0)
      {
        break;
      }
    }
    if (session_done != 0)
    {
      break;
    }
  }
  return (int32_t)size;
	//FLASH_Lock();
}

/**
  * @brief  check response using the ymodem protocol
  * @param  buf: Address of the first byte
  * @retval The size of the file
  */
int32_t Ymodem_CheckResponse(uint8_t c)
{
  return 0;
}

/**
  * @brief  Prepare the first block
  * @param  timeout
  *     0: end of transmission
  */
void Ymodem_PrepareIntialPacket(uint8_t *data, const uint8_t* fileName, uint32_t *length)
{
  uint16_t i, j;
  uint8_t file_ptr[10];
  
  /* Make first three packet */
  data[0] = SOH;
  data[1] = 0x00;
  data[2] = 0xff;
  
  /* Filename packet has valid data */
  for (i = 0; (fileName[i] != '\0') && (i < FILE_NAME_LENGTH);i++)
  {
     data[i + PACKET_HEADER] = fileName[i];
  }

  data[i + PACKET_HEADER] = 0x00;
  
  Int2Str (file_ptr, *length);
  for (j =0, i = i + PACKET_HEADER + 1; file_ptr[j] != '\0' ; )
  {
     data[i++] = file_ptr[j++];
  }
  
  for (j = i; j < PACKET_SIZE + PACKET_HEADER; j++)
  {
    data[j] = 0;
  }
}

/**
  * @brief  Prepare the data packet
  * @param  timeout
  *     0: end of transmission
  */
void Ymodem_PreparePacket(uint8_t *SourceBuf, uint8_t *data, uint8_t pktNo, uint32_t sizeBlk)
{
  uint16_t i, size, packetSize;
  uint8_t* file_ptr;
  
  /* Make first three packet */
  packetSize = sizeBlk >= PACKET_1K_SIZE ? PACKET_1K_SIZE : PACKET_SIZE;
  size = sizeBlk < packetSize ? sizeBlk :packetSize;
  if (packetSize == PACKET_1K_SIZE)
  {
     data[0] = STX;
  }
  else
  {
     data[0] = SOH;
  }
  data[1] = pktNo;
  data[2] = (~pktNo);
  file_ptr = SourceBuf;
  
  /* Filename packet has valid data */
  for (i = PACKET_HEADER; i < size + PACKET_HEADER;i++)
  {
     data[i] = *file_ptr++;
  }
  if ( size  <= packetSize)
  {
    for (i = size + PACKET_HEADER; i < packetSize + PACKET_HEADER; i++)
    {
      data[i] = 0x1A; /* EOF (0x1A) or 0x00 */
    }
  }
}

/**
  * @brief  Update CRC16 for input byte
  * @param  CRC input value 
  * @param  input byte
   * @retval None
  */
uint16_t UpdateCRC16(uint16_t crcIn, uint8_t byte)
{
 uint32_t crc = crcIn;
 uint32_t in = byte|0x100;
 do
 {
 crc <<= 1;
 in <<= 1;
 if(in&0x100)
 ++crc;
 if(crc&0x10000)
 crc ^= 0x1021;
 }
 while(!(in&0x10000));
 return crc&0xffffu;
}


/**
  * @brief  Cal CRC16 for YModem Packet
  * @param  data
  * @param  length
   * @retval None
  */
uint16_t Cal_CRC16(const uint8_t* data, uint32_t size)
{
 uint32_t crc = 0;
 const uint8_t* dataEnd = data+size;
 while(data<dataEnd)
  crc = UpdateCRC16(crc,*data++);
 
 crc = UpdateCRC16(crc,0);
 crc = UpdateCRC16(crc,0);
 return crc&0xffffu;
}

/**
  * @brief  Cal Check sum for YModem Packet
  * @param  data
  * @param  length
   * @retval None
  */
uint8_t CalChecksum(const uint8_t* data, uint32_t size)
{
 uint32_t sum = 0;
 const uint8_t* dataEnd = data+size;
 while(data < dataEnd )
   sum += *data++;
 return sum&0xffu;
}

/**
  * @brief  Transmit a data packet using the ymodem protocol
  * @param  data
  * @param  length
   * @retval None
  */
void Ymodem_SendPacket(uint8_t *data, uint16_t length)
{
  uint16_t i;
  i = 0;
  while (i < length)
  {
    Send_Byte(data[i]);
    i++;
  }
}

/**
  * @brief  Transmit a file using the ymodem protocol
  * @param  buf: Address of the first byte
  * @retval The size of the file
  */
uint8_t Ymodem_Transmit (uint8_t *buf, const uint8_t* sendFileName, uint32_t sizeFile)
{
  
  uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD];
  uint8_t FileName[FILE_NAME_LENGTH];
  uint8_t *buf_ptr, tempCheckSum ;
  uint16_t tempCRC, blkNumber;
  uint8_t receivedC[2], CRC16_F = 0, i;
  uint32_t errors, ackReceived, size = 0, pktSize;

  errors = 0;
  ackReceived = 0;
  for (i = 0; i < (FILE_NAME_LENGTH - 1); i++)
  {
    FileName[i] = sendFileName[i];
  }
  CRC16_F = 1;       
    
  /* Prepare first block */
  Ymodem_PrepareIntialPacket(&packet_data[0], FileName, &sizeFile);
  
  do 
  {
    /* Send Packet */
    Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);
    /* Send CRC or Check Sum based on CRC16_F */
    if (CRC16_F)
    {
       tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
       Send_Byte(tempCRC >> 8);
       Send_Byte(tempCRC & 0xFF);
    }
    else
    {
       tempCheckSum = CalChecksum (&packet_data[3], PACKET_SIZE);
       Send_Byte(tempCheckSum);
    }
  
    /* Wait for Ack and 'C' */
    if (Receive_Byte(&receivedC[0], 10000) == 0)  
    {
      if (receivedC[0] == ACK)
      { 
        /* Packet transfered correctly */
        ackReceived = 1;
      }
    }
    else
    {
        errors++;
    }
  }while (!ackReceived && (errors < 0x0A));
  
  if (errors >=  0x0A)
  {
    return errors;
  }
  buf_ptr = buf;
  size = sizeFile;
  blkNumber = 0x01;
  /* Here 1024 bytes package is used to send the packets */
  
  
  /* Resend packet if NAK  for a count of 10 else end of commuincation */
  while (size)
  {
    /* Prepare next packet */
    Ymodem_PreparePacket(buf_ptr, &packet_data[0], blkNumber, size);
    ackReceived = 0;
    receivedC[0]= 0;
    errors = 0;
    do
    {
      /* Send next packet */
      if (size >= PACKET_1K_SIZE)
      {
        pktSize = PACKET_1K_SIZE;
       
      }
      else
      {
        pktSize = PACKET_SIZE;
      }
      Ymodem_SendPacket(packet_data, pktSize + PACKET_HEADER);
      /* Send CRC or Check Sum based on CRC16_F */
      /* Send CRC or Check Sum based on CRC16_F */
      if (CRC16_F)
      {
         tempCRC = Cal_CRC16(&packet_data[3], pktSize);
         Send_Byte(tempCRC >> 8);
         Send_Byte(tempCRC & 0xFF);
      }
      else
      {
        tempCheckSum = CalChecksum (&packet_data[3], pktSize);
        Send_Byte(tempCheckSum);
      }
      
      /* Wait for Ack */
      if ((Receive_Byte(&receivedC[0], 100000) == 0)  && (receivedC[0] == ACK))
      {
        ackReceived = 1;  
        if (size > pktSize)
        {
           buf_ptr += pktSize;  
           size -= pktSize;
           if (blkNumber == (FLASH_IMAGE_SIZE/1024))
           {
             return 0xFF; /*  error */
           }
           else
           {
              blkNumber++;
           }
        }
        else
        {
          buf_ptr += pktSize;
          size = 0;
        }
      }
      else
      {
        errors++;
      }
    }while(!ackReceived && (errors < 0x0A));
    /* Resend packet if NAK  for a count of 10 else end of commuincation */
    
    if (errors >=  0x0A)
    {
      return errors;
    }
    
  }
  ackReceived = 0;
  receivedC[0] = 0x00;
  errors = 0;
  do 
  {
    Send_Byte(EOT);
    /* Send (EOT); */
    /* Wait for Ack */
      if ((Receive_Byte(&receivedC[0], 10000) == 0)  && receivedC[0] == ACK)
      {
        ackReceived = 1;  
      }
      else
      {
        errors++;
      }
  }while (!ackReceived && (errors < 0x0A));
    
  if (errors >=  0x0A)
  {
    return errors;
  }
  
  /* Last packet preparation */
  ackReceived = 0;
  receivedC[0] = 0x00;
  errors = 0;

  packet_data[0] = SOH;
  packet_data[1] = 0;
  packet_data [2] = 0xFF;

  for (i = PACKET_HEADER; i < (PACKET_SIZE + PACKET_HEADER); i++)
  {
     packet_data [i] = 0x00;
  }
  
  do 
  {
    /* Send Packet */
    Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);
    /* Send CRC or Check Sum based on CRC16_F */
    tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
    Send_Byte(tempCRC >> 8);
    Send_Byte(tempCRC & 0xFF);
  
    /* Wait for Ack and 'C' */
    if (Receive_Byte(&receivedC[0], 10000) == 0)  
    {
      if (receivedC[0] == ACK)
      { 
        /* Packet transfered correctly */
        ackReceived = 1;
      }
    }
    else
    {
        errors++;
    }
 
  }while (!ackReceived && (errors < 0x0A));
  /* Resend packet if NAK  for a count of 10  else end of commuincation */
  if (errors >=  0x0A)
  {
    return errors;
  }  
  
  do 
  {
    Send_Byte(EOT);
    /* Send (EOT); */
    /* Wait for Ack */
      if ((Receive_Byte(&receivedC[0], 10000) == 0)  && receivedC[0] == ACK)
      {
        ackReceived = 1;  
      }
      else
      {
        errors++;
      }
  }while (!ackReceived && (errors < 0x0A));
    
  if (errors >=  0x0A)
  {
    return errors;
  }
  return 0; /* file trasmitted successfully */
}

const unsigned short ccitt_table[256] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

uint16_t crc_ccitt(unsigned char *q, int len)
{
	unsigned short crc = 0;

	while (len-- > 0)
		crc = ccitt_table[(crc >> 8 ^ *q++) & 0xff] ^ (crc << 8);
	return ((~crc)&0xFFFF);
}

/**
  * @}
  */

/*******************(C)COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
