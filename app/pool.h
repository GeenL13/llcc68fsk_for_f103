/*
 * pool.h
 *
 *  Created on: 2026年8月10日
 *      Author: lianz
 */

#ifndef POOL_H_
#define POOL_H_
#include <stdint.h>


/**
 * @brief 将数据包放入数据池中
 * @param data 数据包指针
 * @param data_len 数据包长度
 */
void pool_push_data(const uint8_t *data, uint8_t data_len);

/**
 * @brief 将数据池中的无线接收数据放入串口发送缓冲区
 * @note 需要轮询调用，确保数据池中的数据被及时发送
 */
void pool_push_to_serial_data(void);


#endif /* POOL_H_ */
