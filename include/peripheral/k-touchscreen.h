#ifndef __K_TOUCHSCREEN_H
#define __K_TOUCHSCREEN_H

#ifdef __cplusplus
extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/* Includes ------------------------------------------------------------------*/
#include "core/core_common.h"

typedef struct {
    uint16_t TouchDetected;
    uint16_t X;
    uint16_t Y;
    uint16_t Z;
} TS_StateTypeDef;

typedef struct {
    void (*Init)(uint16_t);
    uint16_t (*ReadID)(uint16_t);
    void (*Reset)(uint16_t);
    void (*Start)(uint16_t);
    uint8_t (*DetectTouch)(uint16_t);
    void (*GetXY)(uint16_t, uint16_t *, uint16_t *);
    void (*EnableIT)(uint16_t);
    void (*ClearIT)(uint16_t);
    uint8_t (*GetITStatus)(uint16_t);
    void (*DisableIT)(uint16_t);
} TS_DrvTypeDef;

typedef enum {
    TS_OK       = 0x00,
    TS_ERROR    = 0x01,
    TS_TIMEOUT  = 0x02
} TS_StatusTypeDef;

uint8_t K_TS_Init(uint16_t XSize, uint16_t YSize);
void    K_TS_GetState(TS_StateTypeDef *TsState);
uint8_t K_TS_ITConfig(void);
uint8_t K_TS_ITGetStatus(void);
void    K_TS_ITClear(void);

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/


#ifdef __cplusplus
}
#endif

#endif /* __K_TOUCHSCREEN_H */
