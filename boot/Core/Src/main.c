/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "stm32f1xx_hal_uart.h"
#include "update.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "flash_layout.h"
#include "update_uart.h"
#include "ymodem.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile COM_StatusTypeDef g_upgrade_status = COM_ERROR;  // 升级状态
volatile uint32_t g_upgrade_size = 0;                     // 升级文件大小

// 写入目标应用程序的起始地址和大小
uint32_t app_base = APP_A_BASE;
uint32_t app_size = APP_SIZE;
// 获取到固件文件标志
bool firmware_received = false;
// 程序损坏标志
bool app_corrupted = false;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
extern bool boot_jump(uint32_t base);

/* 保留原扇区擦除函数；F103 HAL 没有该接口，本芯片不编译它。 */
#if defined(FLASH_TYPEERASE_SECTORS)
// 擦除flash扇区
static void Flash_Erase_Sector(uint32_t sector) 
{
    FLASH_EraseInitTypeDef EraseInitStruct = {0};
    uint32_t SectorError = 0;

    // 配置擦除参数
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS; // 按扇区擦除
    EraseInitStruct.Banks = FLASH_BANK_1; // 对于单BANK芯片
    EraseInitStruct.Sector = sector;      // 要擦除的扇区号
    EraseInitStruct.NbSectors = 1;        // 擦除1个扇区
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3; // 电压范围，通常为3

    // 执行擦除，并检查错误
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK) {
        // SectorError会返回出错的扇区号
        Error_Handler();
    }
}
#endif

/* F103 按页擦除；调用前必须解锁 Flash。 */
static bool Flash_Erase_Pages(uint32_t start, uint32_t page_count)
{
  FLASH_EraseInitTypeDef erase = {0}; // 擦除参数结构体
  uint32_t page_error = 0;            // 擦除错误页号

  erase.TypeErase = FLASH_TYPEERASE_PAGES;  // 按页擦除
  erase.Banks = FLASH_BANK_1;               // 对于单BANK芯片
  erase.PageAddress = start;                // 起始页地址
  erase.NbPages = page_count;               // 擦除页数
  // 执行擦除，并检查错误
  return HAL_FLASHEx_Erase(&erase, &page_error) == HAL_OK;
}

// 将程序包写入flash
static bool write_flash(void *context, update_event_t event,
                                  const update_file_t *file, 
                                  uint32_t offset,
                                  const uint8_t *data, 
                                  uint16_t length)
{
  static uint32_t next_offset;  // 下一个预期的偏移量
  static bool erased;
  (void)context;

  if (event == UPDATE_EVENT_HEADER)
  {
    firmware_received = true;
    erased = false;
    next_offset = 0;
    if (file == NULL || file->size == 0u || file->size > app_size ||
        (app_base % PAGE_SIZE) != 0u)
    {
      return false;
    }
    /* 向上取整，只擦除文件占用的页。 */
    uint32_t page_count = (file->size - 1u) / PAGE_SIZE + 1u;
    if (HAL_FLASH_Unlock() != HAL_OK)
    {
      return false;
    }
    if (!Flash_Erase_Pages(app_base, page_count))
    {
      return false;
    }
    erased = true;
    return true;
  }
  else if (event == UPDATE_EVENT_DATA)
  {
    if (!erased || file == NULL || data == NULL || length == 0u ||
        file->size > app_size || offset != next_offset ||
        offset > file->size || length > file->size - offset ||
        (offset & 1u) != 0u)
    {
      return false;
    }
    /* F103 每次写入 16 位；文件末尾不足 16 位时用 0xFF 填高字节。 */
    for (uint32_t i = 0; i < length; i += 2u)
    {
      uint16_t high = (i + 1u < length) ? data[i + 1u] : 0xFFu;
      uint16_t value = (uint16_t)data[i] | (uint16_t)(high << 8);
      uint32_t address = app_base + offset + i;
      if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, address, value) != HAL_OK ||
          *(volatile const uint16_t *)address != value)
      {
        return false;
      }
    }
    next_offset += length;
    return true;
  }
  return false;
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  update_file_t file = {0};
  HAL_FLASH_Unlock(); /* 接收前解锁 Flash，接收失败或成功后重新锁定。 */
  update_result_t result = update_receive_uart(&huart1, 
    app_size, 
    &file, 
    write_flash, 
    NULL);
  HAL_FLASH_Lock(); /* 接收成功或失败后都重新锁定 Flash。 */
  if (result != UPDATE_OK && firmware_received)
  {
    // 接收到固件文件，但没有完成固件升级，说明出错
    g_upgrade_status = COM_ERROR;
    g_upgrade_size = 0;
    app_corrupted = true; /* 接收失败，程序可能损坏。 */
  }
  else if (false == firmware_received)
  {
    // 没有接收到固件文件，可以直接跳转到原来的程序
    app_corrupted = false;
    HAL_UART_Transmit(&huart1, 
      (uint8_t *)"【INFO】未接收到固件文件，跳转到原程序...\r\n", 
      60, 
      HAL_MAX_DELAY);
  }
  else 
  {
    // 接收成功，固件升级完成
    g_upgrade_status = COM_OK;
    g_upgrade_size = file.size;
    HAL_UART_Transmit(&huart1, 
      (uint8_t *)"【INFO】升级成功，跳转到新程序...\r\n", 
      48, 
      HAL_MAX_DELAY);
    app_corrupted = false;
  }
  /* Activation and rollback metadata are separate from file reception. */
  if (app_corrupted)
  {
    HAL_UART_Transmit(&huart1, 
      (uint8_t *)"【INFO】升级失败，请复位重试。\r\n", 
      50, 
      HAL_MAX_DELAY);
    while (1)
    {
      HAL_Delay(1000);
    }
  }
  boot_jump(APP_A_BASE);

  /* If App A is invalid, keep the serial recovery path available. */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
