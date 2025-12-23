#ifndef __HYDROLOGY_IDENTIFIER_H
#define __HYDROLOGY_IDENTIFIER_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#define HYDROLOGY_EA_TT_LEN             5

#define HYDROLOGY_EA_TT                 0
#define HYDROLOGY_EA_ST                 HYDROLOGY_EA_TT + 5 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_RGZS               HYDROLOGY_EA_ST + 5 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_PIC                HYDROLOGY_EA_RGZS + 0
#define HYDROLOGY_EA_DRP                HYDROLOGY_EA_PIC + 0 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_DRZ1               HYDROLOGY_EA_DRP + 12 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_DRZ2               HYDROLOGY_EA_DRZ1 + 24 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_DRZ3               HYDROLOGY_EA_DRZ2 + 24 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_DRZ4               HYDROLOGY_EA_DRZ3 + 24 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_DRZ5               HYDROLOGY_EA_DRZ4 + 24 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_DRZ6               HYDROLOGY_EA_DRZ5 + 24 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_DRZ7               HYDROLOGY_EA_DRZ6 + 24 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_DRZ8               HYDROLOGY_EA_DRZ7 + 24 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_DATA               HYDROLOGY_EA_DRZ8 + 24 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_AC                 HYDROLOGY_EA_DATA + 0
#define HYDROLOGY_EA_AI                 HYDROLOGY_EA_AC + 4 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_C                  HYDROLOGY_EA_AI + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_DRxnn              HYDROLOGY_EA_C + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_DT                 HYDROLOGY_EA_DRxnn + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_ED                 HYDROLOGY_EA_DT + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_EJ                 HYDROLOGY_EA_ED + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_FL                 HYDROLOGY_EA_EJ + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_GH                 HYDROLOGY_EA_FL + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_GN                 HYDROLOGY_EA_GH + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_GS                 HYDROLOGY_EA_GN + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_GT                 HYDROLOGY_EA_GS + 1 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_GTP                HYDROLOGY_EA_GT + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_H                  HYDROLOGY_EA_GTP + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_HW                 HYDROLOGY_EA_H + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_M10                HYDROLOGY_EA_HW + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_M20                HYDROLOGY_EA_M10 + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_M30                HYDROLOGY_EA_M20 + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_M40                HYDROLOGY_EA_M30 + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_M50                HYDROLOGY_EA_M40 + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_M60                HYDROLOGY_EA_M50 + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_M80                HYDROLOGY_EA_M60 + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_M100               HYDROLOGY_EA_M80 + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_MST                HYDROLOGY_EA_M100 + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_NS                 HYDROLOGY_EA_MST + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_P1                 HYDROLOGY_EA_NS + 1 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_P2                 HYDROLOGY_EA_P1 + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_P3                 HYDROLOGY_EA_P2 + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_P6                 HYDROLOGY_EA_P3 + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_P12                HYDROLOGY_EA_P6 + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_PD                 HYDROLOGY_EA_P12 + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_PJ                 HYDROLOGY_EA_PD + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_PN01               HYDROLOGY_EA_PJ + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_PN05               HYDROLOGY_EA_PN01 + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_PN10               HYDROLOGY_EA_PN05 + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_PN30               HYDROLOGY_EA_PN10 + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_PR                 HYDROLOGY_EA_PN30 + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_PT                 HYDROLOGY_EA_PR + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_Q                  HYDROLOGY_EA_PT + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_Q1                 HYDROLOGY_EA_Q + 5 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_Q2                 HYDROLOGY_EA_Q1 + 5 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_Q3                 HYDROLOGY_EA_Q2 + 5 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_Q4                 HYDROLOGY_EA_Q3 + 5 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_Q5                 HYDROLOGY_EA_Q4 + 5 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_Q6                 HYDROLOGY_EA_Q5 + 5 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_Q7                 HYDROLOGY_EA_Q6 + 5 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_Q8                 HYDROLOGY_EA_Q7 + 5 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_QA                 HYDROLOGY_EA_Q8 + 5 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_QZ                 HYDROLOGY_EA_QA + 5 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_SW                 HYDROLOGY_EA_QZ + 5 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_UC                 HYDROLOGY_EA_SW + 6 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_UE                 HYDROLOGY_EA_UC + 1 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_US                 HYDROLOGY_EA_UE + 1 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_VA                 HYDROLOGY_EA_US + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_VJ                 HYDROLOGY_EA_VA + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_VT                 HYDROLOGY_EA_VJ + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_Z                  HYDROLOGY_EA_VT + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_ZB                 HYDROLOGY_EA_Z + 4 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_ZU                 HYDROLOGY_EA_ZB + 4 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_Z1                 HYDROLOGY_EA_ZU + 4 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_Z2                 HYDROLOGY_EA_Z1 + 4 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_Z3                 HYDROLOGY_EA_Z2 + 4 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_Z4                 HYDROLOGY_EA_Z3 + 4 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_Z5                 HYDROLOGY_EA_Z4 + 4 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_Z6                 HYDROLOGY_EA_Z5 + 4 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_Z7                 HYDROLOGY_EA_Z6 + 4 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_Z8                 HYDROLOGY_EA_Z7 + 4 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_SQ                 HYDROLOGY_EA_Z8 + 4 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_ZT                 HYDROLOGY_EA_SQ + 5 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_pH                 HYDROLOGY_EA_ZT + 4 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_DO                 HYDROLOGY_EA_pH + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_COND               HYDROLOGY_EA_DO + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_TURB               HYDROLOGY_EA_COND + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_CODMN              HYDROLOGY_EA_TURB + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_REDOX              HYDROLOGY_EA_CODMN + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_NH4N               HYDROLOGY_EA_REDOX + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_TP                 HYDROLOGY_EA_NH4N + 3  + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_TN                 HYDROLOGY_EA_TP + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_TOC                HYDROLOGY_EA_TN + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_CU                 HYDROLOGY_EA_TOC + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_ZN                 HYDROLOGY_EA_CU + 4 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_SE                 HYDROLOGY_EA_ZN + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_AS                 HYDROLOGY_EA_SE + 4 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_THG                HYDROLOGY_EA_AS + 4 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_CD                 HYDROLOGY_EA_THG + 4 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_PB                 HYDROLOGY_EA_CD + 4 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_CHLA               HYDROLOGY_EA_PB + 4 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_WP1                HYDROLOGY_EA_CHLA + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_WP2                HYDROLOGY_EA_WP1 + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_WP3                HYDROLOGY_EA_WP2 + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_WP4                HYDROLOGY_EA_WP3 + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_WP5                HYDROLOGY_EA_WP4 + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_WP6                HYDROLOGY_EA_WP5 + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_WP7                HYDROLOGY_EA_WP6 + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_WP8                HYDROLOGY_EA_WP7 + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_SYL1               HYDROLOGY_EA_WP8 + 3 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_SYL2               HYDROLOGY_EA_SYL1 + 6 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_SYL3               HYDROLOGY_EA_SYL2 + 6 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_SYL4               HYDROLOGY_EA_SYL3 + 6 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_SYL5               HYDROLOGY_EA_SYL4 + 6 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_SYL6               HYDROLOGY_EA_SYL5 + 6 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_SYL7               HYDROLOGY_EA_SYL6 + 6 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_SYL8               HYDROLOGY_EA_SYL7 + 6 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_SBL1               HYDROLOGY_EA_SYL8 + 6 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_SBL2               HYDROLOGY_EA_SBL1 + 5 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_SBL3               HYDROLOGY_EA_SBL2 + 5 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_SBL4               HYDROLOGY_EA_SBL3 + 5 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_SBL5               HYDROLOGY_EA_SBL4 + 5 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_SBL6               HYDROLOGY_EA_SBL5 + 5 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_SBL7               HYDROLOGY_EA_SBL6 + 5 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_SBL8               HYDROLOGY_EA_SBL7 + 5 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_VTA                HYDROLOGY_EA_SBL8 + 5 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_VTB                HYDROLOGY_EA_VTA + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_VTC                HYDROLOGY_EA_VTB + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_VIA                HYDROLOGY_EA_VTC + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_VIB                HYDROLOGY_EA_VIA + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_EA_VIC                HYDROLOGY_EA_VIB + 2 + HYDROLOGY_EA_TT_LEN

