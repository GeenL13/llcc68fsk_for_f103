/*
 * radio.c
 *
 *  Created on: 2026年8月10日
 *      Author: lianz
 */

#include "radio.h"
#include "llcc68.h"
#include "gpio.h"
#include "packet.h"
#include "pool.h"
#include "stm32f1xx_hal_gpio.h"
#include "tick.h"
#include "main.h"
#include <stdint.h>
#include <string.h>

/* debug变量 */
static volatile uint8_t a_11; // 用于调试的变量
static volatile uint32_t lost_count = 0;
static volatile uint32_t last_id = 0;
static volatile uint32_t tx_last_id = 0;
static volatile uint32_t tx_id_error = 0;

static volatile uint32_t rx_id_last;
static volatile uint32_t rx_id_lost_count = 0;

static volatile uint32_t tx_count = 0;
static volatile uint32_t rx_count = 0;
static volatile uint32_t tx_count_per_time = 0;
static volatile uint32_t rx_count_per_time = 0;
static volatile uint32_t timeout_count = 0;
static volatile uint32_t crc_count = 0;

static volatile uint32_t g_airtime = 0; // 用于调试的变量，记录发送时间
static volatile uint8_t g_tx_remaining[256];  // 记录每个周期调用发送函数时的剩余时间，单位毫秒
static volatile uint8_t g_tx_remaining_index = 0; // 记录remaining的索引
static volatile uint8_t g_rx_remaining[256];  // 记录每个周期调用发送函数时的剩余时间，单位毫秒
static volatile uint8_t g_rx_remaining_index = 0; // 记录remaining的索引

#define MASTER  1


// 从进入rx到退出rx的时间，单位毫秒
static volatile uint32_t rx_period_debug;
static volatile uint32_t rx_start_debug;
// 从进入tx到退出tx的时间，单位毫秒
static volatile uint32_t tx_period_debug;
static volatile uint32_t tx_start_debug;


/* llcc68配置项 */
typedef struct
{
    uint32_t rf_freq;  // 射频频率
    uint8_t tx_power;   // 发射功率
} llcc68_user_config_t;
llcc68_user_config_t g_llcc68_user_config = 
{
    .rf_freq = 433000000, // 433MHz
    .tx_power = 22,     // 发射功率7dBm
};

// PA配置参数
llcc68_pa_cfg_params_t g_llcc68_pa_cfg_params = 
{
    .pa_duty_cycle = 0x04, // 4/15占空比
    .hp_max = 0x07,       // 最大功率
    .device_sel = 0x00,   // PA选择，0为PA_BOOST，1为RFO
    .pa_lut = 0x01,       // PA LUT选择，1为默认LUT
};
// GFSK调制参数
llcc68_mod_params_gfsk_t g_gfsk_mod_params =
{
    .bw_dsb_param = LLCC68_GFSK_BW_117300, // 带宽125kHz
    .br_in_bps = 50000,                 // 比特率50kbps
    .fdev_in_hz = 20000,                // 频偏25kHz
    .pulse_shape = LLCC68_GFSK_PULSE_SHAPE_BT_1,    // 高斯脉冲整形，BT=1
};
// GFSK包参数
llcc68_pkt_params_gfsk_t g_gfsk_pkt_params =
{
    .preamble_len_in_bits = 64,         // 前导码长度64bit
    .preamble_detector = LLCC68_GFSK_PREAMBLE_DETECTOR_MIN_8BITS, // 前导码检测，最小8bit
    .sync_word_len_in_bits = 24,         // 同步字长度24bit
    .address_filtering = LLCC68_GFSK_ADDRESS_FILTERING_DISABLE, // 地址过滤关闭
    .header_type = LLCC68_GFSK_PKT_VAR_LEN, // 可变长度包
    .pld_len_in_bytes = 0x80,              // 可变长度包下，代表128个字节最大载荷长度
    .crc_type = LLCC68_GFSK_CRC_2_BYTES, // CRC2字节
    .dc_free = LLCC68_GFSK_DC_FREE_WHITENING, // 数据白化
};
// 同步字
uint8_t g_gfsk_sync_word[3] = 
{
    0xC1,
    0x94, 
    0xC1
};


