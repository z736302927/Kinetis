#include "kinetis/shell.h"

#include "string.h"
#include "stdio.h"

#include <linux/gfp.h>
#include <linux/types.h>
#include <linux/printk.h>

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#include "usart.h"

static u16 shell_buffer[128];
static u16 *shell_cur_pos = shell_buffer;

void shell_init(void)
{
    HAL_UART_Receive_DMA(&huart1, (u8 *)shell_buffer, 128);
    memset(shell_buffer, 0xFF, sizeof(shell_buffer));
}

static inline u8 shell_port_receive(void)
{
    u8 tmp = 0;

    if (*shell_cur_pos & 0xFF00)
        tmp = 0xFF;
    else {
        tmp = (u8) * shell_cur_pos;
        *shell_cur_pos = 0xFFFF;

        if (shell_cur_pos == &shell_buffer[127])
            shell_cur_pos = shell_buffer;
        else
            shell_cur_pos++;
    }

//    HAL_UART_Receive_IT(&huart1, &Data, 1);

//    if(shell_PortState() == false) {}

//    struct serial_port Instance;

//    Instance.PortNbr = 1;
//    Instance.Txbuffer_Size = 1;
//    Instance.Tempbuffer_Size = 1;
//    Instance.RxScanInterval = 10;
//    Instance.Endchar = NULL;
//    serial_port_Open(&Instance);

//    if(serial_port_Receive(&Instance) == true)
//    {
//        Data = Instance.Rxbuffer[0];
//        kfree(Instance.Rxbuffer);
//    }

//    serial_port_Close(&Instance);

    return tmp;
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

bool shell_get_user_input(char *cur_pos)
{
    char input = '\0';
    u16 length = 0;

    input = shell_port_receive();

    if (input == 0xFF)
        return false;
    else
        goto get_char;
    
    do {
        input = shell_port_receive();

get_char:
        if (input == 0xFF)
            continue;
        else if (input == '\b') {
            if (length != 0) {
                cur_pos--;
                length--;
                printk("\b \b");
            }
        } else {
            *cur_pos = input;
            cur_pos++;
            length++;
            printk("%c", input);
        }
    } while (input != '\r');

    cur_pos--;

    if (length == 1)
        *cur_pos = '\r';
    else
        *cur_pos = '\0';

    pr_cont("\n");
    
    return true;
}