#define HYDROLOGY_BA_CENTER             HYDROLOGY_EA_VIC + 2 + HYDROLOGY_EA_TT_LEN
#define HYDROLOGY_BA_REMOTE             HYDROLOGY_BA_CENTER + 4
#define HYDROLOGY_BA_PASSWORD           HYDROLOGY_BA_REMOTE + 5
#define HYDROLOGY_BAL_CENTER1_IP        HYDROLOGY_BA_PASSWORD + 2
#define HYDROLOGY_BA_CENTER1_IP         HYDROLOGY_BAL_CENTER1_IP + 1
#define HYDROLOGY_BAL_BACKUP1_IP        HYDROLOGY_BA_CENTER1_IP + 10
#define HYDROLOGY_BA_BACKUP1_IP         HYDROLOGY_BAL_BACKUP1_IP + 1
#define HYDROLOGY_BAL_CENTER2_IP        HYDROLOGY_BA_BACKUP1_IP + 10
#define HYDROLOGY_BA_CENTER2_IP         HYDROLOGY_BAL_CENTER2_IP + 1
#define HYDROLOGY_BAL_BACKUP2_IP        HYDROLOGY_BA_CENTER2_IP + 10
#define HYDROLOGY_BA_BACKUP2_IP         HYDROLOGY_BAL_BACKUP2_IP + 1
#define HYDROLOGY_BAL_CENTER3_IP        HYDROLOGY_BA_BACKUP2_IP + 10
#define HYDROLOGY_BA_CENTER3_IP         HYDROLOGY_BAL_CENTER3_IP + 1
#define HYDROLOGY_BAL_BACKUP3_IP        HYDROLOGY_BA_CENTER3_IP + 10
#define HYDROLOGY_BA_BACKUP3_IP         HYDROLOGY_BAL_BACKUP3_IP + 1
#define HYDROLOGY_BAL_CENTER4_IP        HYDROLOGY_BA_BACKUP3_IP + 10
#define HYDROLOGY_BA_CENTER4_IP         HYDROLOGY_BAL_CENTER4_IP + 1
#define HYDROLOGY_BAL_BACKUP4_IP        HYDROLOGY_BA_CENTER4_IP + 10
#define HYDROLOGY_BA_BACKUP4_IP         HYDROLOGY_BAL_BACKUP4_IP + 1
#define HYDROLOGY_BA_WORK_MODE          HYDROLOGY_BA_BACKUP4_IP + 10
#define HYDROLOGY_BA_ELEMENT_SELECT     HYDROLOGY_BA_WORK_MODE + 2
#define HYDROLOGY_BA_REPEATER_STATION   HYDROLOGY_BA_ELEMENT_SELECT + 8
#define HYDROLOGY_BAL_DEVICE_ID         HYDROLOGY_BA_REPEATER_STATION + 12
#define HYDROLOGY_BA_DEVICE_ID          HYDROLOGY_BAL_DEVICE_ID + 1

