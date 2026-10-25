/**
  ******************************************************************************
  * @file    IAP_Main/Src/flash_if.c
  * @brief   Erase and program only the configured receive slot.
  ******************************************************************************
  */

#include "flash_if.h"

_Static_assert((UPGRADE_TARGET_BASE % PAGE_SIZE) == 0u, "upgrade slot page alignment");
_Static_assert((UPGRADE_TARGET_SIZE % PAGE_SIZE) == 0u, "upgrade slot page count");
_Static_assert(UPGRADE_TARGET_BASE == APP_B_BASE, "receive only into inactive App B");
_Static_assert(UPGRADE_TARGET_BASE + UPGRADE_TARGET_SIZE == META0_BASE,
               "upgrade slot must stop before metadata");

uint32_t FLASH_If_Erase(uint32_t start)
{
  FLASH_EraseInitTypeDef erase = {0};
  uint32_t page_error = 0;
  HAL_StatusTypeDef status;

  if (start != UPGRADE_TARGET_BASE)
  {
    return FLASHIF_ERASEKO;
  }

  if (HAL_FLASH_Unlock() != HAL_OK)
  {
    return FLASHIF_ERASEKO;
  }

  erase.TypeErase = FLASH_TYPEERASE_PAGES;
  erase.Banks = FLASH_BANK_1;               // 对于单BANK芯片
  erase.PageAddress = UPGRADE_TARGET_BASE;  // 起始页地址
  erase.NbPages = UPGRADE_TARGET_SIZE / PAGE_SIZE;  // 擦除页数
  status = HAL_FLASHEx_Erase(&erase, &page_error);
  HAL_FLASH_Lock();

  return (status == HAL_OK) ? FLASHIF_OK : FLASHIF_ERASEKO;
}

uint32_t FLASH_If_Write(uint32_t destination, const uint8_t *p_source, uint32_t length)
{
  uint32_t i;
  uint32_t result = FLASHIF_OK;

  if ((p_source == NULL) || (length == 0u) ||
      (destination < UPGRADE_TARGET_BASE) ||
      (destination > UPGRADE_TARGET_BASE + UPGRADE_TARGET_SIZE) ||
      (length > UPGRADE_TARGET_BASE + UPGRADE_TARGET_SIZE - destination) ||
      (((length + 1u) & ~1u) > UPGRADE_TARGET_BASE + UPGRADE_TARGET_SIZE - destination) ||
      ((destination & 1u) != 0u))
  {
    return FLASHIF_WRITING_ERROR;
  }

  if (HAL_FLASH_Unlock() != HAL_OK)
  {
    return FLASHIF_WRITING_ERROR;
  }

  for (i = 0; i < length; i += 2u)
  {
    uint16_t high = (i + 1u < length) ? p_source[i + 1u] : 0xFFu;
    uint16_t halfword = (uint16_t)p_source[i] | (high << 8);

    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, destination + i, halfword) != HAL_OK)
    {
      result = FLASHIF_WRITING_ERROR;
      break;
    }
    if (*(volatile const uint16_t *)(destination + i) != halfword)
    {
      result = FLASHIF_WRITINGCTRL_ERROR;
      break;
    }
  }

  HAL_FLASH_Lock();
  return result;
}

uint32_t FLASH_If_ValidateImage(uint32_t length)
{
  uint32_t sp;
  uint32_t pc;
  uint32_t code;

  if ((length < 8u) || (length > UPGRADE_TARGET_SIZE))
  {
    return FLASHIF_WRITINGCTRL_ERROR;
  }

  sp = *(volatile const uint32_t *)UPGRADE_TARGET_BASE;
  pc = *(volatile const uint32_t *)(UPGRADE_TARGET_BASE + 4u);
  code = pc & ~1u;

  if ((sp <= RAM_FIRST) || (sp > RAM_END) || ((sp & 7u) != 0u) ||
      ((pc & 1u) == 0u) || (code < UPGRADE_TARGET_BASE) ||
      (code >= UPGRADE_TARGET_BASE + length))
  {
    return FLASHIF_WRITINGCTRL_ERROR;
  }

  return FLASHIF_OK;
}
