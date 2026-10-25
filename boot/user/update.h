#ifndef UPDATE_H
#define UPDATE_H

#include <stdbool.h>
#include <stdint.h>

/* YMODEM 单包上限固定；文件大小上限由调用者指定。 */
#define UPDATE_NAME_MAX       64u
#define UPDATE_PACKET_MAX     1024u
#define UPDATE_RX_BUFFER_SIZE (UPDATE_PACKET_MAX + 4u)
#define UPDATE_RX_TIMEOUT_MS  1000u
#define UPDATE_MAX_ERRORS     30u

typedef enum
{
    UPDATE_OK = 0,          // 成功完成
    UPDATE_BAD_ARGUMENT,    // 参数无效
    UPDATE_TIMEOUT,         // 接收超时
    UPDATE_IO_FAILURE,      // 串口读写失败 
    UPDATE_PARTIAL_PACKET,  // 接收半包，串口中可能残留数据
    UPDATE_BAD_PACKET,      // 包格式错误或 CRC 校验失败
    UPDATE_BAD_HEADER,      // 文件头格式错误或文件大小超限
    UPDATE_TOO_LARGE,       // 文件大小超出调用者指定的最大值
    UPDATE_CANCELLED,       // 接收被取消（发送端发出两次 CAN）
    UPDATE_CONSUMER_ERROR   // 回调函数返回 false，取消接收
} update_result_t;

typedef enum
{
    UPDATE_EVENT_HEADER,
    UPDATE_EVENT_DATA
} update_event_t;

typedef enum
{
    UPDATE_IO_OK,
    UPDATE_IO_TIMEOUT,
    UPDATE_IO_ERROR
} update_io_result_t;

/* 返回 UPDATE_IO_OK 表示恰好传输了 length 字节。
 * 包体读取超时后，串口中可能残留半包；本次接收直接结束。
 * 再次接收前，调用者须清空残留字节并重新同步发送端。
 */
typedef update_io_result_t (*update_read_t)(void *context, uint8_t *data,
                                            uint16_t length, uint32_t timeout_ms);
typedef update_io_result_t (*update_write_t)(void *context, const uint8_t *data,
                                             uint16_t length, uint32_t timeout_ms);

typedef struct
{
    update_read_t read;
    update_write_t write;
    void *context;
} update_io_t;

typedef struct
{
    char name[UPDATE_NAME_MAX + 1u];
    uint32_t size;
} update_file_t;

/* 调用者提供 1028 字节收包缓冲区；小容量 MCU 上宜定义为静态变量。 */
typedef struct
{
    uint8_t rx[UPDATE_RX_BUFFER_SIZE];
} update_workspace_t;

/* HEADER 事件：data=NULL、length=0、offset=0；DATA 事件：已校验的数据及文件偏移。
 * 数据处理成功后才返回 true，库随后发送 ACK；返回 false 则取消接收。
 * data 仅在本次回调期间有效。回调内不要用同一串口打印日志。
 */
typedef bool (*update_consumer_t)(void *context, update_event_t event,
                                  const update_file_t *file, uint32_t offset,
                                  const uint8_t *data, uint16_t length);

/* 接收一个文件；不访问 Flash，也不解释固件格式。
 * max_file_size 是目标存储区容量；收到最终空文件头后才填写 out_file。
 * 返回错误时，consumer 可能已收到部分数据，调用者必须将其标为无效。
 * 文件名是不可信元数据；CRC16 只检查传输差错，不验证固件来源。
 */
update_result_t update_receive_file(const update_io_t *io,
                                    uint32_t max_file_size,
                                    update_workspace_t *workspace,
                                    update_file_t *out_file,
                                    update_consumer_t consume,
                                    void *context);

#endif /* UPDATE_H */