#define HYDROLOGY_PA_TI                 HYDROLOGY_BA_DEVICE_ID + 12
#define HYDROLOGY_PA_AI                 HYDROLOGY_PA_TI + 1
#define HYDROLOGY_PA_RBT                HYDROLOGY_PA_AI + 1
#define HYDROLOGY_PA_SI                 HYDROLOGY_PA_RBT + 1
#define HYDROLOGY_PA_WSI                HYDROLOGY_PA_SI + 2
#define HYDROLOGY_PA_RR                 HYDROLOGY_PA_WSI + 1
#define HYDROLOGY_PA_WR                 HYDROLOGY_PA_RR + 1
#define HYDROLOGY_PA_RAT                HYDROLOGY_PA_WR + 1
#define HYDROLOGY_PA_WB1                HYDROLOGY_PA_RAT + 1
#define HYDROLOGY_PA_WB2                HYDROLOGY_PA_WB1 + 4
#define HYDROLOGY_PA_WB3                HYDROLOGY_PA_WB2 + 4
#define HYDROLOGY_PA_WB4                HYDROLOGY_PA_WB3 + 4
#define HYDROLOGY_PA_WB5                HYDROLOGY_PA_WB4 + 4
#define HYDROLOGY_PA_WB6                HYDROLOGY_PA_WB5 + 4
#define HYDROLOGY_PA_WB7                HYDROLOGY_PA_WB6 + 4
#define HYDROLOGY_PA_WB8                HYDROLOGY_PA_WB7 + 4
#define HYDROLOGY_PA_WC1                HYDROLOGY_PA_WB8 + 4
#define HYDROLOGY_PA_WC2                HYDROLOGY_PA_WC1 + 3
#define HYDROLOGY_PA_WC3                HYDROLOGY_PA_WC2 + 3
#define HYDROLOGY_PA_WC4                HYDROLOGY_PA_WC3 + 3
#define HYDROLOGY_PA_WC5                HYDROLOGY_PA_WC4 + 3
#define HYDROLOGY_PA_WC6                HYDROLOGY_PA_WC5 + 3
#define HYDROLOGY_PA_WC7                HYDROLOGY_PA_WC6 + 3
#define HYDROLOGY_PA_WC8                HYDROLOGY_PA_WC7 + 3
#define HYDROLOGY_PA_AW1                HYDROLOGY_PA_WC8 + 3
#define HYDROLOGY_PA_AW2                HYDROLOGY_PA_AW1 + 2
#define HYDROLOGY_PA_AW3                HYDROLOGY_PA_AW2 + 2
#define HYDROLOGY_PA_AW4                HYDROLOGY_PA_AW3 + 2
#define HYDROLOGY_PA_AW5                HYDROLOGY_PA_AW4 + 2
#define HYDROLOGY_PA_AW6                HYDROLOGY_PA_AW5 + 2
#define HYDROLOGY_PA_AW7                HYDROLOGY_PA_AW6 + 2
#define HYDROLOGY_PA_AW8                HYDROLOGY_PA_AW7 + 2
#define HYDROLOGY_PA_AAT                HYDROLOGY_PA_AW8 + 2
#define HYDROLOGY_PA_ABT                HYDROLOGY_PA_AAT + 2
#define HYDROLOGY_PA_TAT                HYDROLOGY_PA_ABT + 2
#define HYDROLOGY_PA_FRAT               HYDROLOGY_PA_TAT + 3
#define HYDROLOGY_PA_GPAT               HYDROLOGY_PA_FRAT + 3
#define HYDROLOGY_PA_PPT                HYDROLOGY_PA_GPAT + 2
#define HYDROLOGY_PA_BAT                HYDROLOGY_PA_PPT + 3
#define HYDROLOGY_PA_WSWT               HYDROLOGY_PA_BAT + 2
#define HYDROLOGY_PA_WTAT               HYDROLOGY_PA_WSWT + 2
#define HYDROLOGY_PA_UWLI1              HYDROLOGY_PA_WTAT + 1
#define HYDROLOGY_PA_LWLI1              HYDROLOGY_PA_UWLI1 + 2
#define HYDROLOGY_PA_UWLI2              HYDROLOGY_PA_LWLI1 + 2
#define HYDROLOGY_PA_LWLI2              HYDROLOGY_PA_UWLI2 + 2
#define HYDROLOGY_PA_UWLI3              HYDROLOGY_PA_LWLI2 + 2
#define HYDROLOGY_PA_LWLI3              HYDROLOGY_PA_UWLI3 + 2
#define HYDROLOGY_PA_UWLI4              HYDROLOGY_PA_LWLI3 + 2
#define HYDROLOGY_PA_LWLI4              HYDROLOGY_PA_UWLI4 + 2
#define HYDROLOGY_PA_UWLI5              HYDROLOGY_PA_LWLI4 + 2
#define HYDROLOGY_PA_LWLI5              HYDROLOGY_PA_UWLI5 + 2
#define HYDROLOGY_PA_UWLI6              HYDROLOGY_PA_LWLI5 + 2
#define HYDROLOGY_PA_LWLI6              HYDROLOGY_PA_UWLI6 + 2
#define HYDROLOGY_PA_UWLI7              HYDROLOGY_PA_LWLI6 + 2
#define HYDROLOGY_PA_LWLI7              HYDROLOGY_PA_UWLI7 + 2
#define HYDROLOGY_PA_UWLI8              HYDROLOGY_PA_LWLI7 + 2
#define HYDROLOGY_PA_LWLI8              HYDROLOGY_PA_UWLI8 + 2
#define HYDROLOGY_PA_ULWPI1             HYDROLOGY_PA_LWLI8 + 2
#define HYDROLOGY_PA_LLWPI1             HYDROLOGY_PA_ULWPI1 + 4
#define HYDROLOGY_PA_ULWPI2             HYDROLOGY_PA_LLWPI1 + 4
#define HYDROLOGY_PA_LLWPI2             HYDROLOGY_PA_ULWPI2 + 4
#define HYDROLOGY_PA_ULWPI3             HYDROLOGY_PA_LLWPI2 + 4
#define HYDROLOGY_PA_LLWPI3             HYDROLOGY_PA_ULWPI3 + 4
#define HYDROLOGY_PA_ULWPI4             HYDROLOGY_PA_LLWPI3 + 4
#define HYDROLOGY_PA_LLWPI4             HYDROLOGY_PA_ULWPI4 + 4
#define HYDROLOGY_PA_ULWPI5             HYDROLOGY_PA_LLWPI4 + 4
#define HYDROLOGY_PA_LLWPI5             HYDROLOGY_PA_ULWPI5 + 4
#define HYDROLOGY_PA_ULWPI6             HYDROLOGY_PA_LLWPI5 + 4
#define HYDROLOGY_PA_LLWPI6             HYDROLOGY_PA_ULWPI6 + 4
#define HYDROLOGY_PA_ULWPI7             HYDROLOGY_PA_LLWPI6 + 4
#define HYDROLOGY_PA_LLWPI7             HYDROLOGY_PA_ULWPI7 + 4
#define HYDROLOGY_PA_ULWPI8             HYDROLOGY_PA_LLWPI7 + 4
#define HYDROLOGY_PA_LLWPI8             HYDROLOGY_PA_ULWPI8 + 4
#define HYDROLOGY_PA_ULWT               HYDROLOGY_PA_LLWPI8 + 4
#define HYDROLOGY_PA_LLWT               HYDROLOGY_PA_ULWT + 2
#define HYDROLOGY_PA_ULpHV              HYDROLOGY_PA_LLWT + 2
#define HYDROLOGY_PA_LLpHV              HYDROLOGY_PA_ULpHV + 1
#define HYDROLOGY_PA_ULDO               HYDROLOGY_PA_LLpHV + 1
#define HYDROLOGY_PA_LLDO               HYDROLOGY_PA_ULDO + 2
#define HYDROLOGY_PA_ULPI               HYDROLOGY_PA_LLDO + 2
#define HYDROLOGY_PA_LLPI               HYDROLOGY_PA_ULPI + 2
#define HYDROLOGY_PA_ULCO               HYDROLOGY_PA_LLPI + 2
#define HYDROLOGY_PA_LLCO               HYDROLOGY_PA_ULCO + 3
#define HYDROLOGY_PA_ULRP               HYDROLOGY_PA_LLCO + 3
#define HYDROLOGY_PA_LLRP               HYDROLOGY_PA_ULRP + 3
#define HYDROLOGY_PA_ULT                HYDROLOGY_PA_LLRP + 3
#define HYDROLOGY_PA_LLT                HYDROLOGY_PA_ULT + 2
#define HYDROLOGY_PA_ULAN               HYDROLOGY_PA_LLT + 2
#define HYDROLOGY_PA_LLAN               HYDROLOGY_PA_ULAN + 3
#define HYDROLOGY_PA_ULTN               HYDROLOGY_PA_LLAN + 3
#define HYDROLOGY_PA_LLTN               HYDROLOGY_PA_ULTN + 3
#define HYDROLOGY_PA_ULC                HYDROLOGY_PA_LLTN + 3
#define HYDROLOGY_PA_LLC                HYDROLOGY_PA_ULC + 4
#define HYDROLOGY_PA_ULZ                HYDROLOGY_PA_LLC + 4
#define HYDROLOGY_PA_LLZ                HYDROLOGY_PA_ULZ + 3
#define HYDROLOGY_PA_ULF                HYDROLOGY_PA_LLZ + 3
#define HYDROLOGY_PA_LLF                HYDROLOGY_PA_ULF + 3
#define HYDROLOGY_PA_ULS                HYDROLOGY_PA_LLF + 3
#define HYDROLOGY_PA_LLS                HYDROLOGY_PA_ULS + 4
#define HYDROLOGY_PA_ULA                HYDROLOGY_PA_LLS + 4
#define HYDROLOGY_PA_LLA                HYDROLOGY_PA_ULA + 4
#define HYDROLOGY_PA_ULM                HYDROLOGY_PA_LLA + 4
#define HYDROLOGY_PA_LLM                HYDROLOGY_PA_ULM + 4
#define HYDROLOGY_PA_ULCA               HYDROLOGY_PA_LLM + 4
#define HYDROLOGY_PA_LLCA               HYDROLOGY_PA_ULCA + 4
#define HYDROLOGY_PA_ULTO               HYDROLOGY_PA_LLCA + 4
#define HYDROLOGY_PA_LLTO               HYDROLOGY_PA_ULTO + 2
#define HYDROLOGY_PA_ULCH               HYDROLOGY_PA_LLTO + 2
#define HYDROLOGY_PA_LLCH               HYDROLOGY_PA_ULCH + 2
#define HYDROLOGY_PA_ULFL               HYDROLOGY_PA_LLCH + 2
#define HYDROLOGY_PA_RWQM1              HYDROLOGY_PA_ULFL + 6
#define HYDROLOGY_PA_RWQM2              HYDROLOGY_PA_RWQM1 + 6
#define HYDROLOGY_PA_RWQM3              HYDROLOGY_PA_RWQM2 + 6
#define HYDROLOGY_PA_RWQM4              HYDROLOGY_PA_RWQM3 + 6
#define HYDROLOGY_PA_RWQM5              HYDROLOGY_PA_RWQM4 + 6
#define HYDROLOGY_PA_RWQM6              HYDROLOGY_PA_RWQM5 + 6
#define HYDROLOGY_PA_RWQM7              HYDROLOGY_PA_RWQM6 + 6
#define HYDROLOGY_PA_RWQM8              HYDROLOGY_PA_RWQM7 + 6
#define HYDROLOGY_PA_FVWQ               HYDROLOGY_PA_RWQM8 + 6
#define HYDROLOGY_PA_DISSS              HYDROLOGY_PA_FVWQ + 6
#define HYDROLOGY_PA_TTPRFS             HYDROLOGY_PA_DISSS + 1
#define HYDROLOGY_PA_BVWM1              HYDROLOGY_PA_TTPRFS + 1
#define HYDROLOGY_PA_BVWM2              HYDROLOGY_PA_BVWM1 + 6
#define HYDROLOGY_PA_BVWM3              HYDROLOGY_PA_BVWM2 + 6
#define HYDROLOGY_PA_BVWM4              HYDROLOGY_PA_BVWM3 + 6
#define HYDROLOGY_PA_BVWM5              HYDROLOGY_PA_BVWM4 + 6
#define HYDROLOGY_PA_BVWM6              HYDROLOGY_PA_BVWM5 + 6
#define HYDROLOGY_PA_BVWM7              HYDROLOGY_PA_BVWM6 + 6
#define HYDROLOGY_PA_BVWM8              HYDROLOGY_PA_BVWM7 + 6
#define HYDROLOGY_PA_WMRWAV1            HYDROLOGY_PA_BVWM8 + 6
#define HYDROLOGY_PA_WMRWAV2            HYDROLOGY_PA_WMRWAV1 + 6
#define HYDROLOGY_PA_WMRWAV3            HYDROLOGY_PA_WMRWAV2 + 6
#define HYDROLOGY_PA_WMRWAV4            HYDROLOGY_PA_WMRWAV3 + 6
#define HYDROLOGY_PA_WMRWAV5            HYDROLOGY_PA_WMRWAV4 + 6
#define HYDROLOGY_PA_WMRWAV6            HYDROLOGY_PA_WMRWAV5 + 6
#define HYDROLOGY_PA_WMRWAV7            HYDROLOGY_PA_WMRWAV6 + 6
#define HYDROLOGY_PA_WMRWAV8            HYDROLOGY_PA_WMRWAV7 + 6

