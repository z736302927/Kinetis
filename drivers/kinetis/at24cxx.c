#include <linux/printk.h>
#include <linux/errno.h>
#include <linux/delay.h>

#include "kinetis/at24cxx.h"
#include "kinetis/iic_soft.h"
#include "kinetis/idebug.h"
#include "kinetis/delay.h"
#include "kinetis/design_verification.h"

/* AT24CXX device related constants and definitions */
#define AT24CXX_IIC                     IIC_HW_1
#define AT24CXX_ADDR                    0x50
#define PAGE_SIZE                       8
#define AT24CXX_MAX_ADDR                255
#define AT24CXX_VOLUME                  256

/* Timing related constants */
#define BYTE_WRITE_DELAY_MS             10    /* Byte write delay time */
#define PAGE_WRITE_DELAY_MS             10    /* Page write delay time */
#define SEQUENTIAL_WRITE_DELAY_MS       10    /* Sequential write delay time */
#define WRITE_CYCLE_TIMEOUT_MS          20    /* Write cycle timeout time */

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

static inline void at24cxx_port_multi_transmit(u8 addr, u8 *pdata, u32 length)
{
	iic_master_port_multi_transmmit(AT24CXX_IIC, AT24CXX_ADDR, addr, pdata, length);
}

static inline void at24cxx_port_multi_receive(u8 addr, u8 *pdata, u32 length)
{
	iic_master_port_multi_receive(AT24CXX_IIC, AT24CXX_ADDR, addr, pdata, length);
}
/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

void at24cxx_byte_write(u8 addr, u8 pdata)
{
	at24cxx_port_multi_transmit(addr, &pdata, 1);
	mdelay(BYTE_WRITE_DELAY_MS);
}

void at24cxx_page_write(u8 addr, u8 *pdata, u32 length)
{
	at24cxx_port_multi_transmit(addr, pdata, length);
	mdelay(PAGE_WRITE_DELAY_MS);
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
		if (num_of_page == 0) {
			at24cxx_page_write(addr, pdata, length);
		} else { /* length > PAGE_SIZE */
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
			} else { /* The remaining count position of the current page can write Num_of_Single data */
				at24cxx_page_write(addr, pdata, length);
			}
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
			if (num_of_single != 0) {
				at24cxx_page_write(addr, pdata, num_of_single);
			}
		}
	}
}

/* Write cycle counting and wear leveling functions */
#define WEAR_LEVELING_ENABLED           1
#define MAX_WRITE_CYCLES_PER_PAGE       100000  /* Maximum write cycles per page */
#define WEAR_LEVELING_THRESHOLD         1000    /* Wear leveling threshold */

static u32 at24cxx_write_cycles[AT24CXX_VOLUME / PAGE_SIZE] = {0}; /* Page write counter */
static u32 at24cxx_total_write_cycles = 0; /* Total write cycles */

void at24cxx_init_wear_leveling(void)
{
	memset(at24cxx_write_cycles, 0, sizeof(at24cxx_write_cycles));
	at24cxx_total_write_cycles = 0;
	pr_info("AT24CXX: Wear leveling initialized\n");
}

static u8 at24cxx_get_page_number(u8 addr)
{
	return addr / PAGE_SIZE;
}

void at24cxx_update_write_cycle_count(u8 addr, u32 length)
{
	u8 start_page, end_page;
	u32 pages_affected;

	if (!WEAR_LEVELING_ENABLED) {
		return;
	}

	start_page = at24cxx_get_page_number(addr);
	end_page = at24cxx_get_page_number(addr + length - 1);
	pages_affected = end_page - start_page + 1;

	/* Update write count for all affected pages */
	while (pages_affected > 0) {
		if (start_page < (AT24CXX_VOLUME / PAGE_SIZE)) {
			at24cxx_write_cycles[start_page]++;
			at24cxx_total_write_cycles++;
		}
		start_page++;
		pages_affected--;
	}

	/* Check if wear leveling is needed */
	at24cxx_check_wear_leveling();
}

