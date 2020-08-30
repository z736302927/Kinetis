#ifndef __HYDROLOGY_CONFIG_H
#define __HYDROLOGY_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/* Includes ------------------------------------------------------------------*/
#include "core_common.h"

//#define HYDROLOGY_FILE_PATH             "0:/Hydrology_Data/"
//#define HYDROLOGY_D_FILE_E_DATA          "HydrologyElement.txt"
//#define HYDROLOGY_D_FILE_PICTURE          "HydrologyPicture.jpg"

#define HYDROLOGY_H_PORT_TIMEOUT        10
#define HYDROLOGY_D_PORT_TIMEOUT        10
#define HYDROLOGY_BODY_MAX_LEN          128

#define HYDROLOGY_FILE_PATH             "0:/Hydrology/"

#define HYDROLOGY_D_FILE_E_DATA         "HydrologyD_Element_Data.txt"
#define HYDROLOGY_D_FILE_E_INFO         "HydrologyD_Element_Info.txt"
#define HYDROLOGY_D_FILE_PICTURE        "HydrologyD_PIC.jpg"
#define HYDROLOGY_D_FILE_RGZS           "HydrologyD_RGZS.txt"
#define HYDROLOGY_D_PIC_REVSPACE        4096
#define HYDROLOGY_D_RGZS_REVSPACE       512

#define HYDROLOGY_H_FILE_E_DATA         "HydrologyH_Element_Data.txt"
#define HYDROLOGY_H_FILE_E_INFO         "HydrologyH_Element_Info.txt"


/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/


#ifdef __cplusplus
}
#endif

#endif /* __HYDROLOGY_CONFIG_H */