#define HYDROLOGY_PDA_INIT_MARK         HYDROLOGY_PA_WMRWAV8 + 6
#define HYDROLOGY_PDA_RTUTYPE           HYDROLOGY_PDA_INIT_MARK + 5
#define HYDROLOGY_PDA_PERIOD_BT         HYDROLOGY_PDA_RTUTYPE + 1
#define HYDROLOGY_PDA_PERIOD_ET         HYDROLOGY_PDA_PERIOD_BT + 4
#define HYDROLOGY_PDA_SW_VERSION_LEN    HYDROLOGY_PDA_PERIOD_ET + 4
#define HYDROLOGY_PDA_SW_VERSION        HYDROLOGY_PDA_SW_VERSION_LEN + 1
#define HYDROLOGY_PDA_PUMP_LEN          HYDROLOGY_PDA_SW_VERSION + 20
#define HYDROLOGY_PDA_PUMP              HYDROLOGY_PDA_PUMP_LEN + 1
#define HYDROLOGY_PDA_VALVE_LEN         HYDROLOGY_PDA_PUMP + 10
#define HYDROLOGY_PDA_VALVE             HYDROLOGY_PDA_VALVE_LEN + 1
#define HYDROLOGY_PDA_GATE_LEN          HYDROLOGY_PDA_VALVE + 10
#define HYDROLOGY_PDA_GATE              HYDROLOGY_PDA_GATE_LEN + 1
#define HYDROLOGY_PDA_WATERSETTING      HYDROLOGY_PDA_GATE + 10
#define HYDROLOGY_PDA_RECORD            HYDROLOGY_PDA_WATERSETTING + 1
#define HYDROLOGY_PDA_NEWPASSWORD       HYDROLOGY_PDA_RECORD + 2 * 32

#define HYDROLOGY_END                   HYDROLOGY_PDA_NEWPASSWORD + 2