/* llcc68状态 */
static volatile 
llcc68_status_t g_llcc68_status;        // 当前llcc68状态
static volatile 
llcc68_chip_status_t g_chip_status;     // 当前芯片状态
static volatile 
llcc68_irq_mask_t g_irq_status;       // 当前中断状态
static volatile 
llcc68_rx_buffer_status_t g_rx_buffer_status; // 当前接收缓冲区
// llcc68新数据标志位
static volatile uint8_t g_llcc68_new_rx_flag;   // 1代表有新数据，取完后务必清零
// 中断标志位
// 1代表中断发生，取完后务必清零
// 只有一个中断引脚的情况下使用g_llcc68_irq即可
// static volatile uint8_t g_llcc68_irq_tx_done;
// static volatile uint8_t g_llcc68_irq_rx_done;
// static volatile uint8_t g_llcc68_irq_timeout;
static volatile uint8_t g_llcc68_irq;


/* 时间管理 */
static volatile uint32_t time_now; // 当前时间，单位毫秒
static volatile uint32_t time_last; // 上一次时间，单位毫秒
static volatile uint32_t time_remaining; // 剩余时间，单位毫秒
static volatile uint32_t time_delta; // 时间差，单位毫秒
//static volatile uint32_t tx_remaining_time; // 发送剩余时间，单位毫秒
//static volatile uint32_t rx_remaining_time; // 接收剩余时间，单位毫秒
//static volatile uint32_t silence_remaining_time; // 静默剩余时间，单位毫秒
uint32_t tx_period = 28; // 发送周期，单位毫秒
uint32_t rx_period = 28; // 接收周期，单位毫秒
uint32_t silence_period = 1; // 静默周期，单位毫秒

/* 状态管理 */
typedef enum
{
    RADIO_STATE_RX,
    RADIO_STATE_SILENCE1,
    RADIO_STATE_TX,
    RADIO_STATE_SILENCE2,
} radio_state_t;
radio_state_t g_radio_state;
llcc68_status_t g_radio_status;

/* 包管理 */
static uint8_t pkt_tx[200]; // 发送包缓冲区
static uint8_t payload_buffer[128]; // 发送载荷缓冲区
static uint16_t pkt_tx_size; // 发送包大小,存储实际发送数据的大小,不包括包头包尾
static packet_t g_packet; // 当前包
static volatile uint32_t packet_transmitted_count = 0; // 发送数据包计数


static uint8_t pkg_rx[200]; // 接收包缓冲区
static uint16_t pkg_rx_size; // 接收包大小,存储实际接收数据的大小,不包括包头包尾
static packet_t g_packet_rx; // 当前接收包
static volatile uint32_t packet_received_count = 0; // 接收数据包计数

// 置1时才允许取新包
volatile uint8_t able_to_get_new_pkt = 1;
// 置1时代表接收已开启
volatile uint8_t receive_activated = 0;
// 置1时代表发送已开启
volatile uint8_t transmit_activated = 0;


void llcc68_wait_for_idle(void);
void llcc68_init(void);
void update_state();
uint32_t radio_get_tx_time_on_air_ms(uint16_t payload_len);
void process_llcc68_irq(void);


void radio_init(void)
{
    llcc68_init();
    packet_init(&g_packet);
    // debug
    HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_RESET);
}

