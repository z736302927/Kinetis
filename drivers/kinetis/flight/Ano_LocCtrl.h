/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __WXY_CTRL_H
#define __WXY_CTRL_H
/* Includes ------------------------------------------------------------------*/
#include "Ano_FcData.h"
#include "Ano_Filter.h"
#include "Ano_Math.h"
#include "Ano_Pid.h"
/* Exported types ------------------------------------------------------------*/

typedef struct {
    float exp[VEC_XYZ];
    float fb[VEC_XYZ];


    float out[VEC_XYZ];
} _loc_ctrl_st; // loc_ctrl;
extern _loc_ctrl_st loc_ctrl_1;
extern _loc_ctrl_st loc_ctrl_2;
/* Exported constants --------------------------------------------------------*/

extern struct pid_coefficient position_coe[] ;
extern struct pid_op_result position_res[] ;
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

void position_pid_init(void);
void Loc_1level_Ctrl(u16 dT_ms, s16 *CH_N);
#endif