#define HYDROLOGY_E_TT                  {0xF0, 10, 0, HYDROLOGY_EA_TT}
#define HYDROLOGY_E_ST                  {0xF1, 10, 0, HYDROLOGY_EA_ST}
#define HYDROLOGY_E_RGZS                {0xF2, 0, 0, HYDROLOGY_EA_RGZS}
#define HYDROLOGY_E_PIC                 {0xF3, 0, 0, HYDROLOGY_EA_PIC}
#define HYDROLOGY_E_DRP                 {0xF4, 24, 0, HYDROLOGY_EA_DRP}
#define HYDROLOGY_E_DRZ1                {0xF5, 48, 0, HYDROLOGY_EA_DRZ1}
#define HYDROLOGY_E_DRZ2                {0xF6, 48, 0, HYDROLOGY_EA_DRZ2}
#define HYDROLOGY_E_DRZ3                {0xF7, 48, 0, HYDROLOGY_EA_DRZ3}
#define HYDROLOGY_E_DRZ4                {0xF8, 48, 0, HYDROLOGY_EA_DRZ4}
#define HYDROLOGY_E_DRZ5                {0xF9, 48, 0, HYDROLOGY_EA_DRZ5}
#define HYDROLOGY_E_DRZ6                {0xFA, 48, 0, HYDROLOGY_EA_DRZ6}
#define HYDROLOGY_E_DRZ7                {0xFB, 48, 0, HYDROLOGY_EA_DRZ7}
#define HYDROLOGY_E_DRZ8                {0xFC, 48, 0, HYDROLOGY_EA_DRZ8}
#define HYDROLOGY_E_DATA                {0xFD, 0, 0, HYDROLOGY_EA_DATA}
#define HYDROLOGY_E_AC                  {0x01, 8, 2, HYDROLOGY_EA_AC}
#define HYDROLOGY_E_AI                  {0x02, 3, 1, HYDROLOGY_EA_AI}
#define HYDROLOGY_E_C                   {0x03, 3, 1, HYDROLOGY_EA_C}
#define HYDROLOGY_E_DRxnn               {0x04, 6, 0, HYDROLOGY_EA_DRxnn}
#define HYDROLOGY_E_DT                  {0x05, 6, 0, HYDROLOGY_EA_DT}
#define HYDROLOGY_E_ED                  {0x06, 5, 1, HYDROLOGY_EA_ED}
#define HYDROLOGY_E_EJ                  {0x07, 5, 1, HYDROLOGY_EA_EJ}
#define HYDROLOGY_E_FL                  {0x08, 5, 0, HYDROLOGY_EA_FL}
#define HYDROLOGY_E_GH                  {0x09, 5, 2, HYDROLOGY_EA_GH}
#define HYDROLOGY_E_GN                  {0x0A, 3, 0, HYDROLOGY_EA_GN}
#define HYDROLOGY_E_GS                  {0x0B, 1, 0, HYDROLOGY_EA_GS}
#define HYDROLOGY_E_GT                  {0x0C, 3, 0, HYDROLOGY_EA_GT}
#define HYDROLOGY_E_GTP                 {0x0D, 3, 1, HYDROLOGY_EA_GTP}
#define HYDROLOGY_E_H                   {0x0E, 6, 2, HYDROLOGY_EA_H}
#define HYDROLOGY_E_HW                  {0x0F, 5, 2, HYDROLOGY_EA_HW}
#define HYDROLOGY_E_M10                 {0x10, 4, 1, HYDROLOGY_EA_M10}
#define HYDROLOGY_E_M20                 {0x11, 4, 1, HYDROLOGY_EA_M20}
#define HYDROLOGY_E_M30                 {0x12, 4, 1, HYDROLOGY_EA_M30}
#define HYDROLOGY_E_M40                 {0x13, 4, 1, HYDROLOGY_EA_M40}
#define HYDROLOGY_E_M50                 {0x14, 4, 1, HYDROLOGY_EA_M50}
#define HYDROLOGY_E_M60                 {0x15, 4, 1, HYDROLOGY_EA_M60}
#define HYDROLOGY_E_M80                 {0x16, 4, 1, HYDROLOGY_EA_M80}
#define HYDROLOGY_E_M100                {0x17, 4, 1, HYDROLOGY_EA_M100}
#define HYDROLOGY_E_MST                 {0x18, 4, 1, HYDROLOGY_EA_MST}
#define HYDROLOGY_E_NS                  {0x19, 2, 0, HYDROLOGY_EA_NS}
#define HYDROLOGY_E_P1                  {0x1A, 5, 1, HYDROLOGY_EA_P1}
#define HYDROLOGY_E_P2                  {0x1B, 5, 1, HYDROLOGY_EA_P2}
#define HYDROLOGY_E_P3                  {0x1C, 5, 1, HYDROLOGY_EA_P3}
#define HYDROLOGY_E_P6                  {0x1D, 5, 1, HYDROLOGY_EA_P6}
#define HYDROLOGY_E_P12                 {0x1E, 5, 1, HYDROLOGY_EA_P12}
#define HYDROLOGY_E_PD                  {0x1F, 5, 1, HYDROLOGY_EA_PD}
#define HYDROLOGY_E_PJ                  {0x20, 5, 1, HYDROLOGY_EA_PJ}
#define HYDROLOGY_E_PN01                {0x21, 5, 1, HYDROLOGY_EA_PN01}
#define HYDROLOGY_E_PN05                {0x22, 5, 1, HYDROLOGY_EA_PN05}
#define HYDROLOGY_E_PN10                {0x23, 5, 1, HYDROLOGY_EA_PN10}
#define HYDROLOGY_E_PN30                {0x24, 5, 1, HYDROLOGY_EA_PN30}
#define HYDROLOGY_E_PR                  {0x25, 5, 1, HYDROLOGY_EA_PR}
#define HYDROLOGY_E_PT                  {0x26, 6, 1, HYDROLOGY_EA_PT}
#define HYDROLOGY_E_Q                   {0x27, 9, 3, HYDROLOGY_EA_Q}
#define HYDROLOGY_E_Q1                  {0x28, 9, 3, HYDROLOGY_EA_Q1}
#define HYDROLOGY_E_Q2                  {0x29, 9, 3, HYDROLOGY_EA_Q2}
#define HYDROLOGY_E_Q3                  {0x2A, 9, 3, HYDROLOGY_EA_Q3}
#define HYDROLOGY_E_Q4                  {0x2B, 9, 3, HYDROLOGY_EA_Q4}
#define HYDROLOGY_E_Q5                  {0x2C, 9, 3, HYDROLOGY_EA_Q5}
#define HYDROLOGY_E_Q6                  {0x2D, 9, 3, HYDROLOGY_EA_Q6}
#define HYDROLOGY_E_Q7                  {0x2E, 9, 3, HYDROLOGY_EA_Q7}
#define HYDROLOGY_E_Q8                  {0x2F, 9, 3, HYDROLOGY_EA_Q8}
#define HYDROLOGY_E_QA                  {0x30, 9, 3, HYDROLOGY_EA_QA}
#define HYDROLOGY_E_QZ                  {0x31, 9, 3, HYDROLOGY_EA_QZ}
#define HYDROLOGY_E_SW                  {0x32, 11, 3, HYDROLOGY_EA_SW}
#define HYDROLOGY_E_UC                  {0x33, 2, 0, HYDROLOGY_EA_UC}
#define HYDROLOGY_E_UE                  {0x34, 2, 0, HYDROLOGY_EA_UE}
#define HYDROLOGY_E_US                  {0x35, 4, 1, HYDROLOGY_EA_US}
#define HYDROLOGY_E_VA                  {0x36, 5, 3, HYDROLOGY_EA_VA}
#define HYDROLOGY_E_VJ                  {0x37, 5, 3, HYDROLOGY_EA_VJ}
#define HYDROLOGY_E_VT                  {0x38, 4, 2, HYDROLOGY_EA_VT}
#define HYDROLOGY_E_Z                   {0x39, 7, 3, HYDROLOGY_EA_Z}
#define HYDROLOGY_E_ZB                  {0x3A, 7, 3, HYDROLOGY_EA_ZB}
#define HYDROLOGY_E_ZU                  {0x3B, 7, 3, HYDROLOGY_EA_ZU}
#define HYDROLOGY_E_Z1                  {0x3C, 7, 3, HYDROLOGY_EA_Z1}
#define HYDROLOGY_E_Z2                  {0x3D, 7, 3, HYDROLOGY_EA_Z2}
#define HYDROLOGY_E_Z3                  {0x3E, 7, 3, HYDROLOGY_EA_Z3}
#define HYDROLOGY_E_Z4                  {0x3F, 7, 3, HYDROLOGY_EA_Z4}
#define HYDROLOGY_E_Z5                  {0x40, 7, 3, HYDROLOGY_EA_Z5}
#define HYDROLOGY_E_Z6                  {0x41, 7, 3, HYDROLOGY_EA_Z6}
#define HYDROLOGY_E_Z7                  {0x42, 7, 3, HYDROLOGY_EA_Z7}
#define HYDROLOGY_E_Z8                  {0x43, 7, 3, HYDROLOGY_EA_Z8}
#define HYDROLOGY_E_SQ                  {0x44, 9, 3, HYDROLOGY_EA_SQ}
#define HYDROLOGY_E_ZT                  {0x45, 8, 0, HYDROLOGY_EA_ZT}
#define HYDROLOGY_E_pH                  {0x46, 4, 2, HYDROLOGY_EA_pH}
#define HYDROLOGY_E_DO                  {0x47, 4, 1, HYDROLOGY_EA_DO}
#define HYDROLOGY_E_COND                {0x48, 5, 0, HYDROLOGY_EA_COND}
#define HYDROLOGY_E_TURB                {0x49, 3, 0, HYDROLOGY_EA_TURB}
#define HYDROLOGY_E_CODMN               {0x4A, 4, 1, HYDROLOGY_EA_CODMN}
#define HYDROLOGY_E_REDOX               {0x4B, 5, 1, HYDROLOGY_EA_REDOX}
#define HYDROLOGY_E_NH4N                {0x4C, 6, 2, HYDROLOGY_EA_NH4N}
#define HYDROLOGY_E_TP                  {0x4D, 5, 3, HYDROLOGY_EA_TP}
#define HYDROLOGY_E_TN                  {0x4E, 5, 2, HYDROLOGY_EA_TN}
#define HYDROLOGY_E_TOC                 {0x4F, 4, 2, HYDROLOGY_EA_TOC}
#define HYDROLOGY_E_CU                  {0x50, 7, 4, HYDROLOGY_EA_CU}
#define HYDROLOGY_E_ZN                  {0x51, 6, 4, HYDROLOGY_EA_ZN}
#define HYDROLOGY_E_SE                  {0x52, 7, 5, HYDROLOGY_EA_SE}
#define HYDROLOGY_E_AS                  {0x53, 7, 5, HYDROLOGY_EA_AS}
#define HYDROLOGY_E_THG                 {0x54, 7, 5, HYDROLOGY_EA_THG}
#define HYDROLOGY_E_CD                  {0x55, 7, 5, HYDROLOGY_EA_CD}
#define HYDROLOGY_E_PB                  {0x56, 7, 5, HYDROLOGY_EA_PB}
#define HYDROLOGY_E_CHLA                {0x57, 4, 2, HYDROLOGY_EA_CHLA}
#define HYDROLOGY_E_WP1                 {0x58, 5, 2, HYDROLOGY_EA_WP1}
#define HYDROLOGY_E_WP2                 {0x59, 5, 2, HYDROLOGY_EA_WP2}
#define HYDROLOGY_E_WP3                 {0x5A, 5, 2, HYDROLOGY_EA_WP3}
#define HYDROLOGY_E_WP4                 {0x5B, 5, 2, HYDROLOGY_EA_WP4}
#define HYDROLOGY_E_WP5                 {0x5C, 5, 2, HYDROLOGY_EA_WP5}
#define HYDROLOGY_E_WP6                 {0x5D, 5, 2, HYDROLOGY_EA_WP6}
#define HYDROLOGY_E_WP7                 {0x5E, 5, 2, HYDROLOGY_EA_WP7}
#define HYDROLOGY_E_WP8                 {0x5F, 5, 2, HYDROLOGY_EA_WP8}
#define HYDROLOGY_E_SYL1                {0x60, 11, 3, HYDROLOGY_EA_SYL1}
#define HYDROLOGY_E_SYL2                {0x61, 11, 3, HYDROLOGY_EA_SYL2}
#define HYDROLOGY_E_SYL3                {0x62, 11, 3, HYDROLOGY_EA_SYL3}
#define HYDROLOGY_E_SYL4                {0x63, 11, 3, HYDROLOGY_EA_SYL4}
#define HYDROLOGY_E_SYL5                {0x64, 11, 3, HYDROLOGY_EA_SYL5}
#define HYDROLOGY_E_SYL6                {0x65, 11, 3, HYDROLOGY_EA_SYL6}
#define HYDROLOGY_E_SYL7                {0x66, 11, 3, HYDROLOGY_EA_SYL7}
#define HYDROLOGY_E_SYL8                {0x67, 11, 3, HYDROLOGY_EA_SYL8}
#define HYDROLOGY_E_SBL1                {0x68, 10, 2, HYDROLOGY_EA_SBL1}
#define HYDROLOGY_E_SBL2                {0x69, 10, 2, HYDROLOGY_EA_SBL2}
#define HYDROLOGY_E_SBL3                {0x6A, 10, 2, HYDROLOGY_EA_SBL3}
#define HYDROLOGY_E_SBL4                {0x6B, 10, 2, HYDROLOGY_EA_SBL4}
#define HYDROLOGY_E_SBL5                {0x6C, 10, 2, HYDROLOGY_EA_SBL5}
#define HYDROLOGY_E_SBL6                {0x6D, 10, 2, HYDROLOGY_EA_SBL6}
#define HYDROLOGY_E_SBL7                {0x6E, 10, 2, HYDROLOGY_EA_SBL7}
#define HYDROLOGY_E_SBL8                {0x6F, 10, 2, HYDROLOGY_EA_SBL8}
#define HYDROLOGY_E_VTA                 {0x70, 4, 1, HYDROLOGY_EA_VTA}
#define HYDROLOGY_E_VTB                 {0x71, 4, 1, HYDROLOGY_EA_VTB}
#define HYDROLOGY_E_VTC                 {0x72, 4, 1, HYDROLOGY_EA_VTC}
#define HYDROLOGY_E_VIA                 {0x73, 4, 1, HYDROLOGY_EA_VIA}
#define HYDROLOGY_E_VIB                 {0x74, 4, 1, HYDROLOGY_EA_VIB}
#define HYDROLOGY_E_VIC                 {0x75, 4, 1, HYDROLOGY_EA_VIC}

