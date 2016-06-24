/*
 * =====================================================================================
 *
 *       Filename:  timer.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2015/12/3 ÐÇÆÚËÄ ÉÏÎç 10:48:35
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  lell 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef  __TIMER_H__
#define  __TIMER_H__
#include "stm32f10x.h"


#ifndef NULL
#define NULL ((void *)0)
#endif



#define	TIMEREVENT_MAX_SIZE      20			/*max size of timer event number  */
#define TIMER_LOOP_FOREVER       (uint16_t)0xffff

#define TIMER_INVALID            0xff

#define TIMER_EVENT_IDLE         0x00
#define TIMER_EVENT_ACTIVE       0x01
#define TIMER_EVENT_RECYCLE      0x02

void timer_init ( void );
void timer_tick (void);
void timer_mainloop ( void );
void timer_loop ( uint16_t delayms, void (*callback)(void), uint16_t times);
void timer_runlater ( uint16_t delayms, void (*callback)(void));
void timer_delay ( uint16_t delayms);
uint8_t timer_get_eventnum(void);

#endif     /* -----  E__TIMER_H__  ----- */
