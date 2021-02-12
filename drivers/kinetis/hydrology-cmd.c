#include "kinetis/hydrology.h"
#include "kinetis/hydrology-config.h"
#include "kinetis/hydrology-identifier.h"
#include <string.h>
#include <linux/slab.h>

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  .
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "kinetis/idebug.h"

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

void Hydrology_ChangeMode(u8 M)
{
//    HYDROLOGY_MODE = M;
}

static int Hydrology_SendRealtimeData(void)
{
    HydrologyElementInfo Element_table[] = {
        HYDROLOGY_E_PJ,
        HYDROLOGY_E_PT,
        HYDROLOGY_E_Z,
        HYDROLOGY_E_VT
    };

    if (HydrologyD_ProcessSend(Element_table,
            sizeof(Element_table) / sizeof(HydrologyElementInfo),
            HYDROLOGY_M4, Realtime) == false)
        return false;

    return true;
}

static int Hydrology_SendPeriodData(void)
{
    HydrologyDownBody *downbody = (HydrologyDownBody *)g_Hydrology.downpacket->body;
    HydrologyElementInfo Element_table[] = {
        HYDROLOGY_E_DRxnn,
        NULL
    };

    Element_table[1].ID = downbody->element[1]->guide[0];
    Hydrology_ReadSpecifiedElementInfo(&Element_table[1], Period, Element_table[1].ID);

    if (HydrologyD_ProcessSend(Element_table,
            sizeof(Element_table) / sizeof(HydrologyElementInfo),
            HYDROLOGY_M4, Period) == false)
        return false;

    return true;
}

static int Hydrology_SendSpecifiedElement(void)
{
    HydrologyDownHeader *header = g_Hydrology.downpacket->header;
    HydrologyDownBody *downbody = (HydrologyDownBody *)g_Hydrology.downpacket->body;
    HydrologyElementInfo *Element_table;
    u8 i;
    int ret;

    if (downbody->count != 0) {
        Element_table =
            (HydrologyElementInfo *)kmalloc(sizeof(HydrologyElementInfo) * downbody->count, __GFP_ZERO);

        if (Element_table == NULL) {
            printk(KERN_DEBUG "Element_table malloc failed");
            return false;
        }
    }

    for (i = 0; i < downbody->count; i++) {
        Hydrology_ReadSpecifiedElementInfo(&Element_table[i], (HydrologyBodyType)header->funcode,
            downbody->element[i]->guide[0]);
    }

    ret = HydrologyD_ProcessSend(Element_table, downbody->count,
            HYDROLOGY_M4, SpecifiedElement);

    kfree(Element_table);

    return ret;
}

static int Hydrology_BasicInfoConfig(void)
{
    HydrologyDownHeader *header = g_Hydrology.downpacket->header;
    HydrologyDownBody *downbody = (HydrologyDownBody *)g_Hydrology.downpacket->body;
    HydrologyElementInfo *Element_table;
    u8 i;

    if (downbody->count != 0) {
        Element_table =
            (HydrologyElementInfo *)kmalloc(sizeof(HydrologyElementInfo) * downbody->count, __GFP_ZERO);

        if (Element_table == NULL) {
            printk(KERN_DEBUG "Element_table malloc failed");
            return false;
        }
    }

    for (i = 0; i < downbody->count; i++) {
        Hydrology_ReadSpecifiedElementInfo(&Element_table[i], (HydrologyBodyType)header->funcode,
            downbody->element[i]->guide[0]);

        Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, Element_table[i].Addr,
            downbody->element[i]->value, downbody->element[i]->guide[1] >> 3);
    }

    kfree(Element_table);

    return true;
}

static int Hydrology_BasicInfoRead(HydrologyBodyType Funcode)
{
    HydrologyDownHeader *header = g_Hydrology.downpacket->header;
    HydrologyDownBody *downbody = (HydrologyDownBody *)g_Hydrology.downpacket->body;
    HydrologyElementInfo *Element_table;
    u8 i;
    int ret;

    if (downbody->count != 0) {
        Element_table =
            (HydrologyElementInfo *)kmalloc(sizeof(HydrologyElementInfo) * downbody->count, __GFP_ZERO);

        if (Element_table == NULL) {
            printk(KERN_DEBUG "Element_table malloc failed");
            return false;
        }
    }

    for (i = 0; i < downbody->count; i++) {
        Hydrology_ReadSpecifiedElementInfo(&Element_table[i], (HydrologyBodyType)header->funcode,
            downbody->element[i]->guide[0]);
    }

    ret = HydrologyD_ProcessSend(Element_table, downbody->count,
            HYDROLOGY_M4, Funcode);

    kfree(Element_table);

    return ret;
}

