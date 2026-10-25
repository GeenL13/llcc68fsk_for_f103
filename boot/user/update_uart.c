#include "update_uart.h"

#include <stddef.h>

static update_workspace_t packet_workspace;
static bool receiving;

static update_io_result_t uart_read(void *context, uint8_t *data,
                                    uint16_t length, uint32_t timeout_ms)
{
    HAL_StatusTypeDef status = HAL_UART_Receive((UART_HandleTypeDef *)context,
                                                data, length, timeout_ms);
    if (status == HAL_OK)
    {
        return UPDATE_IO_OK;
    }
    return (status == HAL_TIMEOUT) ? UPDATE_IO_TIMEOUT : UPDATE_IO_ERROR;
}

static update_io_result_t uart_write(void *context, const uint8_t *data,
                                     uint16_t length, uint32_t timeout_ms)
{
    HAL_StatusTypeDef status = HAL_UART_Transmit((UART_HandleTypeDef *)context,
                                                 data, length, timeout_ms);
    if (status == HAL_OK)
    {
        return UPDATE_IO_OK;
    }
    return (status == HAL_TIMEOUT) ? UPDATE_IO_TIMEOUT : UPDATE_IO_ERROR;
}

update_result_t update_receive_uart(UART_HandleTypeDef *uart,
                                    uint32_t max_file_size,
                                    update_file_t *out_file,
                                    update_consumer_t consumer,
                                    void *consumer_context)
{
    update_io_t io;
    update_result_t result;

    if ((uart == NULL) || (out_file == NULL) || (consumer == NULL) ||
        (max_file_size == 0u) || receiving)
    {
        return UPDATE_BAD_ARGUMENT;
    }

    receiving = true;
    io.read = uart_read;
    io.write = uart_write;
    io.context = uart;
    result = update_receive_file(&io, max_file_size, &packet_workspace,
                                 out_file, consumer, consumer_context);
    receiving = false;
    return result;
}
