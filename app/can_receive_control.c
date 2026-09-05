/*
 * can_receive_control.c
 *
 *  Created on: Jan 29, 2026
 *      Author: Geen
 */

#include "can_receive_control.h"
#include "stm32f1xx_hal.h"

#define LIST_LENGTH     (sizeof(can_id_cooldown_list) / sizeof(can_id_cooldown_list[0]))


typedef struct __attribute__((__packed__))
{
    uint32_t id;
    uint32_t cooldown_ms;
    uint32_t next_allowed_time;
}can_id_cooldown_t;

can_id_cooldown_t can_id_cooldown_list[] =
{
    {0xC4, 100, 0},
    {0xC1, 100, 0},
    {0xC7, 100, 0},
    {0xC9, 100, 0},
    {0x150, 100, 0},
    {0x151, 100, 0},
    {0x711, 100, 0},
    {0x283, 100, 0},
    {0x184, 100, 0},
    {0x284, 100, 0},
    {0x185, 100, 0},
    {0x1A, 100, 0},
};




int check_and_update_cooldown(uint32_t can_id)
{
    for (uint8_t i = 0; i < LIST_LENGTH; i++)
    {
        if (can_id_cooldown_list[i].id == can_id)
        {
            // 判断当前时间是否超过允许接收的时间
            if (HAL_GetTick() < can_id_cooldown_list[i].next_allowed_time)
            {
                // 仍在冷却时间内，拒绝接收
                return 1;
            }
            // 更新下一个允许接收的时间: 当前时间 + 冷却时间
            can_id_cooldown_list[i].next_allowed_time = HAL_GetTick() + can_id_cooldown_list[i].cooldown_ms;
            return 0; // 允许接收
        }
    }
    return 1; // 如果ID不在列表中，默认拒绝接收
}