#define HYDROLOGY_B_CENTER              {0x01, 8, 0, HYDROLOGY_BA_CENTER}
#define HYDROLOGY_B_REMOTE              {0x02, 10, 0, HYDROLOGY_BA_REMOTE}
#define HYDROLOGY_B_PASSWORD            {0x03, 4, 0, HYDROLOGY_BA_PASSWORD}
#define HYDROLOGY_B_CENTER1_IP          {0x04, 0, 0, HYDROLOGY_BA_CENTER1_IP}
#define HYDROLOGY_B_BACKUP1_IP          {0x05, 0, 0, HYDROLOGY_BA_BACKUP1_IP}
#define HYDROLOGY_B_CENTER2_IP          {0x06, 0, 0, HYDROLOGY_BA_CENTER2_IP}
#define HYDROLOGY_B_BACKUP2_IP          {0x07, 0, 0, HYDROLOGY_BA_BACKUP2_IP}
#define HYDROLOGY_B_CENTER3_IP          {0x08, 0, 0, HYDROLOGY_BA_CENTER3_IP}
#define HYDROLOGY_B_BACKUP3_IP          {0x09, 0, 0, HYDROLOGY_BA_BACKUP3_IP}
#define HYDROLOGY_B_CENTER4_IP          {0x0A, 0, 0, HYDROLOGY_BA_CENTER4_IP}
#define HYDROLOGY_B_BACKUP4_IP          {0x0B, 0, 0, HYDROLOGY_BA_BACKUP4_IP}
#define HYDROLOGY_B_WORK_MODE           {0x0C, 2, 0, HYDROLOGY_BA_WORK_MODE}
#define HYDROLOGY_B_ELEMENT_SELECT      {0x0D, 16, 0, HYDROLOGY_BA_ELEMENT_SELECT}
#define HYDROLOGY_B_REPEATER_STATION    {0x0E, 12, 0, HYDROLOGY_BA_REPEATER_STATION}
#define HYDROLOGY_B_DEVICE_ID           {0x0F, 12, 0, HYDROLOGY_BA_DEVICE_ID}

