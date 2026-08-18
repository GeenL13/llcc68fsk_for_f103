/*
 * serial.h
 *
 *  Created on: 2026年8月10日
 *      Author: lianz
 */

#ifndef SERIAL_H_
#define SERIAL_H_

#include <stdint.h>

typedef enum
{
    SERIAL_OK,
    SERIAL_ERROR,
} serial_status_t;

/**
 * @brief 开启串口接收
 */
serial_status_t serial_receive_byte_on(void);

/**
 * @brief 取串口接收数据
 */
uint16_t serial_get_received_byte(uint8_t *data, uint16_t len);

/**
 * @brief 开启串口发送
 */
void serial_transmit_byte_on(void);

/**
 * @brief 推送串口发送数据
 * @param data
 * @param len
 * @return true_len 实际推送的字节数
 */
uint16_t serial_push_transmit_byte(const uint8_t *data, uint16_t len);

#endif /* SERIAL_H_ */
