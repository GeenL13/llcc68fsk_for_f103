#include "update.h"

#include <stddef.h>
#include <string.h>

/* 帧格式参照本项目中的 ST STM32CubeF1 YMODEM 示例。
 * CRC16_REQUEST 是请求 CRC 模式的字符 'C'，不是计算出的 CRC 值。
 */
#define UPDATE_SOH           0x01u
#define UPDATE_STX           0x02u
#define UPDATE_EOT           0x04u
#define UPDATE_ACK           0x06u
#define UPDATE_NAK           0x15u
#define UPDATE_CAN           0x18u
#define UPDATE_CRC16_REQUEST 0x43u
#define UPDATE_TX_TIMEOUT_MS 100u

_Static_assert(UPDATE_PACKET_MAX == 1024u, "YMODEM STX payload is 1024 bytes");
_Static_assert(UPDATE_RX_BUFFER_SIZE >= UPDATE_PACKET_MAX + 4u,
               "workspace must hold sequence, payload and CRC");

typedef enum
{
    FRAME_DATA,
    FRAME_EOT,
    FRAME_CANCEL
} frame_kind_t;

typedef struct
{
    frame_kind_t kind;
    uint8_t number;
    uint16_t length;
    const uint8_t *data; /* 指向调用者提供的收包缓冲区。 */
} frame_t;

static update_result_t send_byte(const update_io_t *io, uint8_t value)
{
    return (io->write(io->context, &value, 1u, UPDATE_TX_TIMEOUT_MS) == UPDATE_IO_OK)
               ? UPDATE_OK : UPDATE_IO_FAILURE;
}

static void send_cancel(const update_io_t *io)
{
    (void)send_byte(io, UPDATE_CAN);
    (void)send_byte(io, UPDATE_CAN);
}

