/*
 * serial.c
 *
 *  Created on: 2026年8月10日
 *      Author: lianz
 */

#include "serial.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_uart.h"
#include "usart.h"
#include <string.h>

#define UART_PORT huart1 // 使用的UART端口


typedef enum
{
    NONE,
    IS_WRITING,
    IS_READING,
    WRITING_DONE,
    READING_DONE,
} buffer_status_t;

#define RX_BUFFER_SIZE 1024
static uint8_t rx_buffer[2][RX_BUFFER_SIZE]; // 接收缓冲区
static volatile uint8_t rx_write_buffer_index = 0; // 当前接收缓冲区写指针索引
static volatile uint8_t rx_read_buffer_index = 0; // 当前接收缓冲区读指针索引
static volatile uint16_t rx_write_index = 0; // 当前接收缓冲区写指针位置
static volatile uint16_t rx_read_index = 0; // 当前接收缓冲区读指针位置
static volatile uint32_t rx_buffer_write_done_count = 0; // 写完的缓冲区计数器
static volatile uint32_t rx_buffer_read_done_count = 0; // 读完的缓冲区计数器
// 计数器加1，应对环形缓冲区读索引进行切换
static volatile uint8_t rx_read_buffer_index_toggle_count; 

#define TX_BUFFER_SIZE 1024
static uint8_t tx_buffer[2][TX_BUFFER_SIZE]; // 发送缓冲区
static volatile uint8_t tx_write_buffer_index = 0; // 当前发送缓冲区写指针索引
static volatile uint8_t tx_read_buffer_index = 0; // 当前发送缓冲区读指针索引
static volatile uint16_t tx_write_index = 0; // 当前发送缓冲区写指针位置
static volatile uint16_t tx_read_index = 0; // 当前发送缓冲区读指针位置
static volatile uint32_t tx_read_index_add;// 读指针增量，发送完成中断赋值，加进读指针后置0 
static volatile uint32_t tx_dma_len; // DMA发送长度，发送前赋值，中断中置零

// 每次完成一次缓冲区填充
// 计数器加1，应对环形缓冲区读索引进行切换
static volatile uint8_t tx_read_buffer_index_toggle_count; 

// 开启串口接收
serial_status_t serial_receive_byte_on(void)
{
    if (HAL_UART_Receive_DMA(&UART_PORT, &rx_buffer[0][0], 
        sizeof(rx_buffer)) != HAL_OK)
    {
        return SERIAL_ERROR;
    }
    return SERIAL_OK;
}

// 取串口接收数据
uint16_t serial_get_received_byte(uint8_t *data, uint16_t len)
{
    uint16_t available_data = 0;    // 可用数据长度
    uint16_t bytes_to_read = 0;     // 实际读取长度
    uint16_t n1;                    // 第一段数据长度(应对环形缓冲区)
    // 更新写指针位置
    rx_write_index = ((sizeof(rx_buffer) - __HAL_DMA_GET_COUNTER(UART_PORT.hdmarx)) % RX_BUFFER_SIZE);
    // 计算读缓冲区索引
    if (rx_buffer_write_done_count 
        - rx_buffer_read_done_count > 1)
    {
        // 如果写好的数据超过读完的数据有2个缓冲区或以上
        // 将读缓冲区索引切换到最新写完的缓冲区
        rx_read_buffer_index = !rx_write_buffer_index;
        // 更新读指针位置
        rx_read_index = 0;
        // 读缓冲区读完计数器变为写完计数器减去1
        rx_buffer_read_done_count = rx_buffer_write_done_count - 1;
    }
    // 计算实际可读数据
    if (rx_read_buffer_index == rx_write_buffer_index)
    {
        // 读写缓冲区相同
        available_data = rx_write_index - rx_read_index;
    }
    else
    {
        // 读写缓冲区不同
        available_data = RX_BUFFER_SIZE - rx_read_index + rx_write_index;
    }
    bytes_to_read = (len < available_data) ? len : available_data;
    // 复制数据到用户缓冲区
    if (bytes_to_read >= RX_BUFFER_SIZE - rx_read_index)
    {
        n1 = RX_BUFFER_SIZE - rx_read_index;
        memcpy(data, &rx_buffer[rx_read_buffer_index][rx_read_index], n1);
        memcpy(data + n1, &rx_buffer[!rx_read_buffer_index][0],     // 切换缓冲区，读取剩余数据
            bytes_to_read - n1);
        // 更新读指针索引和位置
        rx_read_buffer_index = !rx_read_buffer_index;   // 切换缓冲区索引
        rx_read_index = bytes_to_read - n1;             // 更新读指针位置
        // 读缓冲区读完计数器加1
        rx_buffer_read_done_count++;
    }
    else
    {
        memcpy(data, &rx_buffer[rx_read_buffer_index][rx_read_index], bytes_to_read);
        // 更新读指针索引和位置
        rx_read_index += bytes_to_read;     // 不必切换缓冲区，直接更新读指针位置
    }
    // 返回实际长度
    return bytes_to_read;
}