void radio_loop(void)
{
    // debug
    if (rx_count_per_time < 2 && !MASTER)
    {
        // 增长接收时间窗口
         rx_period = 100;
    }
    else
    {
        // 恢复接收时间窗口
        rx_period = 28;
    }
    // 开头更新时间
    time_last = time_now;
    time_now = get_tick_ms();  
    // 执行DIO1中断处理
    process_llcc68_irq();
    // 更新收发计数到packet模块
    packet_update_rx_tx_count(packet_received_count, packet_transmitted_count);
    // 进入状态机 
    switch (g_radio_state)
    {
        case RADIO_STATE_RX:
        radio_set_mode(0); // 设置为接收模式
        // debug
        HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);
        // 接收状态
        // 若有，处理新数据
        if (g_llcc68_new_rx_flag)
        {
            // debug
            HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_RESET);
            rx_id_last = g_packet_rx.header.id;

            // 取新数据
            memset(pkg_rx, 0, sizeof(pkg_rx));
            memset(&g_packet_rx, 0, sizeof(g_packet_rx));
            pkg_rx_size = 0;
            llcc68_wait_for_idle();
            llcc68_get_rx_buffer_status(NULL, 
                &g_rx_buffer_status);
            pkg_rx_size = g_rx_buffer_status.pld_len_in_bytes;
            // debug
            if (pkg_rx_size > sizeof(pkg_rx))
            {
                pkg_rx_size = sizeof(pkg_rx);
            }
            llcc68_wait_for_idle();
            llcc68_read_buffer(NULL, 
                g_rx_buffer_status.buffer_start_pointer, 
                pkg_rx, 
                pkg_rx_size);
            // 解析包头包尾
            packet_parse(&g_packet_rx, 
                pkg_rx, &pkg_rx_size);
            // 暂时不推送心跳包
            if (g_packet_rx.header.type == DATA)
            {
                // 清除0x00数据
                uint8_t data_temp[sizeof(pkg_rx)];
                memset(data_temp, 0, sizeof(data_temp));
                uint16_t data_size = 0;
                for (uint16_t i = 0; i < pkg_rx_size; i++)
                {
                    if (pkg_rx[data_size] != 0x00)
                    {
                        data_temp[data_size] = pkg_rx[i];
                        data_size++;
                    }
                }
                // 推送数据到数据池
                pool_push_data(data_temp, data_size);
            }
            // 将数据池中的数据推送到串口发送缓冲区
            pool_push_to_serial_data();
            // 清除新数据标志位
            g_llcc68_new_rx_flag = 0;
            // 同步时间
            time_remaining = g_packet_rx.header.remaining_time;   // 更新剩余时间
            // debug
            if (pkg_rx_size > 0 && g_packet_rx.header.id - rx_id_last > 1 && rx_id_last != 0)
            {
                uint32_t lost = g_packet_rx.header.id - rx_id_last - 1;
                rx_id_lost_count += lost;
            }
        }
        // 同步时间
        time_now = get_tick_ms();
        time_delta = time_now - time_last;
        time_last = time_now;


        
        // 开启接收
        if (receive_activated == 0 
            && g_llcc68_new_rx_flag == 0)
        {
            llcc68_wait_for_idle();
            g_radio_status = llcc68_set_gfsk_pkt_params(NULL, 
                &g_gfsk_pkt_params);
            llcc68_wait_for_idle();
            if (g_radio_status != LLCC68_STATUS_OK) while (1);    // debug
            // 设置gpio为接收模式（rxen和txen）
            g_radio_status = llcc68_set_rx(NULL, 
                30); // 开启接收，超时为0表示无限等待
            if (g_radio_status != LLCC68_STATUS_OK) while (1);    // debug
            // 置接收已开启标志位 
            receive_activated = 1;
        }
        // 检查时间到达后切换下一个状态
        update_state();
        break;

        case RADIO_STATE_SILENCE1:
        radio_set_mode(0); // 设置为接收模式
        // 静默状态1
        // 同步时间
        time_now = get_tick_ms();
        time_delta = time_now - time_last;
        time_last = time_now;
        // 检查时间到达后切换下一个状态
        update_state();
        break;

        case RADIO_STATE_TX:
        // 发送状态
        radio_set_mode(1); // 设置为发送模式
        if (able_to_get_new_pkt)
        {
            // debug
            tx_last_id = g_packet.header.id;

            // 取包
            memset(payload_buffer, 0, sizeof(payload_buffer));
            memset(pkt_tx, 0, sizeof(pkt_tx));
            pkt_tx_size = 0;
            packet_get_new_payload(&g_packet, 
                payload_buffer, &pkt_tx_size);
            able_to_get_new_pkt = 0; // 取完包后禁止再次取包，直到送出包
        }
        // 填充空口发送时间
        uint32_t airtime = radio_get_tx_time_on_air_ms(
            pkt_tx_size + sizeof(g_packet.header) + sizeof(g_packet.trailer));
        
        // debug
        g_airtime = airtime;

        if (airtime < time_remaining 
            && transmit_activated == 0)
        {
            g_packet.header.remaining_time = time_remaining - airtime;
            // 拼包
            memcpy(pkt_tx, &g_packet.header, 
                sizeof(g_packet.header));
            memcpy(pkt_tx + sizeof(g_packet.header), 
            payload_buffer, pkt_tx_size);
            memcpy(pkt_tx + sizeof(g_packet.header) + pkt_tx_size, 
                &g_packet.trailer, sizeof(g_packet.trailer));
            // 发送包
            llcc68_wait_for_idle();
            llcc68_write_buffer(NULL, 0x00, 
                pkt_tx,
                pkt_tx_size + sizeof(g_packet.header) + sizeof(g_packet.trailer));
               
            // debug
            g_tx_remaining[g_tx_remaining_index] = time_remaining;
            g_tx_remaining_index++;

            tx_id_error = g_packet.header.id - tx_last_id;
            if (tx_id_error != 1)
            {
                tx_id_error = 10;
            }

            llcc68_pkt_params_gfsk_t pkt_tx_params = g_gfsk_pkt_params; // 重新设置发送包参数
            pkt_tx_params.pld_len_in_bytes = pkt_tx_size + 
            sizeof(g_packet.header) + sizeof(g_packet.trailer);         // 填写实际发送包长度
            llcc68_wait_for_idle();
            llcc68_set_gfsk_pkt_params(NULL, 
                &pkt_tx_params);
            llcc68_wait_for_idle();
            llcc68_set_tx(NULL, 30);
            // 置发送已开启标志位
            transmit_activated = 1;
        }
        // 同步时间
        time_now = get_tick_ms();
        time_delta = time_now - time_last;
        time_last = time_now;
        // 检查时间到达后切换下一个状态
        update_state();
        break;

        case RADIO_STATE_SILENCE2:
        radio_set_mode(0); // 设置为接收模式
        // 静默状态2
        // 同步时间
        time_now = get_tick_ms();
        time_delta = time_now - time_last;
        time_last = time_now;
        // 检查时间到达后切换下一个状态
        update_state();
        break;

        default:
            radio_set_mode(0); // 设置为接收模式
            break;
    }
}