static uint16_t crc16_xmodem(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0u;
    uint16_t i;
    uint8_t bit;

    for (i = 0u; i < length; ++i)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (bit = 0u; bit < 8u; ++bit)
        {
            crc = ((crc & 0x8000u) != 0u)
                      ? (uint16_t)((crc << 1) ^ 0x1021u)
                      : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

/* 读取一帧；起始字节单独读取，不存入 rx[]。 */
static update_result_t read_frame(const update_io_t *io,
                                  update_workspace_t *workspace,
                                  frame_t *frame)
{
    uint8_t start;
    uint8_t second;
    uint16_t length;
    uint16_t received_crc;
    update_io_result_t status;

    status = io->read(io->context, &start, 1u, UPDATE_RX_TIMEOUT_MS);
    if (status == UPDATE_IO_TIMEOUT)
    {
        return UPDATE_TIMEOUT;
    }
    if (status != UPDATE_IO_OK)
    {
        return UPDATE_IO_FAILURE;
    }
    if (start == UPDATE_EOT)
    {
        frame->kind = FRAME_EOT;
        return UPDATE_OK;
    }
    if (start == UPDATE_CAN)
    {
        status = io->read(io->context, &second, 1u, UPDATE_RX_TIMEOUT_MS);
        if (status != UPDATE_IO_OK)
        {
            return (status == UPDATE_IO_TIMEOUT) ? UPDATE_PARTIAL_PACKET : UPDATE_IO_FAILURE;
        }
        if (second != UPDATE_CAN)
        {
            return UPDATE_BAD_PACKET;
        }
        frame->kind = FRAME_CANCEL;
        return UPDATE_OK;
    }
    if (start == UPDATE_SOH)
    {
        length = 128u;
    }
    else if (start == UPDATE_STX)
    {
        length = UPDATE_PACKET_MAX;
    }
    else
    {
        return UPDATE_BAD_PACKET;
    }

    /* rx[] 依次存放包序号、反码、数据、CRC 高字节和低字节。 */
    status = io->read(io->context, workspace->rx, (uint16_t)(length + 4u),
                      UPDATE_RX_TIMEOUT_MS);
    if (status != UPDATE_IO_OK)
    {
        return (status == UPDATE_IO_TIMEOUT) ? UPDATE_PARTIAL_PACKET : UPDATE_IO_FAILURE;
    }
    if ((uint8_t)(workspace->rx[0] ^ workspace->rx[1]) != 0xFFu)
    {
        return UPDATE_BAD_PACKET;
    }
    received_crc = (uint16_t)((uint16_t)workspace->rx[length + 2u] << 8);
    received_crc |= workspace->rx[length + 3u];
    if (crc16_xmodem(&workspace->rx[2], length) != received_crc)
    {
        return UPDATE_BAD_PACKET;
    }

    frame->kind = FRAME_DATA;
    frame->number = workspace->rx[0];
    frame->length = length;
    frame->data = &workspace->rx[2];
    return UPDATE_OK;
}

static update_result_t parse_header(const frame_t *frame, uint32_t max_file_size,
                                    update_file_t *file)
{
    uint16_t name_length = 0u;
    uint16_t pos;
    uint32_t size = 0u;
    bool has_digit = false;

    while ((name_length < frame->length) && (frame->data[name_length] != 0u))
    {
        ++name_length;
    }
    if ((name_length == 0u) || (name_length > UPDATE_NAME_MAX) ||
        (name_length >= frame->length))
    {
        return UPDATE_BAD_HEADER;
    }

    pos = (uint16_t)(name_length + 1u);
    while ((pos < frame->length) &&
           (frame->data[pos] >= (uint8_t)'0') &&
           (frame->data[pos] <= (uint8_t)'9'))
    {
        uint8_t digit = (uint8_t)(frame->data[pos] - (uint8_t)'0');
        if (size > (UINT32_MAX - digit) / 10u)
        {
            return UPDATE_BAD_HEADER;
        }
        size = size * 10u + digit;
        has_digit = true;
        ++pos;
    }
    if (!has_digit || (size == 0u) ||
        ((pos < frame->length) && (frame->data[pos] != 0u) &&
         (frame->data[pos] != (uint8_t)' ')))
    {
        return UPDATE_BAD_HEADER;
    }
    if (size > max_file_size)
    {
        return UPDATE_TOO_LARGE;
    }

    memcpy(file->name, frame->data, name_length);
    file->name[name_length] = '\0';
    file->size = size;
    return UPDATE_OK;
}

/* 包损坏或缺失时请求重传，不接收该包中的数据。 */
static update_result_t retry(const update_io_t *io, uint32_t *errors,
                             uint8_t reply, update_result_t last_error)
{
    ++*errors;
    if (*errors >= UPDATE_MAX_ERRORS)
    {
        send_cancel(io);
        return last_error;
    }
    return send_byte(io, reply);
}

update_result_t update_receive_file(const update_io_t *io,
                                    uint32_t max_file_size,
                                    update_workspace_t *workspace,
                                    update_file_t *out_file,
                                    update_consumer_t consume,
                                    void *context)
{
    update_file_t file = {0};
    frame_t frame;
    update_result_t result;
    uint32_t received = 0u;
    uint32_t errors = 0u;
    uint8_t expected = 1u;
    bool have_header = false;
    bool have_eot = false;

    if ((io == NULL) || (io->read == NULL) || (io->write == NULL) ||
        (workspace == NULL) || (out_file == NULL) ||
        (consume == NULL) || (max_file_size == 0u))
    {
        return UPDATE_BAD_ARGUMENT;
    }
    memset(out_file, 0, sizeof(*out_file));
    result = send_byte(io, UPDATE_CRC16_REQUEST);
    if (result != UPDATE_OK)
    {
        return result;
    }

    for (;;)
    {
        result = read_frame(io, workspace, &frame);
        if ((result == UPDATE_TIMEOUT) || (result == UPDATE_BAD_PACKET))
        {
            update_result_t failure = result;
            uint8_t reply = (!have_header || have_eot)
                                ? UPDATE_CRC16_REQUEST : UPDATE_NAK;
            result = retry(io, &errors, reply, failure);
            if (result != UPDATE_OK)
            {
                return result;
            }
            continue;
        }
        if (result != UPDATE_OK)
        {
            if (result == UPDATE_PARTIAL_PACKET)
            {
                send_cancel(io);
            }
            return result;
        }
        if (frame.kind == FRAME_CANCEL)
        {
            (void)send_byte(io, UPDATE_ACK);
            return UPDATE_CANCELLED;
        }
        if (frame.kind == FRAME_EOT)
        {
            if (!have_header || (received != file.size))
            {
                send_cancel(io);
                return UPDATE_BAD_PACKET;
            }
            if (have_eot && (++errors >= UPDATE_MAX_ERRORS))
            {
                send_cancel(io);
                return UPDATE_BAD_PACKET;
            }
            result = send_byte(io, UPDATE_ACK);
            if (result == UPDATE_OK)
            {
                result = send_byte(io, UPDATE_CRC16_REQUEST);
            }
            if (result != UPDATE_OK)
            {
                return result;
            }
            have_eot = true;
            continue;
        }

        if (have_eot)
        {
            if ((frame.number != 0u) || (frame.data[0] != 0u))
            {
                send_cancel(io);
                return UPDATE_BAD_PACKET;
            }
            result = send_byte(io, UPDATE_ACK);
            if (result == UPDATE_OK)
            {
                *out_file = file;
            }
            return result;
        }
        if (!have_header)
        {
            if (frame.number != 0u)
            {
                result = retry(io, &errors, UPDATE_NAK, UPDATE_BAD_PACKET);
                if (result != UPDATE_OK)
                {
                    return result;
                }
                continue;
            }
            result = parse_header(&frame, max_file_size, &file);
            if (result != UPDATE_OK)
            {
                send_cancel(io);
                return result;
            }
            if (!consume(context, UPDATE_EVENT_HEADER, &file, 0u, NULL, 0u))
            {
                send_cancel(io);
                return UPDATE_CONSUMER_ERROR;
            }
            result = send_byte(io, UPDATE_ACK);
            if (result == UPDATE_OK)
            {
                result = send_byte(io, UPDATE_CRC16_REQUEST);
            }
            if (result != UPDATE_OK)
            {
                return result;
            }
            have_header = true;
            errors = 0u;
            continue;
        }

        if (frame.number == (uint8_t)(expected - 1u))
        {
            /* 上次 ACK 可能丢失；重复包只重发 ACK，不重复交付数据。 */
            if (++errors >= UPDATE_MAX_ERRORS)
            {
                send_cancel(io);
                return UPDATE_BAD_PACKET;
            }
            result = send_byte(io, UPDATE_ACK);
            if ((result == UPDATE_OK) && (expected == 1u))
            {
                result = send_byte(io, UPDATE_CRC16_REQUEST);
            }
            if (result != UPDATE_OK)
            {
                return result;
            }
            continue;
        }
        if (frame.number != expected)
        {
            result = retry(io, &errors, UPDATE_NAK, UPDATE_BAD_PACKET);
            if (result != UPDATE_OK)
            {
                return result;
            }
            continue;
        }
        if (received >= file.size)
        {
            send_cancel(io);
            return UPDATE_BAD_PACKET;
        }

        /* 最后一包可能有填充字节，只向 consumer 交付文件实际长度内的数据。 */
        uint32_t remaining = file.size - received;
        uint16_t valid_length = (remaining < frame.length)
                                    ? (uint16_t)remaining : frame.length;
        if (!consume(context, UPDATE_EVENT_DATA, &file, received,
                     frame.data, valid_length))
        {
            send_cancel(io);
            return UPDATE_CONSUMER_ERROR;
        }
        received += valid_length;
        expected = (uint8_t)(expected + 1u);
        errors = 0u;
        result = send_byte(io, UPDATE_ACK);
        if (result != UPDATE_OK)
        {
            return result;
        }
    }
}
