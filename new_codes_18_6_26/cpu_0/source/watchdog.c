/***************************************************************************//**
 * @file     watchdog.c
 * @brief    All application specific source files
 * @version  1.0.0
 * @details
 * Compiler : Keil uVision
 * Target : Nuvoton M263
 * Module : Central Controller Card
 * Date Created : Jan 09, 2026
 * @copyright Copyright (C) 2026 Anaya Tech Systems Pvt. Ltd. . All rights reserved.
 * @author    SG
 * @par Functions Included
 * void WDT_IRQHandler(void); <br>
 * void watchdog(void); <br>
 * void watchdog_init(void);
 ******************************************************************************/

#include <stdio.h>
#include "NuMicro.h"

static volatile uint8_t g_u8IsINTEvent;
static volatile uint32_t g_u32WakeupCounts;
void watchdog(void);

/**
 * @brief   Watchdog Timer Interrupt Handler.
 *
 * This function handles Watchdog Timer (WDT) interrupts. It refreshes
 * the WDT counter for a limited number of wake-up events, processes
 * timeout interrupts, and tracks wake-up occurrences.
 *
 * @param  None
 *
 * @return None
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_POST_045
 * PI_SSBPAC_SwRS_POST_046
 * PI_SSBPAC_SwRS_POST_047
 * PI_SSBPAC_SwRS_POST_048
 * PI_SSBPAC_SwRS_INTR_010
 * PI_SSBPAC_SwRS_INTR_011
 * PI_SSBPAC_SwRS_GENR_009
 *
 * @note
 *  Resets the WDT counter while the wake-up count is less than 10.
 *  Detects and clears the WDT timeout interrupt flag.
 *  Detects and clears the WDT timeout wake-up flag.
 *  Updates global status flags and counters accordingly.
 */

void WDT_IRQHandler(void)
{
    if(g_u32WakeupCounts < 10U)
    {
        WDT_RESET_COUNTER();
    }

    if(WDT_GET_TIMEOUT_INT_FLAG() == 1)
    {
        /* Clear WDT time-out interrupt flag */
        WDT_CLEAR_TIMEOUT_INT_FLAG();

        g_u8IsINTEvent = 1;
    }

    if(WDT_GET_TIMEOUT_WAKEUP_FLAG() == 1)
    {
        /* Clear WDT time-out wake-up flag */
        WDT_CLEAR_TIMEOUT_WAKEUP_FLAG();

        g_u32WakeupCounts++;
    }
}

#if 0

/**
 * @brief  Demonstrates Watchdog Timer (WDT) configuration, interrupt handling,
 *         wake-up from power-down mode, and system reset behavior.
 *
 * @param  None
 *
 * @return None
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note   - Enables WDT module clock and selects LIRC as clock source.
 *         - Configures WDT with:
 *             * Timeout interval: 2^14 WDT clock cycles (~1.6–1.7 seconds)
 *             * Interrupt enabled
 *             * Wake-up function enabled
 *             * Reset enabled with delay
 *         - Checks if previous reset was caused by WDT and halts if true.
 *         - Uses PA.2 GPIO pin to indicate WDT timeout interval (toggle).
 *         - Enables WDT interrupt via NVIC.
 *         - Unlocks protected registers before WDT and power control configuration.
 *         - Enters power-down mode in loop and wakes up via WDT interrupt.
 *         - Waits for WDT interrupt event (g_u8IsINTEvent) after wake-up.
 *         - Toggles PA.2 on each wake-up cycle and prints wake-up count.
 *         - If WDT interrupt count exceeds threshold (handled in ISR),
 *           system will not reset WDT counter and will trigger system reset.
 *         - Intended for testing/debugging WDT behavior and low-power wake-up.
 *         - Contains blocking loops and debug prints; not suitable for production use.
 */
 
