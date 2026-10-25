#ifndef UPDATE_UART_H
#define UPDATE_UART_H

#include "update.h"
#include "stm32f1xx_hal.h"

/* 通过已初始化的 UART 阻塞接收一个 YMODEM 文件。
 * 本函数负责协议解析和应答；consumer 负责处理文件头与数据，不写入 Flash。
 * 内部共用一份收包缓冲区，不能并发或递归调用。
 */
update_result_t update_receive_uart(UART_HandleTypeDef *uart,
                                    uint32_t max_file_size,
                                    update_file_t *out_file,
                                    update_consumer_t consumer,
                                    void *consumer_context);

#endif /* UPDATE_UART_H */
