/**
  ******************************************************************************
  * @file    IAP_Main/Inc/common.h
  * @brief   USART adapter for the YMODEM receiver.
  ******************************************************************************
  */

#ifndef __COMMON_H
#define __COMMON_H

#include "usart.h"

/* The ST example names its UART handle UartHandle. CubeMX names ours huart1. */
#define UartHandle huart1

HAL_StatusTypeDef Serial_PutByte(uint8_t param);

#endif /* __COMMON_H */
