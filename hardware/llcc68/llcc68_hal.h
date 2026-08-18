/**
 * @file      llcc68_hal.h
 *
 * @brief     Hardware Abstraction Layer for LLCC68
 *            LLCC68 硬件抽象层
 *
 * The Clear BSD License
 * Copyright Semtech Corporation 2021. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Semtech corporation nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SEMTECH CORPORATION BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Clear BSD 许可证
 * Copyright Semtech Corporation 2021。保留所有权利。
 *
 * 在满足以下条件的情况下，允许以源代码和二进制形式进行重新分发和使用（无论是否修改）（受以下免责声明的限制）：
 *     * 源代码的重新分发必须保留上述版权声明、此条件列表和以下免责声明。
 *     * 二进制形式的重新分发必须在随分发提供的文档和/或其他材料中复制上述版权声明、此条件列表和以下免责声明。
 *     * 未经事先书面许可，不得使用 Semtech Corporation 的名称或其贡献者的名称来认可或推广源自此软件的产品。
 *
 * 本许可证不授予任何一方的专利权的任何明示或暗示的许可。
 * 本软件由版权所有者和贡献者“按原样”提供，并且不提供任何明示或暗示的保证，包括但不限于适销性和特定用途适用性的暗示保证。
 * 在任何情况下，SEMTECH CORPORATION 均不对任何直接、间接、偶然、特殊、惩罚性或后果性损害（包括但不限于替代商品或服务的采购；使用、数据或利润的损失；或业务中断）承担责任，
 * 无论是由何种原因引起的，也无论是基于合同、严格责任还是侵权（包括疏忽或其他原因）引起的，即使已被告知发生此类损害的可能性。
 */

#ifndef LLCC68_HAL_H
#define LLCC68_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdint.h>
#include <stdbool.h>

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC MACROS -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC CONSTANTS --------------------------------------------------------
 */

/**
 * @brief Write this to SPI bus while reading data, or as a dummy/placeholder
 *        读取数据时将其写入 SPI 总线，或作为虚拟/占位符
 */
#define LLCC68_NOP ( 0x00 )

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC TYPES ------------------------------------------------------------
 */

typedef enum llcc68_hal_status_e
{
    LLCC68_HAL_STATUS_OK    = 0,
    LLCC68_HAL_STATUS_ERROR = 3,
} llcc68_hal_status_t;

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS PROTOTYPES ---------------------------------------------
 */

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
                                      const uint8_t* data, const uint16_t data_length );

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
                                     uint8_t* data, const uint16_t data_length );

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
llcc68_hal_status_t llcc68_hal_reset( const void* context );

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
llcc68_hal_status_t llcc68_hal_wakeup( const void* context );

#ifdef __cplusplus
}
#endif

#endif  // LLCC68_HAL_H

/* --- EOF ------------------------------------------------------------------ */
