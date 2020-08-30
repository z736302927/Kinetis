#include "protocol/hydrology.h"
#include "protocol/hydrology-config.h"
#include "protocol/hydrology-cmd.h"
#include "protocol/hydrology-identifier.h"
#include "string.h"
#include "core/k-memory.h"
#include "timer/k-basictimer.h"
#include "timer/k-delay.h"
#include <linux/crc16.h>

/*The following program is modified by the user according to the hardware device, otherwise the driver cannot run.*/

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "core/idebug.h"

/*The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run.*/

int HydrologyD_InitSend(uint8_t Count, HydrologyBodyType Funcode)
{
    int i = 0;
    HydrologyUpBody *upbody;

    g_Hydrology.uppacket = (HydrologyPacket *)kmalloc(sizeof(HydrologyPacket), __GFP_ZERO);

    if(g_Hydrology.uppacket == NULL)
    {
        kinetis_debug_trace(KERN_DEBUG, "g_Hydrology.uppacket malloc failed");
        return false;
    }

    g_Hydrology.uppacket->header = (HydrologyUpHeader *)kmalloc(sizeof(HydrologyUpHeader), __GFP_ZERO);

    if(g_Hydrology.uppacket->header == NULL)
    {
        kinetis_debug_trace(KERN_DEBUG, "g_Hydrology.uppacket->header malloc failed");
        return false;
    }

    g_Hydrology.uppacket->body = kmalloc(sizeof(HydrologyUpBody), __GFP_ZERO);

    if(g_Hydrology.uppacket->body == NULL)
    {
        kinetis_debug_trace(KERN_DEBUG, "g_Hydrology.uppacket->body malloc failed");
        return false;
    }

    upbody = (HydrologyUpBody *)g_Hydrology.uppacket->body;
    upbody->count = Count;

    switch(Funcode)
    {
        case LinkMaintenance:
        case InitializeSolidStorage:
        case Reset:
        case SetClock:
        case Time:
            upbody->count = 0;
            break;

        case Test:
        case EvenPeriodInformation:
        case TimerReport:
        case AddReport:
        case Hour:
        case Realtime:
        case Period:
        case SpecifiedElement:
        case ConfigurationModification:
        case ConfigurationRead:
        case ParameterModification:
        case ParameterRead:
        case WaterPumpMotor:
            break;

        case ArtificialNumber:
        case Picture:
        case InquireArtificialNumber:
        case SoftwareVersion:
        case Status:
        case ChangePassword:
        case SetICCard:
        case Pump:
        case Valve:
        case Gate:
        case WaterSetting:
        case Record:
            upbody->count = 1;
            break;
    }

    if(upbody->count > 0)
    {
        upbody->element =
            (HydrologyElement **)kmalloc(sizeof(HydrologyElement *) * upbody->count, __GFP_ZERO);

        if(upbody->element == NULL)
        {
            kinetis_debug_trace(KERN_DEBUG, "upbody->element malloc failed");
            return false;
        }
    }

    for(i = 0; i < upbody->count; ++i)
    {
        upbody->element[i] = (HydrologyElement *)kmalloc(sizeof(HydrologyElement), __GFP_ZERO);

        if(upbody->element[i] == NULL)
        {
            kinetis_debug_trace(KERN_DEBUG, "upbody->element[%d] malloc failed", i);
            return false;
        }
    }

    return true;
}

void HydrologyD_ExitSend(void)
{
    int i = 0;
    HydrologyUpBody *upbody = (HydrologyUpBody *)g_Hydrology.uppacket->body;

    for(i = 0; i < upbody->count; i++)
    {
        if(upbody->element[i]->value != NULL)
        {
            kfree(upbody->element[i]->value);
            upbody->element[i]->value = NULL;
        }

        if(upbody->element[i] != NULL)
        {
            kfree(upbody->element[i]);
            upbody->element[i] = NULL;
        }
    }

    if(upbody->element != NULL)
    {
        kfree(upbody->element);
        upbody->element = NULL;
    }

    if(g_Hydrology.uppacket->header != NULL)
    {
        kfree(g_Hydrology.uppacket->header);
        g_Hydrology.uppacket->header = NULL;
    }

    if(g_Hydrology.uppacket->body != NULL)
    {
        kfree(g_Hydrology.uppacket->body);
        g_Hydrology.uppacket->body = NULL;
    }

    if(g_Hydrology.uppacket->buffer != NULL)
    {
        kfree(g_Hydrology.uppacket->buffer);
        g_Hydrology.uppacket->buffer = NULL;
    }

    if(g_Hydrology.uppacket != NULL)
    {
        kfree(g_Hydrology.uppacket);
        g_Hydrology.uppacket = NULL;
    }
}

int HydrologyD_InitReceieve()
{
    g_Hydrology.downpacket = (HydrologyPacket *)kmalloc(sizeof(HydrologyPacket), __GFP_ZERO);

    if(g_Hydrology.downpacket == NULL)
    {
        kinetis_debug_trace(KERN_DEBUG, "g_Hydrology.downpacket malloc failed");
        return false;
    }

    g_Hydrology.downpacket->header = (HydrologyDownHeader *)kmalloc(sizeof(HydrologyDownHeader), __GFP_ZERO);

    if(g_Hydrology.downpacket->header == NULL)
    {
        kinetis_debug_trace(KERN_DEBUG, "g_Hydrology.downpacket->header malloc failed");
        return false;
    }

    g_Hydrology.downpacket->body = kmalloc(sizeof(HydrologyDownBody), __GFP_ZERO);

    if(g_Hydrology.downpacket->body == NULL)
    {
        kinetis_debug_trace(KERN_DEBUG, "g_Hydrology.downpacket->body malloc failed");
        return false;
    }

    return true;
}

void HydrologyD_ExitReceieve()
{
    int i = 0;
    HydrologyDownBody *downbody = (HydrologyDownBody *)g_Hydrology.downpacket->body;

    for(i = 0; i < downbody->count; i++)
    {
        if(downbody->element[i]->value != NULL)
        {
            kfree(downbody->element[i]->value);
            downbody->element[i]->value = NULL;
        }

        if(downbody->element[i] != NULL)
        {
            kfree(downbody->element[i]);
            downbody->element[i] = NULL;
        }
    }

    if(downbody->element != NULL)
    {
        kfree(downbody->element);
        downbody->element = NULL;
    }

    if(g_Hydrology.downpacket->header != NULL)
    {
        kfree(g_Hydrology.downpacket->header);
        g_Hydrology.downpacket->header = NULL;
    }

    if(g_Hydrology.downpacket->body != NULL)
    {
        kfree(g_Hydrology.downpacket->body);
        g_Hydrology.downpacket->body = NULL;
    }

    if(g_Hydrology.downpacket->buffer != NULL)
    {
        kfree(g_Hydrology.downpacket->buffer);
        g_Hydrology.downpacket->buffer = NULL;
    }

    if(g_Hydrology.downpacket != NULL)
    {
        kfree(g_Hydrology.downpacket);
        g_Hydrology.downpacket = NULL;
    }
}

static void HydrologyD_SetUpHeaderSequence(uint16_t Count, uint16_t Total)
{
    HydrologyUpHeader *header = g_Hydrology.uppacket->header;

    header->count_seq[0] = Total >> 4;
    header->count_seq[1] = ((Total & 0x000F) << 4) + (Count >> 8);
    header->count_seq[2] = Count & 0x00FF;
}