#define HYDROLOGY_P_TI                  {0x20, 2, 0, HYDROLOGY_PA_TI}
#define HYDROLOGY_P_AI                  {0x21, 2, 0, HYDROLOGY_PA_AI}
#define HYDROLOGY_P_RBT                 {0x22, 2, 0, HYDROLOGY_PA_RBT}
#define HYDROLOGY_P_SI                  {0x23, 4, 0, HYDROLOGY_PA_SI}
#define HYDROLOGY_P_WSI                 {0x24, 2, 0, HYDROLOGY_PA_WSI}
#define HYDROLOGY_P_RR                  {0x25, 2, 1, HYDROLOGY_PA_RR}
#define HYDROLOGY_P_WR                  {0x26, 2, 1, HYDROLOGY_PA_WR}
#define HYDROLOGY_P_RAT                 {0x27, 2, 0, HYDROLOGY_PA_RAT}
#define HYDROLOGY_P_WB1                 {0x28, 7, 3, HYDROLOGY_PA_WB1}
#define HYDROLOGY_P_WB2                 {0x29, 7, 3, HYDROLOGY_PA_WB2}
#define HYDROLOGY_P_WB3                 {0x2A, 7, 3, HYDROLOGY_PA_WB3}
#define HYDROLOGY_P_WB4                 {0x2B, 7, 3, HYDROLOGY_PA_WB4}
#define HYDROLOGY_P_WB5                 {0x2C, 7, 3, HYDROLOGY_PA_WB5}
#define HYDROLOGY_P_WB6                 {0x2D, 7, 3, HYDROLOGY_PA_WB6}
#define HYDROLOGY_P_WB7                 {0x2E, 7, 3, HYDROLOGY_PA_WB7}
#define HYDROLOGY_P_WB8                 {0x2F, 7, 3, HYDROLOGY_PA_WB8}
#define HYDROLOGY_P_WC1                 {0x30, 5, 3, HYDROLOGY_PA_WC1}
#define HYDROLOGY_P_WC2                 {0x31, 5, 3, HYDROLOGY_PA_WC2}
#define HYDROLOGY_P_WC3                 {0x32, 5, 3, HYDROLOGY_PA_WC3}
#define HYDROLOGY_P_WC4                 {0x33, 5, 3, HYDROLOGY_PA_WC4}
#define HYDROLOGY_P_WC5                 {0x34, 5, 3, HYDROLOGY_PA_WC5}
#define HYDROLOGY_P_WC6                 {0x35, 5, 3, HYDROLOGY_PA_WC6}
#define HYDROLOGY_P_WC7                 {0x36, 5, 3, HYDROLOGY_PA_WC7}
#define HYDROLOGY_P_WC8                 {0x37, 5, 3, HYDROLOGY_PA_WC8}
#define HYDROLOGY_P_AW1                 {0x38, 4, 2, HYDROLOGY_PA_AW1}
#define HYDROLOGY_P_AW2                 {0x39, 4, 2, HYDROLOGY_PA_AW2}
#define HYDROLOGY_P_AW3                 {0x3A, 4, 2, HYDROLOGY_PA_AW3}
#define HYDROLOGY_P_AW4                 {0x3B, 4, 2, HYDROLOGY_PA_AW4}
#define HYDROLOGY_P_AW5                 {0x3C, 4, 2, HYDROLOGY_PA_AW5}
#define HYDROLOGY_P_AW6                 {0x3D, 4, 2, HYDROLOGY_PA_AW6}
#define HYDROLOGY_P_AW7                 {0x3E, 4, 2, HYDROLOGY_PA_AW7}
#define HYDROLOGY_P_AW8                 {0x3F, 4, 2, HYDROLOGY_PA_AW8}
#define HYDROLOGY_P_AAT                 {0x40, 3, 2, HYDROLOGY_PA_AAT}
#define HYDROLOGY_P_ABT                 {0x41, 3, 2, HYDROLOGY_PA_ABT}
#define HYDROLOGY_P_TAT                 {0x42, 6, 3, HYDROLOGY_PA_TAT}
#define HYDROLOGY_P_FRAT                {0x43, 5, 3, HYDROLOGY_PA_FRAT}
#define HYDROLOGY_P_GPAT                {0x44, 3, 2, HYDROLOGY_PA_GPAT}
#define HYDROLOGY_P_PPT                 {0x45, 6, 0, HYDROLOGY_PA_PPT}
#define HYDROLOGY_P_BAT                 {0x46, 4, 0, HYDROLOGY_PA_BAT}
#define HYDROLOGY_P_WSWT                {0x47, 4, 2, HYDROLOGY_PA_WSWT}
#define HYDROLOGY_P_WTAT                {0x48, 2, 1, HYDROLOGY_PA_WTAT}
#define HYDROLOGY_P_UWLI1               {0x49, 4, 2, HYDROLOGY_PA_UWLI1}
#define HYDROLOGY_P_LWLI1               {0x4A, 4, 2, HYDROLOGY_PA_LWLI1}
#define HYDROLOGY_P_UWLI2               {0x4B, 4, 2, HYDROLOGY_PA_UWLI2}
#define HYDROLOGY_P_LWLI2               {0x4C, 4, 2, HYDROLOGY_PA_LWLI2}
#define HYDROLOGY_P_UWLI3               {0x4D, 4, 2, HYDROLOGY_PA_UWLI3}
#define HYDROLOGY_P_LWLI3               {0x4E, 4, 2, HYDROLOGY_PA_LWLI3}
#define HYDROLOGY_P_UWLI4               {0x4F, 4, 2, HYDROLOGY_PA_UWLI4}
#define HYDROLOGY_P_LWLI4               {0x50, 4, 2, HYDROLOGY_PA_LWLI4}
#define HYDROLOGY_P_UWLI5               {0x51, 4, 2, HYDROLOGY_PA_UWLI5}
#define HYDROLOGY_P_LWLI5               {0x52, 4, 2, HYDROLOGY_PA_LWLI5}
#define HYDROLOGY_P_UWLI6               {0x53, 4, 2, HYDROLOGY_PA_UWLI6}
#define HYDROLOGY_P_LWLI6               {0x54, 4, 2, HYDROLOGY_PA_LWLI6}
#define HYDROLOGY_P_UWLI7               {0x55, 4, 2, HYDROLOGY_PA_UWLI7}
#define HYDROLOGY_P_LWLI7               {0x56, 4, 2, HYDROLOGY_PA_LWLI7}
#define HYDROLOGY_P_UWLI8               {0x57, 4, 2, HYDROLOGY_PA_UWLI8}
#define HYDROLOGY_P_LWLI8               {0x58, 4, 2, HYDROLOGY_PA_LWLI8}
#define HYDROLOGY_P_ULWPI1              {0x59, 8, 2, HYDROLOGY_PA_ULWPI1}
#define HYDROLOGY_P_LLWPI1              {0x5A, 8, 2, HYDROLOGY_PA_LLWPI1}
#define HYDROLOGY_P_ULWPI2              {0x5B, 8, 2, HYDROLOGY_PA_ULWPI2}
#define HYDROLOGY_P_LLWPI2              {0x5C, 8, 2, HYDROLOGY_PA_LLWPI2}
#define HYDROLOGY_P_ULWPI3              {0x5D, 8, 2, HYDROLOGY_PA_ULWPI3}
#define HYDROLOGY_P_LLWPI3              {0x5E, 8, 2, HYDROLOGY_PA_LLWPI3}
#define HYDROLOGY_P_ULWPI4              {0x5F, 8, 2, HYDROLOGY_PA_ULWPI4}
#define HYDROLOGY_P_LLWPI4              {0x60, 8, 2, HYDROLOGY_PA_LLWPI4}
#define HYDROLOGY_P_ULWPI5              {0x61, 8, 2, HYDROLOGY_PA_ULWPI5}
#define HYDROLOGY_P_LLWPI5              {0x62, 8, 2, HYDROLOGY_PA_LLWPI5}
#define HYDROLOGY_P_ULWPI6              {0x63, 8, 2, HYDROLOGY_PA_ULWPI6}
#define HYDROLOGY_P_LLWPI6              {0x64, 8, 2, HYDROLOGY_PA_LLWPI6}
#define HYDROLOGY_P_ULWPI7              {0x65, 8, 2, HYDROLOGY_PA_ULWPI7}
#define HYDROLOGY_P_LLWPI7              {0x66, 8, 2, HYDROLOGY_PA_LLWPI7}
#define HYDROLOGY_P_ULWPI8              {0x67, 8, 2, HYDROLOGY_PA_ULWPI8}
#define HYDROLOGY_P_LLWPI8              {0x68, 8, 2, HYDROLOGY_PA_LLWPI8}
#define HYDROLOGY_P_ULWT                {0x69, 3, 1, HYDROLOGY_PA_ULWT}
#define HYDROLOGY_P_LLWT                {0x6A, 3, 1, HYDROLOGY_PA_LLWT}
#define HYDROLOGY_P_ULpHV               {0x6B, 2, 1, HYDROLOGY_PA_ULpHV}
#define HYDROLOGY_P_LLpHV               {0x6C, 2, 1, HYDROLOGY_PA_LLpHV}
#define HYDROLOGY_P_ULDO                {0x6D, 4, 1, HYDROLOGY_PA_ULDO}
#define HYDROLOGY_P_LLDO                {0x6E, 4, 1, HYDROLOGY_PA_LLDO}
#define HYDROLOGY_P_ULPI                {0x6F, 4, 1, HYDROLOGY_PA_ULPI}
#define HYDROLOGY_P_LLPI                {0x70, 4, 1, HYDROLOGY_PA_LLPI}
#define HYDROLOGY_P_ULCO                {0x71, 5, 0, HYDROLOGY_PA_ULCO}
#define HYDROLOGY_P_LLCO                {0x72, 5, 0, HYDROLOGY_PA_LLCO}
#define HYDROLOGY_P_ULRP                {0x73, 5, 1, HYDROLOGY_PA_ULRP}
#define HYDROLOGY_P_LLRP                {0x74, 5, 1, HYDROLOGY_PA_LLRP}
#define HYDROLOGY_P_ULT                 {0x75, 3, 0, HYDROLOGY_PA_ULT}
#define HYDROLOGY_P_LLT                 {0x76, 3, 0, HYDROLOGY_PA_LLT}
#define HYDROLOGY_P_ULAN                {0x77, 6, 2, HYDROLOGY_PA_ULAN}
#define HYDROLOGY_P_LLAN                {0x78, 6, 2, HYDROLOGY_PA_LLAN}
#define HYDROLOGY_P_ULTN                {0x79, 5, 2, HYDROLOGY_PA_ULTN}
#define HYDROLOGY_P_LLTN                {0x7A, 5, 2, HYDROLOGY_PA_LLTN}
#define HYDROLOGY_P_ULC                 {0x7B, 7, 4, HYDROLOGY_PA_ULC}
#define HYDROLOGY_P_LLC                 {0x7C, 7, 4, HYDROLOGY_PA_LLC}
#define HYDROLOGY_P_ULZ                 {0x7D, 6, 4, HYDROLOGY_PA_ULZ}
#define HYDROLOGY_P_LLZ                 {0x7E, 6, 4, HYDROLOGY_PA_LLZ}
#define HYDROLOGY_P_ULF                 {0x7F, 5, 2, HYDROLOGY_PA_ULF}
#define HYDROLOGY_P_LLF                 {0x80, 5, 2, HYDROLOGY_PA_LLF}
#define HYDROLOGY_P_ULS                 {0x81, 7, 5, HYDROLOGY_PA_ULS}
#define HYDROLOGY_P_LLS                 {0x82, 7, 5, HYDROLOGY_PA_LLS}
#define HYDROLOGY_P_ULA                 {0x83, 7, 5, HYDROLOGY_PA_ULA}
#define HYDROLOGY_P_LLA                 {0x84, 7, 5, HYDROLOGY_PA_LLA}
#define HYDROLOGY_P_ULM                 {0x85, 7, 5, HYDROLOGY_PA_ULM}
#define HYDROLOGY_P_LLM                 {0x86, 7, 5, HYDROLOGY_PA_LLM}
#define HYDROLOGY_P_ULCA                {0x87, 7, 5, HYDROLOGY_PA_ULCA}
#define HYDROLOGY_P_LLCA                {0x88, 7, 5, HYDROLOGY_PA_LLCA}
#define HYDROLOGY_P_ULTO                {0x89, 4, 2, HYDROLOGY_PA_ULTO}
#define HYDROLOGY_P_LLTO                {0x8A, 4, 2, HYDROLOGY_PA_LLTO}
#define HYDROLOGY_P_ULCH                {0x8B, 4, 2, HYDROLOGY_PA_ULCH}
#define HYDROLOGY_P_LLCH                {0x8C, 4, 2, HYDROLOGY_PA_LLCH}
#define HYDROLOGY_P_ULFL                {0x8D, 11, 3, HYDROLOGY_PA_ULFL}
#define HYDROLOGY_P_RWQM1               {0x8E, 11, 3, HYDROLOGY_PA_RWQM1}
#define HYDROLOGY_P_RWQM2               {0x8F, 11, 3, HYDROLOGY_PA_RWQM2}
#define HYDROLOGY_P_RWQM3               {0x90, 11, 3, HYDROLOGY_PA_RWQM3}
#define HYDROLOGY_P_RWQM4               {0x91, 11, 3, HYDROLOGY_PA_RWQM4}
#define HYDROLOGY_P_RWQM5               {0x92, 11, 3, HYDROLOGY_PA_RWQM5}
#define HYDROLOGY_P_RWQM6               {0x93, 11, 3, HYDROLOGY_PA_RWQM6}
#define HYDROLOGY_P_RWQM7               {0x94, 11, 3, HYDROLOGY_PA_RWQM7}
#define HYDROLOGY_P_RWQM8               {0x95, 11, 3, HYDROLOGY_PA_RWQM8}
#define HYDROLOGY_P_FVWQ                {0x96, 11, 3, HYDROLOGY_PA_FVWQ}
#define HYDROLOGY_P_DISSS               {0x97, 0, 0, HYDROLOGY_PA_DISSS}
#define HYDROLOGY_P_TTPRFS              {0x98, 0, 0, HYDROLOGY_PA_TTPRFS}
#define HYDROLOGY_P_BVWM1               {0x99, 11, 3, HYDROLOGY_PA_BVWM1}
#define HYDROLOGY_P_BVWM2               {0x9A, 11, 3, HYDROLOGY_PA_BVWM2}
#define HYDROLOGY_P_BVWM3               {0x9B, 11, 3, HYDROLOGY_PA_BVWM3}
#define HYDROLOGY_P_BVWM4               {0x9C, 11, 3, HYDROLOGY_PA_BVWM4}
#define HYDROLOGY_P_BVWM5               {0x9D, 11, 3, HYDROLOGY_PA_BVWM5}
#define HYDROLOGY_P_BVWM6               {0x9E, 11, 3, HYDROLOGY_PA_BVWM6}
#define HYDROLOGY_P_BVWM7               {0x9F, 11, 3, HYDROLOGY_PA_BVWM7}
#define HYDROLOGY_P_BVWM8               {0xA0, 11, 3, HYDROLOGY_PA_BVWM8}
#define HYDROLOGY_P_WMRWAV1             {0xA1, 11, 3, HYDROLOGY_PA_WMRWAV1}
#define HYDROLOGY_P_WMRWAV2             {0xA2, 11, 3, HYDROLOGY_PA_WMRWAV2}
#define HYDROLOGY_P_WMRWAV3             {0xA3, 11, 3, HYDROLOGY_PA_WMRWAV3}
#define HYDROLOGY_P_WMRWAV4             {0xA4, 11, 3, HYDROLOGY_PA_WMRWAV4}
#define HYDROLOGY_P_WMRWAV5             {0xA5, 11, 3, HYDROLOGY_PA_WMRWAV5}
#define HYDROLOGY_P_WMRWAV6             {0xA6, 11, 3, HYDROLOGY_PA_WMRWAV6}
#define HYDROLOGY_P_WMRWAV7             {0xA7, 11, 3, HYDROLOGY_PA_WMRWAV7}
#define HYDROLOGY_P_WMRWAV8             {0xA8, 11, 3, HYDROLOGY_PA_WMRWAV8}