void at24cxx_check_wear_leveling(void)
{
	u32 max_cycles = 0, min_cycles = UINT_MAX;
	u32 max_page = 0, min_page = 0;
	u32 i;

	if (!WEAR_LEVELING_ENABLED) {
		return;
	}

	/* Find pages with maximum and minimum write cycles */
	for (i = 0; i < (AT24CXX_VOLUME / PAGE_SIZE); i++) {
		if (at24cxx_write_cycles[i] > max_cycles) {
			max_cycles = at24cxx_write_cycles[i];
			max_page = i;
		}
		if (at24cxx_write_cycles[i] < min_cycles) {
			min_cycles = at24cxx_write_cycles[i];
			min_page = i;
		}
	}

	/* Trigger wear leveling if write cycle difference exceeds threshold */
	if ((max_cycles - min_cycles) > WEAR_LEVELING_THRESHOLD) {
		printk(KERN_WARNING "AT24CXX: Wear leveling needed. Max: %u cycles (page %u), Min: %u cycles (page %u)\n",
			max_cycles, max_page, min_cycles, min_page);
		pr_info("AT24CXX: Total write cycles: %u\n", at24cxx_total_write_cycles);
	}
}

void at24cxx_get_wear_leveling_info(u32 *p_max_cycles, u32 *p_min_cycles, u32 *p_total_cycles)
{
	u32 max_cycles = 0, min_cycles = UINT_MAX;
	u32 i;

	if (!WEAR_LEVELING_ENABLED) {
		*p_max_cycles = 0;
		*p_min_cycles = 0;
		*p_total_cycles = 0;
		return;
	}

	for (i = 0; i < (AT24CXX_VOLUME / PAGE_SIZE); i++) {
		if (at24cxx_write_cycles[i] > max_cycles) {
			max_cycles = at24cxx_write_cycles[i];
		}
		if (at24cxx_write_cycles[i] < min_cycles) {
			min_cycles = at24cxx_write_cycles[i];
		}
	}

	*p_max_cycles = max_cycles;
	*p_min_cycles = (min_cycles == UINT_MAX) ? 0 : min_cycles;
	*p_total_cycles = at24cxx_total_write_cycles;
}
/* Write function with retry mechanism */
int at24cxx_write_data_with_retry(u8 addr, u8 *pdata, u32 length, u8 max_retry)
{
	u8 retry_count = 0;
	int ret;

	if (addr + length > AT24CXX_VOLUME) {
		pr_err("AT24CXX: Address range exceeded: addr=0x%02x, length=%u, max_addr=0x%02x\n",
			addr, length, AT24CXX_VOLUME);
		return -EINVAL;
	}

	for (retry_count = 0; retry_count < max_retry; retry_count++) {
		ret = at24cxx_write_data(addr, pdata, length);
		if (ret == 0) {
			return 0; /* Write successful */
		}

		printk(KERN_WARNING "AT24CXX: Write retry %u/%u failed, retrying...\n",
			retry_count + 1, max_retry);
		mdelay(10); /* Wait before retry */
	}

	pr_err("AT24CXX: Write failed after %u retries\n", max_retry);
	return -EIO;
}

/* Write verification function */
int at24cxx_verify_write(u8 addr, u8 *pdata, u32 length)
{
	u8 verify_buffer[256]; /* Static buffer, suitable for small data verification */
	int i, ret;

	if (length > sizeof(verify_buffer)) {
		pr_err("AT24CXX: Buffer too small for verification (length: %u, buffer: %zu)\n",
			length, sizeof(verify_buffer));
		return -ENOMEM;
	}

	/* Read recently written data */
	ret = at24cxx_read_data(addr, verify_buffer, length);
	if (ret) {
		return ret;
	}

	/* Verify data consistency */
	for (i = 0; i < length; i++) {
		if (pdata[i] != verify_buffer[i]) {
			pr_err("AT24CXX: Verification failed at offset %u: expected 0x%02x, got 0x%02x\n",
				i, pdata[i], verify_buffer[i]);
			return -EIO;
		}
	}

	return 0;
}

