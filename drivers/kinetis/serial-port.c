#include "kinetis/serial-port.h"
#include "kinetis/basic-timer.h"
#include "kinetis/idebug.h"
#include "kinetis/delay.h"

#include <linux/gfp.h>
#include <linux/slab.h>

#include "string.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  According to the example in structure serial_port_1, design the function you need and initialize it in the main function.
  * @step 3:  You need to provide an ms timer for function basic_timer_get_ms.
  * @step 4:  For receiving data, you need to put it in like a ring, using interrupts or DMA.
  * @step 5:  Finally, you can process the received data in function serial_port_rx_buffer_Process.Note: maximum 256 bytes received.
  */

#include "usart.h"

int serial_port_rx_flag = 0;
u16 serialport_index;

void serial_port_set_rx_state(int state)
{
    serial_port_rx_flag = state;
}

int serial_port_get_rx_state(void)
{
    return serial_port_rx_flag;
}

void serial_port_get_rx_data(u16 data)
{
    struct serial_port *serial_port;
    
    serial_port->tmp_buffer[serialport_index] = data;
    serialport_index++;

    if (serialport_index >= serial_port->tmp_buffer_size)
        serialport_index = 0;
}

void serial_port_rx_buffer_init(struct serial_port *serial_port)
{
    memset(serial_port->tmp_buffer, 0xFF, serial_port->tmp_buffer_size * sizeof(u16));
}

u8 serial_port_alloc(struct serial_port *serial_port)
{
    serial_port->tmp_buffer = (u16 *)kmalloc(serial_port->tmp_buffer_size, __GFP_ZERO);

    if (serial_port->tmp_buffer == NULL) {
        printk(KERN_DEBUG "SerialPort Rx malloc failed !");
        return false;
    } else
        serial_port_rx_buffer_init(serial_port);

    return true;
}

void serial_port_free(struct serial_port *serial_port)
{
    kfree(serial_port->tmp_buffer);
    serial_port->tmp_buffer = NULL;
}

u32 serial_port_get_baud(struct serial_port *serial_port)
{
    u32 retval;

    if (serial_port->port_nbr == 1)
        retval = huart1.Init.BaudRate;
    else if (serial_port->port_nbr == 2)
        retval = huart2.Init.BaudRate;
    else if (serial_port->port_nbr == 3)
        retval = huart3.Init.BaudRate;

    return retval;
}

u32 serial_port_get_min_interval(struct serial_port *serial_port)
{
    u32 Baud = 0;
    u32 Interval;
    Baud = serial_port_get_baud(serial_port);
    Interval = 1000 / (Baud / 10) + 1;
    return Interval;
}

u8 serial_port_open(struct serial_port *serial_port)
{
    serial_port->rx_scan_interval += serial_port_get_min_interval(serial_port);

    if (serial_port_alloc(serial_port) == false)
        return false;

    if (serial_port->port_nbr == 1)
        HAL_UART_Receive_DMA(&huart1, (u8 *)serial_port->tmp_buffer, serial_port->tmp_buffer_size);
    else if (serial_port->port_nbr == 2)
        HAL_UART_Receive_DMA(&huart2, (u8 *)serial_port->tmp_buffer, serial_port->tmp_buffer_size);
    else if (serial_port->port_nbr == 3)
        HAL_UART_Receive_DMA(&huart3, (u8 *)serial_port->tmp_buffer, serial_port->tmp_buffer_size);

    return true;
}

void serial_port_close(struct serial_port *serial_port)
{
//    if(serial_port->port_nbr == 1)
//        HAL_UART_MspDeInit(&huart1);
//    else if(serial_port->port_nbr == 2)
//        HAL_UART_MspDeInit(&huart2);
//    else if(serial_port->port_nbr == 3)
//        HAL_UART_MspDeInit(&huart3);
    serial_port_free(serial_port);
}

