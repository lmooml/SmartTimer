/*
 * =====================================================================================
 *
 *       Filename:  smarttimer.c
 *
 *    Description:  software timer dispath
 *
 *        Version:  1.1
 *        Created:  2015/7/14 ÐÇÆÚËÄ ÉÏÎç 10:37:39
 *       Revision:  none
 *       Compiler:  armcc
 *
 *         Author:  lell(elecsunxin@gmail.com)  
 *   Organization:  
 *
 * =====================================================================================
 */
//#include <stdlib.h>
#include "smarttimer.h"

#ifdef STIM_DEBUG
#include <stdio.h>
#endif

struct stim_event{
  uint32_t tick_punch;         
  uint32_t interval;    
  uint32_t looptimes;   
  uint8_t id;     
  uint8_t stat;
  struct stim_event *next;
  struct stim_event *prev;
};

struct stim_event_list{
  struct stim_event *head;
  struct stim_event *tail;
  uint8_t count;
};


struct stim_event_list_manager{
  struct stim_event_list list[2]; 
  uint8_t cur_index;
};


static struct stim_event event_pool[STIM_EVENT_MAX_SIZE];
static struct stim_event_list_manager list_manager;
static struct stim_event_list recycle_list;

static void (*callback_list[STIM_EVENT_MAX_SIZE])(void);
static uint8_t mark_list[STIM_EVENT_MAX_SIZE];
static uint32_t current_tick;



/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  init_linked
 *  Description:  init a stim_event_list linked
 * =====================================================================================
 */
static void init_linked ( struct stim_event_list *list )
{
  list->head = list->tail = NULL;
  list->count = 0;
}		/* -----  end of static function init_linked  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  remove_node
 *  Description:  remove a node from a stim_event_list linked
 * =====================================================================================
 */
static void remove_node ( struct stim_event *event, struct stim_event_list *list )
{

  if(list->head == event){
    list->head = event->next;
    if(list->head == NULL){
      list->tail = NULL;
    }else{
      event->next->prev = NULL;
    }
  }else{
    event->prev->next = event->next;
    if(event->next == NULL){
      list->tail = event->prev;
    }else{
      event->next->prev = event->prev;
    }
  }

  list->count--;
}		/* -----  end of static function remove_event  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  insert_node_prev
 *  Description:  insert a nodes in stim_event_list linked
 * =====================================================================================
 */
static void insert_node_prev ( struct stim_event *new_node,struct stim_event *node,struct stim_event_list *list )
{
  new_node->next = node;
  if(node->prev == NULL){
    list->head = new_node;
    new_node->prev = NULL;
  }else{
    new_node->prev = node->prev;
    node->prev->next = new_node;
  }
  node->prev = new_node;

  list->count++;
}		/* -----  end of static function insert_node_prev  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  insert_to_tail
 *  Description:  insert a nodes in stim_event_list linked
 * =====================================================================================
 */
static void insert_to_tail ( struct stim_event *new_node ,struct stim_event_list *list)
{
  struct stim_event *node = list->tail;

  if(list->count == 0){
    list->head = new_node;
    list->tail = new_node;
    new_node->next = NULL;
    new_node->prev = NULL;
  }else{
    node->next = new_node;
    new_node->prev = node;
    new_node->next = NULL;
    list->tail = new_node;
  }

  list->count++;
}		/* -----  end of static function insert_event  ----- */

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
 *  Description:  release event to event_pool
 * =====================================================================================
 */
static void free_event (struct stim_event *event)
{
  callback_list[event->id] = NULL;
  mark_list[event->id] = STIM_INVALID;

  event->stat = STIM_EVENT_IDLE;
  event->interval = 0;
  event->looptimes = 0;
  event->tick_punch = 0;
  event->prev = NULL;
  event->next = NULL;

}		/* -----  end of static function stim_free_event  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  insert_event
 *  Description:  insert a timer event to a special list
 * =====================================================================================
 */
static void insert_event ( struct stim_event *event,struct stim_event_list *list )
{
  uint8_t i;
  struct stim_event *node;

  if(list->count == 0){
    //insert event to a empty linked
    insert_to_tail(event,list);
  }else{
    node = list->head;
    for(i = 0; i < list->count; i++){
      if(event->tick_punch > node->tick_punch){
        node = node->next;
      }else{
        break;
      }
    }

    if(node == NULL){
      //insert event to linked tail
      insert_to_tail(event,list);
    }else{
      //insert event to a linked
      insert_node_prev(event,node,list);
    }
  }
}		/* -----  end of static function insert_event  ----- */



