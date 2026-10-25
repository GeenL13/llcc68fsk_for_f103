/**
  ******************************************************************************
  * @file    IAP_Main/Src/ymodem.c
  * @author  MCD Application Team
  * @brief   YMODEM receive functions adapted from STM32CubeF1 V1.8.7.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/** @addtogroup STM32F1xx_IAP
  * @{
  */

/* Includes ------------------------------------------------------------------*/
#include "flash_if.h"
#include "common.h"
#include "ymodem.h"
#include "main.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* This receive-only port uses CRC16. */
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* @note ATTENTION - please keep this variable 32bit aligned */
static uint8_t aPacketData[PACKET_1K_SIZE + PACKET_DATA_INDEX + PACKET_TRAILER_SIZE];
uint8_t aFileName[FILE_NAME_LENGTH + 1u];

/* Private function prototypes -----------------------------------------------*/
static HAL_StatusTypeDef ReceivePacket(uint8_t *p_data, uint32_t *p_length, uint32_t timeout);
static uint16_t UpdateCRC16(uint16_t crc_in, uint8_t byte);
static uint16_t Cal_CRC16(const uint8_t* p_data, uint32_t size);
static COM_StatusTypeDef ParseFileHeader(const uint8_t *p_data, uint32_t length, uint32_t *p_size);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Receive a packet from sender
  * @param  p_data: packet buffer
  * @param  p_length: 0 for EOT, 2 for sender abort, otherwise data length
  * @param  timeout: receive timeout in milliseconds
  * @retval HAL_OK: normally return, HAL_BUSY: abort by user
  */
static HAL_StatusTypeDef ReceivePacket(uint8_t *p_data, uint32_t *p_length, uint32_t timeout)
{
  uint32_t crc;
  uint32_t packet_size = 0;
  HAL_StatusTypeDef status;
  uint8_t char1;

  *p_length = 0;
  status = HAL_UART_Receive(&UartHandle, &char1, 1, timeout);

  if (status == HAL_OK)
  {
    switch (char1)
    {
      case SOH:
        packet_size = PACKET_SIZE;
        break;
      case STX:
        packet_size = PACKET_1K_SIZE;
        break;
      case EOT:
        break;
      case CA:
        if ((HAL_UART_Receive(&UartHandle, &char1, 1, timeout) == HAL_OK) && (char1 == CA))
        {
          packet_size = 2;
        }
        else
        {
          status = HAL_ERROR;
        }
        break;
      case ABORT1:
      case ABORT2:
        status = HAL_BUSY;
        break;
      default:
        status = HAL_ERROR;
        break;
    }
    *p_data = char1;

    if (packet_size >= PACKET_SIZE)
    {
      status = HAL_UART_Receive(&UartHandle, &p_data[PACKET_NUMBER_INDEX],
                                packet_size + PACKET_OVERHEAD_SIZE, timeout);

      /* Simple packet sanity check */
      if (status == HAL_OK)
      {
        if (p_data[PACKET_NUMBER_INDEX] !=
            ((p_data[PACKET_CNUMBER_INDEX]) ^ NEGATIVE_BYTE))
        {
          packet_size = 0;
          status = HAL_ERROR;
        }
        else
        {
          /* Check packet CRC */
          crc = p_data[packet_size + PACKET_DATA_INDEX] << 8;
          crc += p_data[packet_size + PACKET_DATA_INDEX + 1];
          if (Cal_CRC16(&p_data[PACKET_DATA_INDEX], packet_size) != crc)
          {
            packet_size = 0;
            status = HAL_ERROR;
          }
        }
      }
      else
      {
        packet_size = 0;
      }
    }
  }
  *p_length = packet_size;
  return status;
}

/**
  * @brief  Update CRC16 for input byte
  */
static uint16_t UpdateCRC16(uint16_t crc_in, uint8_t byte)
{
  uint32_t crc = crc_in;
  uint32_t in = byte | 0x100;

  do
  {
    crc <<= 1;
    in <<= 1;
    if (in & 0x100)
      ++crc;
    if (crc & 0x10000)
      crc ^= 0x1021;
  }
  while (!(in & 0x10000));

  return crc & 0xffffu;
}

/**
  * @brief  Cal CRC16 for YMODEM Packet
  */
static uint16_t Cal_CRC16(const uint8_t* p_data, uint32_t size)
{
  uint32_t crc = 0;
  const uint8_t* dataEnd = p_data + size;

  while (p_data < dataEnd)
    crc = UpdateCRC16(crc, *p_data++);

  crc = UpdateCRC16(crc, 0);
  crc = UpdateCRC16(crc, 0);

  return crc & 0xffffu;
}

/**
  * @brief  Parse the filename and decimal byte count in packet number zero.
  */
static COM_StatusTypeDef ParseFileHeader(const uint8_t *p_data, uint32_t length, uint32_t *p_size)
{
  uint32_t i = 0;
  uint32_t size = 0;
  uint32_t digits = 0;

  while ((i < length) && (p_data[i] != 0u) && (i < FILE_NAME_LENGTH))
  {
    aFileName[i] = p_data[i];
    i++;
  }
  if ((i == 0u) || (i >= length) || (i >= FILE_NAME_LENGTH))
  {
    return COM_DATA;
  }
  aFileName[i++] = '\0';

  while ((i < length) && (p_data[i] >= '0') && (p_data[i] <= '9') &&
         (digits < FILE_SIZE_LENGTH - 1u))
  {
    uint32_t digit = p_data[i++] - '0';
    if (size > (UINT32_MAX - digit) / 10u)
    {
      return COM_LIMIT;
    }
    size = size * 10u + digit;
    digits++;
  }

  if ((digits == 0u) || (i >= length) ||
      ((p_data[i] != 0u) && (p_data[i] != ' ')))
  {
    return COM_DATA;
  }
  if ((size < 8u) || (size > UPGRADE_TARGET_SIZE))
  {
    return COM_LIMIT;
  }

  *p_size = size;
  return COM_OK;
}