static int Hydrology_SetParameter(void)
{
    HydrologyDownHeader *header = g_Hydrology.downpacket->header;
    HydrologyDownBody *downbody = (HydrologyDownBody *)g_Hydrology.downpacket->body;
    HydrologyElementInfo *Element_table;
    u8 i;

    if (downbody->count != 0) {
        Element_table =
            (HydrologyElementInfo *)kmalloc(sizeof(HydrologyElementInfo) * downbody->count, __GFP_ZERO);

        if (Element_table == NULL) {
            printk(KERN_DEBUG "Element_table malloc failed");
            return false;
        }
    }

    for (i = 0; i < downbody->count; i++) {
        Hydrology_ReadSpecifiedElementInfo(&Element_table[i], (HydrologyBodyType)header->funcode,
            downbody->element[i]->guide[0]);

        Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, Element_table[i].Addr,
            downbody->element[i]->value, downbody->element[i]->guide[1] >> 3);
    }

    kfree(Element_table);

    return true;
}

static int Hydrology_ReadParameter(HydrologyBodyType Funcode)
{
    HydrologyDownHeader *header = g_Hydrology.downpacket->header;
    HydrologyDownBody *downbody = (HydrologyDownBody *)g_Hydrology.downpacket->body;
    HydrologyElementInfo *Element_table;
    u8 i;
    int ret;

    if (downbody->count != 0) {
        Element_table =
            (HydrologyElementInfo *)kmalloc(sizeof(HydrologyElementInfo) * downbody->count, __GFP_ZERO);

        if (Element_table == NULL) {
            printk(KERN_DEBUG "Element_table malloc failed");
            return false;
        }
    }

    for (i = 0; i < downbody->count; i++) {
        Hydrology_ReadSpecifiedElementInfo(&Element_table[i], (HydrologyBodyType)header->funcode,
            downbody->element[i]->guide[0]);
    }

    ret = HydrologyD_ProcessSend(Element_table, downbody->count,
            HYDROLOGY_M4, Funcode);

    kfree(Element_table);

    return ret;
}

static int Hydrology_SendWaterPumpMotorData(void)
{
    HydrologyElementInfo Element_table[] = {
        HYDROLOGY_E_VTA,
        HYDROLOGY_E_VTB,
        HYDROLOGY_E_VTC,
        HYDROLOGY_E_VIA,
        HYDROLOGY_E_VIB,
        HYDROLOGY_E_VIC
    };

    if (HydrologyD_ProcessSend(Element_table,
            sizeof(Element_table) / sizeof(HydrologyElementInfo),
            HYDROLOGY_M4, WaterPumpMotor) == false)
        return false;

    return true;
}

static int Hydrology_SendStatusData(void)
{
    HydrologyElementInfo Element_table[] = {
        HYDROLOGY_E_ZT,
    };

    if (HydrologyD_ProcessSend(Element_table,
            sizeof(Element_table) / sizeof(HydrologyElementInfo),
            HYDROLOGY_M4, Status) == false)
        return false;

    return true;
}

static int Hydrology_InitializeSolidStorage(void)
{
    int ret;

//    f_unlink(HYDROLOGY_D_FILE_E_DATA);
//    f_unlink(HYDROLOGY_D_FILE_E_DATA);
//    f_unlink(HYDROLOGY_D_FILE_E_DATA);
//    f_unlink(HYDROLOGY_D_FILE_E_DATA);

    return ret;
}

