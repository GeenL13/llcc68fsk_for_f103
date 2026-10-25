/**
  ******************************************************************************
  * @file    IAP_Main/Inc/flash_if.h
  * @brief   Flash interface for the configured receive slot.
  ******************************************************************************
  */

#ifndef __FLASH_IF_H
#define __FLASH_IF_H

#include "stm32f1xx_hal.h"
#include "upgrade_config.h"

#define FLASHIF_OK                  0u
#define FLASHIF_ERASEKO             1u
#define FLASHIF_WRITING_ERROR       2u
#define FLASHIF_WRITINGCTRL_ERROR   3u
uint32_t FLASH_If_Erase(uint32_t start);
uint32_t FLASH_If_Write(uint32_t destination, const uint8_t *p_source, uint32_t length);
uint32_t FLASH_If_ValidateImage(uint32_t length);

#endif /* __FLASH_IF_H */
