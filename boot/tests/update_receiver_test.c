#include "../user/update.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
    uint8_t input[6000];
    size_t input_size;
    size_t input_pos;
    uint8_t output[128];
    size_t output_size;
    uint8_t consumed[2048];
    size_t consumed_size;
    unsigned int headers;
    unsigned int data_blocks;
    bool reject_data;
} fake_link_t;

static update_io_result_t fake_read(void *context, uint8_t *data,
                                    uint16_t length, uint32_t timeout_ms)
{
    fake_link_t *link = context;
    size_t available = link->input_size - link->input_pos;
    size_t count = (available < length) ? available : length;
    (void)timeout_ms;
    memcpy(data, &link->input[link->input_pos], count);
    link->input_pos += count;
    return (count == length) ? UPDATE_IO_OK : UPDATE_IO_TIMEOUT;
}

static update_io_result_t fake_write(void *context, const uint8_t *data,
                                     uint16_t length, uint32_t timeout_ms)
{
    fake_link_t *link = context;
    (void)timeout_ms;
    if (length > sizeof(link->output) - link->output_size)
    {
        return UPDATE_IO_ERROR;
    }
    memcpy(&link->output[link->output_size], data, length);
    link->output_size += length;
    return UPDATE_IO_OK;
}

static bool fake_consume(void *context, update_event_t event,
                         const update_file_t *file, uint32_t offset,
                         const uint8_t *data, uint16_t length)
{
    fake_link_t *link = context;
    if (event == UPDATE_EVENT_HEADER)
    {
        assert(file->size != 0u && offset == 0u && data == NULL && length == 0u);
        ++link->headers;
        return true;
    }
    if (link->reject_data || offset != link->consumed_size ||
        length > sizeof(link->consumed) - link->consumed_size)
    {
        return false;
    }
    memcpy(&link->consumed[link->consumed_size], data, length);
    link->consumed_size += length;
    ++link->data_blocks;
    return true;
}

