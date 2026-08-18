/*
 * pool.c
 *
 *  Created on: 2026年8月10日
 *      Author: lianz
 */

#include "pool.h"
#include "serial.h"
#include <string.h>

static uint8_t radio_rx_data_pool[2048];  // 数据池缓冲区
static uint16_t radio_rx_data_pool_write = 0;  // 数据池写指针
static uint16_t radio_rx_data_pool_read = 0;  // 数据池读指针




// 将数据包放入数据池中
void pool_push_data(const uint8_t *data, uint8_t data_len)
{
    // 取得可用长度
    uint16_t available_data;
    uint16_t secure_len = 500;
    if (radio_rx_data_pool_write < radio_rx_data_pool_read)
    {
        if (radio_rx_data_pool_read - radio_rx_data_pool_write > secure_len)
        {
            available_data = radio_rx_data_pool_read - radio_rx_data_pool_write - secure_len;
        }
        else
        {
            available_data = 0;
        }
    }
    else
    {
        uint16_t n1;
        if (radio_rx_data_pool_read >= secure_len)
        {
            n1 = sizeof(radio_rx_data_pool) - radio_rx_data_pool_write;
            available_data = n1 + radio_rx_data_pool_read - secure_len;
        }
        else
        {
            n1 = secure_len - radio_rx_data_pool_read;
            available_data = sizeof(radio_rx_data_pool) - radio_rx_data_pool_write - n1;
        }
    }
    available_data = (available_data < data_len) ? available_data : data_len;
    if (available_data == 0) return; // 没有可用空间，丢弃数据
    // 将数据放入数据池
    uint16_t n1;
    if (sizeof(radio_rx_data_pool) - radio_rx_data_pool_write < available_data)
    {
        n1 = sizeof(radio_rx_data_pool) - radio_rx_data_pool_write;
        // 分两次拷贝
        memcpy(&radio_rx_data_pool[radio_rx_data_pool_write], data, n1);
        memcpy(&radio_rx_data_pool[0], &data[n1], available_data - n1);
        // 更新写指针
        radio_rx_data_pool_write = (radio_rx_data_pool_write + available_data) % sizeof(radio_rx_data_pool);
    }
    else
    {
        n1 = sizeof(radio_rx_data_pool) - radio_rx_data_pool_write;
        // 一次拷贝
        memcpy(&radio_rx_data_pool[radio_rx_data_pool_write], data, available_data);
        // 更新写指针
        radio_rx_data_pool_write = (radio_rx_data_pool_write + available_data) % sizeof(radio_rx_data_pool);
    }
}


// 将数据池中的无线接收数据放入串口发送缓冲区
void pool_push_to_serial_data(void)
{
    // 将数据池中的无线接收数据放入串口发送缓冲区
    uint16_t available_data;
    if (radio_rx_data_pool_write >= radio_rx_data_pool_read)
    {
        available_data = radio_rx_data_pool_write - radio_rx_data_pool_read;
    }
    else
    {
        available_data = sizeof(radio_rx_data_pool) - radio_rx_data_pool_read + radio_rx_data_pool_write;
    }
    available_data = (available_data < 256) ? available_data : 256; // 每次最多发送256字节
    if (available_data == 0) return; // 没有数据可发送
    // 将数据放入串口发送缓冲区
    uint16_t n1;
    if (sizeof(radio_rx_data_pool) - radio_rx_data_pool_read < available_data)
    {
        n1 = sizeof(radio_rx_data_pool) - radio_rx_data_pool_read;
        // 分两次拷贝
        serial_push_transmit_byte(&radio_rx_data_pool[radio_rx_data_pool_read], n1);
        serial_push_transmit_byte(&radio_rx_data_pool[0], available_data - n1);
        // 更新读指针
        radio_rx_data_pool_read = (radio_rx_data_pool_read + available_data) % sizeof(radio_rx_data_pool);
    }
    else
    {
        n1 = sizeof(radio_rx_data_pool) - radio_rx_data_pool_read;
        // 一次拷贝
        serial_push_transmit_byte(&radio_rx_data_pool[radio_rx_data_pool_read], available_data);
        // 更新读指针
        radio_rx_data_pool_read = (radio_rx_data_pool_read + available_data) % sizeof(radio_rx_data_pool);
    }
    // 触发串口发送
    serial_transmit_byte_on();
}