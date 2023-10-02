/* C.Parascan
 *  module version v1.0.4 - reviewed and tested on 05.06.2023
 *  module version v1.0.5 - adding watchdog supervision on 20.09.2023
 */

/* Simple time triggered Scheduler 
 *    - all tasks are non-preemptive
 *    - tasks have static priority
 *    - tasks don't have own stack
 */
 
#ifndef __OS_SCHEDULER_H__
#define __OS_SCHEDULER_H__


#include <Arduino.h>

/*
 * Static module version
 */
#define _MAIN_SCHED_VERSION_MAJOR_ (1)
#define _MAIN_SCHED_VERSION_MINOR_ (0)
#define _MAIN_SCHED_VERSION_PATCH_ (4)


/*  0 - disable
 *  1 - display log level - medium
 *  2 - display log level - full
 */
#define _OS_SCHED_DEBUG_ENABLE_ (0)


/*
 * Simple task reccurent function format ...
 */
typedef void (*Task_func_ptr_Type)(void);


/*
 * TCB used by the OS ...
 */
typedef struct
{
	Task_func_ptr_Type ptrTask;

	Task_func_ptr_Type ptrInitFcn;
	
	short Task_MaxItter;

	short Task_CurrentItter;
}T_TaskControlBlock;


/* OS exported API's */
void OS_init(void);
void OS_main(void);

#endif /* __OS_SCHEDULER_H__ */