void llcc68_wait_for_idle(void)
{
    // 等待芯片空闲
    while (HAL_GPIO_ReadPin(BUSY_GPIO_Port, 
        BUSY_Pin) == GPIO_PIN_SET)
    {    
        __NOP();
    }
}



void llcc68_init(void)
{
    // 复位
    llcc68_reset(NULL);
    // 设置为standby模式
    llcc68_wait_for_idle();
    llcc68_set_standby(NULL, 
        LLCC68_STANDBY_CFG_XOSC);
    // 设为GFSK模式
    llcc68_wait_for_idle();
    llcc68_set_pkt_type(NULL, 
        LLCC68_PKT_TYPE_GFSK);
    // 设定射频频率
    llcc68_wait_for_idle();
    llcc68_set_rf_freq(NULL, 
        g_llcc68_user_config.rf_freq); // 433MHz
    // 配置放大器功率
    llcc68_wait_for_idle();
    llcc68_set_pa_cfg(NULL, 
        &g_llcc68_pa_cfg_params);
    // 配置发射参数
    llcc68_wait_for_idle();
    llcc68_set_tx_params(NULL, 
        g_llcc68_user_config.tx_power, // 发射功率7dBm
        LLCC68_RAMP_40_US); // 上升时间
    // 配置有效荷载存储位置
    llcc68_wait_for_idle();
    llcc68_set_buffer_base_address(NULL, 
        0x00, // Tx base address
        0x80); // Rx base address
    // 配置GFSK调制参数
    llcc68_wait_for_idle();
    llcc68_set_gfsk_mod_params(NULL, 
        &g_gfsk_mod_params);
    // 配置GFSK包参数
    llcc68_wait_for_idle();
    llcc68_set_gfsk_pkt_params(NULL, 
        &g_gfsk_pkt_params);
    // 配置DIO引脚映射
    llcc68_wait_for_idle();
    llcc68_set_dio_irq_params
    (
        NULL, 
        LLCC68_IRQ_TX_DONE 
        | LLCC68_IRQ_RX_DONE 
        | LLCC68_IRQ_TIMEOUT
        | LLCC68_IRQ_CRC_ERROR, // 使能TX_DONE、RX_DONE、TIMEOUT、CRC_ERROR中断
        LLCC68_IRQ_TX_DONE 
        | LLCC68_IRQ_RX_DONE 
        | LLCC68_IRQ_TIMEOUT
        | LLCC68_IRQ_CRC_ERROR, // DIO1映射TX_DONE、RX_DONE、TIMEOUT、CRC_ERROR中断
        LLCC68_IRQ_NONE, // DIO2不映射中断
        LLCC68_IRQ_NONE  // DIO3不映射中断
    );
    // 配置同步字
    llcc68_wait_for_idle();
    llcc68_set_gfsk_sync_word(NULL, 
        g_gfsk_sync_word, 
        sizeof(g_gfsk_sync_word));
    // 开启rx_boost模式
    llcc68_wait_for_idle();
    llcc68_cfg_rx_boosted(NULL, true);
    // test
    llcc68_set_dio2_as_rf_sw_ctrl(NULL, true);
}

// DIO1中断回调函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == DIO1_Pin)
    {
        // 置位中断标志位
        g_llcc68_irq = 1;
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
    }
}


