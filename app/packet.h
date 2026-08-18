/*
 * packet.h
 *
 *  Created on: 2026年8月11日
 *      Author: lianz
 */

#ifndef PACKET_H_
#define PACKET_H_

#include <stdint.h>

/* 包配置 */

// 包头
// header在packet.c中初始化
// type由packet调用函数设置
// remaining_time由外部设置
// id由packet调用函数设置
typedef struct
{
    uint8_t header1;    
    uint8_t header2;    
    uint8_t type;       
    uint8_t remaining_time;
    uint32_t id;
} packet_header_t;

// 包尾
// crc由外部计算设置，是全部payload的CRC8
// footer在packet.c中初始化
typedef struct
{
    uint8_t crc;
    uint8_t footer;
} packet_trailer_t;

typedef struct
{
    packet_header_t header;
    packet_trailer_t trailer;
} packet_t;

typedef enum
{
    COMMAND,
    DATA,
    HEARTBEAT,
} packet_type_t;

// 广播信息数据
typedef struct
{
    uint32_t received_count;    // 接收数据包计数
    uint32_t transmitted_count; // 发送数据包计数
} broadcast_info_t;

/**
 @brief 初始化数据包
 @param packet 数据包配置指针
 */
void packet_init(packet_t *packet);


/**
 * @brief 获取数据包载荷
 * @param packet 数据包配置指针
 * @param buf 数据指针
 * @param len 数据长度指针
 */
void packet_get_new_payload(packet_t *packet, uint8_t *buf, uint16_t* len);


/**
 * @brief 解析数据包
 * @param packet 数据包配置指针
 * @param buf 数据指针
 * @param len 数据长度指针
 * @note 传入包配置结构体，完整数据包，长度。
 *       解析后会将数据包的载荷部分拷贝到buf中，
 *       并将长度写入len中,且会将包头和
 *       包尾解析到packet中
 */
void packet_parse(packet_t *packet, uint8_t *buf, uint16_t* len);

/**
 * @brief 更新发送计数和接收计数
 * @param received_count 接收计数
 * @param transmitted_count 发送计数
 */
void packet_update_rx_tx_count(uint32_t received_count, uint32_t transmitted_count);



#endif /* PACKET_H_ */