int HydrologyD_Reset(void)
{
    int ret;
    u8 temp[256];
    u16 Data[200];
    u16 i, j;

    for (i = 0; i < 10; i++) {
        for (j = 0; j < 200; j++)
            Data[j] = j + i * 200;

        Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_PICTURE, i * 200, (u8 *)Data, 200);
    }

    for (i = 0; i < 2; i++) {
        for (j = 0; j < 200; j++)
            Data[j] = j + i * 200;

        Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_RGZS, i * 200, (u8 *)Data, 200);
    }

    temp[0] = 0x50;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_RTUTYPE, temp, 1);
    temp[0] = 0x01;
    temp[1] = 0x02;
    temp[2] = 0x03;
    temp[3] = 0x04;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_CENTER, temp, 4);
    temp[0] = 0x00;
    temp[1] = 0x12;
    temp[2] = 0x34;
    temp[3] = 0x56;
    temp[4] = 0x78;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_REMOTE, temp, 5);
    temp[0] = 0x12;
    temp[1] = 0x34;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_PASSWORD, temp, 2);
    temp[0] = 10;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BAL_CENTER1_IP, temp, 1);
    temp[0] = 0x02;
    temp[1] = 0x05;
    temp[2] = 0x80;
    temp[3] = 0x49;
    temp[4] = 0x14;
    temp[5] = 0x02;
    temp[6] = 0x02;
    temp[7] = 0x00;
    temp[8] = 0x89;
    temp[9] = 0x86;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_CENTER1_IP, temp, 10);
    temp[0] = 10;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BAL_BACKUP1_IP, temp, 1);
    temp[0] = 0x02;
    temp[1] = 0x18;
    temp[2] = 0x30;
    temp[3] = 0x92;
    temp[4] = 0x03;
    temp[5] = 0x30;
    temp[6] = 0x30;
    temp[7] = 0x00;
    temp[8] = 0x66;
    temp[9] = 0x66;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_BACKUP1_IP, temp, 10);
    temp[0] = 10;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BAL_CENTER2_IP, temp, 1);
    temp[0] = 0x02;
    temp[1] = 0x22;
    temp[2] = 0x21;
    temp[3] = 0x60;
    temp[4] = 0x24;
    temp[5] = 0x52;
    temp[6] = 0x06;
    temp[7] = 0x00;
    temp[8] = 0x66;
    temp[9] = 0x66;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_CENTER2_IP, temp, 10);
    temp[0] = 10;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BAL_BACKUP2_IP, temp, 1);
    temp[0] = 0x02;
    temp[1] = 0x12;
    temp[2] = 0x00;
    temp[3] = 0x78;
    temp[4] = 0x13;
    temp[5] = 0x91;
    temp[6] = 0x49;
    temp[7] = 0x00;
    temp[8] = 0x99;
    temp[9] = 0x99;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_BACKUP2_IP, temp, 10);
    temp[0] = 10;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BAL_CENTER3_IP, temp, 1);
    temp[0] = 0x02;
    temp[1] = 0x12;
    temp[2] = 0x00;
    temp[3] = 0x78;
    temp[4] = 0x13;
    temp[5] = 0x91;
    temp[6] = 0x49;
    temp[7] = 0x00;
    temp[8] = 0x99;
    temp[9] = 0x99;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_CENTER3_IP, temp, 10);
    temp[0] = 10;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BAL_BACKUP3_IP, temp, 1);
    temp[0] = 0x02;
    temp[1] = 0x12;
    temp[2] = 0x00;
    temp[3] = 0x78;
    temp[4] = 0x13;
    temp[5] = 0x91;
    temp[6] = 0x49;
    temp[7] = 0x00;
    temp[8] = 0x99;
    temp[9] = 0x99;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_BACKUP3_IP, temp, 10);
    temp[0] = 10;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BAL_CENTER4_IP, temp, 1);
    temp[0] = 0x02;
    temp[1] = 0x12;
    temp[2] = 0x00;
    temp[3] = 0x78;
    temp[4] = 0x13;
    temp[5] = 0x91;
    temp[6] = 0x49;
    temp[7] = 0x00;
    temp[8] = 0x99;
    temp[9] = 0x99;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_CENTER4_IP, temp, 10);
    temp[0] = 10;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BAL_BACKUP4_IP, temp, 1);
    temp[0] = 0x02;
    temp[1] = 0x12;
    temp[2] = 0x00;
    temp[3] = 0x78;
    temp[4] = 0x13;
    temp[5] = 0x91;
    temp[6] = 0x49;
    temp[7] = 0x00;
    temp[8] = 0x99;
    temp[9] = 0x99;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_BACKUP4_IP, temp, 10);
    temp[0] = 0x02;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_WORK_MODE, temp, 2);
    temp[0] = 0x80;
    temp[1] = 0x01;
    temp[2] = 0x06;
    temp[3] = 0x01;
    temp[4] = 0x00;
    temp[5] = 0x00;
    temp[6] = 0x00;
    temp[7] = 0x00;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_ELEMENT_SELECT, temp, 8);
    temp[0] = 0x00;
    temp[1] = 0x00;
    temp[2] = 0x00;
    temp[3] = 0x00;
    temp[4] = 0x00;
    temp[5] = 0x00;
    temp[6] = 0x00;
    temp[7] = 0x00;
    temp[8] = 0x00;
    temp[9] = 0x00;
    temp[10] = 0x00;
    temp[11] = 0x00;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_REPEATER_STATION, temp, 12);
    temp[0] = 12;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BAL_DEVICE_ID, temp, 1);
    temp[0] = '1';
    temp[1] = '1';
    temp[2] = '2';
    temp[3] = '3';
    temp[4] = '4';
    temp[5] = '5';
    temp[6] = '6';
    temp[7] = '7';
    temp[8] = '8';
    temp[9] = '9';
    temp[10] = '0';
    temp[11] = '1';
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_DEVICE_ID, temp, 12);
    temp[0] = 0x01;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PA_TI, temp, 1);
    temp[0] = 0x05;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PA_AI, temp, 1);
    temp[0] = 0x08;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PA_RBT, temp, 1);
    temp[0] = 0x03;
    temp[1] = 0x00;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PA_SI, temp, 2);
    temp[0] = 0x05;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PA_WSI, temp, 1);
    temp[0] = 0x05;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PA_RR, temp, 1);
    temp[0] = 0x01;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PA_WR, temp, 1);
    temp[0] = 0x01;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PA_RAT, temp, 1);
    temp[0] = 0x01;
    temp[1] = 0x00;
    temp[2] = 0x00;
    temp[3] = 0x00;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PA_WB1, temp, 4);
    temp[0] = 0x01;
    temp[1] = 0x00;
    temp[2] = 0x00;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PA_WC1, temp, 3);
    temp[0] = 0x25;
    temp[1] = 0x01;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PA_WC1, temp, 2);
    temp[0] = 0x01;
    temp[1] = 0x00;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PA_AAT, temp, 2);
    temp[0] = 0x03;
    temp[1] = 0x00;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PA_ABT, temp, 2);
    temp[0] = strlen("*WHU-2020-V3.0");
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_SW_VERSION_LEN, temp, 1);
    memcpy(temp, "*WHU-2020-V3.0", strlen("*WHU-2020-V3.0"));
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_SW_VERSION, temp,
        strlen("*WHU-2020-V3.0"));

    static HydrologyElementInfo Element_table[] = {
        HYDROLOGY_E_TT,
        HYDROLOGY_E_ST,
        HYDROLOGY_E_RGZS,
        HYDROLOGY_E_PIC,
        HYDROLOGY_E_DRP,
        HYDROLOGY_E_DRZ1,
        HYDROLOGY_E_DRZ2,
        HYDROLOGY_E_DRZ3,
        HYDROLOGY_E_DRZ4,
        HYDROLOGY_E_DRZ5,
        HYDROLOGY_E_DRZ6,
        HYDROLOGY_E_DRZ7,
        HYDROLOGY_E_DRZ8,
        HYDROLOGY_E_DATA,
        HYDROLOGY_E_AC,
        HYDROLOGY_E_AI,
        HYDROLOGY_E_C,
        HYDROLOGY_E_DRxnn,
        HYDROLOGY_E_DT,
        HYDROLOGY_E_ED,
        HYDROLOGY_E_EJ,
        HYDROLOGY_E_FL,
        HYDROLOGY_E_GH,
        HYDROLOGY_E_GN,
        HYDROLOGY_E_GS,
        HYDROLOGY_E_GT,
        HYDROLOGY_E_GTP,
        HYDROLOGY_E_H,
        HYDROLOGY_E_HW,
        HYDROLOGY_E_M10,
        HYDROLOGY_E_M20,
        HYDROLOGY_E_M30,
        HYDROLOGY_E_M40,
        HYDROLOGY_E_M50,
        HYDROLOGY_E_M60,
        HYDROLOGY_E_M80,
        HYDROLOGY_E_M100,
        HYDROLOGY_E_MST,
        HYDROLOGY_E_NS,
        HYDROLOGY_E_P1,
        HYDROLOGY_E_P2,
        HYDROLOGY_E_P3,
        HYDROLOGY_E_P6,
        HYDROLOGY_E_P12,
        HYDROLOGY_E_PD,
        HYDROLOGY_E_PJ,
        HYDROLOGY_E_PN01,
        HYDROLOGY_E_PN05,
        HYDROLOGY_E_PN10,
        HYDROLOGY_E_PN30,
        HYDROLOGY_E_PR,
        HYDROLOGY_E_PT,
        HYDROLOGY_E_Q,
        HYDROLOGY_E_Q1,
        HYDROLOGY_E_Q2,
        HYDROLOGY_E_Q3,
        HYDROLOGY_E_Q4,
        HYDROLOGY_E_Q5,
        HYDROLOGY_E_Q6,
        HYDROLOGY_E_Q7,
        HYDROLOGY_E_Q8,
        HYDROLOGY_E_QA,
        HYDROLOGY_E_QZ,
        HYDROLOGY_E_SW,
        HYDROLOGY_E_UC,
        HYDROLOGY_E_UE,
        HYDROLOGY_E_US,
        HYDROLOGY_E_VA,
        HYDROLOGY_E_VJ,
        HYDROLOGY_E_VT,
        HYDROLOGY_E_Z,
        HYDROLOGY_E_ZB,
        HYDROLOGY_E_ZU,
        HYDROLOGY_E_Z1,
        HYDROLOGY_E_Z2,
        HYDROLOGY_E_Z3,
        HYDROLOGY_E_Z4,
        HYDROLOGY_E_Z5,
        HYDROLOGY_E_Z6,
        HYDROLOGY_E_Z7,
        HYDROLOGY_E_Z8,
        HYDROLOGY_E_SQ,
        HYDROLOGY_E_ZT,
        HYDROLOGY_E_pH,
        HYDROLOGY_E_DO,
        HYDROLOGY_E_COND,
        HYDROLOGY_E_TURB,
        HYDROLOGY_E_CODMN,
        HYDROLOGY_E_REDOX,
        HYDROLOGY_E_NH4N,
        HYDROLOGY_E_TP,
        HYDROLOGY_E_TN,
        HYDROLOGY_E_TOC,
        HYDROLOGY_E_CU,
        HYDROLOGY_E_ZN,
        HYDROLOGY_E_SE,
        HYDROLOGY_E_AS,
        HYDROLOGY_E_THG,
        HYDROLOGY_E_CD,
        HYDROLOGY_E_PB,
        HYDROLOGY_E_CHLA,
        HYDROLOGY_E_WP1,
        HYDROLOGY_E_WP2,
        HYDROLOGY_E_WP3,
        HYDROLOGY_E_WP4,
        HYDROLOGY_E_WP5,
        HYDROLOGY_E_WP6,
        HYDROLOGY_E_WP7,
        HYDROLOGY_E_WP8,
        HYDROLOGY_E_SYL1,
        HYDROLOGY_E_SYL2,
        HYDROLOGY_E_SYL3,
        HYDROLOGY_E_SYL4,
        HYDROLOGY_E_SYL5,
        HYDROLOGY_E_SYL6,
        HYDROLOGY_E_SYL7,
        HYDROLOGY_E_SYL8,
        HYDROLOGY_E_SBL1,
        HYDROLOGY_E_SBL2,
        HYDROLOGY_E_SBL3,
        HYDROLOGY_E_SBL4,
        HYDROLOGY_E_SBL5,
        HYDROLOGY_E_SBL6,
        HYDROLOGY_E_SBL7,
        HYDROLOGY_E_SBL8,
        HYDROLOGY_E_VTA,
        HYDROLOGY_E_VTB,
        HYDROLOGY_E_VTC,
        HYDROLOGY_E_VIA,
        HYDROLOGY_E_VIB,
        HYDROLOGY_E_VIC,

        HYDROLOGY_B_CENTER,
        HYDROLOGY_B_REMOTE,
        HYDROLOGY_B_PASSWORD,
        HYDROLOGY_B_CENTER1_IP,
        HYDROLOGY_B_BACKUP1_IP,
        HYDROLOGY_B_CENTER2_IP,
        HYDROLOGY_B_BACKUP2_IP,
        HYDROLOGY_B_CENTER3_IP,
        HYDROLOGY_B_BACKUP3_IP,
        HYDROLOGY_B_CENTER4_IP,
        HYDROLOGY_B_BACKUP4_IP,
        HYDROLOGY_B_WORK_MODE,
        HYDROLOGY_B_ELEMENT_SELECT,
        HYDROLOGY_B_REPEATER_STATION,
        HYDROLOGY_B_DEVICE_ID,

        HYDROLOGY_P_TI,
        HYDROLOGY_P_AI,
        HYDROLOGY_P_RBT,
        HYDROLOGY_P_SI,
        HYDROLOGY_P_WSI,
        HYDROLOGY_P_RR,
        HYDROLOGY_P_WR,
        HYDROLOGY_P_RAT,
        HYDROLOGY_P_WB1,
        HYDROLOGY_P_WB2,
        HYDROLOGY_P_WB3,
        HYDROLOGY_P_WB4,
        HYDROLOGY_P_WB5,
        HYDROLOGY_P_WB6,
        HYDROLOGY_P_WB7,
        HYDROLOGY_P_WB8,
        HYDROLOGY_P_WC1,
        HYDROLOGY_P_WC2,
        HYDROLOGY_P_WC3,
        HYDROLOGY_P_WC4,
        HYDROLOGY_P_WC5,
        HYDROLOGY_P_WC6,
        HYDROLOGY_P_WC7,
        HYDROLOGY_P_WC8,
        HYDROLOGY_P_AW1,
        HYDROLOGY_P_AW2,
        HYDROLOGY_P_AW3,
        HYDROLOGY_P_AW4,
        HYDROLOGY_P_AW5,
        HYDROLOGY_P_AW6,
        HYDROLOGY_P_AW7,
        HYDROLOGY_P_AW8,
        HYDROLOGY_P_AAT,
        HYDROLOGY_P_ABT,
        HYDROLOGY_P_TAT,
        HYDROLOGY_P_FRAT,
        HYDROLOGY_P_GPAT,
        HYDROLOGY_P_PPT,
        HYDROLOGY_P_BAT,
        HYDROLOGY_P_WSWT,
        HYDROLOGY_P_WTAT,
        HYDROLOGY_P_UWLI1,
        HYDROLOGY_P_LWLI1,
        HYDROLOGY_P_UWLI2,
        HYDROLOGY_P_LWLI2,
        HYDROLOGY_P_UWLI3,
        HYDROLOGY_P_LWLI3,
        HYDROLOGY_P_UWLI4,
        HYDROLOGY_P_LWLI4,
        HYDROLOGY_P_UWLI5,
        HYDROLOGY_P_LWLI5,
        HYDROLOGY_P_UWLI6,
        HYDROLOGY_P_LWLI6,
        HYDROLOGY_P_UWLI7,
        HYDROLOGY_P_LWLI7,
        HYDROLOGY_P_UWLI8,
        HYDROLOGY_P_LWLI8,
        HYDROLOGY_P_ULWPI1,
        HYDROLOGY_P_LLWPI1,
        HYDROLOGY_P_ULWPI2,
        HYDROLOGY_P_LLWPI2,
        HYDROLOGY_P_ULWPI3,
        HYDROLOGY_P_LLWPI3,
        HYDROLOGY_P_ULWPI4,
        HYDROLOGY_P_LLWPI4,
        HYDROLOGY_P_ULWPI5,
        HYDROLOGY_P_LLWPI5,
        HYDROLOGY_P_ULWPI6,
        HYDROLOGY_P_LLWPI6,
        HYDROLOGY_P_ULWPI7,
        HYDROLOGY_P_LLWPI7,
        HYDROLOGY_P_ULWPI8,
        HYDROLOGY_P_LLWPI8,
        HYDROLOGY_P_ULWT,
        HYDROLOGY_P_LLWT,
        HYDROLOGY_P_ULpHV,
        HYDROLOGY_P_LLpHV,
        HYDROLOGY_P_ULDO,
        HYDROLOGY_P_LLDO,
        HYDROLOGY_P_ULPI,
        HYDROLOGY_P_LLPI,
        HYDROLOGY_P_ULCO,
        HYDROLOGY_P_LLCO,
        HYDROLOGY_P_ULRP,
        HYDROLOGY_P_LLRP,
        HYDROLOGY_P_ULT,
        HYDROLOGY_P_LLT,
        HYDROLOGY_P_ULAN,
        HYDROLOGY_P_LLAN,
        HYDROLOGY_P_ULTN,
        HYDROLOGY_P_LLTN,
        HYDROLOGY_P_ULC,
        HYDROLOGY_P_LLC,
        HYDROLOGY_P_ULZ,
        HYDROLOGY_P_LLZ,
        HYDROLOGY_P_ULF,
        HYDROLOGY_P_LLF,
        HYDROLOGY_P_ULS,
        HYDROLOGY_P_LLS,
        HYDROLOGY_P_ULA,
        HYDROLOGY_P_LLA,
        HYDROLOGY_P_ULM,
        HYDROLOGY_P_LLM,
        HYDROLOGY_P_ULCA,
        HYDROLOGY_P_LLCA,
        HYDROLOGY_P_ULTO,
        HYDROLOGY_P_LLTO,
        HYDROLOGY_P_ULCH,
        HYDROLOGY_P_LLCH,
        HYDROLOGY_P_ULFL,
        HYDROLOGY_P_RWQM1,
        HYDROLOGY_P_RWQM2,
        HYDROLOGY_P_RWQM3,
        HYDROLOGY_P_RWQM4,
        HYDROLOGY_P_RWQM5,
        HYDROLOGY_P_RWQM6,
        HYDROLOGY_P_RWQM7,
        HYDROLOGY_P_RWQM8,
        HYDROLOGY_P_FVWQ,
        HYDROLOGY_P_DISSS,
        HYDROLOGY_P_TTPRFS,
        HYDROLOGY_P_BVWM1,
        HYDROLOGY_P_BVWM2,
        HYDROLOGY_P_BVWM3,
        HYDROLOGY_P_BVWM4,
        HYDROLOGY_P_BVWM5,
        HYDROLOGY_P_BVWM6,
        HYDROLOGY_P_BVWM7,
        HYDROLOGY_P_BVWM8,
        HYDROLOGY_P_WMRWAV1,
        HYDROLOGY_P_WMRWAV2,
        HYDROLOGY_P_WMRWAV3,
        HYDROLOGY_P_WMRWAV4,
        HYDROLOGY_P_WMRWAV5,
        HYDROLOGY_P_WMRWAV6,
        HYDROLOGY_P_WMRWAV7,
        HYDROLOGY_P_WMRWAV8,

        HYDROLOGY_PD_INIT_MARK,
        HYDROLOGY_PD_RTUTYPE,
        HYDROLOGY_PD_PERIOD_BT,
        HYDROLOGY_PD_PERIOD_ET,
        HYDROLOGY_PD_SW_VERSION_LEN,
        HYDROLOGY_PD_SW_VERSION,
        HYDROLOGY_PD_PUMP_LEN,
        HYDROLOGY_PD_PUMP,
        HYDROLOGY_PD_VALVE_LEN,
        HYDROLOGY_PD_VALVE,
        HYDROLOGY_PD_GATE_LEN,
        HYDROLOGY_PD_GATE,
        HYDROLOGY_PD_WATERSETTING,
        HYDROLOGY_PD_RECORD,
        HYDROLOGY_PD_NEWPASSWORD,
    };

    ret = Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_INFO, 0, (u8 *)Element_table,
            sizeof(Element_table));

    return ret;
}