/* Public functions ---------------------------------------------------------*/
/**
  * @brief  Receive one firmware file using YMODEM with CRC16.
  * @param  p_size: exact file size after a successful transfer
  * @retval COM_StatusTypeDef result of reception/programming
  */
COM_StatusTypeDef Ymodem_Receive(uint32_t *p_size)
{
  uint32_t packet_length, filesize = 0, received = 0, write_length;
  uint32_t errors = 0;
  uint8_t packets_received = 0;
  uint8_t header_received = 0, file_done = 0, session_done = 0;
  COM_StatusTypeDef result = COM_OK;

  if (p_size == NULL)
  {
    return COM_ERROR;
  }
  *p_size = 0;

  /* Request the YMODEM header packet with CRC16. */
  Serial_PutByte(CRC16);

  while ((session_done == 0u) && (result == COM_OK))
  {
    switch (ReceivePacket(aPacketData, &packet_length, DOWNLOAD_TIMEOUT))
    {
      case HAL_OK:
        errors = 0;
        switch (packet_length)
        {
          case 2:
            /* Abort by sender */
            Serial_PutByte(ACK);
            result = COM_ABORT;
            break;
          case 0:
            /* End of transmission: the ST example acknowledges the first EOT. */
            if (file_done != 0u)
            {
              /* The EOT ACK was lost; sender repeated EOT. */
              Serial_PutByte(ACK);
              Serial_PutByte(CRC16);
              break;
            }
            if ((header_received == 0u) || (received != filesize) ||
                (FLASH_If_ValidateImage(filesize) != FLASHIF_OK))
            {
              Serial_PutByte(CA);
              Serial_PutByte(CA);
              result = COM_DATA;
              break;
            }
            Serial_PutByte(ACK);
            Serial_PutByte(CRC16);
            file_done = 1;
            break;
          default:
            /* Normal packet */
            if (file_done != 0u)
            {
              /* The empty packet number zero ends a single-file session. */
              if ((aPacketData[PACKET_NUMBER_INDEX] == 0u) &&
                  (aPacketData[PACKET_DATA_INDEX] == 0u))
              {
                Serial_PutByte(ACK);
                *p_size = filesize;
                session_done = 1;
              }
              else
              {
                Serial_PutByte(CA);
                Serial_PutByte(CA);
                result = COM_DATA;
              }
            }
            else if (header_received == 0u)
            {
              /* File name packet */
              if (aPacketData[PACKET_NUMBER_INDEX] != 0u)
              {
                Serial_PutByte(NAK);
                break;
              }
              result = ParseFileHeader(&aPacketData[PACKET_DATA_INDEX],
                                       packet_length, &filesize);
              if (result == COM_OK)
              {
                if (FLASH_If_Erase(UPGRADE_TARGET_BASE) != FLASHIF_OK)
                {
                  result = COM_DATA;
                }
              }
              if (result != COM_OK)
              {
                Serial_PutByte(CA);
                Serial_PutByte(CA);
                break;
              }
              header_received = 1;
              packets_received = 1;
              Serial_PutByte(ACK);
              Serial_PutByte(CRC16);
            }
            else if (aPacketData[PACKET_NUMBER_INDEX] == packets_received)
            {
              /* Data packet: discard padding beyond the size in packet zero. */
              if (received >= filesize)
              {
                Serial_PutByte(CA);
                Serial_PutByte(CA);
                result = COM_LIMIT;
                break;
              }
              write_length = packet_length;
              if (write_length > filesize - received)
              {
                write_length = filesize - received;
              }
              if (FLASH_If_Write(UPGRADE_TARGET_BASE + received,
                                 &aPacketData[PACKET_DATA_INDEX], write_length) == FLASHIF_OK)
              {
                received += write_length;
                packets_received++;
                Serial_PutByte(ACK);
              }
              else
              {
                Serial_PutByte(CA);
                Serial_PutByte(CA);
                result = COM_DATA;
              }
            }
            else if (aPacketData[PACKET_NUMBER_INDEX] == (uint8_t)(packets_received - 1u))
            {
              /* The previous ACK was lost: acknowledge the repeated packet. */
              Serial_PutByte(ACK);
              if (packets_received == 1u)
              {
                Serial_PutByte(CRC16);
              }
            }
            else
            {
              Serial_PutByte(NAK);
            }
            break;
        }
        break;
      case HAL_BUSY:
        Serial_PutByte(CA);
        Serial_PutByte(CA);
        result = COM_ABORT;
        break;
      default:
        errors++;
        if (errors >= MAX_ERRORS)
        {
          Serial_PutByte(CA);
          Serial_PutByte(CA);
          result = COM_TIMEOUT;
        }
        else if ((header_received == 0u) || (file_done != 0u))
        {
          Serial_PutByte(CRC16);
        }
        else
        {
          Serial_PutByte(NAK);
        }
        break;
    }
  }
  return result;
}

/**
  * @}
  */
