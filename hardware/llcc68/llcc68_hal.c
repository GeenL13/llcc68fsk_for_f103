/**
 * @file      llcc68_hal.c
 *
 * @brief     LLCC68 radio driver implementation of the HAL interface
 *            用于llcc68在具体MCU平台上的硬件抽象层接口实现
 */

#include "llcc68_hal.h"

#include <stdio.h>
#include <string.h>
#include "stm32f1xx_hal.h"
#include "main.h"
#include "spi.h"

/**
 * Radio data transfer - write
 * 无线电数据传输 - 写入
 *
 * @remark Shall be implemented by the user
 *         应由用户实现
 *
 * @param [in] context          Radio implementation parameters
 *                              无线电实现参数
 * @param [in] command          Pointer to the buffer to be transmitted
 *                              指向要发送的缓冲区的指针
 * @param [in] command_length   Buffer size to be transmitted
 *                              要发送的缓冲区大小
 * @param [in] data             Pointer to the buffer to be transmitted
 *                              指向要发送的缓冲区的指针
 * @param [in] data_length      Buffer size to be transmitted
 *                              要发送的缓冲区大小
 *
 * @returns Operation status
 *          操作状态
 */
llcc68_hal_status_t llcc68_hal_write( const void* context, const uint8_t* command, const uint16_t command_length,
                                      const uint8_t* data, const uint16_t data_length )
{
    HAL_StatusTypeDef res;
    // 整合command和data为一个发送缓冲区
    uint8_t send_buf[command_length + data_length];
    memset(send_buf, 0, sizeof(send_buf));
    for (uint16_t i = 0; i < command_length; i++) {
        send_buf[i] = command[i];
    }
    for (uint16_t i = 0; i < data_length; i++) {
        send_buf[command_length + i] = data[i];
    }
    // 进行SPI发送
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
    res = HAL_SPI_Transmit(&hspi1, send_buf, (uint16_t)(command_length + data_length), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
    if (res != HAL_OK) {
        return LLCC68_HAL_STATUS_ERROR;
    }
    return LLCC68_HAL_STATUS_OK;
}

/**
 * Radio data transfer - read
 * 无线电数据传输 - 读取
 *
 * @remark Shall be implemented by the user
 *         应由用户实现
 *
 * @param [in] context          Radio implementation parameters
 *                              无线电实现参数
 * @param [in] command          Pointer to the buffer to be transmitted
 *                              指向要发送的缓冲区的指针
 * @param [in] command_length   Buffer size to be transmitted
 *                              要发送的缓冲区大小
 * @param [in] data             Pointer to the buffer to be received
 *                              指向要接收的缓冲区的指针
 * @param [in] data_length      Buffer size to be received
 *                              要接收的缓冲区大小
 *
 * @returns Operation status
 *          操作状态
 */
llcc68_hal_status_t llcc68_hal_read( const void* context, const uint8_t* command, const uint16_t command_length,
                                     uint8_t* data, const uint16_t data_length )
{
    HAL_StatusTypeDef res;
    // 整合command和data为一个发送缓冲区，中间用NOP填充
    uint8_t send_buf[command_length + data_length];
    uint8_t recv_buf[command_length + data_length];
    memset(send_buf, 0, sizeof(send_buf));
    memset(recv_buf, 0, sizeof(recv_buf));
    for (uint16_t i = 0; i < command_length; i++) {
        send_buf[i] = command[i];
    }
    for (uint16_t i = 0; i < data_length; i++) {
        send_buf[command_length + i] = LLCC68_NOP;
    }
    // 进行SPI收发
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);
    res = HAL_SPI_TransmitReceive(&hspi1, send_buf, recv_buf, (uint16_t)(command_length + data_length), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
    if (res != HAL_OK) {
        return LLCC68_HAL_STATUS_ERROR;
    }
    // data仅保留接收的数据部分
    for (uint16_t i = 0; i < data_length; i++) {
        data[i] = recv_buf[i + command_length];
    }
    return LLCC68_HAL_STATUS_OK;
}

/**
 * Reset the radio
 * 复位无线电
 *
 * @remark Shall be implemented by the user
 *         应由用户实现
 *
 * @param [in] context Radio implementation parameters
 *                     无线电实现参数
 *
 * @returns Operation status
 *          操作状态
 */
llcc68_hal_status_t llcc68_hal_reset( const void* context )
{
    HAL_GPIO_TogglePin(NRESET_GPIO_Port, NRESET_Pin);
    HAL_Delay(1);
    HAL_GPIO_TogglePin(NRESET_GPIO_Port, NRESET_Pin);
    return LLCC68_HAL_STATUS_OK;
}

/**
 * Wake the radio up.
 * 唤醒无线电。
 *
 * @remark Shall be implemented by the user
 *         应由用户实现
 *
 * @param [in] context Radio implementation parameters
 *                     无线电实现参数
 *
 * @returns Operation status
 *          操作状态
 */
llcc68_hal_status_t llcc68_hal_wakeup( const void* context )
{
    HAL_GPIO_TogglePin(CS_GPIO_Port, CS_Pin);
    HAL_Delay(1);
    HAL_GPIO_TogglePin(CS_GPIO_Port, CS_Pin);
    return LLCC68_HAL_STATUS_OK;
}