static uint16_t test_crc(const uint8_t *data, size_t length)
{
    uint16_t crc = 0u;
    while (length-- != 0u)
    {
        crc ^= (uint16_t)*data++ << 8;
        for (unsigned int i = 0u; i < 8u; ++i)
        {
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u)
                                  : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

static void append_byte(fake_link_t *link, uint8_t value)
{
    assert(link->input_size < sizeof(link->input));
    link->input[link->input_size++] = value;
}

static void append_packet(fake_link_t *link, uint8_t start, uint8_t number,
                          const uint8_t *payload, size_t useful_length,
                          uint8_t padding)
{
    uint8_t block[1024];
    size_t length = (start == 0x01u) ? 128u : 1024u;
    uint16_t crc;
    assert((start == 0x01u || start == 0x02u) && useful_length <= length);
    memset(block, padding, length);
    memcpy(block, payload, useful_length);
    crc = test_crc(block, length);
    append_byte(link, start);
    append_byte(link, number);
    append_byte(link, (uint8_t)~number);
    for (size_t i = 0u; i < length; ++i)
    {
        append_byte(link, block[i]);
    }
    append_byte(link, (uint8_t)(crc >> 8));
    append_byte(link, (uint8_t)crc);
}

static void append_header(fake_link_t *link, const char *size)
{
    uint8_t header[128] = {0};
    size_t size_length = strlen(size);
    memcpy(header, "fw.bin", 7u);
    assert(size_length + 7u < sizeof(header));
    memcpy(&header[7], size, size_length);
    append_packet(link, 0x01u, 0u, header, sizeof(header), 0u);
}

static void append_end(fake_link_t *link)
{
    uint8_t empty[128] = {0};
    append_byte(link, 0x04u);
    append_packet(link, 0x01u, 0u, empty, sizeof(empty), 0u);
}

static update_result_t run_receiver(fake_link_t *link, uint32_t capacity,
                                    update_file_t *file)
{
    static update_workspace_t workspace;
    update_io_t io = {fake_read, fake_write, link};
    return update_receive_file(&io, capacity, &workspace, file,
                               fake_consume, link);
}

static void test_crc_reference(void)
{
    static const uint8_t sample[] = "123456789";
    assert(test_crc(sample, sizeof(sample) - 1u) == 0x31C3u);
}

static void test_normal_1k_and_duplicate(void)
{
    fake_link_t link = {0};
    update_file_t file;
    uint8_t firmware[1500];
    static const uint8_t replies[] = {0x43u, 0x06u, 0x43u, 0x06u,
                                      0x06u, 0x06u, 0x06u, 0x43u, 0x06u};
    for (size_t i = 0u; i < sizeof(firmware); ++i)
    {
        firmware[i] = (uint8_t)i;
    }
    append_header(&link, "1500");
    append_packet(&link, 0x02u, 1u, firmware, 1024u, 0x1Au);
    append_packet(&link, 0x02u, 1u, firmware, 1024u, 0x1Au);
    append_packet(&link, 0x02u, 2u, firmware + 1024u, 476u, 0x1Au);
    append_end(&link);
    assert(run_receiver(&link, 2048u, &file) == UPDATE_OK);
    assert(file.size == 1500u && strcmp(file.name, "fw.bin") == 0);
    assert(link.headers == 1u && link.data_blocks == 2u);
    assert(link.consumed_size == sizeof(firmware));
    assert(memcmp(link.consumed, firmware, sizeof(firmware)) == 0);
    assert(link.output_size == sizeof(replies));
    assert(memcmp(link.output, replies, sizeof(replies)) == 0);
}

static void test_bad_crc_then_retransmit(void)
{
    fake_link_t link = {0};
    update_file_t file;
    const uint8_t firmware[] = {0x12u, 0x34u, 0x56u};
    append_header(&link, "3");
    append_packet(&link, 0x01u, 1u, firmware, sizeof(firmware), 0x1Au);
    link.input[link.input_size - 1u] ^= 1u;
    append_packet(&link, 0x01u, 1u, firmware, sizeof(firmware), 0x1Au);
    append_end(&link);
    assert(run_receiver(&link, 3u, &file) == UPDATE_OK);
    assert(file.size == 3u && link.consumed_size == 3u);
    assert(link.data_blocks == 1u);
    assert(memchr(link.output, 0x15, link.output_size) != NULL);
}

static void test_invalid_header_and_capacity(void)
{
    fake_link_t link = {0};
    update_file_t file;
    uint8_t invalid[128];
    memset(invalid, 'A', sizeof(invalid));
    append_packet(&link, 0x01u, 0u, invalid, sizeof(invalid), 0u);
    assert(run_receiver(&link, 2048u, &file) == UPDATE_BAD_HEADER);
    assert(link.headers == 0u && link.output_size == 3u);
    assert(link.output[1] == 0x18u && link.output[2] == 0x18u);

    memset(&link, 0, sizeof(link));
    append_header(&link, "42949672960");
    assert(run_receiver(&link, UINT32_MAX, &file) == UPDATE_BAD_HEADER);
    assert(link.headers == 0u);

    memset(&link, 0, sizeof(link));
    append_header(&link, "1500");
    assert(run_receiver(&link, 1499u, &file) == UPDATE_TOO_LARGE);
    assert(link.headers == 0u);
}

static void test_partial_packet_and_early_eot(void)
{
    fake_link_t link = {0};
    update_file_t file;
    append_byte(&link, 0x01u);
    append_byte(&link, 0u);
    assert(run_receiver(&link, 2048u, &file) == UPDATE_PARTIAL_PACKET);
    assert(link.output_size == 3u && link.output[1] == 0x18u);

    memset(&link, 0, sizeof(link));
    append_header(&link, "3");
    append_byte(&link, 0x04u);
    assert(run_receiver(&link, 2048u, &file) == UPDATE_BAD_PACKET);
    assert(link.headers == 1u && link.data_blocks == 0u);
}

static void test_consumer_rejects_data(void)
{
    fake_link_t link = {0};
    update_file_t file;
    uint8_t data = 0x55u;
    link.reject_data = true;
    append_header(&link, "1");
    append_packet(&link, 0x01u, 1u, &data, 1u, 0x1Au);
    assert(run_receiver(&link, 2048u, &file) == UPDATE_CONSUMER_ERROR);
    assert(link.output_size == 5u);
    assert(link.output[3] == 0x18u && link.output[4] == 0x18u);
}

int main(void)
{
    test_crc_reference();
    test_normal_1k_and_duplicate();
    test_bad_crc_then_retransmit();
    test_invalid_header_and_capacity();
    test_partial_packet_and_early_eot();
    test_consumer_rejects_data();
    puts("update receiver tests passed");
    return 0;
}
