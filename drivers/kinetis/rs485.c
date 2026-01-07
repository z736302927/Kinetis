// #include "kinetis/rs485.h"
// 
// /* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */
// 
// /**
//   * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
//   * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
//   * @step 3:  .
//   * @step 4:  .
//   * @step 5:
//   */
// 
// #include <linux/gfp.h>
// 
// #include "kinetis/serial-port.h"
// #define DEBUG
// #include "kinetis/idebug.h"
// 
// #if MCU_PLATFORM_STM32
// #include "usart.h"
// #else
// #endif
// 
// #define RS485_printf    p_dbg
// #define RS485_error     p_err_fun
// 
// #define RS485_BUFFER_SIZE      128
// 
// void RS485_Port_Send(u8 *pdata, u16 length)
// {
// #if MCU_PLATFORM_STM32
//     HAL_UART_Transmit_IT(&huart1, pdata, length);
// #else
// #endif
// }
// 
// void RS485_Port_Receive(u8 *pdata, u16 *length)
// {
//     u32 begintime;
//     u32 currenttime;
//     u32 timediff;
// 
//     begintime = basic_timer_get_ms();
// 
//     while (1) {
//         serial_port_Receive(&serial_port_3, pdata, length);
// 
//         if (serial_port_ReadRxState() == 1) {
//             serial_port_SetRxState(0);
//             break;
//         } else {
//             currenttime = basic_timer_get_ms();
//             timediff = currenttime >= begintime ? currenttime - begintime :
//                 currenttime + U32_MAX - begintime;
// 
//             if (timediff > 3000) { /* 10s */
//                 pr_debug("No data came !");
//                 break;
//             }
//         }
//     }
// }
// 
// /* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */
// 
// void RS485_Master_Send(u8 Dev_addr, u8 Fun_code, u16 Reg_addr, u8 length)
// {
//     u8 cmd[8];
//     u16 crc = 0;
// 
//     cmd[0] = Dev_addr;
//     cmd[1] = Fun_code;
//     cmd[2] = Reg_addr >> 8;
//     cmd[3] = Reg_addr % 256;
//     cmd[4] = length >> 8;
//     cmd[5] = length % 256;
//     crc = crc16((char *)cmd, 6);
//     cmd[6] = crc % 256;
//     cmd[7] = crc >> 8;
//     RS485_Port_Send(cmd, 8);
// }
// 
// int RS485_Master_Receive(u8 *pdata, u16 *length)
// {
//     int retValue = false;
//     u8 Data[128];
//     u16 Size;
// 
//     RS485_Port_Receive(Data, &Size);
// 
//     if (Size == 0)
//         return false;
// 
//     if (Data[2] != 0) {
//         if (CRC16_Check((char *)Data, 5 + Data[2]) == true) {
//             pdata = (u8 *)kmalloc(Data[2], __GFP_ZERO);
// 
//             if (pdata == NULL) {
//                 pr_debug("Data to memory malloc failed");
//                 RS485_error;
//                 retValue = false;
//             } else {
//                 memcpy(pdata, &Data[3], Data[2]);
//                 *length = Data[2];
//                 retValue = true;
//             }
//         } else {
//             RS485_printf("Data check error");
//             RS485_error;
//             retValue = false;
//         }
//     } else {
//         RS485_printf("The received data length is 0 and cannot be parsed");
//         RS485_error;
//         retValue = false;
//     }
// 
//     return retValue;
// }
// 
// #ifdef DESIGN_VERIFICATION_RS485
// 
// #endif
