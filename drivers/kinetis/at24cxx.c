#include "kinetis/at24cxx.h"
#include "kinetis/iic_soft.h"
#include "kinetis/idebug.h"
#include "kinetis/delay.h"

#define AT24CXX_ADDR                    0x50
#define PAGE_SIZE                       8
#define AT24CXX_MAX_ADDR                255
#define AT24CXX_VOLUME                  256

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

static inline void at24cxx_port_multi_transmmit(u8 addr, u8 *pdata, u32 length)
{
    iic_port_multi_transmmit(IIC_1, AT24CXX_ADDR, addr, pdata, length);
}

static inline void at24cxx_port_multi_receive(u8 addr, u8 *pdata, u32 length)
{
    iic_port_multi_receive(IIC_1, AT24CXX_ADDR, addr, pdata, length);
}
/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

void at24cxx_byte_write(u8 addr, u8 pdata)
{
    at24cxx_port_multi_transmmit(addr, &pdata, 1);
    mdelay(5);
}

void at24cxx_page_write(u8 addr, u8 *pdata, u32 length)
{
    at24cxx_port_multi_transmmit(addr, pdata, length);
}

static void at24cxx_multi_page_write(u32 addr, u8 *pdata, u16 length)
{
    u8 num_of_page = 0, num_of_single = 0, sub_addr = 0, cnt = 0, remain_of_single = 0;

    /* Mod operation, if addr is an integer multiple of PAGE_SIZE, sub_addr value is 0 */
    sub_addr = addr % PAGE_SIZE;

    /* The difference count is just enough to line up to the page address */
    cnt = PAGE_SIZE - sub_addr;
    /* Figure out how many integer pages to write */
    num_of_page =  length / PAGE_SIZE;
    /* mod operation is used to calculate the number of bytes less than one page */
    num_of_single = length % PAGE_SIZE;

    /* sub_addr=0, then addr is just aligned by page */
    if (sub_addr == 0) {
        /* length < PAGE_SIZE */
        if (num_of_page == 0)
            at24cxx_page_write(addr, pdata, length);
        else { /* length > PAGE_SIZE */
            /* Let me write down all the integer pages */
            while (num_of_page--) {
                at24cxx_page_write(addr, pdata, PAGE_SIZE);
                addr +=  PAGE_SIZE;
                pdata += PAGE_SIZE;
            }

            /* If you have more than one page of data, write it down*/
            at24cxx_page_write(addr, pdata, num_of_single);
        }
    }
    /* If the address is not aligned with PAGE_SIZE */
    else {
        /* length < PAGE_SIZE */
        if (num_of_page == 0) {
            /* The remaining count positions on the current page are smaller than Num_of_Single */
            if (num_of_single > cnt) {
                remain_of_single = num_of_single - cnt;

                /* Fill in the front page first */
                at24cxx_page_write(addr, pdata, cnt);
                addr +=  cnt;
                pdata += cnt;

                /* Let me write the rest of the data */
                at24cxx_page_write(addr, pdata, remain_of_single);
            } else /* The remaining count position of the current page can write Num_of_Single data */
                at24cxx_page_write(addr, pdata, length);
        } else { /* length > PAGE_SIZE */
            /* The address is not aligned and the extra count is treated separately, not added to the operation */
            length -= cnt;
            num_of_page =  length / PAGE_SIZE;
            num_of_single = length % PAGE_SIZE;

            at24cxx_page_write(addr, pdata, cnt);
            addr +=  cnt;
            pdata += cnt;

            /* Write all the integer pages */
            while (num_of_page--) {
                at24cxx_page_write(addr, pdata, PAGE_SIZE);
                addr +=  PAGE_SIZE;
                pdata += PAGE_SIZE;
            }

            /* If you have more than one page of data, write it down */
            if (num_of_single != 0)
                at24cxx_page_write(addr, pdata, num_of_single);
        }
    }
}

void at24cxx_write_data(u8 addr, u8 *pdata, u32 length)
{
    u32 remain = 0;

    remain = AT24CXX_MAX_ADDR - addr;

    if (remain < length) {
        kinetis_print_trace(KERN_DEBUG,
            "There is not enough space left to write the specified length.");
        return ;
    }

    at24cxx_multi_page_write(addr, pdata, length);
}

void at24cxx_current_addr_read(u8 *pdata)
{
    iic_soft_start();
    iic_soft_send_byte((AT24CXX_ADDR << 1) | 0x01);

    if (iic_soft_wait_ack()) {
        iic_soft_stop();
        return ;
    }

    *pdata = iic_soft_read_byte(0);
    iic_soft_stop();
}

static void current_random_read(u8 addr, u8 *pdata, u32 length)
{
    u32 remain = 0;

    remain = AT24CXX_MAX_ADDR - addr;

    if (remain < length) {
        kinetis_print_trace(KERN_DEBUG,
            "There is not enough space left to read the specified length.");
        return ;
    }

    at24cxx_port_multi_receive(addr, pdata, length);
}

void at24cxx_read_data(u8 addr, u8 *pdata, u32 length)
{
    current_random_read(addr, pdata, length);
}

