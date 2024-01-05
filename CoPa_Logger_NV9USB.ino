#include "00_Scheduler.h"
#include "00_Scheduler_cfg.h"

/*
 * Startup Arduino - standard function call 
 */

void setup() 
{

  OS_init();

}

void loop() 
{
  /* Arduino standard call - main code here, to run repeatedly */

  /* !!! non return function !!! */
  OS_main();

}
