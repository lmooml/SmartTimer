/*
 * =====================================================================================
 *
 *       Filename:  smarttimer.h
 *
 *    Description:  
 *
 *        Version:  1.1
 *        Created:  2016/7/14 ÐÇÆÚËÄ ÉÏÎç 10:48:35
 *       Revision:  none
 *       Compiler:  armcc
 *
 *         Author:  lell 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef  __SMARTTIMER_H__
#define  __SMARTTIMER_H__
#include "stm32f10x.h"

//#define STIM_DEBUG


#ifndef NULL
#define NULL ((void *)0)
#endif

#define CLOSE_INTERRUPT()       __ASM("CPSID  I")
#define OPEN_INTERRUPT()        __ASM("CPSIE  I")  


#define	STIM_EVENT_MAX_SIZE      20			            /*max size of timer event number  */
#define STIM_LOOP_FOREVER       (uint16_t)0xffff

#define STIM_INVALID            0xff

#define STIM_EVENT_IDLE         0x00
#define STIM_EVENT_ACTIVE       0x01
#define STIM_EVENT_RECYCLE      0x02

void stim_init ( void );
void stim_tick (void);
void stim_mainloop ( void );
void stim_loop ( uint16_t delayms, void (*callback)(void), uint16_t times);
void stim_runlater ( uint16_t delayms, void (*callback)(void));
void stim_delay ( uint16_t delayms);

#ifdef STIM_DEBUG
uint8_t stim_get_eventnum(void);
void stim_print_status(void);
#endif

#endif    