// 串口接收半完成回调函数(rx_buffer[0]写满)
void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        // 写完缓冲区计数加一
        rx_buffer_write_done_count++;
        // 更新rx缓冲区索引
        rx_write_buffer_index = 1;
    }
}

// 串口接收完成回调函数(rx_buffer[1]写完)
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        // 写完缓冲区计数加一
        rx_buffer_write_done_count++;
        // 更新rx缓冲区索引
        rx_write_buffer_index = 0;
    }
}





// 开启串口发送
void serial_transmit_byte_on(void)
{
    // 更新读指针位置
    tx_read_index += tx_read_index_add;
    tx_read_index_add = 0; // 置零增量
    while (tx_read_buffer_index_toggle_count > 0)
    {
        // 如果有数据准备好，切换读缓冲区索引
        tx_read_buffer_index = !tx_read_buffer_index;
        tx_read_index -= TX_BUFFER_SIZE; // 更新读指针位置
        tx_read_buffer_index_toggle_count--; // 减少计数器
    }
    uint16_t available_data = 0;    // 可发送数据长度
    // 计算可发送数据长度
    if (tx_write_buffer_index == tx_read_buffer_index)
    {
        // 读写缓冲区相同
        available_data = tx_write_index - tx_read_index;
    }
    else
    {
        // 读写缓冲区不同
        available_data = TX_BUFFER_SIZE - tx_read_index;
    }
    // 发送数据
    if (available_data > 0)
    {
        HAL_StatusTypeDef status;
        status = HAL_UART_Transmit_DMA(&UART_PORT, 
            &tx_buffer[tx_read_buffer_index][tx_read_index], 
            available_data);
        if (status == HAL_OK)
        {
            tx_dma_len = available_data; // 记录DMA发送长度
        }
    }
}


// 推送串口发送数据
uint16_t serial_push_transmit_byte(const uint8_t *data, uint16_t len)
{
    // 更新读指针位置
    tx_read_index += tx_read_index_add;
    tx_read_index_add = 0; // 置零增量
    while (tx_read_buffer_index_toggle_count > 0)
    {
        // 如果有数据准备好，切换读缓冲区索引
        tx_read_buffer_index = !tx_read_buffer_index;
        tx_read_index -= TX_BUFFER_SIZE; // 更新读指针位置
        tx_read_buffer_index_toggle_count--; // 减少计数器
    }
    // 计算可用空间
    uint16_t available_data = 0;
    if (tx_write_buffer_index == tx_read_buffer_index)
    {
        // 读写缓冲区相同
        available_data = TX_BUFFER_SIZE - tx_write_index + TX_BUFFER_SIZE;
    }
    else
    {
        // 读写缓冲区不同
        available_data = TX_BUFFER_SIZE - tx_write_index;
    }
    available_data = (available_data < len) ? available_data : len;
    // 复制数据到发送缓冲区
    uint16_t n1;
    if (available_data >= TX_BUFFER_SIZE - tx_write_index)
    {
        n1 = TX_BUFFER_SIZE - tx_write_index;
        memcpy(&tx_buffer[tx_write_buffer_index][tx_write_index], 
            data, n1);
        memcpy(&tx_buffer[!tx_write_buffer_index][0], 
            data + n1, available_data - n1);
        // 更新写指针索引和位置
        tx_write_buffer_index = !tx_write_buffer_index;   // 切换缓冲区索引
        tx_write_index = (tx_write_index + available_data) 
            % TX_BUFFER_SIZE;// 更新写指针位置
    }
    else
    {
        memcpy(&tx_buffer[tx_write_buffer_index][tx_write_index], data, available_data);
        // 更新写指针索引和位置
        tx_write_index = (tx_write_index + available_data) 
            % TX_BUFFER_SIZE;// 更新写指针位置
    }

    return available_data; // 返回实际推送的字节数
}



// 串口发送完成回调函数
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        uint16_t read_index = tx_read_index + tx_dma_len;
        while (read_index >= TX_BUFFER_SIZE)
        {
            read_index -= TX_BUFFER_SIZE;
            tx_read_buffer_index_toggle_count++;
        }
        // 写入读指针增量
        tx_read_index_add = tx_dma_len;
        // 发送完成后置零DMA发送长度
        tx_dma_len = 0;
    }
}