/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  find_event
 *  Description:  
 * =====================================================================================
 */
static struct stim_event* find_event ( int8_t id, struct stim_event_list *list )
{ 
	uint8_t i;
  struct stim_event *event;
  event = list->head;
  for(i = 0; i < list->count; i++){
    if(event->id == id){
      break;
    }else{
      event = event->next;
    }
  }

  return event;
}		/* -----  end of static function find_event  ----- */



/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  recyle_event
 *  Description:  insert an event which stat is STIM_EVENT_RECYCLE into recycle linked
 * =====================================================================================
 */
static void recyle_event ( struct stim_event *event )
{
  struct stim_event_list *list = &list_manager.list[list_manager.cur_index];

  remove_node(event,list);
  insert_to_tail(event,&recycle_list);
}		/* -----  end of static function recyle_event  ----- */





/*
 * ===  FUNCTION  ======================================================================
 *         Name:  push_event
 *  Description:  push a stim_event to event linked list.
 * =====================================================================================
 */
static struct stim_event* push_event ( uint32_t delayms, void (*callback)(void),uint16_t times )
{
  struct stim_event *event;

  event = malloc_event();
  event->interval = delayms;
  event->looptimes = times;
  event->next = NULL;
  event->tick_punch = current_tick + delayms;

  mark_list[event->id] = 0;
  if(callback != NULL){
    callback_list[event->id] = callback;
  }

  if(event->tick_punch < current_tick){
    insert_event(event,&list_manager.list[list_manager.cur_index ^ 0x01]);
  }else{
    insert_event(event,&list_manager.list[list_manager.cur_index]);
  }

  return event;
}		/* -----  end of static function stim_push_delay_event  ----- */


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
  CLOSE_INTERRUPT();  
  event = push_event(delayms,NULL,1);
  OPEN_INTERRUPT();
  while(mark_list[event->id] == 0);
}		/* -----  end of function stim_delay  ----- */




/*
 * ===  FUNCTION  ======================================================================
 *         Name:  stim_runlater
 *  Description:  run callback function after some milliseconds by asynchronous.
 * =====================================================================================
 */
int8_t stim_runlater ( uint16_t delayms, void (*callback)(void))
{
  struct stim_event *event;

  CLOSE_INTERRUPT(); 
  event = push_event(delayms,callback,1);
  OPEN_INTERRUPT(); 
  return event->id;
}		/* -----  end of function stim_runlater  ----- */
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  stim_loop
 *  Description:  looping invok each interval by asynchronous.
 * =====================================================================================
 */
int8_t stim_loop ( uint16_t delayms, void (*callback)(void), uint16_t times)
{
  struct stim_event *event;

  CLOSE_INTERRUPT();  
  event = push_event(delayms,callback,times);
  OPEN_INTERRUPT(); 
  return event->id;
}		/* -----  end of function stim_loop  ----- */



/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  stim_kill_event
 *  Description:  
 * =====================================================================================
 */
