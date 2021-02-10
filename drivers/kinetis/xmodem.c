#include "xmodem/xmodem.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "stdlib.h"
#include "string.h"
#include "stdbool.h"

#define DEBUG
#include "kinetis/idebug.h"

#define xmodem_printf                   p_dbg

#define BLOCK_SIZE1                     128
#define BLOCK_SIZE2                     1024

void xmodem_Delayms(u32 ticks)
{
    mdelay(ticks);
}

u32 xmodem_GetTick(void)
{
    return basic_timer_get_ms_tick();
}

void xmodem_SendReceiveRequest(u8 Data)
{

}

void xmodem_SendPacket(u8 *pData, u32 Length)
{

}

void xmodem_ReceivePacket(u8 *pData, u32 Length)
{

}

void xmodem_ProcessDataBlock(u8 *pData, u32 Length)
{

}

u32 xmodem_GetTransferLength(void)
{

}

void xmodem_GetTransferData(u8 *pData, u16 Length)
{

}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#define SOH                             0x01
#define STX                             0x02
#define EOT                             0x04
#define ACK                             0x06
#define NAK                             0x15
#define CAN                             0x18
#define C                               'C'
#define CTRL_Z                          0x1A

u8 *g_xmodemBuffer = NULL;
u32 g_CurrentPacketNumber = 0;
u32 g_TotalPacketNumber = 0;

u32 xmodem_CheckSUM8(void *pData, u32 Length)
{
    u8 Index = 0;
    u32 CheckSUM = 0;
    u8 *pTmp = (u8 *)pData;

    if ((pData == NULL) || (Length == 0))
        return CheckSUM;

    while (Length > 1) {
        CheckSUM += ((u16)pTmp[Index] << 8 & 0xFF00) | (u16)pTmp[Index + 1] & 0x00FF;
        Length -= 2;
        Index += 2;
    }

    if (Length > 0) {
        CheckSUM += ((u16)pTmp[Index] << 8) & 0xFFFF;
        Index += 1;
    }

    while (CheckSUM >> 16)
        CheckSUM = (CheckSUM & 0xFFFF) + (CheckSUM >> 16);

    return ~CheckSUM;
}

void xmodem_AssemblePacket(void)
{
    g_xmodemBuffer[0] = SOH;
    g_xmodemBuffer[1] = g_CurrentPacketNumber;
    g_xmodemBuffer[2] = ~g_CurrentPacketNumber;
    xmodem_GetTransferData(&g_xmodemBuffer[3], BLOCK_SIZE1);
    g_xmodemBuffer[BLOCK_SIZE1 + 3] = xmodem_CheckSUM8(g_xmodemBuffer, BLOCK_SIZE1 + 3);
    xmodem_SendPacket(g_xmodemBuffer, BLOCK_SIZE1 + 4);
}

void xmodem_ProcessSendPacket(void)
{
    g_CurrentPacketNumber++;
    xmodem_AssemblePacket(pData);
}

void xmodem_ResendErrorPacket(void)
{
    xmodem_SendPacket(g_xmodemBuffer, BLOCK_SIZE1 + 4);
}

void xmodem_ClearSendStatus(void)
{
    g_CurrentPacketNumber = 0;
    g_TotalPacketNumber = 0;
}

void xmodem_ProcessReceivePacket(u8 *pData, u32 Length)
{
    switch (pData[0]) {
        case SOH:
            if (xmodem_CheckSUM8(pData, BLOCK_SIZE1 + 4) == 0) {
                xmodem_SendReceiveRequest(ACK);
                xmodem_ProcessDataBlock(&pData[3], BLOCK_SIZE1);
            } else
                xmodem_SendReceiveRequest(NAK);

            break;

        case STX:
            if (xmodem_CheckSUM8(pData, BLOCK_SIZE1 + 4) == 0) {
                xmodem_SendReceiveRequest(ACK);
                xmodem_ProcessDataBlock(&pData[3], BLOCK_SIZE2);
            } else
                xmodem_SendReceiveRequest(NAK);

            break;

        case EOT:
            xmodem_SendReceiveRequest(ACK);
            break;

        case ACK:
            if (g_CurrentPacketNumber != g_TotalPacketNumber)
                xmodem_ProcessSendPacket();

            break;

        case NAK:
            if (g_CurrentPacketNumber == 0)
                xmodem_ProcessSendPacket();
            else
                xmodem_ResendErrorPacket();

            break;

        case CAN:
            xmodem_ClearSendStatus();
            break;

        case C:
            ;
            break;

        default:
            break;
    }
}

u8 xmodem_Timeout(void)
{
    u32 begintime = 0, currenttime = 0, timediff = 0;

    begintime = xmodem_GetTick();

    while (1) {

        if (xmodem_Read_BUSY() == 0)
            return true;
        else {
            currenttime = xmodem_GetTick();
            timediff = currenttime >= begintime ? currenttime - begintime :
                currenttime + UINT32_MAX - begintime;

            if (timediff > 30000) { /* 3s */
                kinetis_debug_trace(KERN_DEBUG, "Command execution timeout !");
                return false;
            }
        }
    }
}
#ifdef DESIGN_VERIFICATION_XMODEM
{"test", fuction},
#endif