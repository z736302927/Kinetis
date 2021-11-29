/******************** (C) COPYRIGHT 2017 ANO Tech ********************************
 * 作�?   ：匿名科�?
 * 官网    ：www.anotc.com
 * 淘宝    ：anotc.taobao.com
 * 技术Q�?�?90169595
 * 描述    ：USB通信相关函数
**********************************************************************************/
#include "RTL.h"
#include <rl_usb.h>
#include <stm32f4xx.h>                  /* STM32F4xx Definitions              */
#define __NO_USB_LIB_C
#include "usb_config.c"
#include "Ano_DT.h"
#include "Ano_USB.h"

u8 HID_SEND_TIMEOUT = 5;			//hid发送不足一帧时，等待HID_SEND_TIMEOUT周期进行发�?
#define HIDTXBUFLEN		1000
u8 hid_datatemp[HIDTXBUFLEN];					//hid环形缓冲�?
u16 hid_datatemp_begin = 0;		//环形缓冲区数据指针，指向应当发送的数据
u16 hid_datatemp_end = 0;			//环形缓冲区数据结�?
u8 hid_data2send[64];					//hid发送缓存，一个hid数据�?4字节，第一字节表示有效数据字节数，0-63，后面是数据，最�?3字节

void Usb_Hid_Init(void)
{
    usbd_init();
    usbd_connect(__TRUE);
}
void Usb_Hid_Adddata(u8 *dataToSend, u8 length)
{
    for (u8 i = 0; i < length; i++) {
        hid_datatemp[hid_datatemp_end++] = dataToSend[i];

        if (hid_datatemp_end == HIDTXBUFLEN)
            hid_datatemp_end = 0;
    }
}
void Usb_Hid_Send(void)
{
    static u8 notfull_timeout = 0;
    //
    s16 len = (s16)(hid_datatemp_end - hid_datatemp_begin);

    if (len < 0)
        len = HIDTXBUFLEN + len;

    //
    if (len >= 63) {
        notfull_timeout = 0;
        hid_data2send[0] = 63;

        for (u8 i = 0; i < 63; i++) {
            hid_data2send[i + 1] = hid_datatemp[hid_datatemp_begin++];

            if (hid_datatemp_begin == HIDTXBUFLEN)
                hid_datatemp_begin = 0;
        }

        usbd_hid_get_report_trigger(0, hid_data2send, 64);		//send
    } else {
        notfull_timeout++;

        if (notfull_timeout == HID_SEND_TIMEOUT) {
            notfull_timeout = 0;
            hid_data2send[0] = len;

            for (u8 i = 0; i < 63; i++) {
                if (i < len) {
                    hid_data2send[i + 1] = hid_datatemp[hid_datatemp_begin++];

                    if (hid_datatemp_begin == HIDTXBUFLEN)
                        hid_datatemp_begin = 0;
                } else
                    hid_data2send[i + 1] = 0;
            }

            //hid_datatemp_begin = hid_datatemp_end;
            usbd_hid_get_report_trigger(0, hid_data2send, 64);		//send
        }
    }
}

int usbd_hid_get_report(U8 rtype, U8 rid, U8 *buf, U8 req)
{
    switch (rtype) {
    case HID_REPORT_INPUT:
        switch (rid) {
        case 0:
            switch (req) {
            case USBD_HID_REQ_EP_CTRL:
            case USBD_HID_REQ_PERIOD_UPDATE:
                return 64;

            case USBD_HID_REQ_EP_INT:
                break;
            }

            break;
        }

        break;

    case HID_REPORT_FEATURE:
        return (1);
    }

    return (0);
}

void usbd_hid_set_report(U8 rtype, U8 rid, U8 *buf, int len, U8 req)
{

    switch (rtype) {
    case HID_REPORT_OUTPUT:
        for (u8 i = 1; i <= (*(buf)); i++)
            AnoDTRxOneByteUsb(*(buf + i));		//hid接收到数据会调用此函�?

        break;

    case HID_REPORT_FEATURE:
        //feat = buf[0];
        break;
    }
}