static int Hydrology_SetPassword(void)
{
    HydrologyDownBody *downbody = (HydrologyDownBody *)g_Hydrology.downpacket->body;

    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_PASSWORD,
        downbody->element[1]->value, downbody->element[1]->num);

    return true;
}

static int Hydrology_SetClock(void)
{
    HydrologyDownBody *downbody = (HydrologyDownBody *)g_Hydrology.downpacket->body;

    Hydrology_SetTime(downbody->sendtime);

    return true;
}

static int Hydrology_SetICCard(void)
{
    HydrologyDownBody *downbody = (HydrologyDownBody *)g_Hydrology.downpacket->body;

    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_EA_ZT,
        downbody->element[0]->value, downbody->element[0]->num);

    return true;
}

static int Hydrology_SetPump(void)
{
    HydrologyDownBody *downbody = (HydrologyDownBody *)g_Hydrology.downpacket->body;

    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_PUMP,
        downbody->element[0]->value, downbody->element[0]->num);

    return true;
}

static int Hydrology_SetValve(void)
{
    HydrologyDownBody *downbody = (HydrologyDownBody *)g_Hydrology.downpacket->body;

    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_VALVE,
        downbody->element[0]->value, downbody->element[0]->num);

    return true;
}

static int Hydrology_SetGate(void)
{
    char gatesize;
    HydrologyDownBody *downbody = (HydrologyDownBody *)g_Hydrology.downpacket->body;

    gatesize = downbody->element[0]->guide[0];
    gatesize = ((gatesize - 1) / 8 + 1) + 2 * gatesize + 1;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_GATE,
        downbody->element[0]->value, downbody->element[0]->num);

    return true;
}