#define HYDROLOGY_PD_INIT_MARK          {0xA9, 10, 0, HYDROLOGY_PDA_INIT_MARK}
#define HYDROLOGY_PD_RTUTYPE            {0xAA, 2, 0, HYDROLOGY_PDA_RTUTYPE}
#define HYDROLOGY_PD_PERIOD_BT          {0xAB, 8, 0, HYDROLOGY_PDA_PERIOD_BT}
#define HYDROLOGY_PD_PERIOD_ET          {0xAC, 8, 0, HYDROLOGY_PDA_PERIOD_ET}
#define HYDROLOGY_PD_SW_VERSION_LEN     {0xAD, 2, 0, HYDROLOGY_PDA_SW_VERSION_LEN}
#define HYDROLOGY_PD_SW_VERSION         {0xAE, 40, 0, HYDROLOGY_PDA_SW_VERSION}
#define HYDROLOGY_PD_PUMP_LEN           {0xAF, 2, 0, HYDROLOGY_PDA_PUMP_LEN}
#define HYDROLOGY_PD_PUMP               {0xB0, 20, 0, HYDROLOGY_PDA_PUMP}
#define HYDROLOGY_PD_VALVE_LEN          {0xB1, 2, 0, HYDROLOGY_PDA_VALVE_LEN}
#define HYDROLOGY_PD_VALVE              {0xB2, 20, 0, HYDROLOGY_PDA_VALVE}
#define HYDROLOGY_PD_GATE_LEN           {0xB3, 2, 0, HYDROLOGY_PDA_GATE_LEN}
#define HYDROLOGY_PD_GATE               {0xB4, 20, 0, HYDROLOGY_PDA_GATE}
#define HYDROLOGY_PD_WATERSETTING       {0xB5, 2, 0, HYDROLOGY_PDA_WATERSETTING}
#define HYDROLOGY_PD_RECORD             {0xB6, 128, 0, HYDROLOGY_PDA_RECORD}
#define HYDROLOGY_PD_NEWPASSWORD        {0xB7, 4, 0, HYDROLOGY_PDA_NEWPASSWORD}

#ifdef __cplusplus
}
#endif

#endif /* __HYDROLOGY_IDENTIFIER_H */