static void HydrologyD_MakeUpHeader(HydrologyMode Mode, HydrologyBodyType Funcode)
{
    HydrologyUpHeader *header = g_Hydrology.uppacket->header;

    header->framestart[0] = SOH;
    header->framestart[1] = SOH;
    header->len += 2;

    Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_CENTER,
        &(header->centeraddr), 1);
    header->len += 1;
    Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_REMOTE,
        header->remoteaddr, 5);
    header->len += 5;

    Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_PASSWORD,
        header->password, 2);
    header->len += 2;

    header->funcode = Funcode;
    header->len += 1;
    header->dir_len[0] = 0 << 4;
    header->len += 2;

    switch(Mode)
    {
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

static int HydrologyD_MakeUpBody(HydrologyElementInfo *Element_table, HydrologyBodyType Funcode)
{
    HydrologyUpBody *upbody = (HydrologyUpBody *)g_Hydrology.uppacket->body;
    HydrologyElementInfo Element[1] = {HYDROLOGY_E_PIC};
    int i;

    upbody->len = 0;

    switch(Funcode)
    {
        case LinkMaintenance:
            Hydrology_GetStreamID(upbody->streamid);
            upbody->len += 2;
            Hydrology_ReadTime(upbody->sendtime);
            upbody->len += 6;
            break;

        case Test:
        case EvenPeriodInformation:
        case TimerReport:
        case AddReport:
        case Hour:
        case WaterPumpMotor:
        case Realtime:
        case Period:
        case SpecifiedElement:
            Hydrology_GetStreamID(upbody->streamid);
            upbody->len += 2;
            Hydrology_ReadTime(upbody->sendtime);
            upbody->len += 6;
            upbody->rtuaddrid[0] = 0xF1;
            upbody->rtuaddrid[1] = 0xF1;
            upbody->len += 2;
            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_REMOTE,
                upbody->rtuaddr, 5);
            upbody->len += 5;
            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_RTUTYPE,
                &(upbody->rtutype), 1);
            upbody->len += 1;
            upbody->observationtimeid[0] = 0xF0;
            upbody->observationtimeid[1] = 0xF0;
            upbody->len += 2;
            Hydrology_ReadObservationTime(&Element_table[0], upbody->observationtime);
            upbody->len += 5;

            for(i = 0; i < upbody->count; i++)
            {
                if(Hydrology_MallocElement(Element_table[i].ID,
                        Element_table[i].D, Element_table[i].d,
                        upbody->element[i]) == false)
                    return false;

                Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, Element_table[i].Addr,
                    upbody->element[i]->value, upbody->element[i]->num);

                upbody->len += upbody->element[i]->num + 2;
            }

            break;

        case ArtificialNumber:
        case InquireArtificialNumber:
            Hydrology_GetStreamID(upbody->streamid);
            upbody->len += 2;
            Hydrology_ReadTime(upbody->sendtime);
            upbody->len += 6;
            upbody->element[0]->guide[0] = 0xF2;
            upbody->element[0]->guide[1] = 0xF2;
            Hydrology_ReadFileSize(HYDROLOGY_D_FILE_RGZS, &(upbody->element[0]->num));
            upbody->len += upbody->element[0]->num + 2;
            break;

        case Picture:
            Hydrology_GetStreamID(upbody->streamid);
            upbody->len += 2;
            Hydrology_ReadTime(upbody->sendtime);
            upbody->len += 6;
            upbody->rtuaddrid[0] = 0xF1;
            upbody->rtuaddrid[1] = 0xF1;
            upbody->len += 2;
            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_REMOTE,
                upbody->rtuaddr, 5);
            upbody->len += 5;
            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_RTUTYPE,
                &(upbody->rtutype), 1);
            upbody->len += 1;
            upbody->observationtimeid[0] = 0xF0;
            upbody->observationtimeid[1] = 0xF0;
            upbody->len += 2;
            Hydrology_ReadObservationTime(Element, upbody->observationtime);
            upbody->len += 5;

            upbody->element[0]->guide[0] = 0xF3;
            upbody->element[0]->guide[1] = 0xF3;
            Hydrology_ReadFileSize(HYDROLOGY_D_FILE_PICTURE,
                &(upbody->element[0]->num));
            upbody->len += upbody->element[0]->num + 2;

            break;

        case Status:
        case ConfigurationModification:
        case ConfigurationRead:
        case ParameterModification:
        case ParameterRead:
            Hydrology_GetStreamID(upbody->streamid);
            upbody->len += 2;
            Hydrology_ReadTime(upbody->sendtime);
            upbody->len += 6;
            upbody->rtuaddrid[0] = 0xF1;
            upbody->rtuaddrid[1] = 0xF1;
            upbody->len += 2;
            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_REMOTE,
                upbody->rtuaddr, 5);
            upbody->len += 5;

            for(i = 0; i < upbody->count; i++)
            {
                if(Hydrology_MallocElement(Element_table[i].ID,
                        Element_table[i].D, Element_table[i].d,
                        upbody->element[i]) == false)
                    return false;

                Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, Element_table[i].Addr,
                    upbody->element[i]->value, upbody->element[i]->num);

                upbody->len += upbody->element[i]->num + 2;
            }

            break;

        case SoftwareVersion:
            Hydrology_GetStreamID(upbody->streamid);
            upbody->len += 2;
            Hydrology_ReadTime(upbody->sendtime);
            upbody->len += 6;
            upbody->rtuaddrid[0] = 0xF1;
            upbody->rtuaddrid[1] = 0xF1;
            upbody->len += 2;
            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_REMOTE,
                upbody->rtuaddr, 5);
            upbody->len += 5;
            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_SW_VERSION_LEN,
                upbody->element[0]->guide, 1);
            upbody->element[0]->num = upbody->element[0]->guide[0];
            upbody->element[0]->value =
                (uint8_t *) kmalloc(upbody->element[0]->num, __GFP_ZERO);

            if(NULL == upbody->element[0]->value)
            {
                kinetis_debug_trace(KERN_DEBUG, "upbody->element[0]->value malloc failed");
                return false;
            }

            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_SW_VERSION,
                upbody->element[0]->value, upbody->element[0]->num);
            upbody->len += upbody->element[0]->num + 1;
            break;

        case InitializeSolidStorage:
        case Reset:
        case SetClock:
            Hydrology_GetStreamID(upbody->streamid);
            upbody->len += 2;
            Hydrology_ReadTime(upbody->sendtime);
            upbody->len += 6;
            upbody->rtuaddrid[0] = 0xF1;
            upbody->rtuaddrid[1] = 0xF1;
            upbody->len += 2;
            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_REMOTE,
                upbody->rtuaddr, 5);
            upbody->len += 5;
            break;

        case ChangePassword:
        case SetICCard:
            Hydrology_GetStreamID(upbody->streamid);
            upbody->len += 2;
            Hydrology_ReadTime(upbody->sendtime);
            upbody->len += 6;
            upbody->rtuaddrid[0] = 0xF1;
            upbody->rtuaddrid[1] = 0xF1;
            upbody->len += 2;
            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_REMOTE,
                upbody->rtuaddr, 5);
            upbody->len += 5;

            if(Hydrology_MallocElement(Element_table[0].ID,
                    Element_table[0].D, Element_table[0].d,
                    upbody->element[0]) == false)
                return false;

            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, Element_table[0].Addr,
                upbody->element[0]->value, upbody->element[0]->num);

            upbody->len += upbody->element[0]->num + 2;
            break;

        case Pump:
            Hydrology_GetStreamID(upbody->streamid);
            upbody->len += 2;
            Hydrology_ReadTime(upbody->sendtime);
            upbody->len += 6;
            upbody->rtuaddrid[0] = 0xF1;
            upbody->rtuaddrid[1] = 0xF1;
            upbody->len += 2;
            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_REMOTE,
                upbody->rtuaddr, 5);
            upbody->len += 5;
            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_PUMP_LEN,
                upbody->element[0]->guide, 1);
            upbody->element[0]->num = upbody->element[0]->guide[0];
            upbody->element[0]->value =
                (uint8_t *) kmalloc(upbody->element[0]->num, __GFP_ZERO);

            if(NULL == upbody->element[0]->value)
            {
                kinetis_debug_trace(KERN_DEBUG, "upbody->element[0]->value malloc failed");
                return false;
            }

            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_PUMP,
                upbody->element[0]->value, upbody->element[0]->num);
            upbody->len += upbody->element[0]->num + 1;
            break;

        case Valve:
            Hydrology_GetStreamID(upbody->streamid);
            upbody->len += 2;
            Hydrology_ReadTime(upbody->sendtime);
            upbody->len += 6;
            upbody->rtuaddrid[0] = 0xF1;
            upbody->rtuaddrid[1] = 0xF1;
            upbody->len += 2;
            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_REMOTE,
                upbody->rtuaddr, 5);
            upbody->len += 5;
            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_VALVE_LEN,
                upbody->element[0]->guide, 1);
            upbody->element[0]->num = upbody->element[0]->guide[0];
            upbody->element[0]->value =
                (uint8_t *) kmalloc(upbody->element[0]->num, __GFP_ZERO);

            if(NULL == upbody->element[0]->value)
            {
                kinetis_debug_trace(KERN_DEBUG, "upbody->element[0]->value malloc failed");
                return false;
            }

            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_VALVE,
                upbody->element[0]->value, upbody->element[0]->num);
            upbody->len += upbody->element[0]->num + 1;
            break;

        case Gate:
            Hydrology_GetStreamID(upbody->streamid);
            upbody->len += 2;
            Hydrology_ReadTime(upbody->sendtime);
            upbody->len += 6;
            upbody->rtuaddrid[0] = 0xF1;
            upbody->rtuaddrid[1] = 0xF1;
            upbody->len += 2;
            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_REMOTE,
                upbody->rtuaddr, 5);
            upbody->len += 5;
            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_GATE_LEN,
                upbody->element[0]->guide, 1);

            if(upbody->element[0]->guide[0] % 8 == 0)
                upbody->element[0]->num = upbody->element[0]->guide[0] / 8;
            else
                upbody->element[0]->num = upbody->element[0]->guide[0] / 8 + 1;

            upbody->element[0]->value =
                (uint8_t *) kmalloc(upbody->element[0]->num, __GFP_ZERO);

            if(NULL == upbody->element[0]->value)
            {
                kinetis_debug_trace(KERN_DEBUG, "upbody->element[0]->value malloc failed");
                return false;
            }

            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_GATE,
                upbody->element[0]->value, upbody->element[0]->num);
            upbody->len += upbody->element[0]->num + 1;
            break;

        case WaterSetting:
            Hydrology_GetStreamID(upbody->streamid);
            upbody->len += 2;
            Hydrology_ReadTime(upbody->sendtime);
            upbody->len += 6;
            upbody->rtuaddrid[0] = 0xF1;
            upbody->rtuaddrid[1] = 0xF1;
            upbody->len += 2;
            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_REMOTE,
                upbody->rtuaddr, 5);
            upbody->len += 5;
            upbody->element[0]->guide[0] = 1;
            upbody->element[0]->num = upbody->element[0]->guide[0];
            upbody->element[0]->value =
                (uint8_t *) kmalloc(upbody->element[0]->num, __GFP_ZERO);

            if(NULL == upbody->element[0]->value)
            {
                kinetis_debug_trace(KERN_DEBUG, "upbody->element[0]->value malloc failed");
                return false;
            }

            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_WATERSETTING,
                upbody->element[0]->value, upbody->element[0]->num);
            upbody->len += upbody->element[0]->num;
            break;

        case Record:
            Hydrology_GetStreamID(upbody->streamid);
            upbody->len += 2;
            Hydrology_ReadTime(upbody->sendtime);
            upbody->len += 6;
            upbody->rtuaddrid[0] = 0xF1;
            upbody->rtuaddrid[1] = 0xF1;
            upbody->len += 2;
            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_REMOTE,
                upbody->rtuaddr, 5);
            upbody->len += 5;
            upbody->element[0]->guide[0] = 64;
            upbody->element[0]->num = upbody->element[0]->guide[0];
            upbody->element[0]->value =
                (uint8_t *) kmalloc(upbody->element[0]->num, __GFP_ZERO);

            if(NULL == upbody->element[0]->value)
            {
                kinetis_debug_trace(KERN_DEBUG, "upbody->element[0]->value malloc failed");
                return false;
            }

            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_RECORD,
                upbody->element[0]->value, upbody->element[0]->num);
            upbody->len += upbody->element[0]->num;
            break;

        case Time:
            Hydrology_GetStreamID(upbody->streamid);
            upbody->len += 2;
            Hydrology_ReadTime(upbody->sendtime);
            upbody->len += 6;
            upbody->rtuaddrid[0] = 0xF1;
            upbody->rtuaddrid[1] = 0xF1;
            upbody->len += 2;
            Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_BA_REMOTE,
                upbody->rtuaddr, 5);
            upbody->len += 5;
            break;
    }

    return true;
}