static int Hydrology_SetWaterSetting(void)
{
    HydrologyDownBody *downbody = (HydrologyDownBody *)g_Hydrology.downpacket->body;

    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_WATERSETTING,
        downbody->element[0]->value, downbody->element[0]->num);

    return true;
}

static int Hydrology_RecordERC(int index)
{
    u16 ERC_Couter = 0;
    int addr = (index - 1) * 2;
    u8 _temp_ERC_Couter[2];

    Hydrology_read_store_info(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_RECORD + addr,
        _temp_ERC_Couter, 2);
    ERC_Couter = (_temp_ERC_Couter[0] << 8) + _temp_ERC_Couter[1];
    ERC_Couter++;
    _temp_ERC_Couter[0] = ERC_Couter >> 8;
    _temp_ERC_Couter[1] = ERC_Couter & 0x00FF;
    Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_RECORD + addr,
        _temp_ERC_Couter, 2);

    return true;
}

static int Hydrology_SendPassword(void)
{
    HydrologyElementInfo Element_table[] = {
        HYDROLOGY_B_PASSWORD,
    };

    if (HydrologyD_ProcessSend(Element_table,
            sizeof(Element_table) / sizeof(HydrologyElementInfo),
            HYDROLOGY_M4, ChangePassword) == false)
        return false;

    return true;
}

