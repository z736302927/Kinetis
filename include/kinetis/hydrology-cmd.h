#ifndef  _HYDROLOGYCOMMAND_H_
#define  _HYDROLOGYCOMMAND_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "kinetis/hydrology.h"

int Hydrology_ExecuteCommand(HydrologyBodyType Funcode);
int Hydrology_ResponseDownstream(HydrologyBodyType Funcode);
int Hydrology_ResponseUpstream(HydrologyBodyType Funcode, u8 End);
int HydrologyD_Reset(void);
int HydrologyH_Reset(void);

#ifdef __cplusplus
}
#endif

#endif /* _HYDROLOGYCOMMAND_H_ */
