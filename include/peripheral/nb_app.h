#ifndef __NB_APP_H
#define __NB_APP_H

#ifdef __cplusplus
extern "C"
{
#endif
   
/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/* Includes ------------------------------------------------------------------*/

#include "nb_board.h"


void NB_IOT_UART_Receive(void);
void NB_IOT_Init(void);
void NB_IOT_Turnoff_Pipe(uint8_t onoff);
void NB_IOT_SendData(char* pdata, int len);

extern NB_ConfigTypeDef  nb_config;

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

#ifdef __cplusplus
}
#endif

#endif   /* __NB_APP_H */
