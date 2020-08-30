#ifndef __K_CHINESE_H
#define __K_CHINESE_H

#ifdef __cplusplus
extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/* Includes ------------------------------------------------------------------*/
#include "core_common.h"
#include "GUI_Private.h"

void CNFontInit(void);
void GUIPROP_X_DispChar(U16 c);
int GUIPROP_X_GetCharDistX(U16 c);

extern const GUI_FONT GUI_FontCN12;
extern const GUI_FONT GUI_FontCN16;
extern const GUI_FONT GUI_FontCN24;
extern const GUI_FONT GUI_FontCN32;

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/


#ifdef __cplusplus
}
#endif

#endif /* __K_CHINESE_H */