static int Hydrology_SendICCard(void)
{
    HydrologyElementInfo Element_table[] = {
        HYDROLOGY_E_ZT,
    };

    if (HydrologyD_ProcessSend(Element_table,
            sizeof(Element_table) / sizeof(HydrologyElementInfo),
            HYDROLOGY_M4, SetICCard) == false)
        return false;

    return true;
}

static int Hydrology_SendPump(void)
{
    HydrologyElementInfo Element_table[] = {
        HYDROLOGY_PD_PUMP,
    };

    if (HydrologyD_ProcessSend(Element_table,
            sizeof(Element_table) / sizeof(HydrologyElementInfo),
            HYDROLOGY_M4, Pump) == false)
        return false;

    return true;
}

static int Hydrology_SendValve(void)
{
    HydrologyElementInfo Element_table[] = {
        HYDROLOGY_PD_VALVE,
    };

    if (HydrologyD_ProcessSend(Element_table,
            sizeof(Element_table) / sizeof(HydrologyElementInfo),
            HYDROLOGY_M4, Valve) == false)
        return false;

    return true;
}

static int Hydrology_SendGate(void)
{
    HydrologyElementInfo Element_table[] = {
        HYDROLOGY_PD_GATE,
    };

    if (HydrologyD_ProcessSend(Element_table,
            sizeof(Element_table) / sizeof(HydrologyElementInfo),
            HYDROLOGY_M4, Gate) == false)
        return false;

    return true;
}

