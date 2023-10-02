#include "00_Scheduler.h"
#include "00_Scheduler_cfg.h"

/*
 * 
 */

void setup() 
{

  OS_init();

}

void loop() 
{
  // put your main code here, to run repeatedly:

  /* !!! non return function !!! */
  OS_main();

}
