/*
 *  C.Parascan
 *
 *    Debugging utility module
 *
 */

#include "01_Debug_Utils.h"
#include "99_Automat_Board_cfg.h"

/*
 * Configure Serial 0 - for 9600 baud rate - debugging messages communication 
 */
void Debug_print_init(void)
{
    Serial.begin(115200);

    Serial.println();
    Serial.println();
    Serial.println("**************************");
    Serial.println("* Logger Module          *");
    Serial.print  ("* SW v");
    Serial.print  (LOG_APP_VERSION_MAJOR);
    Serial.print  (".");
    Serial.print  (LOG_APP_VERSION_MINOR);
    Serial.print  (".");
    Serial.print(LOG_APP_VERSION_PATCH);
    Serial.println("              *");
    Serial.println("**************************");

}