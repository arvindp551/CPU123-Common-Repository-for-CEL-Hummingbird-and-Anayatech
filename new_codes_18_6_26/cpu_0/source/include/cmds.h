/***************************************************************************//**
 * @file     cmds.h
 * @brief    All application specific source files
 * @version  1.0.0
 * @details
 * Compiler : Keil uVision
 * Target : Nuvoton M263
 * Module : Central Controller Card
 * Date Created : Dec 24, 2025
 * @copyright Copyright (C) 2026 Anaya Tech Systems Pvt. Ltd. . All rights reserved.
 * @author    SG 
 ******************************************************************************/
 
#ifndef __CMDS_H__
#define __CMDS_H__

#include <stdint.h>
#include "cli.h"

extern cmdFormat_t cmdList[];
extern uint8_t cmdListDepth;

#define HW_VERSION "CPU0-HW_00"
#define SW_VERSION "V1.0"

#define REBOOT  WatchdogResetEnable(WATCHDOG0_BASE);

//void cli_showversion(void)
//void cli_setstation(void)
//void cli_setaddress(void)
//void cli_showstate(void)
//void cli_showstate_json(void)
//void cli_showinterlock(void)
//void cli_settime(void)
//void cli_setdate(void)
//void cli_showdate(void)
//void cli_showtime(void)
//void cli_hangcpu(void)
//void cli_showdebug(void)
#endif