static int HydrologyD_SplitTransfer(uint8_t *body_buffer, uint8_t *buffer,
    HydrologyUpHeader *header, HydrologyUpBody *upbody, uint16_t i,  uint16_t total)
{
    uint16_t pointer;

    HydrologyD_SetUpHeaderSequence(i, total);

    header->dir_len[0] = 0;
    header->dir_len[1] = 0;

    if(i == total)
    {
        header->dir_len[0] |= (upbody->len % HYDROLOGY_BODY_MAX_LEN) >> 8;
        header->dir_len[1] |= (upbody->len % HYDROLOGY_BODY_MAX_LEN) & 0xFF;
    }
    else
    {
        header->dir_len[0] |= HYDROLOGY_BODY_MAX_LEN >> 8;
        header->dir_len[1] |= HYDROLOGY_BODY_MAX_LEN & 0xFF;
    }

    memcpy(buffer, header, sizeof(HydrologyUpHeader) - 1);
    pointer = sizeof(HydrologyUpHeader) - 1;

    if(i == total)
    {
        memcpy(&buffer[pointer], &body_buffer[(i - 1) * HYDROLOGY_BODY_MAX_LEN],
            upbody->len % HYDROLOGY_BODY_MAX_LEN);
        pointer += upbody->len % HYDROLOGY_BODY_MAX_LEN;
        g_Hydrology.uppacket->end = ETX;
    }
    else
    {
        memcpy(&buffer[pointer], &body_buffer[(i - 1) * HYDROLOGY_BODY_MAX_LEN],
            HYDROLOGY_BODY_MAX_LEN);
        pointer += HYDROLOGY_BODY_MAX_LEN;
        g_Hydrology.uppacket->end = ETB;
    }

    buffer[pointer] = g_Hydrology.uppacket->end;
    pointer += 1;

    g_Hydrology.uppacket->crc16 = crc16(0xFFFF, buffer, pointer);
    buffer[pointer] = g_Hydrology.uppacket->crc16 >> 8;
    pointer += 1;
    buffer[pointer] = g_Hydrology.uppacket->crc16 & 0xFF;
    pointer += 1;
    g_Hydrology.uppacket->len = pointer;

    if(Hydrology_PortTransmmitData(g_Hydrology.uppacket->buffer, g_Hydrology.uppacket->len) == false)
        return false;

    return true;
}
static int HydrologyD_MakeUpTailandSend(HydrologyBodyType Funcode)
{
    uint8_t *buffer;
    uint8_t *body_buffer;
    uint16_t i, j, k, l;
    uint16_t buffer_size;
    uint16_t pointer, body_pointer;
    uint16_t total;
    HydrologyUpHeader *header = g_Hydrology.uppacket->header;
    HydrologyUpBody *upbody = (HydrologyUpBody *)g_Hydrology.uppacket->body;

    if(upbody->len <= HYDROLOGY_BODY_MAX_LEN)
    {
        buffer_size = header->len + upbody->len + 3;
        g_Hydrology.uppacket->buffer = (uint8_t *)kmalloc(buffer_size, __GFP_ZERO);

        if(g_Hydrology.uppacket->buffer == NULL)
        {
            kinetis_debug_trace(KERN_DEBUG, "g_Hydrology.uppacket->buffer malloc failed");
            return false;
        }

        buffer = g_Hydrology.uppacket->buffer;

        header->dir_len[0] |= (upbody->len) >> 8;
        header->dir_len[1] |= (upbody->len) & 0xFF;
        memcpy(buffer, header, sizeof(HydrologyUpHeader));
        pointer = sizeof(HydrologyUpHeader) - 4;

        switch(Funcode)
        {
            case LinkMaintenance:
                memcpy(&buffer[pointer], upbody, 8);
                pointer += 8;
                break;

            case EvenPeriodInformation:
            case Period:
                memcpy(&buffer[pointer], upbody, 23);
                pointer += 23;
                memcpy(&buffer[pointer], upbody->element[0]->guide, 2);
                pointer += 2;
                memcpy(&buffer[pointer], upbody->element[0]->value,
                    upbody->element[0]->num);
                pointer += upbody->element[0]->num;

                for(i = 1; i < upbody->count; i++)
                {
                    memcpy(&buffer[pointer], upbody->element[i]->guide, 2);
                    pointer += 2;
                }

                for(k = 0, j = 0, l = 0; k < 12; k++)
                {
                    for(i = 1; i < upbody->count; i++)
                    {
                        if(upbody->element[i]->num / 12 == 1)
                            memcpy(&buffer[pointer], &upbody->element[i]->value[j],
                                upbody->element[i]->num / 12);
                        else if(upbody->element[i]->num / 12 == 2)
                            memcpy(&buffer[pointer], &upbody->element[i]->value[l],
                                upbody->element[i]->num / 12);

                        pointer += upbody->element[i]->num / 12;
                    }

                    j += 1;
                    l += 2;
                }

                break;

            case Test:
            case TimerReport:
            case AddReport:
            case Hour:
            case Realtime:
            case SpecifiedElement:
            case WaterPumpMotor:
                memcpy(&buffer[pointer], upbody, 23);
                pointer += 23;

                for(i = 0; i < upbody->count; i++)
                {
                    memcpy(&buffer[pointer], upbody->element[i]->guide, 2);
                    pointer += 2;
                    memcpy(&buffer[pointer], upbody->element[i]->value, upbody->element[i]->num);
                    pointer += upbody->element[i]->num;
                }

                break;

            case ArtificialNumber:
            case InquireArtificialNumber:
                memcpy(&buffer[pointer], upbody, 8);
                pointer += 8;
                memcpy(&buffer[pointer], upbody->element[0]->guide, 2);
                pointer += 2;
                Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_RGZS,
                    0, &buffer[pointer],  upbody->element[i]->num);
                pointer += upbody->element[i]->num;
                break;

            case Picture:
                memcpy(&buffer[pointer], upbody, 8);
                pointer += 8;
                memcpy(&buffer[pointer], upbody->element[0]->guide, 2);
                pointer += 2;
                Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_PICTURE,
                    0, &buffer[pointer],  upbody->element[i]->num);
                pointer += upbody->element[i]->num;
                break;

            case ConfigurationModification:
            case ConfigurationRead:
            case ParameterModification:
            case ParameterRead:
            case Status:
                memcpy(&buffer[pointer], upbody, 15);
                pointer += 15;

                for(i = 0; i < upbody->count; i++)
                {
                    memcpy(&buffer[pointer], upbody->element[i]->guide, 2);
                    pointer += 2;
                    memcpy(&buffer[pointer], upbody->element[i]->value, upbody->element[i]->num);
                    pointer += upbody->element[i]->num;
                }

                break;

            case SoftwareVersion:
                memcpy(&buffer[pointer], upbody, 15);
                pointer += 15;
                memcpy(&buffer[pointer], upbody->element[0]->guide, 1);
                pointer += 1;
                memcpy(&buffer[pointer], upbody->element[0]->value, upbody->element[0]->num);
                pointer += upbody->element[0]->num;
                break;

            case InitializeSolidStorage:
            case Reset:
            case SetClock:
            case Time:
                memcpy(&buffer[pointer], upbody, 15);
                pointer += 15;
                break;

            case ChangePassword:
            case SetICCard:
                memcpy(&buffer[pointer], upbody, 15);
                pointer += 15;
                memcpy(&buffer[pointer], upbody->element[0]->guide, 2);
                pointer += 2;
                memcpy(&buffer[pointer], upbody->element[0]->value, upbody->element[0]->num);
                pointer += upbody->element[0]->num;
                break;

            case Pump:
            case Valve:
            case Gate:
                memcpy(&buffer[pointer], upbody, 15);
                pointer += 15;
                memcpy(&buffer[pointer], upbody->element[0]->guide, 1);
                pointer += 1;
                memcpy(&buffer[pointer], upbody->element[0]->value, upbody->element[0]->num);
                pointer += upbody->element[0]->num;
                break;

            case WaterSetting:
            case Record:
                memcpy(&buffer[pointer], upbody, 15);
                pointer += 15;
                memcpy(&buffer[pointer], upbody->element[0]->value, upbody->element[0]->num);
                pointer += upbody->element[0]->num;
                break;
        }

        g_Hydrology.uppacket->end = ETX;
        buffer[pointer] = g_Hydrology.uppacket->end;
        pointer += 1;

        g_Hydrology.uppacket->crc16 = crc16(0xFFFF, buffer, pointer);
        buffer[pointer] = g_Hydrology.uppacket->crc16 >> 8;
        pointer += 1;
        buffer[pointer] = g_Hydrology.uppacket->crc16 & 0xFF;
        pointer += 1;

        g_Hydrology.uppacket->len = pointer;

        if(Hydrology_PortTransmmitData(g_Hydrology.uppacket->buffer, g_Hydrology.uppacket->len) == false)
            return false;
    }
    else
    {
        kinetis_debug_trace(KERN_DEBUG, "[warnning]upbody->len(%d) > HYDROLOGY_BODY_MAX_LEN(%d)",
            upbody->len, HYDROLOGY_BODY_MAX_LEN);
        kinetis_debug_trace(KERN_DEBUG, "It will execute split transfer.");

        buffer_size = header->len + HYDROLOGY_BODY_MAX_LEN + 3;
        g_Hydrology.uppacket->buffer = (uint8_t *)kmalloc(buffer_size, __GFP_ZERO);

        if(g_Hydrology.uppacket->buffer == NULL)
        {
            kinetis_debug_trace(KERN_DEBUG, "g_Hydrology.uppacket->buffer malloc failed");
            return false;
        }

        buffer = g_Hydrology.uppacket->buffer;

        body_buffer = (uint8_t *)kmalloc(upbody->len, __GFP_ZERO);

        if(body_buffer == NULL)
        {
            kinetis_debug_trace(KERN_DEBUG, "body_buffer malloc failed");
            return false;
        }

        if(upbody->len % HYDROLOGY_BODY_MAX_LEN == 0)
            total = upbody->len / HYDROLOGY_BODY_MAX_LEN;
        else
            total = upbody->len / HYDROLOGY_BODY_MAX_LEN + 1;

        header->paketstart = SYN;
        body_pointer = 0;

        switch(Funcode)
        {
            case LinkMaintenance:
                memcpy(&body_buffer[body_pointer], upbody, 8);
                body_pointer += 8;

                for(i = 1; i <= total; ++i)
                    HydrologyD_SplitTransfer(body_buffer, buffer, header, upbody, i, total);

                break;

            case EvenPeriodInformation:
            case Period:
                memcpy(&body_buffer[body_pointer], upbody, 23);
                body_pointer += 23;
                memcpy(&body_buffer[body_pointer], upbody->element[0]->guide, 2);
                body_pointer += 2;
                memcpy(&body_buffer[body_pointer], upbody->element[0]->value,
                    upbody->element[0]->num);
                body_pointer += upbody->element[0]->num;

                for(i = 1; i < upbody->count; i++)
                {
                    memcpy(&body_buffer[body_pointer], upbody->element[i]->guide, 2);
                    body_pointer += 2;
                }

                for(k = 0, j = 0, l = 0; k < 12; k++)
                {
                    for(i = 1; i < upbody->count; i++)
                    {
                        if(upbody->element[i]->num / 12 == 1)
                            memcpy(&body_buffer[body_pointer], &upbody->element[i]->value[j],
                                upbody->element[i]->num / 12);
                        else if(upbody->element[i]->num / 12 == 2)
                            memcpy(&body_buffer[body_pointer], &upbody->element[i]->value[l],
                                upbody->element[i]->num / 12);

                        body_pointer += upbody->element[i]->num / 12;
                    }

                    j += 1;
                    l += 2;
                }

                for(i = 1; i <= total; ++i)
                    HydrologyD_SplitTransfer(body_buffer, buffer, header, upbody, i, total);

                break;

            case Test:
            case TimerReport:
            case AddReport:
            case Hour:
            case Realtime:
            case SpecifiedElement:
            case WaterPumpMotor:
                memcpy(&body_buffer[body_pointer], upbody, 23);
                body_pointer += 23;

                for(i = 0; i < upbody->count; i++)
                {
                    memcpy(&body_buffer[body_pointer], upbody->element[i]->guide, 2);
                    body_pointer += 2;
                    memcpy(&body_buffer[body_pointer], upbody->element[i]->value, upbody->element[i]->num);
                    body_pointer += upbody->element[i]->num;
                }

                for(i = 1; i <= total; ++i)
                    HydrologyD_SplitTransfer(body_buffer, buffer, header, upbody, i, total);

                break;

            case ConfigurationModification:
            case ConfigurationRead:
            case ParameterModification:
            case ParameterRead:
            case Status:
                memcpy(&body_buffer[body_pointer], upbody, 15);
                body_pointer += 15;

                for(i = 0; i < upbody->count; i++)
                {
                    memcpy(&body_buffer[body_pointer], upbody->element[i]->guide, 2);
                    body_pointer += 2;
                    memcpy(&body_buffer[body_pointer], upbody->element[i]->value, upbody->element[i]->num);
                    body_pointer += upbody->element[i]->num;
                }

                for(i = 1; i <= total; ++i)
                    HydrologyD_SplitTransfer(body_buffer, buffer, header, upbody, i, total);

                break;

            case SoftwareVersion:
                memcpy(&body_buffer[body_pointer], upbody, 15);
                body_pointer += 15;
                memcpy(&body_buffer[body_pointer], upbody->element[0]->guide, 1);
                body_pointer += 1;
                memcpy(&body_buffer[body_pointer], upbody->element[0]->value, upbody->element[0]->num);
                body_pointer += upbody->element[0]->num;

                for(i = 1; i <= total; ++i)
                    HydrologyD_SplitTransfer(body_buffer, buffer, header, upbody, i, total);

                break;

            case InitializeSolidStorage:
            case Reset:
            case SetClock:
            case Time:
                memcpy(&body_buffer[body_pointer], upbody, 15);
                body_pointer += 15;

                for(i = 1; i <= total; ++i)
                    HydrologyD_SplitTransfer(body_buffer, buffer, header, upbody, i, total);

                break;

            case ChangePassword:
            case SetICCard:
                memcpy(&body_buffer[body_pointer], upbody, 15);
                body_pointer += 15;
                memcpy(&body_buffer[body_pointer], upbody->element[0]->guide, 2);
                body_pointer += 2;
                memcpy(&body_buffer[body_pointer], upbody->element[0]->value, upbody->element[0]->num);
                body_pointer += upbody->element[0]->num;

                for(i = 1; i <= total; ++i)
                    HydrologyD_SplitTransfer(body_buffer, buffer, header, upbody, i, total);

                break;

            case Pump:
            case Valve:
            case Gate:
                memcpy(&body_buffer[body_pointer], upbody, 15);
                body_pointer += 15;
                memcpy(&body_buffer[body_pointer], upbody->element[0]->guide, 1);
                body_pointer += 1;
                memcpy(&body_buffer[body_pointer], upbody->element[0]->value, upbody->element[0]->num);
                body_pointer += upbody->element[0]->num;

                for(i = 1; i <= total; ++i)
                    HydrologyD_SplitTransfer(body_buffer, buffer, header, upbody, i, total);

                break;

            case WaterSetting:
            case Record:
                memcpy(&body_buffer[body_pointer], upbody, 15);
                body_pointer += 15;
                memcpy(&body_buffer[body_pointer], upbody->element[0]->value, upbody->element[0]->num);
                body_pointer += upbody->element[0]->num;

                for(i = 1; i <= total; ++i)
                    HydrologyD_SplitTransfer(body_buffer, buffer, header, upbody, i, total);

                break;

            case ArtificialNumber:
            case InquireArtificialNumber:
                for(i = 1; i <= total; ++i)
                {
                    HydrologyD_SetUpHeaderSequence(i, total);

                    header->dir_len[0] = 0;
                    header->dir_len[1] = 0;

                    if(i == total)
                    {
                        header->dir_len[0] |= (upbody->len % HYDROLOGY_BODY_MAX_LEN) >> 8;
                        header->dir_len[1] |= (upbody->len % HYDROLOGY_BODY_MAX_LEN) & 0xFF;
                    }
                    else
                    {
                        header->dir_len[0] |= HYDROLOGY_BODY_MAX_LEN >> 8;
                        header->dir_len[1] |= HYDROLOGY_BODY_MAX_LEN & 0xFF;
                    }

                    memcpy(buffer, header, sizeof(HydrologyUpHeader) - 1);
                    pointer = sizeof(HydrologyUpHeader) - 1;

                    if(i == total)
                    {
                        Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_RGZS,
                            HYDROLOGY_BODY_MAX_LEN - 10 + (i - 2) * HYDROLOGY_BODY_MAX_LEN,
                            &buffer[pointer],
                            (upbody->element[0]->num - (HYDROLOGY_BODY_MAX_LEN - 10)) % HYDROLOGY_BODY_MAX_LEN);
                        pointer += (upbody->element[0]->num - (HYDROLOGY_BODY_MAX_LEN - 10)) % HYDROLOGY_BODY_MAX_LEN;
                        g_Hydrology.uppacket->end = ETX;
                    }
                    else if(i == 1)
                    {
                        memcpy(&buffer[pointer], upbody, 8);
                        pointer += 8;
                        memcpy(&buffer[pointer], upbody->element[0]->guide, 2);
                        pointer += 2;
                        Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_RGZS,
                            0, &buffer[pointer],  HYDROLOGY_BODY_MAX_LEN - 10);
                        pointer += HYDROLOGY_BODY_MAX_LEN - 10;
                        g_Hydrology.uppacket->end = ETB;
                    }
                    else
                    {
                        Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_RGZS,
                            HYDROLOGY_BODY_MAX_LEN - 10 + (i - 2) * HYDROLOGY_BODY_MAX_LEN,
                            &buffer[pointer], HYDROLOGY_BODY_MAX_LEN);
                        pointer += HYDROLOGY_BODY_MAX_LEN;
                        g_Hydrology.uppacket->end = ETB;
                    }

                    buffer[pointer] = g_Hydrology.uppacket->end;
                    pointer += 1;

                    g_Hydrology.uppacket->crc16 = crc16(0xFFFF, buffer, pointer);
                    buffer[pointer] = g_Hydrology.uppacket->crc16 >> 8;
                    pointer += 1;
                    buffer[pointer] = g_Hydrology.uppacket->crc16 & 0xFF;
                    pointer += 1;

                    g_Hydrology.uppacket->len = pointer;

                    if(Hydrology_PortTransmmitData(g_Hydrology.uppacket->buffer, g_Hydrology.uppacket->len) == false)
                        return false;
                }

                break;

            case Picture:
                for(i = 1; i <= total; ++i)
                {
                    HydrologyD_SetUpHeaderSequence(i, total);

                    header->dir_len[0] = 0;
                    header->dir_len[1] = 0;

                    if(i == total)
                    {
                        header->dir_len[0] |= (upbody->len % HYDROLOGY_BODY_MAX_LEN) >> 8;
                        header->dir_len[1] |= (upbody->len % HYDROLOGY_BODY_MAX_LEN) & 0xFF;
                    }
                    else
                    {
                        header->dir_len[0] |= HYDROLOGY_BODY_MAX_LEN >> 8;
                        header->dir_len[1] |= HYDROLOGY_BODY_MAX_LEN & 0xFF;
                    }

                    memcpy(buffer, header, sizeof(HydrologyUpHeader) - 1);
                    pointer = sizeof(HydrologyUpHeader) - 1;

                    if(i == total)
                    {
                        Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_PICTURE,
                            HYDROLOGY_BODY_MAX_LEN - 25 + (i - 2) * HYDROLOGY_BODY_MAX_LEN,
                            &buffer[pointer],
                            (upbody->element[0]->num - (HYDROLOGY_BODY_MAX_LEN - 25)) % HYDROLOGY_BODY_MAX_LEN);
                        pointer += (upbody->element[0]->num - (HYDROLOGY_BODY_MAX_LEN - 25)) % HYDROLOGY_BODY_MAX_LEN;
                        g_Hydrology.uppacket->end = ETX;
                    }
                    else if(i == 1)
                    {
                        memcpy(&buffer[pointer], upbody, 23);
                        pointer += 23;
                        memcpy(&buffer[pointer], upbody->element[0]->guide, 2);
                        pointer += 2;
                        Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_PICTURE,
                            0, &buffer[pointer],  HYDROLOGY_BODY_MAX_LEN - 25);
                        pointer += HYDROLOGY_BODY_MAX_LEN - 25;
                        g_Hydrology.uppacket->end = ETB;
                    }
                    else
                    {
                        Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_PICTURE,
                            HYDROLOGY_BODY_MAX_LEN - 25 + (i - 2) * HYDROLOGY_BODY_MAX_LEN,
                            &buffer[pointer], HYDROLOGY_BODY_MAX_LEN);
                        pointer += HYDROLOGY_BODY_MAX_LEN;
                        g_Hydrology.uppacket->end = ETB;
                    }

                    buffer[pointer] = g_Hydrology.uppacket->end;
                    pointer += 1;

                    g_Hydrology.uppacket->crc16 = crc16(0xFFFF, buffer, pointer);
                    buffer[pointer] = g_Hydrology.uppacket->crc16 >> 8;
                    pointer += 1;
                    buffer[pointer] = g_Hydrology.uppacket->crc16 & 0xFF;
                    pointer += 1;

                    g_Hydrology.uppacket->len = pointer;

                    if(Hydrology_PortTransmmitData(g_Hydrology.uppacket->buffer, g_Hydrology.uppacket->len) == false)
                        return false;
                }

                break;
        }

        kfree(body_buffer);

    }

    return true;
}