void update_state()
{
    if (time_delta < time_remaining)
    {
        time_remaining -= time_delta;
        return;
    }
    else if (time_delta >= time_remaining)
    {
        g_radio_state = (g_radio_state + 1) % 3; // 切换到下一个状态
    }
    // 更新切换状态后的剩余时间
    if (g_radio_state == RADIO_STATE_RX )
    {
        // debug
        tx_count_per_time = tx_count;
        rx_count = 0;
        rx_start_debug = get_tick_ms();
        tx_period_debug = get_tick_ms() - tx_start_debug;

        time_remaining = rx_period;
        // 设置为standby模式
        llcc68_wait_for_idle();
        llcc68_set_standby(NULL, 
        LLCC68_STANDBY_CFG_XOSC);
        // 切换为接收状态，开启接收
        radio_set_mode(0); // 设置引脚电平为接收模式
        llcc68_wait_for_idle();
        g_radio_status = llcc68_set_gfsk_pkt_params(NULL, 
            &g_gfsk_pkt_params);
        llcc68_wait_for_idle();
        llcc68_set_rx(NULL, 
            0); // 开启接收，超时为0表示无限等待
        receive_activated = 1;
        transmit_activated = 0; // 切换为接收状态，关闭发送

    }
    else if (g_radio_state == RADIO_STATE_TX)
    {
        // debug
        rx_count_per_time = rx_count;
        tx_count = 0;
        rx_period_debug = get_tick_ms() - rx_start_debug;
        tx_start_debug = get_tick_ms();

        // 设置为standby模式
        llcc68_wait_for_idle();
        llcc68_set_standby(NULL, 
        LLCC68_STANDBY_CFG_XOSC);
        time_remaining = tx_period;

    }
    else 
    {
        time_remaining = silence_period;

        // debug
        g_tx_remaining_index = 0;
        
    }
}

// 获取空口发送时间，单位毫秒
uint32_t radio_get_tx_time_on_air_ms(uint16_t payload_len)
{
    // 计算发送时间
    llcc68_pkt_params_gfsk_t pkt_params = g_gfsk_pkt_params;
    pkt_params.pld_len_in_bytes = payload_len;
    uint32_t time_on_air = llcc68_get_gfsk_time_on_air_in_ms(
        &pkt_params, &g_gfsk_mod_params);
    return time_on_air;
}

// 处理中断标志位
void process_llcc68_irq(void)
{
    if (g_llcc68_irq == 0) return;

    llcc68_wait_for_idle();
    llcc68_get_irq_status(NULL, &g_irq_status);
    if (g_irq_status & LLCC68_IRQ_TX_DONE)
    {
        // 更新发送计数
        packet_transmitted_count++;
        // 发送完成后允许取新包
        able_to_get_new_pkt = 1;
        // 置发送已关闭标志位
        transmit_activated = 0;
        // 置接收已关闭标志位
        receive_activated = 0;
        // 解除llcc68发送活动标志位
        llcc68_wait_for_idle();
        llcc68_clear_irq_status(NULL, 
            LLCC68_IRQ_TX_DONE);
        // debug
        tx_count++;
    }
    if (g_irq_status & LLCC68_IRQ_RX_DONE)
    {
        // 更新接收计数
        packet_received_count++;
        // 设置新数据标志位
        g_llcc68_new_rx_flag = 1;
        // 解除接收活动标志位
        receive_activated = 0;
        // 清除llcc68接收完成中断标志位
        llcc68_wait_for_idle();
        llcc68_clear_irq_status(NULL, 
            LLCC68_IRQ_RX_DONE);
        // debug 
        rx_count++;
    }
    if (g_irq_status & LLCC68_IRQ_TIMEOUT)
    {
        // 允许取新包
        able_to_get_new_pkt = 1;
        // 解除接收活动标志位
        receive_activated = 0;
        transmit_activated = 0;
        // 清除llcc68超时中断标志位
        llcc68_wait_for_idle();
        llcc68_clear_irq_status(NULL, 
            LLCC68_IRQ_TIMEOUT);

        // debug
        timeout_count++;
    }
    if (g_irq_status & LLCC68_IRQ_CRC_ERROR)
    {
        // 允许取新包
        able_to_get_new_pkt = 1;
        // 解除接收活动标志位
        receive_activated = 0;
        transmit_activated = 0;
        // 清除llcc68CRC错误中断标志位
        llcc68_wait_for_idle();
        llcc68_clear_irq_status(NULL, 
            LLCC68_IRQ_CRC_ERROR);

        // debug
        crc_count++;
    }
    // 清除中断标志位
    g_llcc68_irq = 0;
}


// GPIO切换收发（高电平有效）    1：发送，0：接收
void radio_set_mode(int mode)
{
    if (mode)
    {
        //HAL_GPIO_WritePin(TXEN_GPIO_Port, TXEN_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(RXEN_GPIO_Port, RXEN_Pin, GPIO_PIN_RESET);
    }
    else
    {
        //HAL_GPIO_WritePin(TXEN_GPIO_Port, TXEN_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RXEN_GPIO_Port, RXEN_Pin, GPIO_PIN_SET);
    }
}