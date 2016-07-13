/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016/4/8 ÐÇÆÚÎå ÏÂÎç 2:29:50
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  lell (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stm32f10x.h>
#include <stm32f10x_conf.h>
#include <stdio.h>
#include <stdlib.h>
#include "board.h"
#include "smarttimer.h"



/* Private function prototypes -----------------------------------------------*/

#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */



struct date{
  uint16_t year;
  uint8_t  month;
  uint8_t  day;
  uint8_t hour;
  uint8_t minute;
	uint8_t second;
};




struct date m_date;

const char datestr[] = "2016:06:23#17:16:00";



static void sys_ledflash(void);
static void simulation_rtc(void);
/*
 * ===  FUNCTION  ======================================================================
 *         Name:  bluetooth_init
 *  Description:
 * =====================================================================================
 */
static void uart_init (void)
{


  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
	
  USART_DeInit(USART1);
	
	
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
      RCC_APB2Periph_AFIO | RCC_APB2Periph_USART1 ,ENABLE);


  /*USART1 tx config*/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /*USART1 rx config*/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	

  USART_InitStructure.USART_BaudRate= 9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx |USART_Mode_Tx;
  USART_Init(USART1, &USART_InitStructure);
  

  /* Enable the USARTy Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
  USART_Cmd(USART1,ENABLE);
	
}		/* -----  end of function bluetooth_init  ----- */


static void sys_init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
 
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

  GPIO_InitStructure.GPIO_Pin = SYS_LEDCTRL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(SYS_LEDPORT, &GPIO_InitStructure);

	stim_loop(50,sys_ledflash,STIM_LOOP_FOREVER);
	
}


static void set_timetick ( void)
{
    //xxxx:xx:xx#xx:xx:xx

	  m_date.year = atoi(datestr);
	  m_date.month = atoi(datestr + 5);
	  m_date.day = atoi(datestr + 8);
	  m_date.hour = atoi(datestr + 11);
	  m_date.minute = atoi(datestr + 14);
	  m_date.second = atoi(datestr + 17);
    
	  stim_loop(100,simulation_rtc,STIM_LOOP_FOREVER);
    
}		/* -----  end of static function time_tick  ----- */




static uint8_t isLeap(uint16_t year)
{
  if(year % 4 == 0 && 
      year %100 != 0 &&
      year % 400 == 0){
    return 1;
  }else{
    return 0;
  }
}


static void increaseday(uint8_t limit)
{
  if(++m_date.day > limit){
    m_date.day = 1;
    if(++m_date.month > 12){
      m_date.month = 1;
      m_date.year++;
    }
  }
}

static void changeday ( void )
{
  switch(m_date.month){
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
      increaseday(31);
      break;
    case 4:
    case 6:
    case 9:
    case 11:
      increaseday(30);
      break;
    case 2:
      if(isLeap(m_date.year) == 1){
        increaseday(29);
      }else{
        increaseday(28);
      }
			break;
  }

  
}


static void simulation_rtc(void)
{

  if(++m_date.second == 60){
    m_date.second = 0;
    if(++m_date.minute == 60){
      m_date.minute = 0;
      if(++m_date.hour == 24){
        m_date.hour = 0;
        changeday();
      }
    }
  }
	printf("time===>[%02d:%02d:%02d]\r\n",m_date.hour,m_date.minute,m_date.second);
}

static void sys_ledflash(void)
{
	static uint8_t flag = 0;
	
	if(flag == 0){
		GPIO_ResetBits(SYS_LEDPORT,SYS_LEDCTRL);
		printf("led off\r\n");
	}else{
		GPIO_SetBits(SYS_LEDPORT,SYS_LEDCTRL);
		printf("led on\r\n");
	}
	
	flag = !flag;

}


static void runlater_test(void)
{

   printf("after runlater===>[%02d:%02d:%02d]\r\n",m_date.hour,m_date.minute,m_date.second);
//	 printf("before delay===>[%02d:%02d:%02d]\r\n",m_date.hour,m_date.minute,m_date.second);
//	 stim_delay(1000);
//	 printf("after delay===>[%02d:%02d:%02d]\r\n",m_date.hour,m_date.minute,m_date.second);
}



/*
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:
 * =====================================================================================
 */
int main (void)
{
  //NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

  uint8_t i = 0;
  uart_init();
	stim_init();
  sys_init();

  set_timetick();

	printf("before runlater===>[%02d:%02d:%02d]\r\n",m_date.hour,m_date.minute,m_date.second);
	stim_runlater(1000,runlater_test);
  i ^= 0x01;
	printf("i = %X\r\n",i);
	i^=0x01;
		printf("i = %X\r\n",i);
	i^=0x01;
		printf("i = %X\r\n",i);
	i^=0x01;
		printf("i = %X\r\n",i);
  while(1){
    stim_mainloop();
  };

}		/* -----  end of function main  ----- */



PUTCHAR_PROTOTYPE
{
  //return show_char(ch);
  USART_SendData(USART1,ch);
  while(USART_GetFlagStatus(USART1,USART_FLAG_TXE) == RESET);
  return ch;  

}

#ifdef  USE_FULL_ASSERT
/*******************************************************************************
 * Function Name  : assert_failed
 * Description    : Reports the name of the source file and the source line number
 *                  where the assert_param error has occurred.
 * Input          : - file: pointer to the source file name
 *                  - line: assert_param error line source number
 * Output         : None
 * Return         : None
 *******************************************************************************/
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */

  print("Wrong parameters value: file %s on line %d\r\n", file, line);
  while (1)
  {}
}
#endif

