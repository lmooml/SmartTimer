/*
 * =====================================================================================
 *
 *       Filename:  timer.c
 *
 *    Description:  timer manager
 *
 *        Version:  1.0
 *        Created:  2015/12/3 ÐÇÆÚËÄ ÉÏÎç 10:37:39
 *       Revision:  none
 *       Compiler:  armcc
 *
 *         Author:  lell(elecsunxin@gmail.com)  
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include "smarttimer.h"
#include <string.h>
#include <stdio.h>


struct stim_event{
  uint16_t interval;    
  uint16_t now;         
  uint16_t looptimes;   
  uint8_t addIndex;     
  uint8_t stat;
  struct stim_event *next;
  struct stim_event *prev;
};

struct stim_event_list{
  struct stim_event *head;
  struct stim_event *tail;
  uint8_t count;
};

static struct stim_event_list event_list; 
static struct stim_event event_pool[STIM_EVENT_MAX_SIZE];
static struct stim_event *recycle_list[STIM_EVENT_MAX_SIZE];
static uint8_t recycle_count;

static void (*callback_list[STIM_EVENT_MAX_SIZE])(void);
static uint8_t mark_list[STIM_EVENT_MAX_SIZE];


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  malloc_event
 *  Description:  get a new event struct from event_pool
 *       return:  a pointer of event struct. 
 *
 * =====================================================================================
 */
static struct stim_event* malloc_event (void)
{
  uint8_t i;
  for(i = 0; i < STIM_EVENT_MAX_SIZE; i++){
    if(event_pool[i].stat == STIM_EVENT_IDLE){
      event_pool[i].stat = STIM_EVENT_ACTIVE;
      return &event_pool[i];
    }
  }
  return NULL;
}		/* -----  end of static function stim_malloc_event  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  free_event
 *  Description:  realse event to event_pool
 * =====================================================================================
 */
static void free_event (struct stim_event *event)
{
	callback_list[event->addIndex] = NULL;
	mark_list[event->addIndex] = STIM_INVALID;

  event->stat = STIM_EVENT_IDLE;
	event->interval = 0;
	event->looptimes = 0;
	event->next = NULL;
	event->now = 0;
	event->prev = NULL;

}		/* -----  end of static function stim_free_event  ----- */

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  push_event
 *  Description:  push a stim_event to event linked list.
 * =====================================================================================
 */
static struct stim_event* push_event ( uint16_t delayms, void (*callback)(void),uint16_t times )
{
  struct stim_event *event;

  event = malloc_event();
  event->interval = delayms;
  event->now = 0;
  event->looptimes = times;
  event->next = NULL;

  mark_list[event->addIndex] = 0;

	if(callback != NULL){
    callback_list[event->addIndex] = callback;
  }

  
  if(event_list.head == NULL){
    event_list.head = event;
    event_list.tail = event;
  }else{
    event_list.tail->next = event;
		event->prev = event_list.tail;
    event_list.tail = event;
  }

  event_list.count++;
  return event;
}		/* -----  end of static function stim_push_delay_event  ----- */



/*
 * ===  FUNCTION  ======================================================================
 *         Name:  pop_event
 *  Description:  pop a timer event from event linked list.
 * =====================================================================================
 */
static void pop_event ( struct stim_event *event )
{
  if(event_list.head == event){
    event_list.head = NULL;
    event_list.tail = NULL;
  } else if (event_list.tail == event){
    event_list.tail = event->prev;
		event_list.tail->next = NULL;
  }else {
    event->prev->next = event->next;
    event->next->prev = event->prev;
  }

  free_event(event);

  event_list.count--;
}		/* -----  end of static function stim_pop_delay_event  ----- */




/*
 *
 * ===  FUNCTION  ======================================================================
 *         Name:  stim_delay
 *  Description:  wait some milliseconds.It will blocking process until time out.
 * =====================================================================================
 */
void stim_delay ( uint16_t delayms)
{

  struct stim_event *event;
  __ASM("CPSID  I");  
  event = push_event(delayms,NULL,1);
  __ASM("CPSIE  I"); 
  while(event->now < event->interval);
}		/* -----  end of function stim_delay  ----- */




/*
 * ===  FUNCTION  ======================================================================
 *         Name:  stim_runlater
 *  Description:  run callback function after some milliseconds by asynchronous.
 * =====================================================================================
 */
void stim_runlater ( uint16_t delayms, void (*callback)(void))
{
  __ASM("CPSID  I"); 
  push_event(delayms,callback,1);
  __ASM("CPSIE  I"); 
}		/* -----  end of function stim_runlater  ----- */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  stim_loop
 *  Description:  looping callback function each interval by asynchronous.
 * =====================================================================================
 */
void stim_loop ( uint16_t delayms, void (*callback)(void), uint16_t times)
{
  __ASM("CPSID  I"); 
  push_event(delayms,callback,times);
  __ASM("CPSIE  I"); 
}		/* -----  end of function stim_loop  ----- */




/*
 * ===  FUNCTION  ======================================================================
 *         Name:  stim_tick
 *  Description:  engine of timer.
 *                It must be execute in systick( or hardware timer) interrupt
 * =====================================================================================
 */
void stim_tick (void)
{
  struct stim_event *tmp;
  if (event_list.count == 0)
    return;

  tmp = event_list.head;
	
  while(tmp != NULL){
    if(tmp->stat != STIM_EVENT_ACTIVE){
			tmp = tmp->next;
      continue;
    }
    
    if(tmp->now == tmp->interval){
      mark_list[tmp->addIndex] += 1;
			if((tmp->looptimes != STIM_LOOP_FOREVER) && 
				(--tmp->looptimes == 0)){
				tmp->stat = STIM_EVENT_RECYCLE;
				recycle_count++;
			}
      tmp->now = 0;
    }

    tmp->now++;
    tmp = tmp->next;
  }
}		/* -----  end of function stim_dispach  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  stim_mainloop
 *  Description:  checkout mark_list,and execute callback funtion if mark be setted.
 *                this function must be execute by while loop in main.c
 * =====================================================================================
 */
void stim_mainloop ( void )
{
  uint8_t i;
  
  for(i = 0; i < STIM_EVENT_MAX_SIZE; i++){
    if((mark_list[i] != STIM_INVALID) && (mark_list[i] > 0)){
      if(callback_list[i] != NULL){
        callback_list[i]();
      }
      mark_list[i] -= 1;
    }
  }

  if(recycle_count > 0){
    for(i = 0; i < STIM_EVENT_MAX_SIZE; i++){
      if(recycle_list[i] != NULL){
        pop_event(recycle_list[i]);
        recycle_count--;
        break;
      }
    }
  }

}		/* -----  end of function stim_loop  ----- */

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  stim_init
 *  Description:  timer init program.  Init event linked list,and setup system clock.
 * =====================================================================================
 */
void stim_init ( void )
{
  uint8_t i;
  struct stim_event *event;
  event_list.head = NULL;
  event_list.tail = NULL;
  event_list.count = 0;

  for(i = 0; i < STIM_EVENT_MAX_SIZE; i++){
    event = &event_pool[i];
    event->stat = STIM_EVENT_IDLE;
    event->interval = 0;
    event->looptimes = 0;
    event->next = NULL;
    event->now = 0;
    event->prev = NULL;
    event->addIndex = i;

    callback_list[i] = NULL;
    mark_list[i] = STIM_INVALID;
  }

  recycle_count = 0;

  SysTick_Config(SystemCoreClock / 100);     //tick is 10ms

}		/* -----  end of function stim_init  ----- */


uint8_t stim_get_eventnum(void){
  return event_list.count;  
}
