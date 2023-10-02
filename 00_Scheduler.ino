/*
 * author: C.Parascan 
 * version 1.0.3
 *
 * comment: basic time triggered scheduler implementation
 *          Uses TIMER 4 - for the ISR OVF !!
 *
 * history: 
 *   1.0.2 - porting from previous version implemented on raspberry PI
 *   1.0.3 - reviewed and tested on 20.02.2023
 *   1.0.4 - 05.06.2023 - updated with 1ms re-calculation
 */

#include "00_Scheduler.h"
#include "00_Scheduler_cfg.h"

#include <avr/wdt.h>


/* used by interrupt as a flag to notify a timeout */
static volatile bool OS_Timer_fired = false;

/* used to control the tasks execution ...         */
volatile T_TaskControlBlock *TCB_OS_idx;
extern   T_TaskControlBlock TCB_OS[MAX_TASK_LIST_LEN];


/*
 *  - Hardware timer used to generate the events that are
 *      driving the OS Scheduler
 *
 *  - timer used is TMR4
 *  - timer ISR reccurence is 1ms
 */
ISR(TIMER4_OVF_vect)
{
  /* in order to cout to 1ms --> 16 timer itterations                   */
  TCNT4  = 0xFFF0;

  /* reset of OVF flags - automatically done when ISR is served  */
  /* TIFR4 -->  – – ICF4 – OCF4C OCF4B OCF4A TOV4        */
  /* TIFR4 |= (1 << ICF4) | (1 << OCF4C) | (1 << OCF4B) | (1 << OCF4A) | (1 << TOV4); */
  
  OS_Timer_fired = true;
 
}


/*
 * - First function that must be executed in order to have an workin OS !
 *      - initializes HW timer
 *
 *      - executes Init functions of the OS Tasks ! (this is done before OS HW timer is configured)
 *                 so no interrupts are fired during long initialization phase !
 */
void OS_init(void)
{

  uint8_t nIdx;

  /* 
   * Disable WDT ... during OS startup ... 
   */
  wdt_disable();


	/*
	 * initialize the TCB with configured values ... 
	 */

	TCB_OS_idx = TCB_OS;


  /****************************************************/
  /*
  /*  Execute all other user specific initializations ... 
  /*
  /****************************************************/
	for (nIdx = 0; nIdx < MAX_TASK_LIST_LEN; nIdx ++)
	{
		if( TCB_OS_idx[nIdx].ptrInitFcn != NULL )
	 		TCB_OS_idx[nIdx].ptrInitFcn();               /* !!!!  Execute all INIT functions !!!!  */
	}


  OS_Timer_fired = false;  

  /****************************************************/
  /*
  /*           TIMER initialization
  /*
  /****************************************************/

  /* disable interrupts ... */
  cli();

  /* TCCR4A -> COM4A1 COM4A0 COM4B1 COM4B0 COM4C1 COM4C0 WGM41 WGM40  */
  TCCR4A = 0;

  /* TCCR4B -> ICNC4 ICES4 – WGM43 WGM42 CS42 CS41 CS40               */
  TCCR4B = 0;

  /* WGMn0 ... WGMn3 == 0 --> normal counter increment --> 0 ... 0xFFFF        */
  /* Set CS12 and CS10 bits for 1024 prescaler                                 */
  /* 16 Mhz / 1024 -->  0.000,000,063 sec (63 ns) * 1024 = 0.000,064 (64 us)   */
  TCCR4B |= (1 << CS42) | (1 << CS40);

  /* TCCR4C -> FOC4A FOC4B FOC4C – – – – –                             */
  TCCR4C = 0;

  /* 0.000,064 (64 us) * 16 = 0.001,024                                 */
  /* in order to cout to 1ms --> 16 timer itterations                   */
  TCNT4  = 0xFFF0;    /* 0xFFFF - 0x10 + 1= 0xFFF0                      */


  /* TIMSK4 ->   – – ICIE4 – OCIE4C OCIE4B OCIE4A TOIE4  */
  TIMSK4 = 0;
  /* enable timer 4 overflow interrupt ... */
  TIMSK4 |= (1 << TOIE4);

  /* TIFR4 -->  – – ICF4 – OCF4C OCF4B OCF4A TOV4        */
  TIFR4 |= (1 << ICF4) | (1 << OCF4C) | (1 << OCF4B) | (1 << OCF4A) | (1 << TOV4);

  /* enable interrupts */
  sei();


  /*
   * Start WDT monitoring ... 
   */
  wdt_enable(WDTO_4S);

}/* end OS_init */

/*
 * OS non-exit function 
 *  - executes user defined tasks according to configured itterations !
 */
void OS_main(void)
{
  uint8_t nIdx;

	/* the famous whie 1 :) ... */
	while( 1 )
	{

		if( OS_Timer_fired == true )
		{

      /* consume one HW TIMER event ... */
			OS_Timer_fired = false;


			/* check to see what function call we should execute first                                     */

			/* priority is given by the order in which we check ... from index 0 to MAX_TASK_LIST_LEN ...  */
			/* highest prio is on index "0"                                                                */

			for (nIdx = 0; nIdx < MAX_TASK_LIST_LEN; nIdx ++)
			{
				/* if delay has expired execute associted task function ...
				 */
				if( TCB_OS_idx[nIdx].Task_CurrentItter > 0 )
				{

					/* wait to get to 0 ... */
					TCB_OS_idx[nIdx].Task_CurrentItter --;

				}
        else        
				/* have we already reached 0 ? */
				//if( TCB_OS_idx[nIdx].Task_CurrentItter <= 0 )
				{

					if( TCB_OS_idx[nIdx].ptrTask != NULL )
						TCB_OS_idx[nIdx].ptrTask();          /* !!!! Execute TASK !!!!  */

					/* reset counter ... */
					/*
					 * 
					 */
					TCB_OS_idx[nIdx].Task_CurrentItter = TCB_OS_idx[nIdx].Task_MaxItter;
				}
				
			}

		}/* if( OS_Timer_fired ) */
		
	}/* while (1) */

}/* end OS_main */

