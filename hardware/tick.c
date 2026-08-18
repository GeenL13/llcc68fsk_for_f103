/*
 * tick.c
 *
 *  Created on: 2026年8月11日
 *      Author: lianz
 */


#include "tick.h"
#include "stm32f1xx_hal.h"

uint32_t get_tick_ms(void)
{
    return HAL_GetTick();
}