int at24cxx_write_data(u8 addr, u8 *pdata, u32 length)
{
	if (addr + length > AT24CXX_VOLUME) {
		printk(KERN_ERR
			"There is not enough space left to write the specified length.\n");
		return -EINVAL;
	}

	at24cxx_multi_page_write(addr, pdata, length);
	mdelay(SEQUENTIAL_WRITE_DELAY_MS);

	/* Update write count */
	at24cxx_update_write_cycle_count(addr, length);

	return 0;
}

int at24cxx_current_addr_read(u8 *pdata)
{
	iic_master_soft_start(AT24CXX_IIC);
	iic_master_soft_send_byte(AT24CXX_IIC, (AT24CXX_ADDR << 1) | 0x01);

	if (iic_master_soft_wait_ack(AT24CXX_IIC)) {
		iic_master_soft_stop(AT24CXX_IIC);
		return -EPIPE;
	}

	*pdata = iic_master_soft_read_byte(AT24CXX_IIC, 0);
	iic_master_soft_stop(AT24CXX_IIC);

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

/* Device identification and status checking functions */
int at24cxx_device_detect(void)
{
	u8 dummy_data;

	/* Attempt to read device address for detection */
	if (iic_master_soft_start(AT24CXX_IIC)) {
		return -ENODEV;
	}

	iic_master_soft_send_byte(AT24CXX_IIC, (AT24CXX_ADDR << 1) | 0x01);

	if (iic_master_soft_wait_ack(AT24CXX_IIC)) {
		iic_master_soft_stop(AT24CXX_IIC);
		return -ENODEV;
	}

	/* Read one byte of data */
	dummy_data = iic_master_soft_read_byte(AT24CXX_IIC, 0);
	iic_master_soft_stop(AT24CXX_IIC);

	return 0;
}

int at24cxx_check_write_protection(void)
{
	u8 original_data, test_data = 0xAA;
	int ret;

	/* Backup original data */
	ret = at24cxx_read_data(AT24CXX_VOLUME - 1, &original_data, 1);
	if (ret) {
		return -EIO;
	}

	/* Attempt to write test data */
	at24cxx_byte_write(AT24CXX_VOLUME - 1, test_data);

	/* Read back data to check if write was successful */
	ret = at24cxx_read_data(AT24CXX_VOLUME - 1, &test_data, 1);
	if (ret) {
		return -EIO;
	}

	/* Restore original data */
	at24cxx_byte_write(AT24CXX_VOLUME - 1, original_data);

	/* If data hasn't changed, write protection is enabled */
	if (test_data != 0xAA) {
		return -EROFS; /* Read-only filesystem error */
	}

	return 0; /* Write normal */
}

int at24cxx_sequential_read(u8 *pdata, u32 length)
{
	iic_master_soft_start(AT24CXX_IIC);
	iic_master_soft_send_byte(AT24CXX_IIC, (AT24CXX_ADDR << 1) | 0x01);

	if (iic_master_soft_wait_ack(AT24CXX_IIC)) {
		iic_master_soft_stop(AT24CXX_IIC);
		return -EPIPE;
	}

	while (length) {
		if (length == 1) {
			*pdata = iic_master_soft_read_byte(AT24CXX_IIC, 0);
		} else {
			*pdata = iic_master_soft_read_byte(AT24CXX_IIC, 1);
		}

		pdata++;
		length--;
	}

	iic_master_soft_stop(AT24CXX_IIC);

	return 0;
}

#ifdef DESIGN_VERIFICATION_AT24CXX
#include "kinetis/test-kinetis.h"
#include "kinetis/random-gene.h"
#include "kinetis/basic-timer.h"

static u8 tx_buffer[AT24CXX_VOLUME];
static u8 rx_buffer[AT24CXX_VOLUME];

int t_at24cxx_loopback(int argc, char **argv)
{
	u16 length = 0;
	u32 test_addr = 0;
	u16 round = 8;
	u16 i = 0, j = 0;
	int ret;

	if (argc > 1) {
		round = simple_strtoul(argv[1], &argv[1], 10);
	}

	for (j = 0; j < round; j++) {
		length = random_get8bit();

		if (length <= 0) {
			length = 10;
		}

		test_addr = random_get8bit();

		if (test_addr + length > AT24CXX_VOLUME) {
			length = AT24CXX_VOLUME - test_addr;
		}

		memset(tx_buffer, 0, length);
		memset(rx_buffer, 0, length);
		pr_debug("test addr@0x%08x, length: %d.\n",
			test_addr, length);

		random_get_array(tx_buffer, length, RNG_8BITS);

		ret = at24cxx_write_data(test_addr, tx_buffer, length);

		if (ret) {
			return FAIL;
		}

		ret = at24cxx_read_data(test_addr, rx_buffer, length);

		if (ret) {
			return FAIL;
		}

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

	pr_debug("at24cxx Read and write TEST PASSED!\n");

	return PASS;
}

int t_at24cxx_current_addr_read(int argc, char **argv)
{
	u8 tmp = 0;
	int ret;

	ret = at24cxx_current_addr_read(&tmp);

	if (ret) {
		return FAIL;
	}

	pr_debug("at24cxx current address data %d.\n", tmp);

	return PASS;
}

int t_at24cxx_current_random_read(int argc, char **argv)
{
	u16 length = 0;
	u32 test_addr = 0;
	u16 round = 8;
	u16 i = 0;
	int ret;

	if (argc > 1) {
		round = simple_strtoul(argv[1], &argv[1], 10);
	}

	pr_debug("at24cxx random read %u round.\n", round);

	for (i = 0; i < round; i++) {
		test_addr = random_get8bit();
		length = random_get8bit() % (AT24CXX_VOLUME - test_addr);

		ret = current_random_read(test_addr, &rx_buffer[test_addr], length);

		if (ret) {
			return FAIL;
		}

		pr_debug("round[%4u], read addr@0x%08x, length: %u.\n",
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

	if (ret) {
		return FAIL;
	}

	pr_debug("at24cxx Sequential Read data %d.\n", tmp);

	return PASS;
}

int t_at24cxx_device_detect(int argc, char **argv)
{
	int ret;

	ret = at24cxx_device_detect();

	if (ret == 0) {
		pr_info("AT24CXX: Device detected successfully\n");
		return PASS;
	} else {
		pr_err("AT24CXX: Device detection failed with error %d\n", ret);
		return FAIL;
	}
}

int t_at24cxx_write_protection_check(int argc, char **argv)
{
	int ret;

	ret = at24cxx_check_write_protection();

	if (ret == 0) {
		pr_info("AT24CXX: Write protection check passed - write operation allowed\n");
		return PASS;
	} else if (ret == -EROFS) {
		printk(KERN_WARNING "AT24CXX: Write protection is enabled - device is read-only\n");
		return PASS; /* This is also normal, depending on specific hardware configuration */
	} else {
		pr_err("AT24CXX: Write protection check failed with error %d\n", ret);
		return FAIL;
	}
}

int t_at24cxx_wear_leveling_info(int argc, char **argv)
{
	u32 max_cycles, min_cycles, total_cycles;

	at24cxx_init_wear_leveling();
	at24cxx_get_wear_leveling_info(&max_cycles, &min_cycles, &total_cycles);

	pr_info("AT24CXX: Wear leveling info - Max cycles: %u, Min cycles: %u, Total cycles: %u\n",
		max_cycles, min_cycles, total_cycles);

	/* Simulate some write operations to test wear leveling */
	u8 test_data[16] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
			0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00
		};

	at24cxx_write_data(0, test_data, 8);
	at24cxx_write_data(8, test_data, 8);

	at24cxx_get_wear_leveling_info(&max_cycles, &min_cycles, &total_cycles);

	pr_info("AT24CXX: After test writes - Max cycles: %u, Min cycles: %u, Total cycles: %u\n",
		max_cycles, min_cycles, total_cycles);

	return PASS;
}

int t_at24cxx_write_with_retry(int argc, char **argv)
{
	u8 test_data[16] = {0xAA, 0x55, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC,
			0xDE, 0xF0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66
		};
	int ret;

	pr_debug("AT24CXX: Testing write with retry mechanism\n");

	ret = at24cxx_write_data_with_retry(0x10, test_data, 16, 3);

	if (ret) {
		pr_err("AT24CXX: Write with retry failed\n");
		return FAIL;
	}

	/* Verify written data */
	ret = at24cxx_verify_write(0x10, test_data, 16);

	if (ret) {
		pr_err("AT24CXX: Write verification failed\n");
		return FAIL;
	}

	pr_info("AT24CXX: Write with retry test passed\n");
	return PASS;
}

int t_at24cxx_edge_cases(int argc, char **argv)
{
	u8 test_byte = 0xFF;
	u8 read_byte;
	u8 test_buffer[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
	u8 read_buffer[8];
	int ret;

	/* Test 1: Write single byte to boundary address */
	pr_debug("AT24CXX: Test 1 - Single byte write at boundary address\n");
	at24cxx_byte_write(AT24CXX_VOLUME - 1, test_byte);

	ret = at24cxx_read_data(AT24CXX_VOLUME - 1, &read_byte, 1);
	if (ret || read_byte != test_byte) {
		pr_err("AT24CXX: Single byte read verification failed\n");
		return FAIL;
	}

	/* Test 2: Page boundary write */
	pr_debug("AT24CXX: Test 2 - Page boundary write\n");
	memset(read_buffer, 0, sizeof(read_buffer));
	ret = at24cxx_write_data(PAGE_SIZE - 2, test_buffer, 4); /* Cross-page write */
	if (ret) {
		pr_err("AT24CXX: Page boundary write failed\n");
		return FAIL;
	}

	ret = at24cxx_read_data(PAGE_SIZE - 2, read_buffer, 4);
	if (ret || memcmp(test_buffer, read_buffer, 4) != 0) {
		pr_err("AT24CXX: Page boundary read verification failed\n");
		return FAIL;
	}

	/* Test 3: Zero length write */
	pr_debug("AT24CXX: Test 3 - Zero length write\n");
	ret = at24cxx_write_data(0x20, test_buffer, 0);
	if (ret != 0) {
		pr_err("AT24CXX: Zero length write should succeed\n");
		return FAIL;
	}

	pr_info("AT24CXX: Edge cases test passed\n");
	return PASS;
}

int t_at24cxx_loopback_speed(int argc, char **argv)
{
	u32 time_stamp = 0;
	u16 i = 0;
	int ret;

	pr_debug("Starting at24cxx raw write test.\n");
	time_stamp = basic_timer_get_us();

	random_get_array(tx_buffer, AT24CXX_VOLUME, RNG_8BITS);

	ret = at24cxx_write_data(0, tx_buffer, AT24CXX_VOLUME);

	if (ret) {
		return FAIL;
	}

	time_stamp = basic_timer_get_us() - time_stamp;
	pr_debug("%u bytes written and it took %uus.\n",
		AT24CXX_VOLUME, time_stamp);

	pr_debug("Starting at24cxx raw read test.\n");
	time_stamp = basic_timer_get_us();

	ret = at24cxx_read_data(0, rx_buffer, AT24CXX_VOLUME);

	if (ret) {
		return FAIL;
	}

	time_stamp = basic_timer_get_us() - time_stamp;
	pr_debug("%u bytes read and it took %uus.\n",
		AT24CXX_VOLUME, time_stamp);
	pr_debug("Test completed.\n");

	return PASS;
}

#endif