static int HydrologyD_MakeErrUpTailandSend(HydrologyBodyType Funcode)
{
    uint8_t *buffer;
    uint8_t *body_buffer;
    uint16_t seq, i, j, k, l;
    uint16_t buffer_size;
    uint16_t pointer, body_pointer;
    uint16_t total;
    HydrologyUpHeader *upheader = g_Hydrology.uppacket->header;
    HydrologyDownHeader *downheader = g_Hydrology.downpacket->header;
    HydrologyUpBody *upbody = (HydrologyUpBody *)g_Hydrology.uppacket->body;

    seq = (downheader->count_seq[1] & 0xFF) + downheader->count_seq[2];
    kinetis_debug_trace(KERN_DEBUG, "[warnning]Packet %u isn't recevied, ready to resend", seq);

    buffer_size = upheader->len + HYDROLOGY_BODY_MAX_LEN + 3;
    g_Hydrology.uppacket->buffer = (uint8_t *)kmalloc(buffer_size, __GFP_ZERO);

    if(g_Hydrology.uppacket->buffer == NULL)
    {
        kinetis_debug_trace(KERN_DEBUG, "g_Hydrology.uppacket->buffer malloc failed");
        return false;
    }

    buffer = g_Hydrology.uppacket->buffer;

    body_buffer = (uint8_t *)kmalloc(upbody->len, __GFP_ZERO);

    if(body_buffer == NULL)
    {
        kinetis_debug_trace(KERN_DEBUG, "body_buffer malloc failed");
        return false;
    }

    if(upbody->len % HYDROLOGY_BODY_MAX_LEN == 0)
        total = upbody->len / HYDROLOGY_BODY_MAX_LEN;
    else
        total = upbody->len / HYDROLOGY_BODY_MAX_LEN + 1;

    upheader->paketstart = SYN;
    body_pointer = 0;

    switch(Funcode)
    {
        case LinkMaintenance:
            memcpy(&body_buffer[body_pointer], upbody, 8);
            body_pointer += 8;
            HydrologyD_SplitTransfer(body_buffer, buffer, upheader, upbody, seq, total);
            break;

        case EvenPeriodInformation:
        case Period:
            memcpy(&body_buffer[body_pointer], upbody, 23);
            body_pointer += 23;
            memcpy(&body_buffer[body_pointer], upbody->element[0]->guide, 2);
            body_pointer += 2;
            memcpy(&body_buffer[body_pointer], upbody->element[0]->value,
                upbody->element[0]->num);
            body_pointer += upbody->element[0]->num;

            for(i = 1; i < upbody->count; i++)
            {
                memcpy(&body_buffer[body_pointer], upbody->element[i]->guide, 2);
                body_pointer += 2;
            }

            for(k = 0, j = 0, l = 0; k < 12; k++)
            {
                for(i = 1; i < upbody->count; i++)
                {
                    if(upbody->element[i]->num / 12 == 1)
                        memcpy(&body_buffer[body_pointer], &upbody->element[i]->value[j],
                            upbody->element[i]->num / 12);
                    else if(upbody->element[i]->num / 12 == 2)
                        memcpy(&body_buffer[body_pointer], &upbody->element[i]->value[l],
                            upbody->element[i]->num / 12);

                    body_pointer += upbody->element[i]->num / 12;
                }

                j += 1;
                l += 2;
            }

            HydrologyD_SplitTransfer(body_buffer, buffer, upheader, upbody, seq, total);
            break;

        case Test:
        case TimerReport:
        case AddReport:
        case Hour:
        case Realtime:
        case SpecifiedElement:
        case WaterPumpMotor:
            memcpy(&body_buffer[body_pointer], upbody, 23);
            body_pointer += 23;

            for(i = 0; i < upbody->count; i++)
            {
                memcpy(&body_buffer[body_pointer], upbody->element[i]->guide, 2);
                body_pointer += 2;
                memcpy(&body_buffer[body_pointer], upbody->element[i]->value, upbody->element[i]->num);
                body_pointer += upbody->element[i]->num;
            }

            HydrologyD_SplitTransfer(body_buffer, buffer, upheader, upbody, seq, total);
            break;

        case ConfigurationModification:
        case ConfigurationRead:
        case ParameterModification:
        case ParameterRead:
        case Status:
            memcpy(&body_buffer[body_pointer], upbody, 15);
            body_pointer += 15;

            for(i = 0; i < upbody->count; i++)
            {
                memcpy(&body_buffer[body_pointer], upbody->element[i]->guide, 2);
                body_pointer += 2;
                memcpy(&body_buffer[body_pointer], upbody->element[i]->value, upbody->element[i]->num);
                body_pointer += upbody->element[i]->num;
            }

            HydrologyD_SplitTransfer(body_buffer, buffer, upheader, upbody, seq, total);
            break;

        case SoftwareVersion:
            memcpy(&body_buffer[body_pointer], upbody, 15);
            body_pointer += 15;
            memcpy(&body_buffer[body_pointer], upbody->element[0]->guide, 1);
            body_pointer += 1;
            memcpy(&body_buffer[body_pointer], upbody->element[0]->value, upbody->element[0]->num);
            body_pointer += upbody->element[0]->num;
            HydrologyD_SplitTransfer(body_buffer, buffer, upheader, upbody, seq, total);
            break;

        case InitializeSolidStorage:
        case Reset:
        case SetClock:
        case Time:
            memcpy(&body_buffer[body_pointer], upbody, 15);
            body_pointer += 15;
            HydrologyD_SplitTransfer(body_buffer, buffer, upheader, upbody, seq, total);
            break;

        case ChangePassword:
        case SetICCard:
            memcpy(&body_buffer[body_pointer], upbody, 15);
            body_pointer += 15;
            memcpy(&body_buffer[body_pointer], upbody->element[0]->guide, 2);
            body_pointer += 2;
            memcpy(&body_buffer[body_pointer], upbody->element[0]->value, upbody->element[0]->num);
            body_pointer += upbody->element[0]->num;
            HydrologyD_SplitTransfer(body_buffer, buffer, upheader, upbody, seq, total);
            break;

        case Pump:
        case Valve:
        case Gate:
            memcpy(&body_buffer[body_pointer], upbody, 15);
            body_pointer += 15;
            memcpy(&body_buffer[body_pointer], upbody->element[0]->guide, 1);
            body_pointer += 1;
            memcpy(&body_buffer[body_pointer], upbody->element[0]->value, upbody->element[0]->num);
            body_pointer += upbody->element[0]->num;
            HydrologyD_SplitTransfer(body_buffer, buffer, upheader, upbody, seq, total);
            break;

        case WaterSetting:
        case Record:
            memcpy(&body_buffer[body_pointer], upbody, 15);
            body_pointer += 15;
            memcpy(&body_buffer[body_pointer], upbody->element[0]->value, upbody->element[0]->num);
            body_pointer += upbody->element[0]->num;
            HydrologyD_SplitTransfer(body_buffer, buffer, upheader, upbody, seq, total);
            break;

        case ArtificialNumber:
        case InquireArtificialNumber:
            HydrologyD_SetUpHeaderSequence(seq, total);

            upheader->dir_len[0] = 0;
            upheader->dir_len[1] = 0;

            if(seq == total)
            {
                upheader->dir_len[0] |= (upbody->len % HYDROLOGY_BODY_MAX_LEN) >> 8;
                upheader->dir_len[1] |= (upbody->len % HYDROLOGY_BODY_MAX_LEN) & 0xFF;
            }
            else
            {
                upheader->dir_len[0] |= HYDROLOGY_BODY_MAX_LEN >> 8;
                upheader->dir_len[1] |= HYDROLOGY_BODY_MAX_LEN & 0xFF;
            }

            memcpy(buffer, upheader, sizeof(HydrologyUpHeader) - 1);
            pointer = sizeof(HydrologyUpHeader) - 1;

            if(seq == total)
            {
                Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_RGZS,
                    HYDROLOGY_BODY_MAX_LEN - 10 + (seq - 2) * HYDROLOGY_BODY_MAX_LEN,
                    &buffer[pointer],
                    (upbody->element[0]->num - (HYDROLOGY_BODY_MAX_LEN - 10)) % HYDROLOGY_BODY_MAX_LEN);
                pointer += (upbody->element[0]->num - (HYDROLOGY_BODY_MAX_LEN - 10)) % HYDROLOGY_BODY_MAX_LEN;
                g_Hydrology.uppacket->end = ETX;
            }
            else if(seq == 1)
            {
                memcpy(&buffer[pointer], upbody, 8);
                pointer += 8;
                memcpy(&buffer[pointer], upbody->element[0]->guide, 2);
                pointer += 2;
                Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_RGZS,
                    0, &buffer[pointer],  HYDROLOGY_BODY_MAX_LEN - 10);
                pointer += HYDROLOGY_BODY_MAX_LEN - 10;
                g_Hydrology.uppacket->end = ETX;
            }
            else
            {
                Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_RGZS,
                    HYDROLOGY_BODY_MAX_LEN - 10 + (seq - 2) * HYDROLOGY_BODY_MAX_LEN,
                    &buffer[pointer], HYDROLOGY_BODY_MAX_LEN);
                pointer += HYDROLOGY_BODY_MAX_LEN;
                g_Hydrology.uppacket->end = ETX;
            }

            buffer[pointer] = g_Hydrology.uppacket->end;
            pointer += 1;

            g_Hydrology.uppacket->crc16 = crc16(0xFFFF, buffer, pointer);
            buffer[pointer] = g_Hydrology.uppacket->crc16 >> 8;
            pointer += 1;
            buffer[pointer] = g_Hydrology.uppacket->crc16 & 0xFF;
            pointer += 1;

            g_Hydrology.uppacket->len = pointer;

            if(Hydrology_PortTransmmitData(g_Hydrology.uppacket->buffer, g_Hydrology.uppacket->len) == false)
                return false;

            break;

        case Picture:
            HydrologyD_SetUpHeaderSequence(seq, total);

            upheader->dir_len[0] = 0;
            upheader->dir_len[1] = 0;

            if(seq == total)
            {
                upheader->dir_len[0] |= (upbody->len % HYDROLOGY_BODY_MAX_LEN) >> 8;
                upheader->dir_len[1] |= (upbody->len % HYDROLOGY_BODY_MAX_LEN) & 0xFF;
            }
            else
            {
                upheader->dir_len[0] |= HYDROLOGY_BODY_MAX_LEN >> 8;
                upheader->dir_len[1] |= HYDROLOGY_BODY_MAX_LEN & 0xFF;
            }

            memcpy(buffer, upheader, sizeof(HydrologyUpHeader) - 1);
            pointer = sizeof(HydrologyUpHeader) - 1;

            if(seq == total)
            {
                Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_PICTURE,
                    HYDROLOGY_BODY_MAX_LEN - 25 + (seq - 2) * HYDROLOGY_BODY_MAX_LEN,
                    &buffer[pointer],
                    (upbody->element[0]->num - (HYDROLOGY_BODY_MAX_LEN - 25)) % HYDROLOGY_BODY_MAX_LEN);
                pointer += (upbody->element[0]->num - (HYDROLOGY_BODY_MAX_LEN - 25)) % HYDROLOGY_BODY_MAX_LEN;
                g_Hydrology.uppacket->end = ETX;
            }
            else if(seq == 1)
            {
                memcpy(&buffer[pointer], upbody, 23);
                pointer += 23;
                memcpy(&buffer[pointer], upbody->element[0]->guide, 2);
                pointer += 2;
                Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_PICTURE,
                    0, &buffer[pointer],  HYDROLOGY_BODY_MAX_LEN - 25);
                pointer += HYDROLOGY_BODY_MAX_LEN - 25;
                g_Hydrology.uppacket->end = ETX;
            }
            else
            {
                Hydrology_ReadStoreInfo(HYDROLOGY_D_FILE_PICTURE,
                    HYDROLOGY_BODY_MAX_LEN - 25 + (seq - 2) * HYDROLOGY_BODY_MAX_LEN,
                    &buffer[pointer], HYDROLOGY_BODY_MAX_LEN);
                pointer += HYDROLOGY_BODY_MAX_LEN;
                g_Hydrology.uppacket->end = ETX;
            }

            buffer[pointer] = g_Hydrology.uppacket->end;
            pointer += 1;

            g_Hydrology.uppacket->crc16 = crc16(0xFFFF, buffer, pointer);
            buffer[pointer] = g_Hydrology.uppacket->crc16 >> 8;
            pointer += 1;
            buffer[pointer] = g_Hydrology.uppacket->crc16 & 0xFF;
            pointer += 1;

            g_Hydrology.uppacket->len = pointer;

            if(Hydrology_PortTransmmitData(g_Hydrology.uppacket->buffer, g_Hydrology.uppacket->len) == false)
                return false;

            break;
    }

    kfree(body_buffer);

    return true;
}

