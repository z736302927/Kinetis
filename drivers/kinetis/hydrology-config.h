#ifndef __HYDROLOGY_CONFIG_H
#define __HYDROLOGY_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/

#define HYDROLOGY_H_PORT_TIMEOUT        10
#define HYDROLOGY_D_PORT_TIMEOUT        10
#define HYDROLOGY_BODY_MAX_LEN          128
#define HYDROLOGY_FF_USE_LFN            0

#if HYDROLOGY_FF_USE_LFN
#define HYDROLOGY_FILE_PATH             "0:/Hydrology/"

#define HYDROLOGY_D_FILE_E_DATA         "hydrology_device_element_Data.txt"
#define HYDROLOGY_D_FILE_E_INFO         "hydrology_device_element_Info.txt"
#define HYDROLOGY_D_FILE_PICTURE        "hydrology_device_PIC.jpg"
#define HYDROLOGY_D_FILE_RGZS           "hydrology_device_RGZS.txt"
#define HYDROLOGY_D_PIC_REVSPACE        4096
#define HYDROLOGY_D_RGZS_REVSPACE       512

#define HYDROLOGY_H_FILE_E_DATA         "hydrology_host_element_Data.txt"
#define HYDROLOGY_H_FILE_E_INFO         "hydrology_host_element_Info.txt"

#else

/* The file name length can't exceed 8 */
#define HYDROLOGY_FILE_PATH             "0:/Hydro/"

#define HYDROLOGY_D_FILE_E_DATA         "HDED.txt"
#define HYDROLOGY_D_FILE_E_INFO         "HDEI.txt"
#define HYDROLOGY_D_FILE_PICTURE        "HDP.jpg"
#define HYDROLOGY_D_FILE_RGZS           "HDR.txt"
#define HYDROLOGY_D_PIC_REVSPACE        4096
#define HYDROLOGY_D_RGZS_REVSPACE       512

#define HYDROLOGY_H_FILE_E_DATA         "HHED.txt"
#define HYDROLOGY_H_FILE_E_INFO         "HHEI.txt"

#endif /* HYDROLOGY_FF_USE_LFN */

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef __cplusplus
}
#endif

#endif /* __HYDROLOGY_CONFIG_H */