void watchdog(void) {
	uint32_t u32TimeOutCnt_wdg = 0;
    /* Enable WDT module clock */
    CLK_EnableModuleClock(WDT_MODULE);
    CLK_SetModuleClock(WDT_MODULE, CLK_CLKSEL1_WDTSEL_LIRC, 0);

    printf("\n\nCPU @ %d Hz\n", SystemCoreClock);
    printf("+----------------------------------------+\n");
    printf("|    WDT Time-out Wake-up Sample Code    |\n");
    printf("+----------------------------------------+\n\n");

    /* To check if system has been reset by WDT time-out reset or not */
    if(WDT_GET_RESET_FLAG() == 1)
    {
        WDT_CLEAR_RESET_FLAG();
        printf("*** System has been reset by WDT time-out event ***\n\n");
        while(1) {}
    }

    printf("# WDT Settings:\n");
    printf("    - Clock source is LIRC                  \n");
    printf("    - Time-out interval is 2^14 * WDT clock \n");
    printf("      (around 1.6384 ~ 1.7408 s)            \n");
    printf("    - Interrupt enable                      \n");
    printf("    - Wake-up function enable               \n");
    printf("    - Reset function enable                 \n");
    printf("    - Reset delay period is 18 * WDT clock  \n");
    printf("# System will generate a WDT time-out interrupt event after 1.6384 ~ 1.7408 s, \n");
    printf("    and will be wake up from power-down mode also.\n");
    printf("    (Use PA.2 high/low period time to check WDT time-out interval)\n");
    printf("# When WDT interrupt counts large than 10, \n");
    printf("    we will not reset WDT counter value and system will be reset immediately by WDT time-out reset signal.\n");

    /* Use PA.2 to check time-out interrupt period time */
    PA->MODE = (PA->MODE & ~GPIO_MODE_MODE2_Msk) | (GPIO_MODE_OUTPUT << GPIO_MODE_MODE2_Pos);
    PA2 = 1;

    /* Enable WDT NVIC */
    NVIC_EnableIRQ(WDT_IRQn);

    /* Because of all bits can be written in WDT Control Register are write-protected;
       To program it needs to disable register protection first. */
    SYS_UnlockReg();

    /* Configure WDT settings and start WDT counting */
    g_u8IsINTEvent = g_u32WakeupCounts = 0;
    WDT_Open(WDT_TIMEOUT_2POW14, WDT_RESET_DELAY_18CLK, TRUE, TRUE);

    /* Enable WDT interrupt function */
    WDT_EnableInt();

    while(1)
    {
        /* System enter to Power-down */
        /* To program PWRCTL register, it needs to disable register protection first. */
        SYS_UnlockReg();
        printf("\nSystem enter to power-down mode ...\n");
        /* To check if all the debug messages are finished */
        u32TimeOutCnt_wdg = SystemCoreClock; /* 1 second time-out */
        while(!UART_IS_TX_EMPTY(DEBUG_PORT))
            if(--u32TimeOutCnt_wdg == 0) break;
        CLK_PowerDown();

        /* Check if WDT time-out interrupt and wake-up occurred or not */
        u32TimeOutCnt_wdg = SystemCoreClock; /* 1 second time-out */
        while(g_u8IsINTEvent == 0)
        {
            if(--u32TimeOutCnt_wdg == 0)
            {
                printf("Wait for WDT interrupt time-out!\n");
                break;
            }
        }
        PA2 ^= 1;

        g_u8IsINTEvent = 0;
        printf("System has been waken up done. WDT wake-up counts: %d.\n\n", g_u32WakeupCounts);
    }
}
#else

/**
 * @brief   Initialize the Watchdog Timer (WDT).
 *
 * This function configures and starts the Watchdog Timer using the
 * internal low-speed clock. It sets the timeout period, reset delay,
 * and initializes related global status variables.
 *
 * @param  None
 *
 * @return None
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_POST_044
 * PI_SSBPAC_SwRS_POST_049
 * PI_SSBPAC_SwRS_POST_050
 * PI_SSBPAC_SwRS_POST_051
 *
 * @details
 *  Enables the WDT module clock and selects LIRC as the clock source.
 *  Unlocks protected system registers to allow WDT configuration.
 *  Configures the WDT timeout and reset delay.
 *  Optionally enables WDT interrupts and NVIC (currently disabled).
 *
 * @note
 * Register protection must be disabled before configuring the WDT.
 * WDT interrupt and NVIC enable calls are commented out and can be
 * enabled if interrupt-based handling is required.
 */

void watchdog_init(void) {
		uint32_t u32TimeOutCnt_wdg = 0;
    /* Enable WDT module clock */
    CLK_EnableModuleClock(WDT_MODULE);
    CLK_SetModuleClock(WDT_MODULE, CLK_CLKSEL1_WDTSEL_LIRC, 0);

		WDT_GET_RESET_FLAG();
    
    /* Enable WDT NVIC */
    //NVIC_EnableIRQ(WDT_IRQn);

    /* Because of all bits can be written in WDT Control Register are write-protected;
       To program it needs to disable register protection first. */
    SYS_UnlockReg();

    /* Configure WDT settings and start WDT counting */
    g_u8IsINTEvent = 0U;
		g_u32WakeupCounts = 0U;
    WDT_Open(WDT_TIMEOUT_2POW14, WDT_RESET_DELAY_18CLK, TRUE, FALSE);

    /* Enable WDT interrupt function */
    //WDT_EnableInt();
}

#endif