int HydrologyD_ProcessSend(HydrologyElementInfo *Element_table, uint8_t Count,
    HydrologyMode Mode, HydrologyBodyType Funcode)
{
    if(HydrologyD_InitSend(Count, Funcode) == false)
        return false;

    HydrologyD_MakeUpHeader(Mode, Funcode);

    if(HydrologyD_MakeUpBody(Element_table, Funcode) == false)
        return false;

    if(HydrologyD_MakeUpTailandSend(Funcode) == false)
        return false;

    HydrologyD_ExitSend();

    return true;
}

static int HydrologyD_CheckDownPacket(uint8_t *input, int inputlen)
{
    uint16_t crcRet = 0;
    uint16_t inputCrc = 0;
    uint16_t bodylen = 0;

    crcRet = crc16(0xFFFF, input, inputlen - 2);

    inputCrc = (input[inputlen - 2] << 8) | input[inputlen - 1];

    if(crcRet != inputCrc)
    {
        kinetis_debug_trace(KERN_DEBUG, "Device crc(0x%04x) != Host crc(0x%04x)",
            inputCrc, crcRet);
        kinetis_debug_trace(KERN_DEBUG, "CRC check failed !");
        return false;
    }

    if((input[0] != SOH) || (input[1] != SOH))
    {
        kinetis_debug_trace(KERN_DEBUG, "Device Frame head(0x%02x, 0x%02x) != Host Frame head(0x%02x, 0x%02x)",
            input[0], input[1], SOH, SOH);
        kinetis_debug_trace(KERN_DEBUG, "Frame head check failed !");
        return false;
    }

    bodylen = (input[11] & 0x0F) * 256 + input[12];

    if(bodylen != (inputlen - 17))
    {
        kinetis_debug_trace(KERN_DEBUG, "Device length(0x%x) != Host length(0x%x)",
            bodylen, inputlen - 17);
        kinetis_debug_trace(KERN_DEBUG, "Hydrolog length check failed !");
        return false;
    }

    return true;
}

