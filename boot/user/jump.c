#include "stm32f1xx.h"
#include "flash_layout.h"
#include <stdbool.h>
#include <stdint.h>

static bool valid_vectors(uint32_t base)
{
    uint32_t sp = *(volatile const uint32_t *)base;
    uint32_t pc = *(volatile const uint32_t *)(base + 4u);
    if (sp <= RAM_FIRST || sp > RAM_END || (sp & 7u)) return false;
    if ((pc & 1u) == 0u) return false;  /* Cortex-M 入口必须是 Thumb */
    uint32_t code = pc & ~1u;
    return code >= base && code < base + APP_SIZE;
}

__attribute__((naked, noreturn))
static void enter_reset_handler(uint32_t base)
{
    __asm volatile(
        "ldr r1, [r0]      \n"  /* r1 = 初始 MSP */
        "ldr r2, [r0, #4]  \n"  /* r2 = Reset_Handler | 1 */
        "msr msp, r1       \n"
        "bx r2             \n"
    );
}

bool boot_jump(uint32_t base)
{
    if (base != APP_A_BASE && base != APP_B_BASE)
    {
        return false;
    }

    if (!valid_vectors(base))
    {
        return false;
    }
    __disable_irq();
    if (RCC->APB2ENR & RCC_APB2ENR_USART1EN)
        USART1->CR1 = 0; /* 若 boot 曾开启串口，跳前先停用 */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;
    for (unsigned i = 0; i < 2; ++i) {
        NVIC->ICER[i] = 0xFFFFFFFFu;
        NVIC->ICPR[i] = 0xFFFFFFFFu;
    }
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;
    SCB->VTOR = base;
    __DSB();
    __ISB();
    __enable_irq(); /* 清空挂起源后交给应用自己的初始化代码 */
    
    enter_reset_handler(base);
}