static int Hydrology_SendWaterSetting(void)
{
    HydrologyElementInfo Element_table[] = {
        HYDROLOGY_PD_WATERSETTING,
    };

    if (HydrologyD_ProcessSend(Element_table,
            sizeof(Element_table) / sizeof(HydrologyElementInfo),
            HYDROLOGY_M4, WaterSetting) == false)
        return false;

    return true;
}

static int Hydrology_SendRecordERC(void)
{
    HydrologyElementInfo Element_table[] = {
        HYDROLOGY_PD_RECORD,
    };

    if (HydrologyD_ProcessSend(Element_table,
            sizeof(Element_table) / sizeof(HydrologyElementInfo),
            HYDROLOGY_M4, Record) == false)
        return false;

    return true;
}

int Hydrology_ExecuteCommand(HydrologyBodyType Funcode)
{
    int ret = false;

    switch (Funcode) {
        case Test:
        case EvenPeriodInformation:
        case TimerReport:
        case AddReport:
        case Hour:
        case ArtificialNumber:
        case Picture:
        case Realtime:
        case Period:
        case InquireArtificialNumber:
        case SpecifiedElement:
        case ConfigurationRead:
        case ParameterRead:
        case WaterPumpMotor:
        case SoftwareVersion:
        case Status:
        case Record:
        case Time:
            break;

        case ConfigurationModification:
            Hydrology_BasicInfoConfig();
            break;

        case ParameterModification:
            Hydrology_SetParameter();
            Hydrology_RecordERC(ERC2);
            break;

        case InitializeSolidStorage:
            Hydrology_InitializeSolidStorage();
            Hydrology_RecordERC(ERC5);
            HydrologyD_Reboot();
            break;

        case Reset:
            HydrologyD_Reset();
            HydrologyD_Reboot();
            break;

        case ChangePassword:
            Hydrology_SetPassword();
            Hydrology_RecordERC(ERC5);
            break;

        case SetClock:
            Hydrology_SetClock();
            break;

        case SetICCard:
            Hydrology_SetICCard();
            break;

        case Pump:
            Hydrology_SetPump();
            break;

        case Valve:
            Hydrology_SetValve();
            break;

        case Gate:
            Hydrology_SetGate();
            break;

        case WaterSetting:
            Hydrology_SetWaterSetting();
            break;

        default:
            break;
    }

    return ret;
}