static int HydrologyD_MakeDownHeader(uint8_t *input, int inputlen, int *position, int *bodylen)
{
    HydrologyDownHeader *header = (HydrologyDownHeader *)g_Hydrology.downpacket->header;

    if(HydrologyD_CheckDownPacket(input, inputlen) != true)
    {
        kinetis_debug_trace(KERN_DEBUG, "Hydrology check fail !");
        return false;
    }

    memcpy(header->framestart, &input[*position], 2);
    *position += 2;

    memcpy(header->remoteaddr, &input[*position], 5);
    *position += 5;

    memcpy(&(header->centeraddr), &input[*position], 1);
    *position += 1;

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

    if(header->paketstart == SYN)
    {
        memcpy(header->count_seq, &input[*position], 3);
        *position += 3;
    }

    return true;
}

static int HydrologyD_MakeDownBody(uint8_t *input, int len, int position,
    HydrologyMode Mode, HydrologyBodyType Funcode)
{
    uint16_t i, offset;
    int16_t tmp_len;
    uint16_t tmp_position;
    HydrologyDownBody *downbody = (HydrologyDownBody *)g_Hydrology.downpacket->body;

    memcpy(downbody->streamid, &input[position], 2);
    position += 2;
    len -= 2;

    memcpy(downbody->sendtime, &input[position], 6);
    position += 6;
    len -= 6;

    downbody->count = 0;

    switch(Funcode)
    {
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
            return true;

        case Period:
            Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_PERIOD_BT,
                &input[position], 4);
            position += 4;
            len -= 4;
            Hydrology_WriteStoreInfo(HYDROLOGY_D_FILE_E_DATA, HYDROLOGY_PDA_PERIOD_ET,
                &input[position], 4);
            position += 4;
            len -= 4;
            downbody->count = 2;
            break;

        case SpecifiedElement:
            tmp_len = len;
            tmp_position = position;

            while(tmp_len > 0)
            {
                tmp_position += 2;
                tmp_len -= 2;

                downbody->count++;
            }

            if(downbody->count == 0)
                return false;

            break;

        case ConfigurationModification:
        case ParameterModification:
            tmp_len = len;
            tmp_position = position;

            while(tmp_len > 0)
            {
                offset = (input[tmp_position + 1] >> 3) + 2;
                tmp_position += offset;
                tmp_len -= offset;

                downbody->count++;
            }

            if(downbody->count == 0)
                return false;

            break;

        case ConfigurationRead:
        case ParameterRead:
            tmp_len = len;
            tmp_position = position;

            while(tmp_len > 0)
            {
                tmp_position += 2;
                tmp_len -= 2;

                downbody->count++;
            }

            if(downbody->count == 0)
                return false;

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

        case ChangePassword:
            downbody->count = 2;
            break;

        default:
            break;
    }

    if(downbody->count > 0)
    {
        downbody->element = (HydrologyElement **)kmalloc(sizeof(HydrologyElement *) * downbody->count, __GFP_ZERO);

        if(downbody->element == NULL)
        {
            kinetis_debug_trace(KERN_DEBUG, "downbody->element malloc failed");
            return false;
        }
    }

    for(i = 0; i < downbody->count; ++i)
    {
        downbody->element[i] = (HydrologyElement *)kmalloc(sizeof(HydrologyElement), __GFP_ZERO);

        if(downbody->element[i] == NULL)
        {
            kinetis_debug_trace(KERN_DEBUG, "downbody->element[%d] malloc failed", i);
            return false;
        }
    }

    switch(Funcode)
    {
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
            break;

        case Period:
            for(i = 0; i < 2; ++i)
            {
                memcpy(downbody->element[i]->guide, &input[position], 2);
                position += 2;
                len -= 2;

                if(i == 0)
                {
                    downbody->element[i]->num =
                        (downbody->element[i]->guide[1] >> 3);
                    downbody->element[i]->value =
                        (uint8_t *) kmalloc(downbody->element[i]->num, __GFP_ZERO);

                    if(NULL == downbody->element[i]->value)
                    {
                        kinetis_debug_trace(KERN_DEBUG, "downbody->element[0]->value malloc failed");
                        return false;
                    }

                    memcpy(downbody->element[i]->value, &input[position],
                        downbody->element[i]->num);
                    position += downbody->element[i]->num;
                    len -= downbody->element[i]->num;
                }
            }

            break;

        case SpecifiedElement:
        case ConfigurationRead:
        case ParameterRead:
            for(i = 0; i < downbody->count; ++i)
            {
                memcpy(downbody->element[i]->guide, &input[position], 2);
                position += 2;
                len -= 2;
            }

            break;

        case ConfigurationModification:
        case ParameterModification:
        case ChangePassword:
        case SetICCard:
            for(i = 0; i < downbody->count; ++i)
            {
                memcpy(downbody->element[i]->guide, &input[position], 2);
                position += 2;
                len -= 2;

                downbody->element[i]->num =
                    (downbody->element[i]->guide[1] >> 3);
                downbody->element[i]->value =
                    (uint8_t *) kmalloc(downbody->element[i]->num, __GFP_ZERO);

                if(NULL == downbody->element[i]->value)
                {
                    kinetis_debug_trace(KERN_DEBUG, "downbody->element[%d]->value malloc failed", i);
                    return false;
                }

                memcpy(downbody->element[i]->value, &input[position],
                    downbody->element[i]->num);
                position += downbody->element[i]->num;
                len -= downbody->element[i]->num;
            }

            break;

        case InitializeSolidStorage:
        case Reset:
            memcpy(downbody->element[0]->guide, &input[position], 2);
            position += 2;
            len -= 2;
            break;

        case Pump:
        case Valve:
            downbody->element[0]->num = input[position];
            position += 1;
            len -= 1;
            downbody->element[0]->value =
                (uint8_t *) kmalloc(downbody->element[0]->num, __GFP_ZERO);

            if(NULL == downbody->element[0]->value)
            {
                kinetis_debug_trace(KERN_DEBUG, "downbody->element[%d]->value malloc failed", 0);
                return false;
            }

            memcpy(downbody->element[0]->value, &input[position],
                downbody->element[0]->num);
            position += downbody->element[0]->num;
            len -= downbody->element[0]->num;
            break;

        case Gate:
            downbody->element[0]->num = input[position] >> 3;
            position += 1;
            len -= 1;
            downbody->element[0]->value =
                (uint8_t *) kmalloc(downbody->element[0]->num, __GFP_ZERO);

            if(NULL == downbody->element[0]->value)
            {
                kinetis_debug_trace(KERN_DEBUG, "downbody->element[%d]->value malloc failed", 0);
                return false;
            }

            memcpy(downbody->element[0]->value, &input[position],
                downbody->element[0]->num);
            position += downbody->element[0]->num;
            len -= downbody->element[0]->num;
            break;

        case WaterSetting:
            downbody->element[0]->num = 1;
            downbody->element[0]->value =
                (uint8_t *) kmalloc(downbody->element[0]->num, __GFP_ZERO);

            if(NULL == downbody->element[0]->value)
            {
                kinetis_debug_trace(KERN_DEBUG, "downbody->element[0]->value malloc failed");
                return false;
            }

            memcpy(downbody->element[0]->value, &input[position],
                downbody->element[0]->num);
            position += downbody->element[0]->num;
            len -= downbody->element[0]->num;
            break;

        default:
            break;
    }

    return true;
}

