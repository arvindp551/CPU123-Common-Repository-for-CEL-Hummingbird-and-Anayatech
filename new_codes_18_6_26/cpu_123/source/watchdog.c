/***************************************************************************//**
 * @file     watchdog.c
 * @brief    All application specific source files
 * @version  1.0.0
 * @details
 * Compiler : Keil uVision
 * Target : Nuvoton M263
 * Module : Central Controller Card
 * Date Created : Dec 24, 2025
 * @copyright Copyright (C) 2026 Anaya Tech Systems Pvt. Ltd. . All rights reserved.
 * @author    SG
 * @par Functions Included
 * void watchdog_init(void); <br>
 * static void WDT_IRQHandler(void);
 ******************************************************************************/
 
#include <stdio.h>
#include "NuMicro.h"

static uint8_t g_u8IsINTEvent;
static uint32_t g_u32WakeupCounts;

/**
 * @brief   Watchdog Timer Interrupt Handler.
 *
 * 		This function handles Watchdog Timer (WDT) interrupts. It refreshes the WDT counter for a limited number of wake-up events, processes timeout interrupts, and tracks wake-up occurrences.
 *
 * @param [in] g_u32WakeupCounts
 * 		Global counter tracking the number of WDT wake-up events. Used to determine whether the watchdog counter should be reset.
 *
 * @param [out] g_u8IsINTEvent
 * 		Global flag set when a WDT timeout interrupt event occurs.
 *
 * @param [out] g_u32WakeupCounts
 * 		Incremented when a WDT wake-up event is detected.
 *
 * @return
 * 		None.
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @details
 *  Resets the WDT counter while the wake-up count is less than 10.
 *  Detects and clears the WDT timeout interrupt flag.
 *  Detects and clears the WDT timeout wake-up flag.
 *  Updates global status flags and counters accordingly.
 */

static void WDT_IRQHandler(void)
{
    if(g_u32WakeupCounts < 10U)
    {
        WDT_RESET_COUNTER();
    }

    if(WDT_GET_TIMEOUT_INT_FLAG() == 1U)
    {
        /* Clear WDT time-out interrupt flag */
        WDT_CLEAR_TIMEOUT_INT_FLAG();

        g_u8IsINTEvent = 1;
    }

    if(WDT_GET_TIMEOUT_WAKEUP_FLAG() == 1U)
    {
        /* Clear WDT time-out wake-up flag */
        WDT_CLEAR_TIMEOUT_WAKEUP_FLAG();

        g_u32WakeupCounts++;
    }
}

/**
 * @brief   Initialize the Watchdog Timer (WDT).
 *
 * 		This function configures and starts the Watchdog Timer using the internal low-speed clock. It sets the timeout period, reset delay, and initializes related global status variables.
 *
 * @param [in] g_u32WakeupCounts
 * 		Global counter used to determine the initial interrupt event state.
 *
 * @param [out] g_u8IsINTEvent
 * 		Global flag initialized to indicate whether a WDT interrupt event should be considered active at startup.
 *
 * @return
 * 		None.
 *
 * @details
 * Requirement ID(s) - ?
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
    g_u8IsINTEvent = (g_u32WakeupCounts == 0U)?1:0;
    WDT_Open(WDT_TIMEOUT_2POW16, WDT_RESET_DELAY_18CLK, TRUE, FALSE);

    /* Enable WDT interrupt function */
    //WDT_EnableInt();
}
