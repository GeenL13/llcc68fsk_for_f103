/**
  ******************************************************************************
  * @file    IAP_Main/Inc/ymodem.h
  * @author  MCD Application Team
  * @brief   YMODEM receiver declarations (adapted from STM32CubeF1 V1.8.7).
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

#ifndef __YMODEM_H_
#define __YMODEM_H_

#include <stdint.h>
#include "upgrade_config.h"

typedef enum
{
  COM_OK       = 0x00,
  COM_ERROR    = 0x01,
  COM_ABORT    = 0x02,
  COM_TIMEOUT  = 0x03,
  COM_DATA     = 0x04,
  COM_LIMIT    = 0x05
} COM_StatusTypeDef;

/* Packet structure defines */
#define PACKET_HEADER_SIZE      ((uint32_t)3)
#define PACKET_DATA_INDEX       ((uint32_t)4)
#define PACKET_START_INDEX      ((uint32_t)1)
#define PACKET_NUMBER_INDEX     ((uint32_t)2)
#define PACKET_CNUMBER_INDEX    ((uint32_t)3)
#define PACKET_TRAILER_SIZE     ((uint32_t)2)
#define PACKET_OVERHEAD_SIZE    (PACKET_HEADER_SIZE + PACKET_TRAILER_SIZE - 1)
#define PACKET_SIZE             ((uint32_t)128)
#define PACKET_1K_SIZE          ((uint32_t)1024)

/* /-------- Packet in IAP memory ------------------------------------------\
 * | 0      |  1    |  2     |  3   |  4      | ... | n+4     | n+5  | n+6  |
 * |------------------------------------------------------------------------|
 * | unused | start | number | !num | data[0] | ... | data[n] | crc0 | crc1 |
 * \------------------------------------------------------------------------/ */

#define FILE_NAME_LENGTH        ((uint32_t)64)
#define FILE_SIZE_LENGTH        ((uint32_t)16)

#define SOH                     ((uint8_t)0x01)
#define STX                     ((uint8_t)0x02)
#define EOT                     ((uint8_t)0x04)
#define ACK                     ((uint8_t)0x06)
#define NAK                     ((uint8_t)0x15)
#define CA                      ((uint8_t)0x18)
#define CRC16                   ((uint8_t)0x43)
#define NEGATIVE_BYTE           ((uint8_t)0xFF)
#define ABORT1                  ((uint8_t)0x41)
#define ABORT2                  ((uint8_t)0x61)

#define DOWNLOAD_TIMEOUT        YMODEM_PACKET_TIMEOUT_MS
#define MAX_ERRORS              YMODEM_MAX_ERRORS

extern uint8_t aFileName[FILE_NAME_LENGTH + 1u];
COM_StatusTypeDef Ymodem_Receive(uint32_t *p_size);

#endif /* __YMODEM_H_ */