void stim_kill_event(int8_t id)
{
  uint8_t i;
  struct stim_event_list *list;
  struct stim_event *event;

  CLOSE_INTERRUPT();  
  for(i = 0; i < 2; i++){
    list = &list_manager.list[i];
    event = find_event(id,list);
    if(event != NULL){
      remove_node(event,list);
      free_event(event);
      break;
    }
  }
  OPEN_INTERRUPT(); 
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  stim_remove_event
 *  Description:  
 * =====================================================================================
 */
void stim_remove_event(int8_t id)
{
  uint8_t i;
  struct stim_event_list *list;
  struct stim_event *event;

  CLOSE_INTERRUPT();  
  for(i = 0; i < 2; i++){
    list = &list_manager.list[i];
    event = find_event(id,list);
    if(event != NULL){
      event->stat = STIM_EVENT_RECYCLE;
      recyle_event(event);
      break;
    }
  }
  OPEN_INTERRUPT(); 
}



/*
 * ===  FUNCTION  ======================================================================
 *         Name:  stim_tick
 *  Description:  engine of smarttimer.
 *                It must be execute in systick( or hardware timer) interrupt
 * =====================================================================================
 */
void stim_tick (void)
{
  struct stim_event *event;
  struct stim_event_list *list;

  if(((current_tick + 1) & 0xffffffff) < current_tick){
    list_manager.cur_index ^= 0x01;
  }
  current_tick++;

  list = &list_manager.list[list_manager.cur_index];
  event = list->head;

  while(event != NULL && event->tick_punch <= current_tick){
    mark_list[event->id] += 1;



    if((event->looptimes != STIM_LOOP_FOREVER) && 
        (--event->looptimes == 0)){
      event->stat = STIM_EVENT_RECYCLE;
      recyle_event(event);
    }else{
      event->tick_punch = current_tick + event->interval;
      remove_node(event,list);
      if(event->tick_punch < current_tick){
        list = &list_manager.list[list_manager.cur_index ^ 0x01];
      }
      insert_event(event,list);
    }
    event = event->next;
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
  struct stim_event *event;

  for(i = 0; i < STIM_EVENT_MAX_SIZE; i++){
    if((mark_list[i] != STIM_INVALID) && (mark_list[i] > 0)){
      if(callback_list[i] != NULL){
        callback_list[i]();
      }
      mark_list[i] -= 1;
    }
  }

  if(recycle_list.count > 0){
    event = recycle_list.head;
    while(event != NULL){
      if(mark_list[event->id] == 0){
        remove_node(event,&recycle_list);
        free_event(event);
        break;
      }else{
        event = event->next;
      }
    }
  }

}		/* -----  end of function stim_loop  ----- */




/*
 * ===  FUNCTION  ======================================================================
 *         Name:  stim_init
 *  Description:  smarttimer init program.  Init event linked list,and setup system clock.
 * =====================================================================================
 */
void stim_init ( void )
{
  uint8_t i;
  struct stim_event *event;

  init_linked(&recycle_list);
  init_linked(&list_manager.list[0]);
  init_linked(&list_manager.list[1]);
  list_manager.cur_index = 0;

  current_tick = 0;

  for(i = 0; i < STIM_EVENT_MAX_SIZE; i++){
    event = &event_pool[i];
    event->stat = STIM_EVENT_IDLE;
    event->interval = 0;
    event->tick_punch = 0;
    event->looptimes = 0;
    event->next = NULL;
    event->prev = NULL;
    event->id = i;

    callback_list[i] = NULL;
    mark_list[i] = STIM_INVALID;
  }

  SysTick_Config(SystemCoreClock / 1000);     //tick is 1ms

}		/* -----  end of function stim_init  ----- */

#ifdef STIM_DEBUG
uint8_t stim_get_eventnum(void){
  return list_manager.list[0].count + list_manager.list[1].count;  
}

void stim_print_status(void)
{
  uint8_t i;
  struct stim_event *node;
  struct stim_event_list *list;

  printf("=============================\r\n");
  printf("current_tick = %X\r\n",current_tick);
  printf("cur_index = %d\r\n",list_manager.cur_index);
  list = &list_manager.list[list_manager.cur_index];
  if(list->count == 0){
    printf("list is empty!\r\n");
  }else{
    node = list->head;
    for(i = 0; i <list->count; i++){
      printf("event tick_punch = %X\r\n",node->tick_punch);
      printf("event id = %d\r\n",node->id);
      node = node->next;
    }
  }
  printf("========another list===========\r\n");
  list = &list_manager.list[list_manager.cur_index ^ 0x01];
  if(list->count == 0){
    printf("list is empty!\r\n");
  }else{
    node = list->head;
    for(i = 0; i <list->count; i++){
      printf("event tick_punch = %X\r\n",node->tick_punch);
      printf("event id = %d\r\n",node->id);
      node = node->next;
    }
  }
  printf("========recycle list===========\r\n");
  list = &recycle_list;
  if(list->count == 0){
    printf("list is empty!\r\n");
  }else{
    node = list->head;
    for(i = 0; i <list->count; i++){
      printf("mark_list[%d] = %d\r\n",node->id,mark_list[node->id]);
      node = node->next;
    }
  }
  printf("=============================\r\n");
}

#endif