void serial_port_send(struct serial_port *serial_port)
{
    if (serial_port->port_nbr == 1)
        HAL_UART_Transmit_IT(&huart1, serial_port->tx_buffer, serial_port->tx_buffer_size);
    else if (serial_port->port_nbr == 2)
        HAL_UART_Transmit_IT(&huart2, serial_port->tx_buffer, serial_port->tx_buffer_size);
    else if (serial_port->port_nbr == 3)
        HAL_UART_Transmit_IT(&huart3, serial_port->tx_buffer, serial_port->tx_buffer_size);
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

static void find_tail(struct serial_port *serial_port)
{
    u16 cnt = 0;

    while ((serial_port->tmp_buffer[serial_port->rx_tail] >> 8) != 0xFF) {
        cnt++;

        if (serial_port->rx_tail == (serial_port->tmp_buffer_size - 1))
            serial_port->rx_tail = 0;
        else
            serial_port->rx_tail++;

        if (cnt == (serial_port->tmp_buffer_size - 1))
            break;
    }
}

static u8 *find_end_char(struct serial_port *serial_port, u16 *size)
{
    u16 i, j;
    u8 *pdata;

    if (serial_port->rx_tail > serial_port->rx_head) {
        *size = serial_port->rx_tail - serial_port->rx_head;
        pdata = (u8 *)kmalloc(*size, __GFP_ZERO);

        if (pdata == NULL)
            return NULL;

        for (i = serial_port->rx_head, j = 0; i < serial_port->rx_tail; i++, j++)
            pdata[j] = (u8)serial_port->tmp_buffer[i];

        return pdata;
    } else if (serial_port->rx_tail < serial_port->rx_head) {
        *size = serial_port->tmp_buffer_size - serial_port->rx_head + serial_port->rx_tail;
        pdata = (u8 *)kmalloc(*size, __GFP_ZERO);

        if (pdata == NULL)
            return NULL;

        for (i = serial_port->rx_head, j = 0; i < serial_port->tmp_buffer_size; i++, j++)
            pdata[j] = (u8)serial_port->tmp_buffer[i];

        for (i = 0; i < serial_port->rx_tail; i++, j++)
            pdata[j] = (u8)serial_port->tmp_buffer[i];

        return pdata;
    }

    return NULL;
}

static void extract_valid_data(struct serial_port *serial_port)
{
    u16 i, j;

    if (serial_port->rx_tail > serial_port->rx_head) {
        serial_port->rx_buffer_size = serial_port->rx_tail - serial_port->rx_head;
        
        serial_port->rx_buffer = kmalloc(serial_port->rx_buffer_size, __GFP_ZERO);

        if (serial_port->rx_buffer == NULL)
            return;

        for (i = serial_port->rx_head, j = 0; i < serial_port->rx_tail; i++, j++)
            serial_port->rx_buffer[j] = serial_port->tmp_buffer[i];

        memset(&serial_port->tmp_buffer[serial_port->rx_head],
            0xFF, serial_port->rx_buffer_size * sizeof(u16));
    } else if (serial_port->rx_tail < serial_port->rx_head) {
        serial_port->rx_buffer_size = 
            serial_port->tmp_buffer_size - serial_port->rx_head + serial_port->rx_tail;
        
        serial_port->rx_buffer = kmalloc(serial_port->rx_buffer_size, __GFP_ZERO);

        if (serial_port->rx_buffer == NULL)
            return;

        for (i = serial_port->rx_head, j = 0; i < serial_port->tmp_buffer_size; i++, j++)
            serial_port->rx_buffer[j] = serial_port->tmp_buffer[i];

        for (i = 0; i < serial_port->rx_tail; i++, j++)
            serial_port->rx_buffer[j] = serial_port->tmp_buffer[i];

        memset(&serial_port->tmp_buffer[serial_port->rx_head],
            0xFF, (serial_port->tmp_buffer_size - serial_port->rx_head) * sizeof(u16));
        memset(&serial_port->tmp_buffer[0],
            0xFF, serial_port->rx_tail * sizeof(u16));
    } else
        serial_port->rx_buffer_size = 0;

    if (serial_port->rx_buffer_size != 0) {
        serial_port->rx_head = serial_port->rx_tail;
        serial_port_set_rx_state(1);
    }
}

u8 serial_port_receive(struct serial_port *serial_port)
{
    static u32 refer = 0;
    u32 delta = 0;
    u8 tmp_tail = serial_port->rx_tail;
    u8 wait_rx_done = 0;
    u16 size;
    
    find_tail(serial_port);

    if (serial_port->rx_head != serial_port->rx_tail) {
        if (tmp_tail != serial_port->rx_tail)
            refer = basic_timer_get_ms();
        else {
            delta = basic_timer_get_ms() >= refer ?
                basic_timer_get_ms() - refer :
                basic_timer_get_ms() + (DELAY_TIMER_UNIT - refer);

            if (delta > serial_port->rx_scan_interval)
                wait_rx_done = 1;
        }
    }

    if (wait_rx_done == 1) {
        if (serial_port->end_char != NULL) {
            serial_port->current_buffer = find_end_char(serial_port, &size);

            if (serial_port->current_buffer != NULL) {
                if (strncmp((char *)serial_port->end_char,
                        (char *)&serial_port->current_buffer[size - serial_port->end_char_size],
                        serial_port->end_char_size) != 0) {
                    kfree(serial_port->current_buffer);
                    wait_rx_done = 0;
                    return false;
                } else {
                    kfree(serial_port->current_buffer);
                    extract_valid_data(serial_port);

                    if (serial_port->rx_buffer_size > 1)
                        serial_port->rx_buffer[serial_port->rx_buffer_size - 1] = '\0';

                    return true;
                }
            }
        } else {
            extract_valid_data(serial_port);
            return true;
        }
    }

    return false;
}

#ifdef DESIGN_VERIFICATION_SEIRALPORT
#include "kinetis/test-kinetis.h"

extern int parse_test_all_case(char *cmd);

int t_serial_port_shell(int argc, char **argv)
{
    struct serial_port serial_port;
    char *word = "\r";
    int ret = FAIL;
    
    memset(&serial_port, 0, sizeof(struct serial_port));
    serial_port.port_nbr = 1;
    serial_port.tx_buffer_size = 128;
    serial_port.tmp_buffer_size = 200;
    serial_port.rx_scan_interval = 10;
    serial_port.end_char = kmalloc(strlen(word), GFP_KERNEL);
    memcpy(serial_port.end_char, word, strlen(word));
    serial_port.end_char_size = strlen(word);
    serial_port_open(&serial_port);

    while (1) {
        if (serial_port_receive(&serial_port) == true) {
            if (serial_port.rx_buffer[0] == '\r')
                printf("\r\n/ # ");
            else if (serial_port.rx_buffer[0] == 27)
                break;
            else {
                if (parse_test_all_case(serial_port.rx_buffer) == PASS)
                    ret = PASS;
                else
                    ret = FAIL;

                printf("\r\n/ # ");
            }

            kfree(serial_port.rx_buffer);
        }
    }

    serial_port_close(&serial_port);
    kfree(serial_port.end_char);
    
    return ret;
}

#endif