int Hydrology_ResponseDownstream(HydrologyBodyType Funcode)
{
    int ret = false;

    switch (Funcode) {
        case LinkMaintenance:
        case Test:
        case EvenPeriodInformation:
        case TimerReport:
        case AddReport:
        case Hour:
        case ArtificialNumber:
        case Picture:
            break;

        case Realtime:
            Hydrology_SendRealtimeData();
            break;

        case Period:
            Hydrology_SendPeriodData();
            break;

        case InquireArtificialNumber:
            return HydrologyD_ProcessSend(NULL, 0, HYDROLOGY_M4, InquireArtificialNumber);

        case SpecifiedElement:
            Hydrology_SendSpecifiedElement();
            break;

        case ConfigurationModification:
            Hydrology_BasicInfoRead(ConfigurationModification);
            break;

        case ConfigurationRead:
            Hydrology_BasicInfoRead(ConfigurationRead);
            break;

        case ParameterModification:
            Hydrology_ReadParameter(ParameterModification);
            break;

        case ParameterRead:
            Hydrology_ReadParameter(ParameterRead);
            break;

        case WaterPumpMotor:
            Hydrology_SendWaterPumpMotorData();
            break;

        case SoftwareVersion:
            return HydrologyD_ProcessSend(NULL, 0, HYDROLOGY_M4, SoftwareVersion);

        case Status:
            Hydrology_SendStatusData();
            break;

        case InitializeSolidStorage:
            return HydrologyD_ProcessSend(NULL, 0, HYDROLOGY_M4, InitializeSolidStorage);

        case Reset:
            return HydrologyD_ProcessSend(NULL, 0, HYDROLOGY_M4, Reset);

        case ChangePassword:
            Hydrology_SendPassword();
            break;

        case SetClock:
            return HydrologyD_ProcessSend(NULL, 0, HYDROLOGY_M4, SetClock);

        case SetICCard:
            Hydrology_SendICCard();
            break;

        case Pump:
            Hydrology_SendPump();
            break;

        case Valve:
            Hydrology_SendValve();
            break;

        case Gate:
            Hydrology_SendGate();
            break;

        case WaterSetting:
            Hydrology_SendWaterSetting();
            break;

        case Record:
            Hydrology_SendRecordERC();
            break;

        case Time:
            return HydrologyD_ProcessSend(NULL, 0, HYDROLOGY_M4, Time);
    }

    return ret;
}

int Hydrology_ResponseUpstream(HydrologyBodyType Funcode, u8 End)
{
    int ret = false;

    switch (Funcode) {
        case LinkMaintenance:
        case Realtime:
        case Period:
        case InquireArtificialNumber:
        case SpecifiedElement:
        case ConfigurationModification:
        case ConfigurationRead:
        case ParameterModification:
        case ParameterRead:
        case WaterPumpMotor:
        case SoftwareVersion:
        case Status:
        case InitializeSolidStorage:
        case Reset:
        case ChangePassword:
        case SetClock:
        case SetICCard:
        case WaterSetting:
        case Record:
        case Time:
            break;

        case Test:
            return HydrologyH_ProcessSend(NULL, 0, HYDROLOGY_M2, Test, End);

        case EvenPeriodInformation:
            return HydrologyH_ProcessSend(NULL, 0, HYDROLOGY_M2, EvenPeriodInformation, End);

        case TimerReport:
            return HydrologyH_ProcessSend(NULL, 0, HYDROLOGY_M2, TimerReport, End);

        case AddReport:
            return HydrologyH_ProcessSend(NULL, 0, HYDROLOGY_M2, AddReport, End);

        case Hour:
            return HydrologyH_ProcessSend(NULL, 0, HYDROLOGY_M2, Hour, End);

        case ArtificialNumber:
            return HydrologyH_ProcessSend(NULL, 0, HYDROLOGY_M2, ArtificialNumber, End);

        case Picture:
            return HydrologyH_ProcessSend(NULL, 0, HYDROLOGY_M2, Picture, End);

        case Pump:
            return HydrologyH_ProcessSend(NULL, 0, HYDROLOGY_M2, Pump, End);

        case Valve:
            return HydrologyH_ProcessSend(NULL, 0, HYDROLOGY_M2, Valve, End);

        case Gate:
            return HydrologyH_ProcessSend(NULL, 0, HYDROLOGY_M2, Gate, End);
    }

    return ret;
}

