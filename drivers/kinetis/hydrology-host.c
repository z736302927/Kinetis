#include "kinetis/hydrology.h"
#include "kinetis/hydrology-config.h"
#include "kinetis/hydrology-cmd.h"
#include "kinetis/hydrology-identifier.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "kinetis/memory.h"
#include "kinetis/basictimer.h"
#include <linux/delay.h>
#include <linux/crc16.h>

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "algorithm/crc.h"
#include "kinetis/idebug.h"

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

int HydrologyH_Reset(void)
{
    int ret;
    u8 temp[15];

    temp[0] = 0x50;
    ret = Hydrology_WriteStoreInfo(HYDROLOGY_H_FILE_E_DATA, HYDROLOGY_PDA_RTUTYPE, temp, 1);

    if (ret != true)
        return ret;

    temp[0] = 0x01;
    temp[1] = 0x02;
    temp[2] = 0x03;
    temp[3] = 0x04;
    ret = Hydrology_WriteStoreInfo(HYDROLOGY_H_FILE_E_DATA, HYDROLOGY_BA_CENTER, temp, 4);

    if (ret != true)
        return ret;

    temp[0] = 0x00;
    temp[1] = 0x12;
    temp[2] = 0x34;
    temp[3] = 0x56;
    temp[4] = 0x78;
    ret = Hydrology_WriteStoreInfo(HYDROLOGY_H_FILE_E_DATA, HYDROLOGY_BA_REMOTE, temp, 5);

    if (ret != true)
        return ret;

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

int HydrologyH_InitSend(u8 Count, HydrologyBodyType Funcode)
{
    int i = 0;
    HydrologyDownBody *downbody;

    g_Hydrology.downpacket =
        (HydrologyPacket *)kmalloc(sizeof(HydrologyPacket), __GFP_ZERO);

    if (g_Hydrology.downpacket == NULL) {
        kinetis_debug_trace(KERN_DEBUG, "g_Hydrology.downpacket malloc failed");
        return false;
    }

    g_Hydrology.downpacket->header = kmalloc(sizeof(HydrologyDownHeader), __GFP_ZERO);

    if (g_Hydrology.downpacket->header == NULL) {
        kinetis_debug_trace(KERN_DEBUG, "g_Hydrology.downpacket->header malloc failed");
        return false;
    }

    g_Hydrology.downpacket->body = kmalloc(sizeof(HydrologyDownBody), __GFP_ZERO);

    if (g_Hydrology.downpacket->body == NULL) {
        kinetis_debug_trace(KERN_DEBUG, "g_Hydrology.downpacket->body malloc failed");
        return false;
    }

    downbody = (HydrologyDownBody *)g_Hydrology.downpacket->body;
    downbody->count = Count;

    switch (Funcode) {
        case LinkMaintenance:
        case Test:
        case EvenPeriodInformation:
        case TimerReport:
        case AddReport:
        case Hour:
        case ArtificialNumber:
        case Picture:
        case Realtime:
        case InquireArtificialNumber:
        case SoftwareVersion:
        case Status:
        case SetClock:
        case Record:
        case Time:
            downbody->count = 0;
            break;

        case Period:
        case SpecifiedElement:
        case ConfigurationModification:
        case ConfigurationRead:
        case ParameterModification:
        case ParameterRead:
        case WaterPumpMotor:
        case ChangePassword:
            break;

        case InitializeSolidStorage:
        case Reset:
        case SetICCard:
        case Pump:
        case Valve:
        case Gate:
        case WaterSetting:
            downbody->count = 1;
            break;
    }

    if (downbody->count > 0) {
        downbody->element =
            (HydrologyElement **)kmalloc(sizeof(HydrologyElement *) * downbody->count, __GFP_ZERO);

        if (downbody->element == NULL) {
            kinetis_debug_trace(KERN_DEBUG, "downbody->element malloc failed");
            return false;
        }
    }

    for (i = 0; i < downbody->count; ++i) {
        downbody->element[i] = (HydrologyElement *)kmalloc(sizeof(HydrologyElement), __GFP_ZERO);

        if (downbody->element[i] == NULL) {
            kinetis_debug_trace(KERN_DEBUG, "downbody->element[%d] malloc failed", i);
            return false;
        }
    }

    return true;
}

void HydrologyH_ExitSend(void)
{
    int i = 0;
    HydrologyDownBody *downbody = (HydrologyDownBody *)g_Hydrology.downpacket->body;

    for (i = 0; i < downbody->count; i++) {
        if (downbody->element[i]->value != NULL) {
            kfree(downbody->element[i]->value);
            downbody->element[i]->value = NULL;
        }

        if (downbody->element[i] != NULL) {
            kfree(downbody->element[i]);
            downbody->element[i] = NULL;
        }
    }

    if (downbody->element != NULL) {
        kfree(downbody->element);
        downbody->element = NULL;
    }

    if (g_Hydrology.downpacket->header != NULL) {
        kfree(g_Hydrology.downpacket->header);
        g_Hydrology.downpacket->header = NULL;
    }

    if (g_Hydrology.downpacket->body != NULL) {
        kfree(g_Hydrology.downpacket->body);
        g_Hydrology.downpacket->body = NULL;
    }

    if (g_Hydrology.downpacket->buffer != NULL) {
        kfree(g_Hydrology.downpacket->buffer);
        g_Hydrology.downpacket->buffer = NULL;
    }

    if (g_Hydrology.downpacket != NULL) {
        kfree(g_Hydrology.downpacket);
        g_Hydrology.downpacket = NULL;
    }
}

int HydrologyH_InitReceieve()
{
    g_Hydrology.uppacket = (HydrologyPacket *)kmalloc(sizeof(HydrologyPacket), __GFP_ZERO);

    if (g_Hydrology.uppacket == NULL) {
        kinetis_debug_trace(KERN_DEBUG, "g_Hydrology.uppacket malloc failed");
        return false;
    }

    g_Hydrology.uppacket->header = (HydrologyUpHeader *)kmalloc(sizeof(HydrologyUpHeader), __GFP_ZERO);

    if (g_Hydrology.uppacket->header == NULL) {
        kinetis_debug_trace(KERN_DEBUG, "g_Hydrology.uppacket->header malloc failed");
        return false;
    }

    g_Hydrology.uppacket->body = kmalloc(sizeof(HydrologyUpBody), __GFP_ZERO);

    if (g_Hydrology.uppacket->body == NULL) {
        kinetis_debug_trace(KERN_DEBUG, "g_Hydrology.uppacket->body malloc failed");
        return false;
    }

    return true;
}

void HydrologyH_ExitReceieve()
{
    int i = 0;
    HydrologyUpBody *upbody = (HydrologyUpBody *)g_Hydrology.uppacket->body;

    for (i = 0; i < upbody->count; i++) {
        if (upbody->element[i]->value != NULL) {
            kfree(upbody->element[i]->value);
            upbody->element[i]->value = NULL;
        }

        if (upbody->element[i] != NULL) {
            kfree(upbody->element[i]);
            upbody->element[i] = NULL;
        }
    }

    if (upbody->element != NULL) {
        kfree(upbody->element);
        upbody->element = NULL;
    }

    if (g_Hydrology.uppacket->header != NULL) {
        kfree(g_Hydrology.uppacket->header);
        g_Hydrology.uppacket->header = NULL;
    }

    if (g_Hydrology.uppacket->body != NULL) {
        kfree(g_Hydrology.uppacket->body);
        g_Hydrology.uppacket->body = NULL;
    }

    if (g_Hydrology.uppacket->buffer != NULL) {
        kfree(g_Hydrology.uppacket->buffer);
        g_Hydrology.uppacket->buffer = NULL;
    }

    if (g_Hydrology.uppacket != NULL) {
        kfree(g_Hydrology.uppacket);
        g_Hydrology.uppacket = NULL;
    }
}

static void HydrologyH_SetDownHeaderSequence(u16 Count, u16 Total)
{
    HydrologyDownHeader *header = g_Hydrology.downpacket->header;

    header->count_seq[0] = Total >> 4;
    header->count_seq[1] = ((Total & 0x000F) << 4) + (Count >> 8);
    header->count_seq[2] = Count & 0x00FF;
}

static void HydrologyH_MakeDownHeader(HydrologyMode Mode, HydrologyBodyType Funcode)
{
    HydrologyDownHeader *header = (HydrologyDownHeader *)g_Hydrology.downpacket->header;

    header->framestart[0] = SOH;
    header->framestart[1] = SOH;
    header->len += 2;

    Hydrology_read_store_info(HYDROLOGY_H_FILE_E_DATA, HYDROLOGY_BA_REMOTE,
        header->remoteaddr, 5);
    header->len += 5;
    Hydrology_read_store_info(HYDROLOGY_H_FILE_E_DATA, HYDROLOGY_BA_CENTER,
        &(header->centeraddr), 1);
    header->len += 1;

    Hydrology_read_store_info(HYDROLOGY_H_FILE_E_DATA, HYDROLOGY_BA_PASSWORD,
        header->password, 2);
    header->len += 2;

    header->funcode = Funcode;
    header->len += 1;
    header->dir_len[0] = 8 << 4;
    header->len += 2;

    switch (Mode) {
        case HYDROLOGY_M1:
        case HYDROLOGY_M2:
        case HYDROLOGY_M4:
            header->paketstart = STX;
            header->len += 1;
            break;

        case HYDROLOGY_M3:
            header->paketstart = SYN;
            header->len += 1;
            break;
    }
}

static int HydrologyH_MakeDownBody(HydrologyElementInfo *Element_table,
    HydrologyMode Mode, HydrologyBodyType Funcode)
{
    HydrologyDownBody *downbody = (HydrologyDownBody *)g_Hydrology.downpacket->body;
    int i;

    downbody->len = 0;

    switch (Funcode) {
        case LinkMaintenance:
            break;

        case Test:
        case EvenPeriodInformation:
        case TimerReport:
        case AddReport:
        case Hour:
        case ArtificialNumber:
        case Picture:
        case Realtime:
        case InquireArtificialNumber:
        case WaterPumpMotor:
        case SoftwareVersion:
        case Status:
        case SetClock:
        case Record:
        case Time:
            Hydrology_GetStreamID(downbody->streamid);
            downbody->len += 2;
            Hydrology_ReadTime(downbody->sendtime);
            downbody->len += 6;
            break;

        case Period:
            downbody->len += 8;
            Hydrology_GetStreamID(downbody->streamid);
            downbody->len += 2;
            Hydrology_ReadTime(downbody->sendtime);
            downbody->len += 6;

            if (Hydrology_MallocElement(Element_table[0].ID,
                    Element_table[0].D, Element_table[0].d,
                    downbody->element[0]) == false)
                return false;

            Hydrology_read_store_info(HYDROLOGY_H_FILE_E_DATA, Element_table[0].Addr,
                downbody->element[0]->value, downbody->element[0]->num);
            downbody->len += downbody->element[0]->num + 2;

            downbody->element[1]->guide[0] = Element_table[1].ID;
            Hydrology_GetGuideID(&(downbody->element[1]->guide[1]),
                Element_table[1].D, Element_table[1].d);
            downbody->len += 2;
            break;

        case SpecifiedElement:
        case ConfigurationRead:
        case ParameterRead:
        case InitializeSolidStorage:
        case Reset:
            Hydrology_GetStreamID(downbody->streamid);
            downbody->len += 2;
            Hydrology_ReadTime(downbody->sendtime);
            downbody->len += 6;

            for (i = 0; i < downbody->count; i++) {
                downbody->element[i]->guide[0] = Element_table[i].ID;
                Hydrology_GetGuideID(&(downbody->element[i]->guide[1]),
                    Element_table[i].D, Element_table[i].d);

                downbody->len += 2;
            }

            break;

        case ConfigurationModification:
        case ParameterModification:
        case ChangePassword:
        case SetICCard:
            Hydrology_GetStreamID(downbody->streamid);
            downbody->len += 2;
            Hydrology_ReadTime(downbody->sendtime);
            downbody->len += 6;

            for (i = 0; i < downbody->count; i++) {
                if (Hydrology_MallocElement(Element_table[i].ID,
                        Element_table[i].D, Element_table[i].d,
                        downbody->element[i]) == false)
                    return false;

                Hydrology_read_store_info(HYDROLOGY_H_FILE_E_DATA, Element_table[i].Addr,
                    downbody->element[i]->value, downbody->element[i]->num);

                downbody->len += downbody->element[i]->num + 2;
            }

            break;

        case Pump:
            Hydrology_GetStreamID(downbody->streamid);
            downbody->len += 2;
            Hydrology_ReadTime(downbody->sendtime);
            downbody->len += 6;

            if (Mode == HYDROLOGY_M4) {
                Hydrology_read_store_info(HYDROLOGY_H_FILE_E_DATA, HYDROLOGY_PDA_PUMP_LEN,
                    downbody->element[0]->guide, 1);
                downbody->element[0]->num = downbody->element[0]->guide[0];
                downbody->element[0]->value =
                    (u8 *) kmalloc(downbody->element[0]->num, __GFP_ZERO);

                if (NULL == downbody->element[0]->value) {
                    kinetis_debug_trace(KERN_DEBUG, "downbody->element[0]->value malloc failed");
                    return false;
                }

                Hydrology_read_store_info(HYDROLOGY_H_FILE_E_DATA, HYDROLOGY_PDA_PUMP,
                    downbody->element[0]->value, downbody->element[0]->num);
                downbody->len += downbody->element[0]->num + 1;
            }

            break;

        case Valve:
            Hydrology_GetStreamID(downbody->streamid);
            downbody->len += 2;
            Hydrology_ReadTime(downbody->sendtime);
            downbody->len += 6;

            if (Mode == HYDROLOGY_M4) {
                Hydrology_read_store_info(HYDROLOGY_H_FILE_E_DATA, HYDROLOGY_PDA_VALVE_LEN,
                    downbody->element[0]->guide, 1);
                downbody->element[0]->num = downbody->element[0]->guide[0];
                downbody->element[0]->value =
                    (u8 *) kmalloc(downbody->element[0]->num, __GFP_ZERO);

                if (NULL == downbody->element[0]->value) {
                    kinetis_debug_trace(KERN_DEBUG, "downbody->element[0]->value malloc failed");
                    return false;
                }

                Hydrology_read_store_info(HYDROLOGY_H_FILE_E_DATA, HYDROLOGY_PDA_VALVE,
                    downbody->element[0]->value, downbody->element[0]->num);
                downbody->len += downbody->element[0]->num + 1;
            }

            break;

        case Gate:
            Hydrology_GetStreamID(downbody->streamid);
            downbody->len += 2;
            Hydrology_ReadTime(downbody->sendtime);
            downbody->len += 6;

            if (Mode == HYDROLOGY_M4) {
                Hydrology_read_store_info(HYDROLOGY_H_FILE_E_DATA, HYDROLOGY_PDA_GATE_LEN,
                    downbody->element[0]->guide, 1);

                if (downbody->element[0]->guide[0] % 8 == 0)
                    downbody->element[0]->num = downbody->element[0]->guide[0] / 8;
                else
                    downbody->element[0]->num = downbody->element[0]->guide[0] / 8 + 1;

                downbody->element[0]->value =
                    (u8 *) kmalloc(downbody->element[0]->num, __GFP_ZERO);

                if (NULL == downbody->element[0]->value) {
                    kinetis_debug_trace(KERN_DEBUG, "downbody->element[0]->value malloc failed");
                    return false;
                }

                Hydrology_read_store_info(HYDROLOGY_H_FILE_E_DATA, HYDROLOGY_PDA_GATE,
                    downbody->element[0]->value, downbody->element[0]->num);
                downbody->len += downbody->element[0]->num + 1;
            }

            break;

        case WaterSetting:
            Hydrology_GetStreamID(downbody->streamid);
            downbody->len += 2;
            Hydrology_ReadTime(downbody->sendtime);
            downbody->len += 6;
            downbody->element[0]->guide[0] = 1;
            downbody->element[0]->num = downbody->element[0]->guide[0];
            downbody->element[0]->value =
                (u8 *) kmalloc(downbody->element[0]->num, __GFP_ZERO);

            if (NULL == downbody->element[0]->value) {
                kinetis_debug_trace(KERN_DEBUG, "downbody->element[0]->value malloc failed");
                return false;
            }

            Hydrology_read_store_info(HYDROLOGY_H_FILE_E_DATA, HYDROLOGY_PDA_WATERSETTING,
                downbody->element[0]->value, downbody->element[0]->num);
            downbody->len += downbody->element[0]->num;
            break;
    }

    return true;
}

static int HydrologyH_MakeDownTailandSend(HydrologyMode Mode,
    HydrologyBodyType Funcode, u8 End)
{
    u8 *buffer;
    u16 i;
    u16 buffer_size;
    u16 pointer;
    u8 stime[6] = {0, 0, 0, 0, 0, 0};
    u8 ctime[6] = {0, 0, 0, 0, 0, 0};
    HydrologyDownHeader *header = (HydrologyDownHeader *)g_Hydrology.downpacket->header;
    HydrologyDownBody *downbody = (HydrologyDownBody *)g_Hydrology.downpacket->body;

    buffer_size = header->len + downbody->len + 3;
    g_Hydrology.downpacket->buffer = (u8 *)kmalloc(buffer_size, __GFP_ZERO);

    if (g_Hydrology.downpacket->buffer == NULL) {
        kinetis_debug_trace(KERN_DEBUG, "g_Hydrology.downpacket->buffer malloc failed");
        return false;
    }

    buffer = g_Hydrology.downpacket->buffer;

    header->dir_len[0] |= (downbody->len) >> 8;
    header->dir_len[1] |= (downbody->len) & 0xFF;
    memcpy(buffer, header, sizeof(HydrologyDownHeader) - 4);
    pointer = sizeof(HydrologyDownHeader) - 4;

    switch (Funcode) {
        case LinkMaintenance:
            break;

        case Test:
        case EvenPeriodInformation:
        case TimerReport:
        case AddReport:
        case Hour:
        case ArtificialNumber:
        case Picture:
        case Realtime:
        case InquireArtificialNumber:
        case WaterPumpMotor:
        case SoftwareVersion:
        case Status:
        case SetClock:
        case Record:
        case Time:
            memcpy(&buffer[pointer], downbody, 8);
            pointer += 8;
            break;

        case Period:
            memcpy(&buffer[pointer], downbody, 8);
            pointer += 8;
            Hydrology_ReadTime(stime);
            memcpy(ctime, stime, 6);
            ctime[3]--;
            memcpy(&buffer[pointer], stime, 4);
            pointer += 4;
            memcpy(&buffer[pointer], ctime, 4);
            pointer += 4;

            memcpy(&buffer[pointer], downbody->element[0]->guide, 2);
            pointer += 2;
            memcpy(&buffer[pointer], downbody->element[0]->value, downbody->element[0]->num);
            pointer += downbody->element[0]->num;
            memcpy(&buffer[pointer], downbody->element[1]->guide, 2);
            pointer += 2;

            break;

        case SpecifiedElement:
        case ConfigurationRead:
        case ParameterRead:
        case InitializeSolidStorage:
        case Reset:
            memcpy(&buffer[pointer], downbody, 8);
            pointer += 8;

            for (i = 0; i < downbody->count; i++) {
                memcpy(&buffer[pointer], downbody->element[i]->guide, 2);
                pointer += 2;
            }

            break;

        case ConfigurationModification:
        case ParameterModification:
        case ChangePassword:
        case SetICCard:
            memcpy(&buffer[pointer], downbody, 8);
            pointer += 8;

            for (i = 0; i < downbody->count; i++) {
                memcpy(&buffer[pointer], downbody->element[i]->guide, 2);
                pointer += 2;
                memcpy(&buffer[pointer], downbody->element[i]->value, downbody->element[i]->num);
                pointer += downbody->element[i]->num;
            }

            break;

        case Pump:
        case Valve:
        case Gate:
            memcpy(&buffer[pointer], downbody, 8);
            pointer += 8;

            if (Mode == HYDROLOGY_M4) {
                memcpy(&buffer[pointer], downbody->element[0]->guide, 1);
                pointer += 1;
                memcpy(&buffer[pointer], downbody->element[0]->value, downbody->element[0]->num);
                pointer += downbody->element[0]->num;
            }

            break;

        case WaterSetting:
            memcpy(&buffer[pointer], downbody, 8);
            pointer += 8;
            memcpy(&buffer[pointer], downbody->element[0]->value, downbody->element[0]->num);
            pointer += downbody->element[0]->num;
            break;
    }

    g_Hydrology.downpacket->end = End;
    buffer[pointer] = g_Hydrology.downpacket->end;
    pointer += 1;

    g_Hydrology.downpacket->crc16 = crc16(0xFFFF, buffer, pointer);
    buffer[pointer] = g_Hydrology.downpacket->crc16 >> 8;
    pointer += 1;
    buffer[pointer] = g_Hydrology.downpacket->crc16 & 0xFF;
    pointer += 1;

    g_Hydrology.downpacket->len = pointer;

    if (Hydrology_PortTransmmitData(g_Hydrology.downpacket->buffer,
            g_Hydrology.downpacket->len) == false)
        return false;

    return true;
}

static int HydrologyH_MakeErrDownTailandSend(HydrologyMode Mode,
    HydrologyBodyType Funcode, u8 Err_Packet)
{
    u8 *buffer;
    u16 i;
    u16 buffer_size;
    u16 pointer;
    u8 stime[6] = {0, 0, 0, 0, 0, 0};
    u8 ctime[6] = {0, 0, 0, 0, 0, 0};
    HydrologyDownHeader *header = (HydrologyDownHeader *)g_Hydrology.downpacket->header;
    HydrologyDownBody *downbody = (HydrologyDownBody *)g_Hydrology.downpacket->body;

    buffer_size = header->len + downbody->len + 3;
    g_Hydrology.downpacket->buffer = (u8 *)kmalloc(buffer_size, __GFP_ZERO);

    if (g_Hydrology.downpacket->buffer == NULL) {
        kinetis_debug_trace(KERN_DEBUG, "g_Hydrology.downpacket->buffer malloc failed");
        return false;
    }

    buffer = g_Hydrology.downpacket->buffer;

    HydrologyH_SetDownHeaderSequence(Err_Packet, 0);

    header->dir_len[0] |= (downbody->len) >> 8;
    header->dir_len[1] |= (downbody->len) & 0xFF;
    memcpy(buffer, header, sizeof(HydrologyDownHeader) - 1);
    pointer = sizeof(HydrologyDownHeader) - 1;

    switch (Funcode) {
        case LinkMaintenance:
            break;

        case Test:
        case EvenPeriodInformation:
        case TimerReport:
        case AddReport:
        case Hour:
        case ArtificialNumber:
        case Picture:
        case Realtime:
        case InquireArtificialNumber:
        case WaterPumpMotor:
        case SoftwareVersion:
        case Status:
        case SetClock:
        case Record:
        case Time:
            memcpy(&buffer[pointer], downbody, 8);
            pointer += 8;
            break;

        case Period:
            memcpy(&buffer[pointer], downbody, 8);
            pointer += 8;
            Hydrology_ReadTime(stime);
            memcpy(ctime, stime, 6);
            ctime[3]--;
            memcpy(&buffer[pointer], stime, 4);
            pointer += 4;
            memcpy(&buffer[pointer], ctime, 4);
            pointer += 4;

            memcpy(&buffer[pointer], downbody->element[0]->guide, 2);
            pointer += 2;
            memcpy(&buffer[pointer], downbody->element[0]->value, downbody->element[0]->num);
            pointer += downbody->element[0]->num;
            memcpy(&buffer[pointer], downbody->element[1]->guide, 2);
            pointer += 2;

            break;

        case SpecifiedElement:
        case ConfigurationRead:
        case ParameterRead:
        case InitializeSolidStorage:
        case Reset:
            memcpy(&buffer[pointer], downbody, 8);
            pointer += 8;

            for (i = 0; i < downbody->count; i++) {
                memcpy(&buffer[pointer], downbody->element[i]->guide, 2);
                pointer += 2;
            }

            break;

        case ConfigurationModification:
        case ParameterModification:
        case ChangePassword:
        case SetICCard:
            memcpy(&buffer[pointer], downbody, 8);
            pointer += 8;

            for (i = 0; i < downbody->count; i++) {
                memcpy(&buffer[pointer], downbody->element[i]->guide, 2);
                pointer += 2;
                memcpy(&buffer[pointer], downbody->element[i]->value, downbody->element[i]->num);
                pointer += downbody->element[i]->num;
            }

            break;

        case Pump:
        case Valve:
        case Gate:
            memcpy(&buffer[pointer], downbody, 8);
            pointer += 8;

            if (Mode == HYDROLOGY_M4) {
                memcpy(&buffer[pointer], downbody->element[0]->guide, 1);
                pointer += 1;
                memcpy(&buffer[pointer], downbody->element[0]->value, downbody->element[0]->num);
                pointer += downbody->element[0]->num;
            }

            break;

        case WaterSetting:
            memcpy(&buffer[pointer], downbody, 8);
            pointer += 8;
            memcpy(&buffer[pointer], downbody->element[0]->value, downbody->element[0]->num);
            pointer += downbody->element[0]->num;
            break;
    }

    g_Hydrology.downpacket->end = NAK;
    buffer[pointer] = g_Hydrology.downpacket->end;
    pointer += 1;

    g_Hydrology.downpacket->crc16 = crc16(0xFFFF, buffer, pointer);
    buffer[pointer] = g_Hydrology.downpacket->crc16 >> 8;
    pointer += 1;
    buffer[pointer] = g_Hydrology.downpacket->crc16 & 0xFF;
    pointer += 1;

    g_Hydrology.downpacket->len = pointer;

    if (Hydrology_PortTransmmitData(g_Hydrology.downpacket->buffer,
            g_Hydrology.downpacket->len) == false)
        return false;

    return true;
}

int HydrologyH_ProcessSend(HydrologyElementInfo *Element_table, u8 Count,
    HydrologyMode Mode, HydrologyBodyType Funcode, u8 End)
{
    if (HydrologyH_InitSend(Count, Funcode) == false)
        return false;

    HydrologyH_MakeDownHeader(Mode, Funcode);

    if (HydrologyH_MakeDownBody(Element_table, Mode, Funcode) == false)
        return false;

    if (HydrologyH_MakeDownTailandSend(Mode, Funcode, End) == false)
        return false;

    HydrologyH_ExitSend();

    return true;
}

static int HydrologyH_CheckUpPacket(u8 *input, int inputlen)
{
    u16 crcRet = 0;
    u16 inputCrc = 0;
    u16 bodylen = 0;

    crcRet = crc16(0xFFFF, input, inputlen - 2);

    inputCrc = (input[inputlen - 2] << 8) | input[inputlen - 1];

    if (crcRet != inputCrc) {
        kinetis_debug_trace(KERN_DEBUG, "Device crc(0x%04x) != Host crc(0x%04x)",
            inputCrc, crcRet);
        kinetis_debug_trace(KERN_DEBUG, "CRC check failed !");
        return false;
    }

    if ((input[0] != SOH) || (input[1] != SOH)) {
        kinetis_debug_trace(KERN_DEBUG, "Device Frame head(0x%02x, 0x%02x) != Host Frame head(0x%02x, 0x%02x)",
            input[0], input[1], SOH, SOH);
        kinetis_debug_trace(KERN_DEBUG, "Frame head check failed !");
        return false;
    }

    bodylen = (input[11] & 0x0F) * 256 + input[12];

    if (bodylen != (inputlen - 17)) {
        kinetis_debug_trace(KERN_DEBUG, "Device length(0x%x) != Host length(0x%x)",
            bodylen, inputlen - 17);
        kinetis_debug_trace(KERN_DEBUG, "Hydrolog length check failed !");
        return false;
    }

    return true;
}

static int HydrologyH_MakeUpHeader(u8 *input, int inputlen, int *position, int *bodylen)
{
    HydrologyUpHeader *header = (HydrologyUpHeader *)g_Hydrology.uppacket->header;

    if (HydrologyH_CheckUpPacket(input, inputlen) != true) {
        kinetis_debug_trace(KERN_DEBUG, "Hydrology check fail !");
        return false;
    }

    memcpy(header->framestart, &input[*position], 2);
    *position += 2;

    memcpy(&(header->centeraddr), &input[*position], 1);
    *position += 1;

    memcpy(header->remoteaddr, &input[*position], 5);
    *position += 5;

    memcpy(header->password, &input[*position], 2);
    *position += 2;

    memcpy(&(header->funcode), &input[*position], 1);
    *position += 1;

    memcpy(header->dir_len, &input[*position], 1);
    header->dir_len[0] >>= 4;

    *bodylen = (input[*position] & 0x0F) * 256 + input[*position + 1];
    *position += 2;

    memcpy(&(header->paketstart), &input[*position], 1);
    *position += 1;

    if (header->paketstart == SYN) {
        memcpy(header->count_seq, &input[*position], 3);
        *position += 3;
    }

    return true;
}

static int HydrologyH_MakeUpBody(u8 *input, int len, int position,
    HydrologyMode Mode, HydrologyBodyType Funcode)
{
    u32 i, j, offset;
    int32_t tmp_len;
    u32 tmp_position;
    HydrologyUpBody *upbody = (HydrologyUpBody *)g_Hydrology.uppacket->body;

    memcpy(upbody->streamid, &input[position], 2);
    position += 2;
    len -= 2;

    memcpy(upbody->sendtime, &input[position], 6);
    position += 6;
    len -= 6;

    switch (Funcode) {
        case LinkMaintenance:
            break;

        case Test:
        case EvenPeriodInformation:
        case TimerReport:
        case AddReport:
        case Hour:
        case Picture:
        case Realtime:
        case Period:
        case SpecifiedElement:
        case WaterPumpMotor:
            memcpy(upbody->rtuaddrid, &input[position], 2);
            position += 2;
            len -= 2;

            memcpy(upbody->rtuaddr, &input[position], 5);
            position += 5;
            len -= 5;

            memcpy(&upbody->rtutype, &input[position], 1);
            position += 1;
            len -= 1;

            memcpy(upbody->observationtimeid, &input[position], 2);
            position += 2;
            len -= 2;

            memcpy(upbody->observationtime, &input[position], 5);
            position += 5;
            len -= 5;
            break;

        case ConfigurationModification:
        case ConfigurationRead:
        case ParameterModification:
        case ParameterRead:
        case SoftwareVersion:
        case Status:
        case InitializeSolidStorage:
        case Reset:
        case ChangePassword:
        case SetClock:
        case SetICCard:
        case Pump:
        case Valve:
        case Gate:
        case WaterSetting:
        case Record:
        case Time:
            memcpy(upbody->rtuaddrid, &input[position], 2);
            position += 2;
            len -= 2;

            memcpy(upbody->rtuaddr, &input[position], 5);
            position += 5;
            len -= 5;
            break;

        case ArtificialNumber:
        case InquireArtificialNumber:
            break;
    }

    upbody->count = 0;

    switch (Funcode) {
        case LinkMaintenance:
            break;

        case Test:
        case TimerReport:
        case AddReport:
        case Hour:
        case Realtime:
        case Period:
        case SpecifiedElement:
        case ConfigurationModification:
        case ParameterModification:
        case ConfigurationRead:
        case ParameterRead:
        case WaterPumpMotor:
        case Status:
        case ChangePassword:
        case SetICCard:
            tmp_len = len;
            tmp_position = position;

            while (tmp_len > 0) {
                offset = (input[tmp_position + 1] >> 3) + 2;
                tmp_position += offset;
                tmp_len -= offset;

                upbody->count++;
            }

            if (upbody->count == 0)
                return false;

            break;

        case EvenPeriodInformation:
            tmp_len = len;
            tmp_position = position;
            offset = (input[tmp_position + 1] >> 3) + 2;
            tmp_position += offset;

            for (i = 1; offset < tmp_len; i++) {
                offset += (input[tmp_position + 1] >> 3) + 2;
                tmp_position += 2;
            }

            upbody->count = i;

            if (upbody->count == 0)
                return false;

            break;

        case ArtificialNumber:
        case Picture:
        case InquireArtificialNumber:
        case SoftwareVersion:
        case Pump:
        case Valve:
        case Gate:
        case WaterSetting:
        case Record:
            upbody->count = 1;
            break;

        case InitializeSolidStorage:
        case Reset:
        case SetClock:
        case Time:
            break;
    }

    if (upbody->count > 0) {
        upbody->element = (HydrologyElement **)kmalloc(sizeof(HydrologyElement *) * upbody->count, __GFP_ZERO);

        if (upbody->element == NULL) {
            kinetis_debug_trace(KERN_DEBUG, "upbody->element malloc failed");
            return false;
        }
    }

    for (i = 0; i < upbody->count; ++i) {
        upbody->element[i] = (HydrologyElement *)kmalloc(sizeof(HydrologyElement), __GFP_ZERO);

        if (upbody->element[i] == NULL) {
            kinetis_debug_trace(KERN_DEBUG, "upbody->element[%d] malloc failed", i);
            return false;
        }
    }

    switch (Funcode) {
        case LinkMaintenance:
            break;

        case Test:
        case TimerReport:
        case AddReport:
        case Realtime:
        case Hour:
        case Period:
        case SpecifiedElement:
        case ConfigurationModification:
        case ConfigurationRead:
        case ParameterModification:
        case ParameterRead:
        case WaterPumpMotor:
        case Status:
        case ChangePassword:
        case SetICCard:
            for (i = 0; i < upbody->count; ++i) {
                memcpy(upbody->element[i]->guide, &input[position], 2);
                position += 2;
                len -= 2;

                upbody->element[i]->num =
                    (upbody->element[i]->guide[1] >> 3);
                upbody->element[i]->value =
                    (u8 *) kmalloc(upbody->element[i]->num, __GFP_ZERO);

                if (NULL == upbody->element[i]->value) {
                    kinetis_debug_trace(KERN_DEBUG, "upbody->element[%d]->value malloc failed", i);
                    return false;
                }

                memcpy(upbody->element[i]->value, &input[position],
                    upbody->element[i]->num);
                position += upbody->element[i]->num;
                len -= upbody->element[i]->num;
            }

            break;

        case EvenPeriodInformation:
            for (i = 0; i < upbody->count; ++i) {
                memcpy(upbody->element[i]->guide, &input[position], 2);
                position += 2;
                len -= 2;

                upbody->element[i]->num =
                    (upbody->element[i]->guide[1] >> 3);
                upbody->element[i]->value =
                    (u8 *) kmalloc(upbody->element[i]->num, __GFP_ZERO);

                if (NULL == upbody->element[i]->value) {
                    kinetis_debug_trace(KERN_DEBUG, "upbody->element[%d]->value malloc failed", i);
                    return false;
                }

                if (i == 0) {
                    memcpy(upbody->element[i]->guide, &input[position],
                        upbody->element[i]->num);
                    position += upbody->element[i]->num;
                    len -= upbody->element[i]->num;
                }
            }

            for (i = 0; i < 12; ++i) {
                for (j = 1; j < upbody->count; ++j) {
                    memcpy(&upbody->element[j]->value[upbody->element[j]->num / 12 * i],
                        &input[position], upbody->element[j]->num / 12);
                    position += upbody->element[j]->num / 12;
                    len -= upbody->element[j]->num / 12;
                }
            }

            break;

        case ArtificialNumber:
        case Picture:
        case InquireArtificialNumber:
            memcpy(upbody->element[0]->guide, &input[position], 2);
            position += 2;
            len -= 2;

            upbody->element[0]->num = len;
            upbody->element[0]->value =
                (u8 *) kmalloc(upbody->element[0]->num, __GFP_ZERO);

            if (NULL == upbody->element[0]->value) {
                kinetis_debug_trace(KERN_DEBUG, "upbody->element[0]->value malloc failed");
                return false;
            }

            memcpy(upbody->element[0]->value, &input[position],
                upbody->element[0]->num);
            position += upbody->element[0]->num;
            len -= upbody->element[0]->num;
            break;

        case SoftwareVersion:
        case Pump:
        case Valve:
            memcpy(upbody->element[0]->guide, &input[position], 1);
            position += 1;
            len -= 1;
            upbody->element[0]->num = upbody->element[0]->guide[0];
            upbody->element[0]->value =
                (u8 *) kmalloc(upbody->element[0]->num, __GFP_ZERO);

            if (NULL == upbody->element[0]->value) {
                kinetis_debug_trace(KERN_DEBUG, "upbody->element[0]->value malloc failed");
                return false;
            }

            memcpy(upbody->element[0]->value, &input[position],
                upbody->element[0]->num);
            position += upbody->element[0]->num;
            len -= upbody->element[0]->num;
            break;

        case InitializeSolidStorage:
        case Reset:
        case SetClock:
        case Time:
            break;

        case Gate:
            memcpy(upbody->element[0]->guide, &input[position], 1);
            position += 1;
            len -= 1;

            if (upbody->element[0]->guide[0] % 8 == 0)
                upbody->element[0]->num = upbody->element[0]->guide[0] / 8;
            else
                upbody->element[0]->num = upbody->element[0]->guide[0] / 8 + 1;

            upbody->element[0]->value =
                (u8 *) kmalloc(upbody->element[0]->num, __GFP_ZERO);

            if (NULL == upbody->element[0]->value) {
                kinetis_debug_trace(KERN_DEBUG, "upbody->element[0]->value malloc failed");
                return false;
            }

            memcpy(upbody->element[0]->value, &input[position],
                upbody->element[0]->num);
            position += upbody->element[0]->num;
            len -= upbody->element[0]->num;
            break;

        case WaterSetting:
            upbody->element[0]->num = 1;
            upbody->element[0]->value =
                (u8 *) kmalloc(upbody->element[0]->num, __GFP_ZERO);

            if (NULL == upbody->element[0]->value) {
                kinetis_debug_trace(KERN_DEBUG, "upbody->element[0]->value malloc failed");
                return false;
            }

            memcpy(upbody->element[0]->value, &input[position],
                upbody->element[0]->num);
            position += upbody->element[0]->num;
            len -= upbody->element[0]->num;
            break;

        case Record:
            upbody->element[0]->num = 64;
            upbody->element[0]->value =
                (u8 *) kmalloc(upbody->element[0]->num, __GFP_ZERO);

            if (NULL == upbody->element[0]->value) {
                kinetis_debug_trace(KERN_DEBUG, "upbody->element[0]->value malloc failed");
                return false;
            }

            memcpy(upbody->element[0]->value, &input[position],
                upbody->element[0]->num);
            position += upbody->element[0]->num;
            len -= upbody->element[0]->num;
            break;
    }

    return true;
}

void HydrologyH_GetPacketTypeString(HydrologyBodyType Funcode, char *Buffer)
{
    switch (Funcode) {
        case LinkMaintenance:
            memcpy(Buffer, "LinkMaintenance", strlen("LinkMaintenance"));
            break;

        case Test:
            memcpy(Buffer, "Test", strlen("Test"));
            break;

        case EvenPeriodInformation:
            memcpy(Buffer, "EvenPeriodInformation", strlen("EvenPeriodInformation"));
            break;

        case TimerReport:
            memcpy(Buffer, "TimerReport", strlen("TimerReport"));
            break;

        case AddReport:
            memcpy(Buffer, "AddReport", strlen("AddReport"));
            break;

        case Hour:
            memcpy(Buffer, "Hour", strlen("Hour"));
            break;

        case ArtificialNumber:
            memcpy(Buffer, "ArtificialNumber", strlen("ArtificialNumber"));
            break;

        case Picture:
            memcpy(Buffer, "Picture", strlen("Picture"));
            break;

        case InitializeSolidStorage:
            memcpy(Buffer, "InitializeSolidStorage", strlen("InitializeSolidStorage"));
            break;

        case Reset:
            memcpy(Buffer, "Reset", strlen("Reset"));
            break;

        case SetClock:
            memcpy(Buffer, "SetClock", strlen("SetClock"));
            break;

        case Time:
            memcpy(Buffer, "Time", strlen("Time"));
            break;

        case Realtime:
            memcpy(Buffer, "Realtime", strlen("Realtime"));
            break;

        case Period:
            memcpy(Buffer, "Period", strlen("Period"));
            break;

        case SpecifiedElement:
            memcpy(Buffer, "SpecifiedElement", strlen("SpecifiedElement"));
            break;

        case ConfigurationModification:
            memcpy(Buffer, "ConfigurationModification", strlen("ConfigurationModification"));
            break;

        case ConfigurationRead:
            memcpy(Buffer, "ConfigurationRead", strlen("ConfigurationRead"));
            break;

        case ParameterModification:
            memcpy(Buffer, "ParameterModification", strlen("ParameterModification"));
            break;

        case ParameterRead:
            memcpy(Buffer, "ParameterRead", strlen("ParameterRead"));
            break;

        case WaterPumpMotor:
            memcpy(Buffer, "WaterPumpMotor", strlen("WaterPumpMotor"));
            break;

        case InquireArtificialNumber:
            memcpy(Buffer, "InquireArtificialNumber", strlen("InquireArtificialNumber"));
            break;

        case SoftwareVersion:
            memcpy(Buffer, "SoftwareVersion", strlen("SoftwareVersion"));
            break;

        case Status:
            memcpy(Buffer, "Status", strlen("Status"));
            break;

        case ChangePassword:
            memcpy(Buffer, "ChangePassword", strlen("ChangePassword"));
            break;

        case SetICCard:
            memcpy(Buffer, "SetICCard", strlen("SetICCard"));
            break;

        case Pump:
            memcpy(Buffer, "Pump", strlen("Pump"));
            break;

        case Valve:
            memcpy(Buffer, "Valve", strlen("Valve"));
            break;

        case Gate:
            memcpy(Buffer, "Gate", strlen("Gate"));
            break;

        case WaterSetting:
            memcpy(Buffer, "WaterSetting", strlen("WaterSetting"));
            break;

        case Record:
            memcpy(Buffer, "Record", strlen("Record"));
            break;
    }
}

void HydrologyH_GetRTUTypeString(HydrologyRTUType Type, char *Buffer)
{
    switch (Type) {
        case Rainfall:
            memcpy(Buffer, "Rainfall", strlen("Rainfall"));
            break;

        case RiverCourse:
            memcpy(Buffer, "RiverCourse", strlen("RiverCourse"));
            break;

        case Reservoir:
            memcpy(Buffer, "Reservoir", strlen("Reservoir"));
            break;

        case GateDam:
            memcpy(Buffer, "GateDam", strlen("GateDam"));
            break;

        case PumpingStation:
            memcpy(Buffer, "PumpingStation", strlen("PumpingStation"));
            break;

        case Tide:
            memcpy(Buffer, "Tide", strlen("Tide"));
            break;

        case SoilMoisture:
            memcpy(Buffer, "SoilMoisture", strlen("SoilMoisture"));
            break;

        case Groundwater:
            memcpy(Buffer, "Groundwater", strlen("Groundwater"));
            break;

        case WaterQuality:
            memcpy(Buffer, "WaterQuality", strlen("WaterQuality"));
            break;

        case WaterIntake:
            memcpy(Buffer, "WaterIntake", strlen("WaterIntake"));
            break;

        case Outfall:
            memcpy(Buffer, "Outfall", strlen("Outfall"));
            break;
    }
}

int HydrologyH_PrintUpPacket(void)
{
    HydrologyUpHeader *header = (HydrologyUpHeader *)g_Hydrology.uppacket->header;
    HydrologyUpBody *upbody = (HydrologyUpBody *)g_Hydrology.uppacket->body;
    char type[30];
    u16 total, current;
    u16 i, j, k, cnt;
    u16 streamid;
    HydrologyElementInfo *Element_table;
    float value;
    char **pbuffer;
    u32 status_val;
    u16 record_val;
    char *version;

    kinetis_debug_trace(KERN_INFO, "Center Address@%02X", header->centeraddr);
    kinetis_debug_trace(KERN_INFO, "Remote Address@%02X%02X%02X%02X%02X",
        header->remoteaddr[0], header->remoteaddr[1], header->remoteaddr[2],
        header->remoteaddr[3], header->remoteaddr[4]);
    kinetis_debug_trace(KERN_INFO, "Password: %02X%02X",
        header->password[0], header->password[1]);
    memset(type, 0, sizeof(type));
    HydrologyH_GetPacketTypeString((HydrologyBodyType)header->funcode, type);
    kinetis_debug_trace(KERN_INFO, "Packet type: %s", type);

    if (header->dir_len[0] & 0x80)
        kinetis_debug_trace(KERN_INFO, "Downstream packet");
    else
        kinetis_debug_trace(KERN_INFO, "Upstream packet");

    total = (header->count_seq[0] << 4) + (header->count_seq[1] >> 4);
    current = (header->count_seq[1] & 0x0F) + header->count_seq[2];
    kinetis_debug_trace(KERN_INFO, "Total packet number: %u", total);
    kinetis_debug_trace(KERN_INFO, "Current packet number: %u", current);

    streamid = (upbody->streamid[0] << 8) + upbody->streamid[1];
    kinetis_debug_trace(KERN_INFO, "Stream ID: %u", streamid);
    kinetis_debug_trace(KERN_INFO, "Packet send time: 20%02X/%02X/%02X %02X:%02X:%02X",
        upbody->sendtime[0], upbody->sendtime[1], upbody->sendtime[2],
        upbody->sendtime[3], upbody->sendtime[4], upbody->sendtime[5]);

    switch ((HydrologyBodyType)header->funcode) {
        case LinkMaintenance:
        case ArtificialNumber:
        case InquireArtificialNumber:
            break;

        case ConfigurationModification:
        case ConfigurationRead:
        case ParameterModification:
        case ParameterRead:
        case SoftwareVersion:
        case Status:
        case InitializeSolidStorage:
        case Reset:
        case SetClock:
        case SetICCard:
        case Pump:
        case Valve:
        case Gate:
        case WaterSetting:
        case Record:
        case Time:
            kinetis_debug_trace(KERN_INFO, "RTU Address@%02X%02X%02X%02X%02X",
                upbody->rtuaddr[0], upbody->rtuaddr[1], upbody->rtuaddr[2],
                upbody->rtuaddr[3], upbody->rtuaddr[4]);
            break;

        default:
            kinetis_debug_trace(KERN_INFO, "RTU Address@%02X%02X%02X%02X%02X",
                upbody->rtuaddr[0], upbody->rtuaddr[1], upbody->rtuaddr[2],
                upbody->rtuaddr[3], upbody->rtuaddr[4]);
            memset(type, 0, sizeof(type));
            HydrologyH_GetRTUTypeString((HydrologyRTUType)upbody->rtutype, type);
            kinetis_debug_trace(KERN_INFO, "RTU type: %s", type);
            kinetis_debug_trace(KERN_INFO, "Element sample time: 20%02X/%02X/%02X %02X:%02X",
                upbody->observationtime[0], upbody->observationtime[1], upbody->observationtime[2],
                upbody->observationtime[3], upbody->observationtime[4]);
            kinetis_debug_trace(KERN_INFO, "Element count: %u", upbody->count);
            break;
    }

    if (upbody->count != 0) {
        Element_table =
            (HydrologyElementInfo *)kmalloc(sizeof(HydrologyElementInfo) * upbody->count, __GFP_ZERO);

        if (Element_table == NULL) {
            kinetis_debug_trace(KERN_DEBUG, "Element_table malloc failed");
            return false;
        }
    }

    switch ((HydrologyBodyType)header->funcode) {
        case LinkMaintenance:
        case InitializeSolidStorage:
        case Reset:
        case SetClock:
        case Time:
            break;

        case Test:
        case TimerReport:
        case AddReport:
        case Realtime:
        case SpecifiedElement:
        case ParameterModification:
        case ParameterRead:
        case WaterPumpMotor:
            pbuffer = kmalloc(sizeof(*pbuffer), __GFP_ZERO);

            if (NULL == pbuffer) {
                kinetis_debug_trace(KERN_DEBUG, "pbuffer malloc failed");
                return false;
            }

            for (i = 0; i < upbody->count; ++i) {
                Hydrology_ReadSpecifiedElementInfo(&Element_table[i], (HydrologyBodyType)header->funcode,
                    upbody->element[i]->guide[0]);
                kinetis_debug_trace(KERN_INFO, "Element[%u].ID: %02X, D: %u, d: %u, Addr@%08X",
                    i, Element_table[i].ID, Element_table[i].D, Element_table[i].d, Element_table[i].Addr);

                *pbuffer = kmalloc(upbody->element[i]->num, __GFP_ZERO);

                if (NULL == *pbuffer) {
                    kinetis_debug_trace(KERN_DEBUG, "*pbuffer malloc failed");
                    return false;
                }

                memcpy(*pbuffer, upbody->element[i]->value, upbody->element[i]->num);
                value = strtof(*pbuffer, pbuffer);
                kfree(*pbuffer);

                for (j = 0; j < Element_table[i].d; ++j)
                    value /= 10;

                kinetis_debug_trace(KERN_INFO, "Element[%u].value: %f", i, value);
            }

            kfree(pbuffer);

            break;

        case EvenPeriodInformation:
            Hydrology_ReadSpecifiedElementInfo(&Element_table[0], (HydrologyBodyType)header->funcode,
                upbody->element[0]->guide[0]);
            kinetis_debug_trace(KERN_INFO, "Element[0].ID: %02X, D: %u, d: %u, Addr@%08X",
                Element_table[0].ID, Element_table[0].D, Element_table[0].d, Element_table[0].Addr);
            kinetis_debug_trace(KERN_INFO, "Time step: %u:%u:%u",
                upbody->element[0]->value[0], upbody->element[0]->value[1], upbody->element[0]->value[2]);

            for (i = 1; i < upbody->count; ++i) {
                Hydrology_ReadSpecifiedElementInfo(&Element_table[i], (HydrologyBodyType)header->funcode,
                    upbody->element[i]->guide[0]);
                kinetis_debug_trace(KERN_INFO, "Element[%u].ID: %02X, D: %u, d: %u, Addr@%08X",
                    i, Element_table[i].ID, Element_table[i].D, Element_table[i].d, Element_table[i].Addr);

                kinetis_debug_trace(KERN_INFO, "Element[%u].value: ", i);

                for (j = 0, k = 0; j < 12; ++j, k += 2) {
                    if (upbody->element[i]->num == 12)
                        kinetis_debug_trace(KERN_INFO, "[%u]%02X", j, upbody->element[i]->value[j]);
                    else
                        kinetis_debug_trace(KERN_INFO, "[%u]%02X%02X", j,
                            upbody->element[i]->value[k], upbody->element[i]->value[k + 1]);
                }
            }

            break;

        case Hour:
            for (i = 0; i < upbody->count; ++i) {
                Hydrology_ReadSpecifiedElementInfo(&Element_table[i], (HydrologyBodyType)header->funcode,
                    upbody->element[i]->guide[0]);
                kinetis_debug_trace(KERN_INFO, "Element[%u].ID: %02X, D: %u, d: %u, Addr@%08X",
                    i, Element_table[i].ID, Element_table[i].D, Element_table[i].d, Element_table[i].Addr);

                kinetis_debug_trace(KERN_INFO, "Element[%u].value: ", i);

                for (j = 0; j < 12; ++j, k += 2) {
                    if (upbody->element[i]->num == 12)
                        kinetis_debug_trace(KERN_INFO, "[%u]%02X", j, upbody->element[i]->value[j]);
                    else
                        kinetis_debug_trace(KERN_INFO, "[%u]%02X%02X", j,
                            upbody->element[i]->value[k], upbody->element[i]->value[k + 1]);
                }
            }

            break;

        case ArtificialNumber:
        case Picture:
        case InquireArtificialNumber:
            break;

        case Period:
            kinetis_debug_trace(KERN_INFO, "Time step: %u:%u:%u",
                upbody->element[0]->value[0], upbody->element[0]->value[1], upbody->element[0]->value[2]);

            Hydrology_ReadSpecifiedElementInfo(&Element_table[1], (HydrologyBodyType)header->funcode,
                upbody->element[1]->guide[0]);
            kinetis_debug_trace(KERN_INFO, "Element[1].ID: %02X, D: %u, d: %u, Addr@%08X",
                Element_table[1].ID, Element_table[1].D, Element_table[1].d, Element_table[1].Addr);

            kinetis_debug_trace(KERN_INFO, "Element[1].value: ");

            for (j = 0; j < 12; ++j, k += 2) {
                if (upbody->element[1]->num == 12)
                    kinetis_debug_trace(KERN_INFO, "[%u]%02X", j, upbody->element[1]->value[j]);
                else
                    kinetis_debug_trace(KERN_INFO, "[%u]%02X%02X", j,
                        upbody->element[1]->value[k], upbody->element[1]->value[k + 1]);
            }

            break;

        case ConfigurationModification:
        case ConfigurationRead:
            for (i = 0; i < upbody->count; ++i) {
                Hydrology_ReadSpecifiedElementInfo(&Element_table[i], (HydrologyBodyType)header->funcode,
                    upbody->element[i]->guide[0]);
                kinetis_debug_trace(KERN_INFO, "Element[%u].ID: %02X, D: %u, d: %u, Addr@%08X",
                    i, Element_table[i].ID, Element_table[i].D, Element_table[i].d, Element_table[i].Addr);

                kinetis_debug_trace(KERN_INFO, "Element[%u].value: ", i);

                for (j = 0; j < upbody->element[i]->num; ++j)
                    printf("%02X", upbody->element[i]->value[j]);
            }

            break;

        case SoftwareVersion:
            version = kmalloc(upbody->element[0]->num + 1, __GFP_ZERO);
            memcpy(version, upbody->element[0]->value, upbody->element[0]->num);
            kinetis_debug_trace(KERN_INFO, "Software version: %s", version);
            kfree(version);
            break;

        case Status:
        case SetICCard:
            Hydrology_ReadSpecifiedElementInfo(&Element_table[0], (HydrologyBodyType)header->funcode,
                upbody->element[0]->guide[0]);
            kinetis_debug_trace(KERN_INFO, "Element[0].ID: %02X, D: %u, d: %u, Addr@%08X",
                Element_table[0].ID, Element_table[0].D, Element_table[0].d, Element_table[0].Addr);

            status_val = *((u32 *)upbody->element[0]->value);

            if (status_val & 0x0001)
                kinetis_debug_trace(KERN_INFO, "BIT[0]: 1, AC charging status: Power off");
            else
                kinetis_debug_trace(KERN_INFO, "BIT[0]: 0, AC charging status: Normal");

            if (status_val & 0x0002)
                kinetis_debug_trace(KERN_INFO, "BIT[1]: 1, Battery voltage status: Low power");
            else
                kinetis_debug_trace(KERN_INFO, "BIT[1]: 0, Battery voltage status: Normal");

            if (status_val & 0x0004)
                kinetis_debug_trace(KERN_INFO, "BIT[2]: 1, Water level over limit alarm status: Alert");
            else
                kinetis_debug_trace(KERN_INFO, "BIT[2]: 0, Water level over limit alarm status: Normal");

            if (status_val & 0x0008)
                kinetis_debug_trace(KERN_INFO, "BIT[3]: 1, Flow overrun alarm status: Alert");
            else
                kinetis_debug_trace(KERN_INFO, "BIT[3]: 0, Flow overrun alarm status: Normal");

            if (status_val & 0x0010)
                kinetis_debug_trace(KERN_INFO, "BIT[4]: 1, Water quality limit alarm status: Alert");
            else
                kinetis_debug_trace(KERN_INFO, "BIT[4]: 0, Water quality limit alarm status: Normal");

            if (status_val & 0x0020)
                kinetis_debug_trace(KERN_INFO, "BIT[5]: 1, Flow meter status: Broken");
            else
                kinetis_debug_trace(KERN_INFO, "BIT[5]: 0, Flow meter status: Normal");

            if (status_val & 0x0040)
                kinetis_debug_trace(KERN_INFO, "BIT[6]: 1, Water level meter status: Broken");
            else
                kinetis_debug_trace(KERN_INFO, "BIT[6]: 0, Water level meter status: Normal");

            if (status_val & 0x0080)
                kinetis_debug_trace(KERN_INFO, "BIT[7]: 1, Terminal box door status: Shut off");
            else
                kinetis_debug_trace(KERN_INFO, "BIT[7]: 0, Terminal box door status: Power on");

            if (status_val & 0x0100)
                kinetis_debug_trace(KERN_INFO, "BIT[8]: 1, Memory status: Abnormal");
            else
                kinetis_debug_trace(KERN_INFO, "BIT[8]: 0, Memory status: Normal");

            if (status_val & 0x0200)
                kinetis_debug_trace(KERN_INFO, "BIT[9]: 1, IC card function is effective: IC Card normal");
            else
                kinetis_debug_trace(KERN_INFO, "BIT[9]: 0, IC card function is effective: Shut off");

            if (status_val & 0x0400)
                kinetis_debug_trace(KERN_INFO, "BIT[10]: 1, Working state of water pump: Water pump power off");
            else
                kinetis_debug_trace(KERN_INFO, "BIT[10]: 0, Working state of water pump: Water pump power on");

            if (status_val & 0x0800)
                kinetis_debug_trace(KERN_INFO, "BIT[11]: 1, Remaining water alarm: Water yield overlimit");
            else
                kinetis_debug_trace(KERN_INFO, "BIT[11]: 0, Remaining water alarm: Water yield normal");

            break;

        case ChangePassword:
            Hydrology_ReadSpecifiedElementInfo(&Element_table[0], (HydrologyBodyType)header->funcode,
                upbody->element[0]->guide[0]);
            kinetis_debug_trace(KERN_INFO, "Element[0].ID: %02X, D: %u, d: %u, Addr@%08X",
                Element_table[0].ID, Element_table[0].D, Element_table[0].d, Element_table[0].Addr);
            kinetis_debug_trace(KERN_INFO, "New password: %02X%02X",
                upbody->element[0]->value[0], upbody->element[0]->value[1]);

            break;

        case Pump:
            kinetis_debug_trace(KERN_INFO, "Total count: %u", upbody->element[0]->guide[0] * 8);

            for (i = 0; i < upbody->element[0]->guide[0]; ++i) {
                for (j = 0; j < 8; ++j) {
                    if (upbody->element[0]->value[i] & (1 << j))
                        kinetis_debug_trace(KERN_INFO, "Pump[%u]: Open", i * 8 + j);
                    else
                        kinetis_debug_trace(KERN_INFO, "Pump[%u]: Close", i * 8 + j);
                }
            }

            break;

        case Valve:
            kinetis_debug_trace(KERN_INFO, "Total count: %u", upbody->element[0]->guide[0] * 8);

            for (i = 0; i < upbody->element[0]->guide[0]; ++i) {
                for (j = 0; j < 8; ++j) {
                    if (upbody->element[0]->value[i] & (1 << j))
                        kinetis_debug_trace(KERN_INFO, "Valve[%u]: Open", i * 8 + j);
                    else
                        kinetis_debug_trace(KERN_INFO, "Valve[%u]: Close", i * 8 + j);
                }
            }

            break;

        case Gate:
            kinetis_debug_trace(KERN_INFO, "Total count: %u", upbody->element[0]->guide[0]);

            if (upbody->element[0]->guide[0] % 8 == 0)
                cnt = upbody->element[0]->guide[0] / 8;
            else
                cnt = upbody->element[0]->guide[0] / 8 + 1;

            for (i = 0, k = 0; i < cnt; ++i) {
                for (j = 0; j < 8; ++j, ++k) {
                    if (k == upbody->element[0]->guide[0])
                        break;

                    if (upbody->element[0]->value[i] & (1 << j))
                        kinetis_debug_trace(KERN_INFO, "Gate[%u]: Open", k);
                    else
                        kinetis_debug_trace(KERN_INFO, "Gate[%u]: Close", k);
                }
            }

            break;

        case WaterSetting:
            if (upbody->element[0]->value[0])
                kinetis_debug_trace(KERN_INFO, "Water value: Enter");
            else
                kinetis_debug_trace(KERN_INFO, "Water value: Exit");

            break;

        case Record:
            record_val = (upbody->element[0]->value[0] << 8) + upbody->element[0]->value[1];
            kinetis_debug_trace(KERN_INFO, "ERC1: Historical data initialization record: %u", record_val);
            record_val = (upbody->element[0]->value[2] << 8) + upbody->element[0]->value[3];
            kinetis_debug_trace(KERN_INFO, "ERC2: Parameter change record: %u", record_val);
            record_val = (upbody->element[0]->value[4] << 8) + upbody->element[0]->value[5];
            kinetis_debug_trace(KERN_INFO, "ERC3: State quantity displacement record: %u", record_val);
            record_val = (upbody->element[0]->value[6] << 8) + upbody->element[0]->value[7];
            kinetis_debug_trace(KERN_INFO, "ERC4: Sensor and instrument fault record: %u", record_val);
            record_val = (upbody->element[0]->value[8] << 8) + upbody->element[0]->value[9];
            kinetis_debug_trace(KERN_INFO, "ERC5: Password modification record: %u", record_val);
            record_val = (upbody->element[0]->value[10] << 8) + upbody->element[0]->value[11];
            kinetis_debug_trace(KERN_INFO, "ERC6: Terminal fault record: %u", record_val);
            record_val = (upbody->element[0]->value[12] << 8) + upbody->element[0]->value[13];
            kinetis_debug_trace(KERN_INFO, "ERC7: AC power loss record: %u", record_val);
            record_val = (upbody->element[0]->value[14] << 8) + upbody->element[0]->value[15];
            kinetis_debug_trace(KERN_INFO, "ERC8: Low battery voltage alarm record: %u", record_val);
            record_val = (upbody->element[0]->value[16] << 8) + upbody->element[0]->value[17];
            kinetis_debug_trace(KERN_INFO, "ERC9: Illegal opening record of terminal box: %u", record_val);
            record_val = (upbody->element[0]->value[18] << 8) + upbody->element[0]->value[19];
            kinetis_debug_trace(KERN_INFO, "ERC10: Water pump fault record: %u", record_val);
            record_val = (upbody->element[0]->value[20] << 8) + upbody->element[0]->value[21];
            kinetis_debug_trace(KERN_INFO, "ERC11: The remaining water volume exceeds the limit alarm record: %u", record_val);
            record_val = (upbody->element[0]->value[22] << 8) + upbody->element[0]->value[23];
            kinetis_debug_trace(KERN_INFO, "ERC12: Water level over-limit alarm record: %u", record_val);
            record_val = (upbody->element[0]->value[24] << 8) + upbody->element[0]->value[25];
            kinetis_debug_trace(KERN_INFO, "ERC13: Water pressure limit alarm record: %u", record_val);
            record_val = (upbody->element[0]->value[26] << 8) + upbody->element[0]->value[27];
            kinetis_debug_trace(KERN_INFO, "ERC14: Water quality parameter exceeding limit alarm record: %u", record_val);
            record_val = (upbody->element[0]->value[28] << 8) + upbody->element[0]->value[29];
            kinetis_debug_trace(KERN_INFO, "ERC15: Data error record: %u", record_val);
            record_val = (upbody->element[0]->value[30] << 8) + upbody->element[0]->value[31];
            kinetis_debug_trace(KERN_INFO, "ERC16: Message record: %u", record_val);
            record_val = (upbody->element[0]->value[32] << 8) + upbody->element[0]->value[33];
            kinetis_debug_trace(KERN_INFO, "ERC17: Receive message record: %u", record_val);
            record_val = (upbody->element[0]->value[34] << 8) + upbody->element[0]->value[35];
            kinetis_debug_trace(KERN_INFO, "ERC18: Send message error record: %u", record_val);
            record_val = (upbody->element[0]->value[36] << 8) + upbody->element[0]->value[37];
            break;
    }

    return true;
}

int HydrologyH_ProcessReceieve(u8 *input, int inputlen, HydrologyMode Mode)
{
    HydrologyUpHeader *header = NULL;
    int i = 0, bodylen = 0;

    if (HydrologyH_InitReceieve() == false)
        return false;

    header = (HydrologyUpHeader *)g_Hydrology.uppacket->header;

    if (HydrologyH_MakeUpHeader(input, inputlen, &i, &bodylen) == false)
        return false;

    if (HydrologyH_MakeUpBody(input, bodylen, i, Mode, (HydrologyBodyType)header->funcode) == false)
        return false;

    if (HydrologyH_PrintUpPacket() == false)
        return false;

    g_Hydrology.uppacket->end = input[inputlen - 3];

    switch (g_Hydrology.uppacket->end) {
        case ETX:
            kinetis_debug_trace(KERN_DEBUG, "[ETX]Wait disconnecting...");
            Hydrology_DisableLinkPacket();

            switch (Mode) {
                case HYDROLOGY_M1:
                case HYDROLOGY_M4:
                    break;

                case HYDROLOGY_M2:
                case HYDROLOGY_M3:
                    Hydrology_ResponseUpstream((HydrologyBodyType)header->funcode, EOT);
                    break;
            }

            break;

        case ETB:
            kinetis_debug_trace(KERN_DEBUG, "[ETB]Stay connecting...");
            Hydrology_EnableLinkPacket();

            switch (Mode) {
                case HYDROLOGY_M1:
                case HYDROLOGY_M4:
                    break;

                case HYDROLOGY_M2:
                case HYDROLOGY_M3:
                    Hydrology_ResponseUpstream((HydrologyBodyType)header->funcode, ACK);
                    break;
            }

            break;

        default:
            kinetis_debug_trace(KERN_ERR, "Unknown end packet identifier");
            break;
    }

    kinetis_debug_trace(KERN_ERR, " ");

    HydrologyH_ExitReceieve();

    return true;
}

void HydrologyH_ProcessEndIdentifier(u8 End)
{
    switch (End) {
        case ETX:
            kinetis_debug_trace(KERN_DEBUG, "[ETX]Wait disconnecting...");
            Hydrology_DisableLinkPacket();
            break;

        case ETB:
            kinetis_debug_trace(KERN_DEBUG, "[ETB]Stay connecting...");
            Hydrology_EnableLinkPacket();
            break;

        default:
            kinetis_debug_trace(KERN_ERR, "Unknown end packet identifier");
            break;
    }
}

int HydrologyH_ProcessM3ErrPacket(HydrologyElementInfo *Element_table, u8 Count,
    HydrologyBodyType Funcode, u8 CErr, u16 Err_Packet)
{
    u8 **Data;
    u16 Len;

    if (HydrologyH_InitSend(Count, Funcode) == false)
        return false;

    HydrologyH_MakeDownHeader(HYDROLOGY_M3, Funcode);

    if (HydrologyH_MakeDownBody(Element_table, HYDROLOGY_M3, Funcode) == false)
        return false;

    if (HydrologyH_MakeErrDownTailandSend(HYDROLOGY_M3, Funcode, Err_Packet) == false)
        return false;

    HydrologyH_ExitSend();

    CErr++;

    if (Hydrology_PortReceiveData(Data, &Len, HYDROLOGY_D_PORT_TIMEOUT) == true) {
        if (HydrologyH_ProcessReceieve(*Data, Len, HYDROLOGY_M3) == true) {
            switch (Data[0][Len - 3]) {
                case EOT:
                    kinetis_debug_trace(KERN_DEBUG, "[EOT]Link is disconnecting");
                    Hydrology_DisconnectLink();
                    break;

                case ESC:
                    kinetis_debug_trace(KERN_DEBUG, "[ESC]Transfer is over, keep on live within 10 minutes");
                    Hydrology_EnableLinkPacket();
                    break;

                default:
                    kinetis_debug_trace(KERN_ERR, "Unknown end packet identifier");
                    break;
            }
        } else
            return false;
    } else {
        kinetis_debug_trace(KERN_DEBUG, "Receive data timeout, retry times %d.",
            CErr);

        if (CErr >= 3) {
            Hydrology_DisconnectLink();
            return false;
        }

        HydrologyH_ProcessM3ErrPacket(Element_table, Count, Funcode, CErr, Err_Packet);
    }

    return true;
}

int HydrologyH_ProcessM1M2(HydrologyMode Mode)
{
    u8 **Data;
    u16 Len;

    for (;;) {
        if (Hydrology_PortReceiveData(Data, &Len, HYDROLOGY_H_PORT_TIMEOUT) == true)
            HydrologyH_ProcessReceieve(*Data, Len, Mode);
        else {
            kinetis_debug_trace(KERN_DEBUG, "[Warning]Port is going to be closed.");
            return false;
        }
    }
}

int HydrologyH_ProcessM3(void)
{
    u8 CErr = 0;
    u8 **Data;
    u16 Len;
    u16 i, packet_cnt;
    u32 bit_map[128];

    memset(bit_map, 0, sizeof(bit_map));

    do {
        if (Hydrology_PortReceiveData(Data, &Len, HYDROLOGY_H_PORT_TIMEOUT) == true) {
            if (HydrologyH_ProcessReceieve(*Data, Len, HYDROLOGY_M3) == true)
                HydrologyH_ProcessEndIdentifier(Data[0][Len - 3]);
            else {
                bit_map[packet_cnt / 32] = 1 << (packet_cnt % 32);
                return false;
            }

            packet_cnt++;
        } else {
            kinetis_debug_trace(KERN_DEBUG, "Receive data timeout.");
            return false;
        }
    } while (Data[0][Len - 3] == ETB);

    for (i = 0; i < packet_cnt; i++) {
        if (bit_map[i / 32] & (1 << (i % 32))) {
            kinetis_debug_trace(KERN_DEBUG, "Packet %u error, request device to resend");
            HydrologyH_ProcessM3ErrPacket(NULL, 0, (HydrologyBodyType)Data[0][10], CErr, i + 1);
        }
    }

    return true;
}

int HydrologyH_ProcessM4(HydrologyElementInfo *Element_table, u8 Count,
    HydrologyBodyType Funcode)
{
    u8 **Data;
    u16 Len;

    if (HydrologyH_ProcessSend(Element_table, Count, HYDROLOGY_M4, Funcode, ENQ) == false)
        return false;

    do {
        if (Hydrology_PortReceiveData(Data, &Len, HYDROLOGY_H_PORT_TIMEOUT) == true) {
            if (HydrologyH_ProcessReceieve(*Data, Len, HYDROLOGY_M4) == true)
                HydrologyH_ProcessEndIdentifier(Data[0][Len - 3]);
            else
                return false;
        } else {
            kinetis_debug_trace(KERN_DEBUG, "Receive data timeout.");
            return false;
        }
    } while (Data[0][Len - 3] == ETB);

    return true;
}

int HydrologyH_Process(HydrologyElementInfo *Element_table, u8 Count,
    HydrologyMode Mode, HydrologyBodyType Funcode)
{
    u8 ret = false;

    switch (Mode) {
        case HYDROLOGY_M1:
            ret = HydrologyH_ProcessM1M2(HYDROLOGY_M1);
            break;

        case HYDROLOGY_M2:
            ret = HydrologyH_ProcessM1M2(HYDROLOGY_M2);
            break;

        case HYDROLOGY_M3:
            ret = HydrologyH_ProcessM3();
            break;

        case HYDROLOGY_M4:
            ret = HydrologyH_ProcessM4(Element_table, Count, Funcode);
            break;
    }

    return ret;
}

#ifdef DESIGN_VERIFICATION_HYDROLOGY
#include "kinetis/test.h"
#include "kinetis/rng.h"

int t_HydrologyH_M1M2M3(HydrologyMode Mode)
{
    g_Hydrology.source = MsgFormClient;

    return HydrologyH_Process(NULL, 0, Mode, (HydrologyBodyType)NULL);
}

int t_HydrologyH_RandomElement(HydrologyMode Mode, HydrologyBodyType Funcode)
{
    HydrologyElementInfo *Element_table;
    u8 s_guide[] = {0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC};
    u8 i, j, count, guide;
    int ret;

    switch (Funcode) {
        case LinkMaintenance:
        case Test:
        case EvenPeriodInformation:
        case TimerReport:
        case AddReport:
        case Hour:
        case ArtificialNumber:
        case Picture:
            ret = true;
            break;

        case Period:
            count = 2;
            Element_table =
                (HydrologyElementInfo *)kmalloc(sizeof(HydrologyElementInfo) * count, __GFP_ZERO);

            if (Element_table == NULL) {
                kinetis_debug_trace(KERN_DEBUG, "Element_table malloc failed");
                return false;
            }

            Hydrology_ReadSpecifiedElementInfo(&Element_table[0], Funcode, 0x04);
            j = Random_Get8bit() % (sizeof(s_guide) - 1);
            Hydrology_ReadSpecifiedElementInfo(&Element_table[1], Funcode, s_guide[j]);

            ret = HydrologyH_Process(Element_table, count, Mode, Funcode);

            kfree(Element_table);
            break;

        case SpecifiedElement:
            count = Random_Get8bit() % (117 - 100);

            if (count == 0)
                count = 1;

            Element_table =
                (HydrologyElementInfo *)kmalloc(sizeof(HydrologyElementInfo) * count, __GFP_ZERO);

            if (Element_table == NULL) {
                kinetis_debug_trace(KERN_DEBUG, "Element_table malloc failed");
                return false;
            }

            for (i = 0;;) {
                guide = Random_Get8bit() % 0x75;

                if (!guide)
                    continue;

                Hydrology_ReadSpecifiedElementInfo(&Element_table[i], Funcode, guide);

                if (i == count - 1)
                    break;
                else
                    i++;
            }

            ret = HydrologyH_Process(Element_table, count, Mode, Funcode);

            kfree(Element_table);
            break;

        case ConfigurationModification:
        case ConfigurationRead:
            count = Random_Get8bit() % 15;

            if (count == 0)
                count = 1;

            Element_table =
                (HydrologyElementInfo *)kmalloc(sizeof(HydrologyElementInfo) * count, __GFP_ZERO);

            if (Element_table == NULL) {
                kinetis_debug_trace(KERN_DEBUG, "Element_table malloc failed");
                return false;
            }

            for (i = 0;;) {
                guide = Random_Get8bit() % 0x0F;

                if (!guide)
                    continue;

                Hydrology_ReadSpecifiedElementInfo(&Element_table[i], Funcode, guide);

                if (i == count - 1)
                    break;
                else
                    i++;
            }

            ret = HydrologyH_Process(Element_table, count, Mode, Funcode);

            kfree(Element_table);
            break;

        case ParameterModification:
        case ParameterRead:
            count = Random_Get8bit() % (137 - 120);

            if (count == 0)
                count = 1;

            Element_table =
                (HydrologyElementInfo *)kmalloc(sizeof(HydrologyElementInfo) * count, __GFP_ZERO);

            if (Element_table == NULL) {
                kinetis_debug_trace(KERN_DEBUG, "Element_table malloc failed");
                return false;
            }

            for (i = 0;;) {
                guide = Random_Get8bit() % 0xA8;

                if (guide < 0x20)
                    continue;

                Hydrology_ReadSpecifiedElementInfo(&Element_table[i], Funcode, guide);

                if (i == count - 1)
                    break;
                else
                    i++;
            }

            ret = HydrologyH_Process(Element_table, count, Mode, Funcode);

            kfree(Element_table);
            break;

        case Status:
        case SetICCard:
            count = 1;
            Element_table =
                (HydrologyElementInfo *)kmalloc(sizeof(HydrologyElementInfo) * count, __GFP_ZERO);

            if (Element_table == NULL) {
                kinetis_debug_trace(KERN_DEBUG, "Element_table malloc failed");
                return false;
            }

            Hydrology_ReadSpecifiedElementInfo(&Element_table[0], Funcode, 0x45);

            ret = HydrologyH_Process(Element_table, count, Mode, Funcode);

            kfree(Element_table);
            break;

        case InitializeSolidStorage:
            count = 1;
            Element_table =
                (HydrologyElementInfo *)kmalloc(sizeof(HydrologyElementInfo) * count, __GFP_ZERO);

            if (Element_table == NULL) {
                kinetis_debug_trace(KERN_DEBUG, "Element_table malloc failed");
                return false;
            }

            Hydrology_ReadSpecifiedElementInfo(&Element_table[0], Funcode, 0x97);

            ret = HydrologyH_Process(Element_table, count, Mode, Funcode);

            kfree(Element_table);
            break;

        case Reset:
            count = 1;
            Element_table =
                (HydrologyElementInfo *)kmalloc(sizeof(HydrologyElementInfo) * count, __GFP_ZERO);

            if (Element_table == NULL) {
                kinetis_debug_trace(KERN_DEBUG, "Element_table malloc failed");
                return false;
            }

            Hydrology_ReadSpecifiedElementInfo(&Element_table[0], Funcode, 0x98);

            ret = HydrologyH_Process(Element_table, count, Mode, Funcode);

            kfree(Element_table);
            break;

        case ChangePassword:
            count = 2;
            Element_table =
                (HydrologyElementInfo *)kmalloc(sizeof(HydrologyElementInfo) * count, __GFP_ZERO);

            if (Element_table == NULL) {
                kinetis_debug_trace(KERN_DEBUG, "Element_table malloc failed");
                return false;
            }

            Hydrology_ReadSpecifiedElementInfo(&Element_table[0], Funcode, 0x03);
            Hydrology_ReadSpecifiedElementInfo(&Element_table[1], Funcode, 0xB7);

            ret = HydrologyH_Process(Element_table, count, Mode, Funcode);

            kfree(Element_table);
            break;

        case Realtime:
        case InquireArtificialNumber:
        case WaterPumpMotor:
        case SoftwareVersion:
        case SetClock:
        case Pump:
        case Valve:
        case Gate:
        case WaterSetting:
        case Record:
        case Time:
            ret = HydrologyH_Process(NULL, 0, Mode, Funcode);
            break;
    }

    return ret;
}

int t_HydrologyH_M4(void)
{
    if (t_HydrologyH_RandomElement(HYDROLOGY_M4, Realtime) == false)
        return false;

    if (t_HydrologyH_RandomElement(HYDROLOGY_M4, Period) == false)
        return false;

    if (t_HydrologyH_RandomElement(HYDROLOGY_M4, InquireArtificialNumber) == false)
        return false;

    if (t_HydrologyH_RandomElement(HYDROLOGY_M4, SpecifiedElement) == false)
        return false;

    if (t_HydrologyH_RandomElement(HYDROLOGY_M4, ConfigurationModification) == false)
        return false;

    if (t_HydrologyH_RandomElement(HYDROLOGY_M4, ConfigurationRead) == false)
        return false;

    if (t_HydrologyH_RandomElement(HYDROLOGY_M4, ParameterModification) == false)
        return false;

    if (t_HydrologyH_RandomElement(HYDROLOGY_M4, ParameterRead) == false)
        return false;

    if (t_HydrologyH_RandomElement(HYDROLOGY_M4, WaterPumpMotor) == false)
        return false;

    if (t_HydrologyH_RandomElement(HYDROLOGY_M4, SoftwareVersion) == false)
        return false;

    if (t_HydrologyH_RandomElement(HYDROLOGY_M4, Status) == false)
        return false;

    if (t_HydrologyH_RandomElement(HYDROLOGY_M4, InitializeSolidStorage) == false)
        return false;

    if (t_HydrologyH_RandomElement(HYDROLOGY_M4, Reset) == false)
        return false;

    if (t_HydrologyH_RandomElement(HYDROLOGY_M4, ChangePassword) == false)
        return false;

    if (t_HydrologyH_RandomElement(HYDROLOGY_M4, SetClock) == false)
        return false;

    if (t_HydrologyH_RandomElement(HYDROLOGY_M4, SetICCard) == false)
        return false;

    if (t_HydrologyH_RandomElement(HYDROLOGY_M4, Pump) == false)
        return false;

    if (t_HydrologyH_RandomElement(HYDROLOGY_M4, Valve) == false)
        return false;

    if (t_HydrologyH_RandomElement(HYDROLOGY_M4, Gate) == false)
        return false;

    if (t_HydrologyH_RandomElement(HYDROLOGY_M4, WaterSetting) == false)
        return false;

    if (t_HydrologyH_RandomElement(HYDROLOGY_M4, Record) == false)
        return false;

    if (t_HydrologyH_RandomElement(HYDROLOGY_M4, Time) == false)
        return false;

    return true;
}

#endif