int HydrologyD_ProcessReceieve(uint8_t *input, int inputlen, HydrologyMode Mode)
{
    HydrologyDownHeader *header = NULL;
    int i = 0, bodylen = 0;

    if(HydrologyD_InitReceieve() == false)
        return false;

    header = g_Hydrology.downpacket->header;

    if(HydrologyD_MakeDownHeader(input, inputlen, &i, &bodylen) == false)
        return false;

    if(HydrologyD_MakeDownBody(input, bodylen, i, Mode, (HydrologyBodyType)header->funcode) == false)
        return false;

    if(Hydrology_ExecuteCommand((HydrologyBodyType)header->funcode) == false)
        return false;

    switch(Mode)
    {
        case HYDROLOGY_M1:
        case HYDROLOGY_M2:

            break;

        case HYDROLOGY_M3:
        case HYDROLOGY_M4:
            Hydrology_ResponseDownstream((HydrologyBodyType)header->funcode);
            break;
    }

    g_Hydrology.downpacket->end = input[inputlen - 3];

    HydrologyD_ExitReceieve();

    return true;
}

int HydrologyD_ProcessM3ErrPacket(HydrologyElementInfo *Element_table,
    uint8_t Count, HydrologyMode Mode, HydrologyBodyType Funcode, uint8_t CErr)
{
    uint8_t **Data;
    uint16_t Len;

    if(HydrologyD_InitSend(Count, Funcode) == false)
        return false;

    HydrologyD_MakeUpHeader(Mode, Funcode);

    if(HydrologyD_MakeUpBody(Element_table, Funcode) == false)
        return false;

    if(HydrologyD_MakeErrUpTailandSend(Funcode) == false)
        return false;

    HydrologyD_ExitSend();

    CErr++;

    if(Hydrology_PortReceiveData(Data, &Len, HYDROLOGY_D_PORT_TIMEOUT) == true)
    {
        if(HydrologyD_ProcessReceieve(*Data, Len, HYDROLOGY_M3) == true)
        {
            switch(Data[0][Len - 3])
            {
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
        }
        else
            return false;
    }
    else
    {
        kinetis_debug_trace(KERN_DEBUG, "Receive data timeout, retry times %d.",
            CErr);

        if(CErr >= 3)
        {
            Hydrology_DisconnectLink();
            return false;
        }

        HydrologyD_ProcessM3ErrPacket(Element_table, Count, HYDROLOGY_M3, Funcode, CErr);
    }

    return true;
}

int HydrologyD_ProcessEndIdentifier(HydrologyElementInfo *Element_table, uint8_t Count,
    HydrologyBodyType Funcode, uint8_t End)
{
    uint8_t CErr = 0;
    int ret;

    switch(End)
    {
        case ETX:
            kinetis_debug_trace(KERN_DEBUG, "[ETX]Wait disconnecting...");
            Hydrology_DisableLinkPacket();
            break;

        case ETB:
            kinetis_debug_trace(KERN_DEBUG, "[ETB]Stay connecting...");
            Hydrology_EnableLinkPacket();
            break;

        case ENQ:
            kinetis_debug_trace(KERN_DEBUG, "[ENQ]Query packet");
            break;

        case EOT:
            kinetis_debug_trace(KERN_DEBUG, "[EOT]Link is disconnecting");
            Hydrology_DisconnectLink();
            break;

        case ACK:
            kinetis_debug_trace(KERN_DEBUG, "[ACK]There will be another packets in the next transfer");
            break;

        case NAK:
            kinetis_debug_trace(KERN_DEBUG, "[NAK]Error packet, resend");
            ret = HydrologyD_ProcessM3ErrPacket(Element_table, Count, HYDROLOGY_M3, Funcode, CErr);
            break;

        case ESC:
            kinetis_debug_trace(KERN_DEBUG, "[ESC]Transfer is over, keep on live within 10 minutes");
            Hydrology_EnableLinkPacket();
            break;

        default:
            kinetis_debug_trace(KERN_ERR, "Unknown end packet identifier");
            break;
    }

    return ret;
}

int HydrologyD_ProcessM1(HydrologyElementInfo *Element_table, uint8_t Count,
    HydrologyMode Mode, HydrologyBodyType Funcode)
{
    return HydrologyD_ProcessSend(Element_table, Count, Mode, Funcode);
}

int HydrologyD_ProcessM2(HydrologyElementInfo *Element_table, uint8_t Count,
    HydrologyBodyType Funcode, uint8_t CErr)
{
    uint8_t **Data;
    uint16_t Len;

    if(HydrologyD_ProcessSend(Element_table, Count, HYDROLOGY_M2, Funcode) == false)
        return false;

    CErr++;

    if(Hydrology_PortReceiveData(Data, &Len, HYDROLOGY_D_PORT_TIMEOUT) == true)
    {
        if(HydrologyD_ProcessReceieve(*Data, Len, HYDROLOGY_M2) == true)
            HydrologyD_ProcessEndIdentifier(Element_table, Count, Funcode, Data[0][Len - 3]);
        else
            return false;
    }
    else
    {
        kinetis_debug_trace(KERN_DEBUG, "Receive data timeout, retry times %d.",
            CErr);

        if(CErr >= 3)
        {
            Hydrology_DisconnectLink();
            return false;
        }

        HydrologyD_ProcessM2(Element_table, Count, Funcode, CErr);
    }

    return true;
}

int HydrologyD_ProcessM3(HydrologyElementInfo *Element_table, uint8_t Count,
    HydrologyBodyType Funcode)
{
    uint8_t **Data;
    uint16_t Len;

    if(HydrologyD_ProcessSend(Element_table, Count, HYDROLOGY_M3, Funcode) == false)
        return false;

    if(Hydrology_PortReceiveData(Data, &Len, HYDROLOGY_D_PORT_TIMEOUT) == true)
    {
        if(HydrologyD_ProcessReceieve(*Data, Len, HYDROLOGY_M3) == true)
            HydrologyD_ProcessEndIdentifier(Element_table, Count, Funcode, Data[0][Len - 3]);
        else
            return false;
    }
    else
    {
        kinetis_debug_trace(KERN_DEBUG, "Receive data timeout.");
        return false;
    }

    return true;
}

int HydrologyD_ProcessM4(void)
{
    uint8_t **Data;
    uint16_t Len;

    for(;;)
    {
        if(Hydrology_PortReceiveData(Data, &Len, HYDROLOGY_D_PORT_TIMEOUT) == true)
            HydrologyD_ProcessReceieve(*Data, Len, HYDROLOGY_M4);
        else
        {
            kinetis_debug_trace(KERN_DEBUG, "[Warning]Device Port is going to be closed.");
            return false;
        }
    }
}

int HydrologyD_Process(HydrologyElementInfo *Element_table, uint8_t Count,
    HydrologyMode Mode, HydrologyBodyType Funcode)
{
    uint8_t ret = false;
    uint8_t CErr = 0;

    switch(Mode)
    {
        case HYDROLOGY_M1:
            ret = HydrologyD_ProcessM1(Element_table, Count, Mode, Funcode);
            break;

        case HYDROLOGY_M2:
            ret = HydrologyD_ProcessM2(Element_table, Count, Funcode, CErr);
            break;

        case HYDROLOGY_M3:
            ret = HydrologyD_ProcessM3(Element_table, Count, Funcode);
            break;

        case HYDROLOGY_M4:
            ret = HydrologyD_ProcessM4();
            break;
    }

    return ret;
}

#ifdef DESIGN_VERIFICATION_HYDROLOGY
#include "dv/k-test.h"
#include "dv/k-rng.h"

int t_HydrologyD_RandomElement(HydrologyMode Mode, HydrologyBodyType Funcode)
{
    HydrologyElementInfo *Element_table;
    uint8_t s_guide[] = {0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC};
    uint8_t i, j, count, guide;
    int ret;

    switch(Funcode)
    {
        case LinkMaintenance:
            ret = true;
            break;

        case Test:
        case TimerReport:
        case AddReport:
        case Realtime:
        case SpecifiedElement:
            count = Random_Get8bit() % (117 - 100);

            if(count == 0)
                count = 1;

            Element_table =
                (HydrologyElementInfo *)kmalloc(sizeof(HydrologyElementInfo) * count, __GFP_ZERO);

            if(Element_table == NULL)
            {
                kinetis_debug_trace(KERN_DEBUG, "Element_table malloc failed");
                return false;
            }

            for(i = 0;;)
            {
                guide = Random_Get8bit() % 0x75;

                if(!guide)
                    continue;

                Hydrology_ReadSpecifiedElementInfo(&Element_table[i], Funcode, guide);

                if(i == count - 1)
                    break;
                else
                    i++;
            }

            ret = HydrologyD_Process(Element_table, count, Mode, Funcode);

            kfree(Element_table);
            break;

        case EvenPeriodInformation:
        case Hour:
            count = Random_Get8bit() % sizeof(s_guide);

            if(count == 0)
                count = 1;

            Element_table =
                (HydrologyElementInfo *)kmalloc(sizeof(HydrologyElementInfo) * count, __GFP_ZERO);

            if(Element_table == NULL)
            {
                kinetis_debug_trace(KERN_DEBUG, "Element_table malloc failed");
                return false;
            }

            for(i = 0; i < count; i++)
            {
                j = Random_Get8bit() % (sizeof(s_guide) - 1);

                Hydrology_ReadSpecifiedElementInfo(&Element_table[i], Funcode, s_guide[j]);
            }

            ret = HydrologyD_Process(Element_table, count, Mode, Funcode);

            kfree(Element_table);
            break;

        case Period:
            count = 2;
            Element_table =
                (HydrologyElementInfo *)kmalloc(sizeof(HydrologyElementInfo) * count, __GFP_ZERO);

            if(Element_table == NULL)
            {
                kinetis_debug_trace(KERN_DEBUG, "Element_table malloc failed");
                return false;
            }

            Hydrology_ReadSpecifiedElementInfo(&Element_table[0], Funcode, 0x04);
            j = Random_Get8bit() % (sizeof(s_guide) - 1);
            Hydrology_ReadSpecifiedElementInfo(&Element_table[1], Funcode, s_guide[j]);

            ret = HydrologyD_Process(Element_table, count, Mode, Funcode);

            kfree(Element_table);
            break;

        case ConfigurationModification:
        case ConfigurationRead:
            count = Random_Get8bit() % 15;

            if(count == 0)
                count = 1;

            Element_table =
                (HydrologyElementInfo *)kmalloc(sizeof(HydrologyElementInfo) * count, __GFP_ZERO);

            if(Element_table == NULL)
            {
                kinetis_debug_trace(KERN_DEBUG, "Element_table malloc failed");
                return false;
            }

            for(i = 0;;)
            {
                guide = Random_Get8bit() % 0x0F;

                if(!guide)
                    continue;

                Hydrology_ReadSpecifiedElementInfo(&Element_table[i], Funcode, guide);

                if(i == count - 1)
                    break;
                else
                    i++;
            }

            ret = HydrologyD_Process(Element_table, count, Mode, Funcode);

            kfree(Element_table);
            break;

        case ParameterModification:
        case ParameterRead:
            count = Random_Get8bit() % (137 - 120);

            if(count == 0)
                count = 1;

            Element_table =
                (HydrologyElementInfo *)kmalloc(sizeof(HydrologyElementInfo) * count, __GFP_ZERO);

            if(Element_table == NULL)
            {
                kinetis_debug_trace(KERN_DEBUG, "Element_table malloc failed");
                return false;
            }

            for(i = 0;;)
            {
                guide = Random_Get8bit() % 0xA8;

                if(guide < 0x20)
                    continue;

                Hydrology_ReadSpecifiedElementInfo(&Element_table[i], Funcode, guide);

                if(i == count - 1)
                    break;
                else
                    i++;
            }

            ret = HydrologyD_Process(Element_table, count, Mode, Funcode);

            kfree(Element_table);
            break;

        case WaterPumpMotor:
            count = 6;
            Element_table =
                (HydrologyElementInfo *)kmalloc(sizeof(HydrologyElementInfo) * count, __GFP_ZERO);

            if(Element_table == NULL)
            {
                kinetis_debug_trace(KERN_DEBUG, "Element_table malloc failed");
                return false;
            }

            Hydrology_ReadSpecifiedElementInfo(&Element_table[0], Funcode, 0x70);
            Hydrology_ReadSpecifiedElementInfo(&Element_table[0], Funcode, 0x71);
            Hydrology_ReadSpecifiedElementInfo(&Element_table[0], Funcode, 0x72);
            Hydrology_ReadSpecifiedElementInfo(&Element_table[0], Funcode, 0x73);
            Hydrology_ReadSpecifiedElementInfo(&Element_table[0], Funcode, 0x74);
            Hydrology_ReadSpecifiedElementInfo(&Element_table[0], Funcode, 0x75);

            ret = HydrologyD_Process(Element_table, count, Mode, Funcode);

            kfree(Element_table);
            break;

        case Status:
        case SetICCard:
            count = 1;
            Element_table =
                (HydrologyElementInfo *)kmalloc(sizeof(HydrologyElementInfo) * count, __GFP_ZERO);

            if(Element_table == NULL)
            {
                kinetis_debug_trace(KERN_DEBUG, "Element_table malloc failed");
                return false;
            }

            Hydrology_ReadSpecifiedElementInfo(&Element_table[0], Funcode, 0x45);

            ret = HydrologyD_Process(Element_table, count, Mode, Funcode);

            kfree(Element_table);
            break;

        case ChangePassword:
            count = 1;
            Element_table =
                (HydrologyElementInfo *)kmalloc(sizeof(HydrologyElementInfo) * count, __GFP_ZERO);

            if(Element_table == NULL)
            {
                kinetis_debug_trace(KERN_DEBUG, "Element_table malloc failed");
                return false;
            }

            Hydrology_ReadSpecifiedElementInfo(&Element_table[0], Funcode, 0xB7);

            ret = HydrologyD_Process(Element_table, count, Mode, Funcode);

            kfree(Element_table);
            break;

        case ArtificialNumber:
        case Picture:
        case InquireArtificialNumber:
        case SoftwareVersion:
        case InitializeSolidStorage:
        case Reset:
        case SetClock:
        case Pump:
        case Valve:
        case Gate:
        case WaterSetting:
        case Record:
        case Time:
            ret = HydrologyD_Process(NULL, 0, Mode, Funcode);
            break;
    }

    return ret;
}

int t_HydrologyD_M1M2(HydrologyMode Mode)
{
    g_Hydrology.source = MsgFormClient;

    if(t_HydrologyD_RandomElement(Mode, LinkMaintenance) == false)
        return false;

    if(t_HydrologyD_RandomElement(Mode, Test) == false)
        return false;

    if(t_HydrologyD_RandomElement(Mode, EvenPeriodInformation) == false)
        return false;

    if(t_HydrologyD_RandomElement(Mode, TimerReport) == false)
        return false;

    if(t_HydrologyD_RandomElement(Mode, AddReport) == false)
        return false;

    if(t_HydrologyD_RandomElement(Mode, Hour) == false)
        return false;

    if(t_HydrologyD_RandomElement(Mode, ArtificialNumber) == false)
        return false;

    if(t_HydrologyD_RandomElement(Mode, Picture) == false)
        return false;

    if(t_HydrologyD_RandomElement(Mode, Pump) == false)
        return false;

    if(t_HydrologyD_RandomElement(Mode, Valve) == false)
        return false;

    if(t_HydrologyD_RandomElement(Mode, Gate) == false)
        return false;

    return true;
}

int t_HydrologyD_M3(void)
{
    g_Hydrology.source = MsgFormClient;

    if(t_HydrologyD_RandomElement(HYDROLOGY_M3, Test) == false)
        return false;

    if(t_HydrologyD_RandomElement(HYDROLOGY_M3, EvenPeriodInformation) == false)
        return false;

    if(t_HydrologyD_RandomElement(HYDROLOGY_M3, TimerReport) == false)
        return false;

    if(t_HydrologyD_RandomElement(HYDROLOGY_M3, AddReport) == false)
        return false;

    if(t_HydrologyD_RandomElement(HYDROLOGY_M3, Hour) == false)
        return false;

    if(t_HydrologyD_RandomElement(HYDROLOGY_M3, ArtificialNumber) == false)
        return false;

    if(t_HydrologyD_RandomElement(HYDROLOGY_M3, Picture) == false)
        return false;

    if(t_HydrologyD_RandomElement(HYDROLOGY_M3, Pump) == false)
        return false;

    if(t_HydrologyD_RandomElement(HYDROLOGY_M3, Valve) == false)
        return false;

    if(t_HydrologyD_RandomElement(HYDROLOGY_M3, Gate) == false)
        return false;

    return true;
}

int t_HydrologyD_M4(void)
{
    g_Hydrology.source = MsgFormClient;

    return HydrologyD_Process(NULL, 0, HYDROLOGY_M4, (HydrologyBodyType)NULL);
}

#endif

