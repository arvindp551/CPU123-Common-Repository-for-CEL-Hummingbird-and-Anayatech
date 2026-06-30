/***************************************************************************//**
 * @file     utils.c
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
 * uint32_t conv2seconds(time_s* tod); <br>
 * void conv2TOD(uint32_t seconds,time_s* tod);
 ******************************************************************************/

#include "stdint.h"
#include "bsp.h"
#include "app.h"

#define SECONDS_PER_DAY 24U*60U*60U
#define SECONDS_PER_YEAR 365U*SECONDS_PER_DAY 
#define SECONDS_PER_LEAP_YEAR 366U*SECONDS_PER_DAY 
#define YEAR_REF 24U
#define MONTH_REF 1U
#define DAY_REF 1U

static uint8_t daysofmonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};

/**
 * @brief  Converts date and time structure to total seconds from a reference time.
 *         Calculates elapsed seconds based on year, month, day, hour, minute,
 *         and second fields relative to predefined reference values.
 *
 * @param  [in]  tod   Pointer to time_s structure containing date and time
 *                     (yy, mo, dd, hh, mm, ss).
 *
 * @return Total number of seconds elapsed since reference date (YEAR_REF,
 *         MONTH_REF, DAY_REF).
 *
 * @details
 * Requirement ID(s) -
 * PI_SSBPAC_SwRS_GENR_042
 *
 * @note   - Accounts for leap years (year divisible by 4).
 *         - Uses predefined constants:
 *             SECONDS_PER_YEAR, SECONDS_PER_LEAP_YEAR,
 *             SECONDS_PER_DAY.
 *         - Uses daysofmonth[] array for month-wise day calculation.
 *         - Adds an extra day for February in leap years.
 *         - Assumes valid input values in the time structure.
 *         - Does not account for century leap year rules (e.g., 100/400 year cases).
 */

// returns seconds from 01/01/2024, 00:00:00
uint32_t conv2seconds(time_s* tod) {
  uint16_t delta,i;
	uint32_t seconds = 0;
	//number of days of complete years
	delta = (tod->yy) - YEAR_REF; 
	for(i=0;i<delta;i++) {
    if(((YEAR_REF+i)%4U) == 0U) 
		{
			seconds += SECONDS_PER_LEAP_YEAR;
		}
		else 
		{
			seconds += SECONDS_PER_YEAR;
		}
	}
	//number of days of complete months
	delta = (tod->mo) - MONTH_REF; 
	for(i=0;i<delta;i++) {
    seconds += daysofmonth[i]*SECONDS_PER_DAY;
		// add one more day if leap ear and month is februrary 
		if((((tod->yy)%4U)== 0U) && (i == 1U) )
		{
		 seconds += SECONDS_PER_DAY;
		}			
	}
	//number of complete days
	seconds += (tod->dd - DAY_REF)*SECONDS_PER_DAY; 
	seconds += ((3600U*tod->hh) + (60U*tod->mm) + (tod->ss));
	return seconds;
}

/**
 * @brief  Converts total elapsed seconds into date and time structure.
 *         Breaks down seconds into year, month, day, hour, minute, and second
 *         components relative to a predefined reference date.
 *
 * @param  [in]  seconds  Total number of seconds elapsed since reference time.
 * @param  [out] tod      Pointer to time_s structure to store calculated
 *                        date and time (yy, mo, dd, hh, mm, ss).
 *
 * @return None
 *
 * @details
 * Requirement ID(s) - ?
 *
 * @note   - Accounts for leap years (year divisible by 4).
 *         - Uses predefined constants:
 *             SECONDS_PER_YEAR, SECONDS_PER_LEAP_YEAR,
 *             SECONDS_PER_DAY.
 *         - Uses daysofmonth[] array for month-wise calculation.
 *         - February is adjusted for leap years (29 days).
 *         - Year is calculated relative to YEAR_REF.
 *         - Month and day values are 1-based.
 *         - Does not handle century leap year rules (e.g., 100/400 year cases).
 *         - Assumes valid input seconds value within supported range.
 */
 
// IN: seconds from 01/01/2024 00:00:00 OUT: tod
void conv2TOD(uint32_t seconds,time_s* tod) {
  uint16_t i=0;
	uint8_t is_leapyear;
	//get complete years
	while(1){
	is_leapyear = (uint8_t)(((YEAR_REF+i)%4U) == 0U);
    if((is_leapyear != 0U) && (seconds >= (SECONDS_PER_LEAP_YEAR))) {
			i++;
			seconds -= SECONDS_PER_LEAP_YEAR;
		}
		else if((is_leapyear == 0U) && (seconds >= (SECONDS_PER_YEAR))) {
			i++;
			seconds -= SECONDS_PER_YEAR;
		}
    else {
			tod->yy = (YEAR_REF+i);
			break;
		}
	}

	//get complete months
	i = 0;
	while(1){
		if((is_leapyear != 0U)  && (i==1U) && (seconds >= ((daysofmonth[i]+1U)*SECONDS_PER_DAY)) != 0U) {
      seconds -= (daysofmonth[i]+1U)*SECONDS_PER_DAY;
			i++;
		}	
		else if(seconds >= (daysofmonth[i]*SECONDS_PER_DAY)) {
      		seconds -= daysofmonth[i]*SECONDS_PER_DAY;
			i++;
		}
    else {
			tod->mo = i+1U;
			break;
		}
	}

	//get complete days
	i = 0;
	while(1){
		if(seconds >= (SECONDS_PER_DAY)) {
      seconds -= SECONDS_PER_DAY;
			i++;
		}
    else {
			tod->dd = i+1U;
			break;
		}
	}

	//get complete hours
	i = 0;
	while(1){
		if(seconds >= 3600U) {
      seconds -= 3600U;
			i++;
		}
    else {
			tod->hh = i;
			break;
		}
	}

	//get complete minutes
	i = 0;
	while(1){
		if(seconds >= 60U) {
      seconds -= 60U;
			i++;
		}
    else {
			tod->mm = i;
			break;
		}
	}

	tod->ss = seconds;
}

