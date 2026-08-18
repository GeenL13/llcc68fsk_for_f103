/*
 * packet.c
 *
 *  Created on: 2026年8月11日
 *      Author: lianz
 */


#include "packet.h"
#include "serial.h"
#include <stdint.h>
#include <string.h>

#define PACKET_HEADER1 0xAA
#define PACKET_HEADER2 0x55
#define PACKET_FOOTER 0x0D

static volatile uint16_t packet_payload_len;
static volatile uint32_t packet_transmitted_count = 0;   // 发送数据包计数
static volatile uint32_t packet_received_count = 0;      // 接收数据包计数

void packet_init(packet_t *packet)
{
    packet->header.header1 = 0xAA;
    packet->header.header2 = 0x55;
    packet->trailer.footer = 0x0D;
    // 默认载荷
    packet_payload_len = 8;
}


// 更新发送计数和接收计数
void packet_update_rx_tx_count(uint32_t received_count, uint32_t transmitted_count)
{
    packet_received_count = received_count;
    packet_transmitted_count = transmitted_count;
}

// 获取包载荷
void packet_get_new_payload(packet_t *packet, uint8_t *buf, uint16_t* len)
{
    if (buf == NULL || len == NULL) return;
    // 从串口获取数据，直接写进buf中
    uint16_t bytes_read = serial_get_received_byte(buf, packet_payload_len);
    if (bytes_read == 0)
    {
        // 串口没有数据
        broadcast_info_t info_payload;
        // 填充心跳包
        packet->header.type = HEARTBEAT;
        packet->header.id++;
        info_payload.received_count = packet_received_count;
        info_payload.transmitted_count = packet_transmitted_count;
        // 将广播信息填充到数据包的载荷中
        memcpy(buf, &info_payload, sizeof(info_payload));
        *len = sizeof(info_payload);
    }
    else
    {
        // 串口有数据
        packet->header.type = DATA;
        packet->header.id++;
        // 将实际长度写入len
        *len = bytes_read;
    }
}


// 解析接收到的数据包
void packet_parse(packet_t *packet, 
    uint8_t *buf, uint16_t* len)
{
    if (packet == NULL || buf == NULL || len == NULL) return;
    if (*len < sizeof(packet_header_t) + sizeof(packet_trailer_t))
    {
        // 数据长度不足，清零
        memset(packet, 0, sizeof(packet_t));
        memset(buf, 0, *len);
        *len = 0;
        return;
    }
    // 检验包头信息
    if (buf[0] != PACKET_HEADER1 || buf[1] != PACKET_HEADER2)
    {
        // 包头不匹配，清零
        memset(packet, 0, sizeof(packet_t));
        memset(buf, 0, *len);
        *len = 0;
        return;
    }
    // 检验包尾信息
//    if (buf[*len - 1] != PACKET_FOOTER)
//    {
//        // 包尾不匹配，清零
//        memset(packet, 0, sizeof(packet_t));
//        memset(buf, 0, *len);
//        *len = 0;
//        return;
//    }
    // 提取包头包尾载荷数据
    uint8_t tmp[*len];
    memcpy(tmp, buf, *len);
    memcpy(&packet->header, tmp, sizeof(packet_header_t)); // 提取包头数据
    memcpy(&packet->trailer,                   // 提取包尾数据
        tmp + *len - sizeof(packet_trailer_t), 
        sizeof(packet_trailer_t));
    memcpy(buf, tmp + sizeof(packet_header_t), // 提取载荷数据
            *len - sizeof(packet_header_t) 
            - sizeof(packet_trailer_t));
    *len = *len - sizeof(packet_header_t)       // 提取载荷长度
            - sizeof(packet_trailer_t);
}
