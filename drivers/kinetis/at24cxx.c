#include "kinetis/at24cxx.h"
#include "kinetis/iic_soft.h"
#include "kinetis/idebug.h"
#include "kinetis/delay.h"

#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/delay.h>

#define AT24CXX_IIC                     IIC_HW_1
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
    iic_port_multi_transmmit(AT24CXX_IIC, AT24CXX_ADDR, addr, pdata, length);
}

static inline void at24cxx_port_multi_receive(u8 addr, u8 *pdata, u32 length)
{
    iic_port_multi_receive(AT24CXX_IIC, AT24CXX_ADDR, addr, pdata, length);
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

int at24cxx_write_data(u8 addr, u8 *pdata, u32 length)
{
    if (addr + length > AT24CXX_VOLUME) {
        printk(KERN_ERR
            "There is not enough space left to write the specified length.\n");
        return -EINVAL;
    }

    at24cxx_multi_page_write(addr, pdata, length);

    return 0;
}

int at24cxx_current_addr_read(u8 *pdata)
{
    iic_soft_start(AT24CXX_IIC);
    iic_soft_send_byte(AT24CXX_IIC, (AT24CXX_ADDR << 1) | 0x01);

    if (iic_soft_wait_ack(AT24CXX_IIC)) {
        iic_soft_stop(AT24CXX_IIC);
        return -EPIPE;
    }

    *pdata = iic_soft_read_byte(AT24CXX_IIC, 0);
    iic_soft_stop(AT24CXX_IIC);

    return 0;
}

static int current_random_read(u8 addr, u8 *pdata, u32 length)
{
    if (addr + length > AT24CXX_VOLUME) {
        printk(KERN_ERR
            "There is not enough space left to read the specified length.\n");
        return -EINVAL;
    }

    at24cxx_port_multi_receive(addr, pdata, length);

    return 0;
}

int at24cxx_read_data(u8 addr, u8 *pdata, u32 length)
{
    return current_random_read(addr, pdata, length);
}

int at24cxx_sequential_read(u8 *pdata, u32 length)
{
    iic_soft_start(AT24CXX_IIC);
    iic_soft_send_byte(AT24CXX_IIC, (AT24CXX_ADDR << 1) | 0x01);

    if (iic_soft_wait_ack(AT24CXX_IIC)) {
        iic_soft_stop(AT24CXX_IIC);
        return -EPIPE;
    }

    while (length) {
        if (length == 1)
            *pdata = iic_soft_read_byte(AT24CXX_IIC, 0);
        else
            *pdata = iic_soft_read_byte(AT24CXX_IIC, 1);

        pdata++;
        length--;
    }

    iic_soft_stop(AT24CXX_IIC);

    return 0;
}

#ifdef DESIGN_VERIFICATION_AT24CXX
#include "kinetis/test-kinetis.h"
#include "kinetis/random-gene.h"
#include "kinetis/basic-timer.h"

#undef abs
#include "stdlib.h"
#include "string.h"

static u8 tx_buffer[AT24CXX_VOLUME];
static u8 rx_buffer[AT24CXX_VOLUME];

int t_at24cxx_loopback(int argc, char **argv)
{
    u16 length = 0;
    u32 test_addr = 0;
    u16 round = 8;
    u16 i = 0, j = 0;
    int ret;

    if (argc > 1)
        round = strtoul(argv[1], &argv[1], 10);

    for (j = 0; j < round; j++) {
        length = random_get8bit();

        if (length <= 0)
            length = 10;

        test_addr = random_get8bit();

        if (test_addr + length > AT24CXX_VOLUME)
            length = AT24CXX_VOLUME - test_addr;

        memset(tx_buffer, 0, length);
        memset(rx_buffer, 0, length);
        printk(KERN_DEBUG "test addr@0x%08x, length: %d.\n",
            test_addr, length);

        random_get_array(tx_buffer, length, RNG_8BITS);

        ret = at24cxx_write_data(test_addr, tx_buffer, length);

        if (ret)
            return FAIL;

        ret = at24cxx_read_data(test_addr, rx_buffer, length);

        if (ret)
            return FAIL;

        for (i = 0; i < length; i++) {
            if (tx_buffer[i] != rx_buffer[i]) {
                printk(KERN_DEBUG
                    "tx[%d]: %#02x, rx[%d]: %#02x\n",
                    i, tx_buffer[i], i, rx_buffer[i]);
                printk(KERN_ERR
                    "Data writes and reads do not match, TEST FAILED!\n");
                return FAIL;
            }
        }
    }

    printk(KERN_DEBUG "at24cxx Read and write TEST PASSED!\n");

    return PASS;
}

int t_at24cxx_current_addr_read(int argc, char **argv)
{
    u8 tmp = 0;
    int ret;

    ret = at24cxx_current_addr_read(&tmp);

    if (ret)
        return FAIL;

    printk(KERN_DEBUG "at24cxx current address data %d.\n", tmp);

    return PASS;
}

int t_at24cxx_current_random_read(int argc, char **argv)
{
    u16 length = 0;
    u32 test_addr = 0;
    u16 round = 8;
    u16 i = 0;
    int ret;

    if (argc > 1)
        round = strtoul(argv[1], &argv[1], 10);

    printk(KERN_DEBUG "at24cxx random read %u round.\n", round);
    
    for (i = 0; i < round; i++) {
        test_addr = random_get8bit();
        length = random_get8bit() % (AT24CXX_VOLUME - test_addr);
        
        ret = current_random_read(test_addr, &rx_buffer[test_addr], length);

        if (ret)
            return FAIL;

        printk(KERN_DEBUG "round[%4u], read addr@0x%08x, length: %u.\n",
            i, test_addr, length);

        print_hex_dump(KERN_DEBUG, "at24cxx rx: ", DUMP_PREFIX_OFFSET,
            16, 1,
            &rx_buffer[test_addr], length, false);
    }

    return PASS;
}

int t_at24cxx_sequential_read(int argc, char **argv)
{
    u8 tmp = 0;
    int ret;

    ret = at24cxx_sequential_read(&tmp, 1);

    if (ret)
        return FAIL;

    printk(KERN_DEBUG "at24cxx Sequential Read data %d.\n", tmp);

    return PASS;
}

int t_at24cxx_loopback_speed(int argc, char **argv)
{
    u32 time_stamp = 0;
    u16 i = 0;
    int ret;

    printk(KERN_DEBUG "Starting at24cxx raw write test.\n");
    time_stamp = basic_timer_get_us();

    random_get_array(tx_buffer, AT24CXX_VOLUME, RNG_8BITS);

    ret = at24cxx_write_data(0, tx_buffer, AT24CXX_VOLUME);

    if (ret)
        return FAIL;

    time_stamp = basic_timer_get_us() - time_stamp;
    printk(KERN_DEBUG "%u bytes written and it took %uus.\n",
        AT24CXX_VOLUME, time_stamp);

    printk(KERN_DEBUG "Starting at24cxx raw read test.\n");
    time_stamp = basic_timer_get_us();

    ret = at24cxx_read_data(0, rx_buffer, AT24CXX_VOLUME);

    if (ret)
        return FAIL;

    time_stamp = basic_timer_get_us() - time_stamp;
    printk(KERN_DEBUG "%u bytes read and it took %uus.\n",
        AT24CXX_VOLUME, time_stamp);
    printk(KERN_DEBUG "Test completed.\n");

    return PASS;
}

#endif
