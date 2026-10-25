/**
  ******************************************************************************
  * @file    IAP_Main/Src/common.c
  * @brief   USART adapter for the YMODEM receiver.
  ******************************************************************************
  */

#include "common.h"

HAL_StatusTypeDef Serial_PutByte(uint8_t param)
{
  return HAL_UART_Transmit(&UartHandle, &param, 1, 100);
}
