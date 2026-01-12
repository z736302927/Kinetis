#include <linux/random.h>

#include "kinetis/random-gene.h"
#include "kinetis/design_verification.h"

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  Modify four areas: GPIO_PORT/GPIO_PIN/Led_TypeDef/LEDn.
  * @step 3:  .
  * @step 4:  .
  * @step 5:
  */

#define USING_LINUX_LIBRARY 1

#if MCU_PLATFORM_STM32
#include "rng.h"
#endif

/* Linear Congruential Generator for pseudo-random numbers */
static u32 seed = 1;

static inline u32 lcg_random(void)
{
	seed = (seed * 1103515245 + 12345) & 0x7fffffff;
	return seed;
}

static inline u32 random_get_int(void)
{
#if MCU_PLATFORM_STM32
    return HAL_RNG_GetRandomNumber(&hrng);
#elif USING_LINUX_LIBRARY
    return get_random_u32();
#else
    return lcg_random();
#endif
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

u8 random_get8bit(void)
{
    return random_get_int() % 0xFF;
}

u16 random_get16bit(void)
{
    return random_get_int() % 0xFFFF;
}

u32 random_get32bit(void)
{
    return random_get_int();
}

u64 random_get64bit(void)
{
    u64 tmp;

    tmp = random_get_int();
    tmp |= ((u64)random_get_int()) << 32;

    return tmp;
}

void random_get_array(void *pdata, u32 length, u8 bits)
{
    u32 i;
    u8 *data_8bits, *data_16bits, *data_32bits, *data_64bits;

    switch (bits) {
        case RNG_8BITS:
            data_8bits = pdata;

            for (i = 0; i < length; ++i)
                data_8bits[i] = random_get_int() % 0xFF;

            break;

        case RNG_16BITS:
            data_16bits = pdata;

            for (i = 0; i < length; ++i)
                data_16bits[i] = random_get_int() % 0xFFFF;

            break;

        case RNG_32BITS:
            data_32bits = pdata;

            for (i = 0; i < length; ++i)
                data_32bits[i] = random_get_int();

            break;

        case RNG_64BITS:
            data_64bits = pdata;

            for (i = 0; i < length; ++i) {
                data_64bits[i] = random_get_int();
                data_64bits[i] |= ((u64)random_get_int()) << 32;
            }

            break;
    }
}

// u32 get_random_range(u32 min, u32 max)
// {
//     return random_get_int() % (max - min) + min;
// }

#ifdef DESIGN_VERIFICATION_DELAY
#include "kinetis/test-kinetis.h"
#include "kinetis/idebug.h"

#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/printk.h>
#include <linux/math64.h>

int t_random_number(int argc, char **argv)
{
    pr_debug("8 bits random number is %u\n", random_get8bit());

    pr_debug("16 bits random number is %u\n", random_get16bit());

    pr_debug("32 bits random number is %u\n", random_get32bit());

    pr_debug("64 bits random number is %llu\n", random_get64bit());

    return PASS;
}

int t_random_array(int argc, char **argv)
{
    u32 *pdata;
    u32 length = 10;
    u8 bits = RNG_32BITS;
    u16 i;

    if (argc > 1)
        length = simple_strtoul(argv[1], &argv[1], 10);

    if (argc > 2)
        bits = simple_strtoul(argv[2], &argv[2], 10);

    pdata = kmalloc(length, __GFP_ZERO);

    if (pdata == NULL) {
        pr_debug("Random array malloc failed !\n");
        return FAIL;
    }

    random_get_array(pdata, length, bits);
    pr_debug("random number is following\n");

    print_hex_dump(KERN_DEBUG, "random: ", DUMP_PREFIX_OFFSET,
        16, 1,
        pdata, length, false);

    return PASS;
}

/**
 * @brief 计算样本平均值
 */
static u32 calculate_mean(u32 *data, u32 size)
{
	u64 sum = 0;
	u32 i;

	for (i = 0; i < size; i++)
		sum += data[i];

	return (u32)(sum / size);
}

/**
 * @brief 计算样本标准差
 */
static u32 calculate_stddev(u32 *data, u32 size, u32 mean)
{
	u64 sum = 0;
	u32 i;

	for (i = 0; i < size; i++) {
		s64 diff = (s64)data[i] - (s64)mean;
		sum += diff * diff;
	}

	return (u32)int_sqrt(div_u64(sum, size));
}

/**
 * @brief 计算频率分布
 */
static void calculate_frequency(u32 *data, u32 size, u32 *freq, u32 bins)
{
	u32 i;
	u32 max_val = 0;
	u32 bin_size;

	/* 找出最大值 */
	for (i = 0; i < size; i++)
		if (data[i] > max_val)
			max_val = data[i];

	/* 计算每个 bin 的大小 */
	bin_size = (max_val / bins) + 1;

	/* 统计频率 */
	for (i = 0; i < size; i++) {
		u32 bin = data[i] / bin_size;
		if (bin >= bins)
			bin = bins - 1;
		freq[bin]++;
	}
}

/**
 * @brief 执行卡方检验
 */
static bool chi_square_test(u32 *freq, u32 bins, u32 total_samples)
{
	u64 chi_square = 0;
	u32 expected = total_samples / bins;
	u32 i;

	for (i = 0; i < bins; i++) {
		s64 diff = (s64)freq[i] - (s64)expected;
		chi_square += (diff * diff) / expected;
	}

	/* 自由度为 bins - 1，95% 置信度 */
	pr_info("Chi-square statistic: %llu\n", chi_square);
	pr_info("Expected: %d per bin, Freedom: %d\n", expected, bins - 1);

	return chi_square < (bins - 1) * 2;
}

/**
 * @brief 执行游程测试
 */
static bool runs_test(u32 *data, u32 size)
{
	u32 runs = 1;
	u32 i;
	u32 median;
	u32 *sorted;

	sorted = kmalloc(size * sizeof(u32), __GFP_ZERO);
	if (!sorted)
		return false;

	for (i = 0; i < size; i++)
		sorted[i] = data[i];

	/* 简单排序找到中位数 */
	for (i = 0; i < size; i++) {
		for (u32 j = i + 1; j < size; j++) {
			if (sorted[i] > sorted[j]) {
				u32 tmp = sorted[i];
				sorted[i] = sorted[j];
				sorted[j] = tmp;
			}
		}
	}

	median = sorted[size / 2];
	kfree(sorted);

	pr_info("Median value: %u\n", median);

	for (i = 1; i < size; i++) {
		u32 above_median = (data[i] > median);
		u32 prev_above_median = (data[i-1] > median);
		if (above_median != prev_above_median)
			runs++;
	}

	double expected_runs = ((2.0 * size) - 1.0) / 3.0;
	double stddev_runs = int_sqrt((16.0 * size - 29.0) / 90.0);
	double z_score = (runs - expected_runs) / stddev_runs;

	pr_info("Runs: %u, Expected: %.2f, Z-score: %.2f\n",
			runs, expected_runs, z_score);

	return (z_score >= -1.96) && (z_score <= 1.96);
}

/**
 * @brief 随机数概率统计测试
 * @param argv[1]: 样本数量（默认 1000）
 * @param argv[2]: bin 数量（默认 10）
 */
int t_random_statistics(int argc, char **argv)
{
	u32 sample_count = 1000;
	u32 bin_count = 10;
	u32 *data;
	u32 *frequency;
	u32 mean, stddev;
	bool chi_pass, runs_pass;
	u32 i;

	if (argc > 1)
		sample_count = simple_strtoul(argv[1], &argv[1], 10);

	if (argc > 2)
		bin_count = simple_strtoul(argv[2], &argv[2], 10);

	if (sample_count < 100) {
		pr_err("Sample count must be >= 100\n");
		return FAIL;
	}

	if (bin_count < 2 || bin_count > 20) {
		pr_err("Bin count must be between 2 and 20\n");
		return FAIL;
	}

	pr_info("=== Random Number Statistical Test ===\n");
	pr_info("Sample count: %u\n", sample_count);
	pr_info("Bin count: %u\n", bin_count);

	data = kmalloc(sample_count * sizeof(u32), __GFP_ZERO);
	frequency = kmalloc(bin_count * sizeof(u32), __GFP_ZERO);

	if (!data || !frequency) {
		pr_err("Memory allocation failed\n");
		kfree(data);
		kfree(frequency);
		return FAIL;
	}

	for (i = 0; i < sample_count; i++)
		data[i] = random_get32bit();

	mean = calculate_mean(data, sample_count);
	stddev = calculate_stddev(data, sample_count, mean);

	pr_info("\n--- Statistical Summary ---\n");
	pr_info("Mean: %u\n", mean);
	pr_info("Standard Deviation: %u\n", stddev);

	calculate_frequency(data, sample_count, frequency, bin_count);

	pr_info("\n--- Frequency Distribution ---\n");
	for (i = 0; i < bin_count; i++) {
		u32 bin_min = (i * (UINT_MAX / bin_count));
		u32 bin_max = ((i + 1) * (UINT_MAX / bin_count)) - 1;

		pr_info("Bin %2d [0x%08x-0x%08x]: %6u (%5.2f%%)\n",
				i, bin_min, bin_max, frequency[i],
				(frequency[i] * 100.0) / sample_count);
	}

	chi_pass = chi_square_test(frequency, bin_count, sample_count);
	pr_info("\n--- Chi-square Test ---\n");
	pr_info("Result: %s\n", chi_pass ? "PASS" : "FAIL");

	runs_pass = runs_test(data, sample_count);
	pr_info("\n--- Runs Test ---\n");
	pr_info("Result: %s\n", runs_pass ? "PASS" : "FAIL");

	kfree(data);
	kfree(frequency);

	pr_info("\n=== Test Summary ===\n");
	pr_info("Overall: %s\n", (chi_pass && runs_pass) ? "PASS" : "FAIL");

	return (chi_pass && runs_pass) ? PASS : FAIL;
}

#endif
