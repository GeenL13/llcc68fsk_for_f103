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
#include "can.h"
#include "dma.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "radio.h"
#include "serial.h"
#include "Can_Config.h"
#include "can_receive_control.h"
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
// CAN数据帧
typedef struct __attribute__((__packed__))
{
    uint8_t  frame_start_1; // 帧头: 0x55
    uint8_t  frame_start_2; // 帧头: 0xAA
    uint8_t  ide;        // 0: 标准帧, 1: 扩展帧
    uint8_t  rtr;        // 0: 数据帧, 1: 远程帧
    uint8_t  dlc;        // 0~8
    uint8_t  reserved;   // 保留位
    uint32_t id;         // ide=0 时，仅低 11 bit 有效；ide=1 时，低 29 bit 有效
    uint8_t  data[8];    // 实际用前 dlc 个字节
    uint16_t crc;       // CRC 校验码
} can_frame_t;
can_frame_t can_frame;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void CAN_Callback(void* context)
{
  // can数据存储变量
	CAN_RxHeaderTypeDef header;
	uint8_t data[8] = {0};

	// 从FIFO中读取数据
	HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &header, data);

	// 符合接收条件则存储数据
  if (check_and_update_cooldown(header.StdId) == 0)
  {
    // 填充数据帧
    can_frame_buffer[can_frame_current_receive_index]
    .frame_start_1 = 0x55;
    can_frame_buffer[can_frame_current_receive_index]
    .frame_start_2 = 0xAA;
    can_frame_buffer[can_frame_current_receive_index]
    .ide = (header.IDE == CAN_ID_STD) ? 0 : 1;
    can_frame_buffer[can_frame_current_receive_index]
    .rtr = (header.RTR == CAN_RTR_DATA) ? 0 : 1;
    can_frame_buffer[can_frame_current_receive_index]
    .dlc = header.DLC;
    can_frame_buffer[can_frame_current_receive_index]
    .id = (header.IDE == CAN_ID_STD) ? header.StdId : header.ExtId;
    memcpy(can_frame_buffer[can_frame_current_receive_index].data, data, 8);	// 此处应当固定8字节，以防内存界限出现错误
    // 计算crc
    can_frame_buffer[can_frame_current_receive_index].crc 
    = Calculate_CRC16((uint8_t*)&can_frame_buffer[can_frame_current_receive_index], 
    sizeof(can_frame_t) - 2);

    // 更新索引和计数
    can_frame_current_receive_index 
    = (can_frame_current_receive_index + 1) % CAN_FRAME_BUFFER_SIZE;

    can_frame_buffer_receive_count++;
  }
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
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_CAN_Init();
  /* USER CODE BEGIN 2 */
  radio_init();
  serial_receive_byte_on();
  // 初始化can
  CAN_Init(&hcan);
  CAN_Filter_Config(&hcan, 
    CAN_FILTER(13) 
    | CAN_FIFO_1 
    | CAN_STDID 
    | CAN_DATA_TYPE, 
    0xC0, 
    0x00);	

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    radio_loop();
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
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV2;
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