void at24cxx_sequential_read(u8 *pdata, u32 length)
{
    iic_soft_start();
    iic_soft_send_byte((AT24CXX_ADDR << 1) | 0x01);

    if (iic_soft_wait_ack()) {
        iic_soft_stop();
        return ;
    }

    while (length) {
        if (length == 1)
            *pdata = iic_soft_read_byte(0);
        else
            *pdata = iic_soft_read_byte(1);

        pdata++;
        length--;
    }

    iic_soft_stop();
}

#ifdef DESIGN_VERIFICATION_AT24CXX
#include "kinetis/test.h"
#include "stdlib.h"
#include "string.h"
#include "kinetis/rng.h"
#include "kinetis/basic-timer.h"

static u8 tx_buffer[AT24CXX_VOLUME];
static u8 rx_buffer[AT24CXX_VOLUME];

int t_at24cxx_loopback(int argc, char **argv)
{
    u16 length = 0;
    u32 test_addr = 0;
    u16 times = 128;
    u16 i = 0, j = 0;

    if (argc > 1)
        times = strtoul(argv[1], &argv[1], 10);

    for (j = 0; j < times; j++) {
        length = Random_Get8bit();

        if (length <= 0)
            length = 10;

        test_addr = Random_Get8bit();

        if (length >= (AT24CXX_MAX_ADDR - test_addr + 1))
            length = AT24CXX_MAX_ADDR - test_addr + 1;

        memset(tx_buffer, 0, length);
        memset(rx_buffer, 0, length);
        kinetis_print_trace(KERN_DEBUG, "test_addr: 0x%02X, length = %d.",
            test_addr, length);

        for (i = 0; i < length; i++)
            tx_buffer[i] = Random_Get8bit();

        at24cxx_write_data(test_addr, tx_buffer, length);
        at24cxx_read_data(test_addr, rx_buffer, length);

        for (i = 0; i < length; i++) {
            if (tx_buffer[i] != rx_buffer[i]) {
                kinetis_print_trace(KERN_DEBUG,
                    "tx[%d] = 0x%02X, rx[%d] = 0x%02X",
                    i, tx_buffer[i], i, rx_buffer[i]);
                kinetis_print_trace(KERN_DEBUG,
                    "Data writes and reads do not match, TEST FAILED !");
                return FAIL;
            }
        }
    }

    kinetis_print_trace(KERN_DEBUG, "at24cxx Read and write TEST PASSED !");

    return PASS;
}

int t_at24cxx_current_addr_read(int argc, char **argv)
{
    u8 tmp = 0;

    at24cxx_current_addr_read(&tmp);
    kinetis_print_trace(KERN_DEBUG, "at24cxx current address data %d", tmp);

    return PASS;
}

int t_current_random_read(int argc, char **argv)
{
    u16 length = 0;
    u32 test_addr = 0;
    u16 times = 128;
    u16 i = 0;

    if (argc > 1)
        times = strtoul(argv[1], &argv[1], 10);

    test_addr = Random_Get8bit();
    length = Random_Get8bit() % (AT24CXX_MAX_ADDR - test_addr);
    current_random_read(test_addr, &rx_buffer[test_addr], length);
    kinetis_print_trace(KERN_DEBUG, "at24cxx Random Read %u", times);

    for (i = 0; i < length; i++)
        kinetis_print_trace(KERN_DEBUG, "Data[%d] = %d",
            i, rx_buffer[test_addr + i]);

    return PASS;
}

int t_at24cxx_sequential_read(int argc, char **argv)
{
    u8 tmp = 0;

    at24cxx_sequential_read(&tmp, 1);
    kinetis_print_trace(KERN_DEBUG, "at24cxx Sequential Read data %d", tmp);

    return PASS;
}

int t_at24cxx_loopback_speed(int argc, char **argv)
{
    u32 time_stamp = 0;
    u16 i = 0;

    kinetis_print_trace(KERN_DEBUG, "Starting at24cxx raw write test");
    time_stamp = basic_timer_get_timer_cnt();

    for (i = 0; i < AT24CXX_VOLUME; i++)
        tx_buffer[i] = Random_Get8bit();

    at24cxx_write_data(0, tx_buffer, AT24CXX_VOLUME);

    time_stamp = basic_timer_get_timer_cnt() - time_stamp;
    kinetis_print_trace(KERN_DEBUG, "%u bytes written and it took %luus.",
        AT24CXX_VOLUME, time_stamp);

    kinetis_print_trace(KERN_DEBUG, "Starting at24cxx raw read test");
    time_stamp = basic_timer_get_timer_cnt();

    at24cxx_read_data(0, rx_buffer, AT24CXX_VOLUME);

    time_stamp = basic_timer_get_timer_cnt() - time_stamp;
    kinetis_print_trace(KERN_DEBUG, "%u bytes read and it took %luus.",
        AT24CXX_VOLUME, time_stamp);
    kinetis_print_trace(KERN_DEBUG, "Test completed.");

    return PASS;
}

#endif
