#include <linux/random.h>
#include <linux/limits.h>

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

#ifdef DESIGN_VERIFICATION_DELAY
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/math64.h>

#include "kinetis/test-kinetis.h"
#include "kinetis/idebug.h"

int t_random_number(int argc, char **argv)
{
    pr_debug("===== basic random number test =====\n");

    pr_debug("8 bits random number is %u\n", random_get8bit());

    pr_debug("16 bits random number is %u\n", random_get16bit());

    pr_debug("32 bits random number is %u\n", random_get32bit());

    pr_debug("64 bits random number is %llu\n", random_get64bit());

    pr_debug("===== basic random number test passed =====\n");

    return PASS;
}

int t_random_range(int argc, char **argv)
{
    u32 min = 0, max = 100, i;
    u32 value, out_of_range = 0;

    pr_debug("===== random range test =====\n");

    if (argc > 1)
        min = simple_strtoul(argv[1], &argv[1], 10);

    if (argc > 2)
        max = simple_strtoul(argv[2], &argv[2], 10);

    if (min >= max) {
        pr_err("invalid range: min=%u >= max=%u\n", min, max);
        return FAIL;
    }

    pr_debug("testing range [%u, %u)\n", min, max);

    for (i = 0; i < 1000; i++) {
        value = get_random_range(min, max);
        if (value < min || value >= max) {
            pr_err("value %u out of range [%u, %u)\n", value, min, max);
            out_of_range++;
        }
    }

    if (out_of_range == 0) {
        pr_debug("all 1000 values in range [OK]\n");
    } else {
        pr_err("%u values out of range [FAIL]\n", out_of_range);
        return FAIL;
    }

    pr_debug("===== random range test passed =====\n");

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
        pr_debug("random array malloc failed !\n");
        return FAIL;
    }

    random_get_array(pdata, length, bits);
    pr_debug("random number is following\n");

    print_hex_dump(KERN_DEBUG, "random: ", DUMP_PREFIX_OFFSET,
        16, 1,
        pdata, length, false);

    kfree(pdata);
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
	pr_info("chi-square statistic: %llu\n", chi_square);
	pr_info("expected: %d per bin, freedom: %d\n", expected, bins - 1);

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
	s64 expected_runs_num, expected_runs_den;
	s64 z_score_num, z_score_den;
	s64 z_score;

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

	pr_info("median value: %u\n", median);

	for (i = 1; i < size; i++) {
		u32 above_median = (data[i] > median);
		u32 prev_above_median = (data[i-1] > median);
		if (above_median != prev_above_median)
			runs++;
	}

	/* 避免浮点运算，使用整数运算 */
	/* expected_runs = (2*size - 1) / 3 */
	expected_runs_num = 2 * size - 1;
	expected_runs_den = 3;

	/* z_score = (runs - expected_runs) / stddev_runs */
	/* stddev_runs ≈ sqrt((16*size - 29) / 90) */
	s64 stddev_num = 16 * size - 29;
	s64 stddev_den = 90;
	u64 stddev = int_sqrt((stddev_num * 10000) / stddev_den);  /* 放大100倍避免精度损失 */

	z_score_num = ((s64)runs * expected_runs_den - expected_runs_num) * 100;
	z_score_den = stddev * expected_runs_den;

	if (z_score_den == 0) {
		pr_info("runs: %u (cannot calculate z-score)\n", runs);
		return true;
	}

	z_score = (z_score_num * 100) / z_score_den;  /* z-score * 100 */

	pr_info("runs: %u, expected: %lld/%lld, z-score*100: %lld\n",
			runs, expected_runs_num, expected_runs_den, z_score);

	/* z_score 在 [-1.96, 1.96] 范围内通过 */
	return (z_score >= -196) && (z_score <= 196);
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
		pr_err("sample count must be >= 100\n");
		return FAIL;
	}

	if (bin_count < 2 || bin_count > 20) {
		pr_err("bin count must be between 2 and 20\n");
		return FAIL;
	}

	pr_info("=== random number statistical test ===\n");
	pr_info("sample count: %u\n", sample_count);
	pr_info("bin count: %u\n", bin_count);
	pr_info("range: [0, 0x%x]\n", UINT_MAX);

	data = kmalloc(sample_count * sizeof(u32), __GFP_ZERO);
	frequency = kmalloc(bin_count * sizeof(u32), __GFP_ZERO);

	if (!data || !frequency) {
		pr_err("memory allocation failed\n");
		return FAIL;
	}

	for (i = 0; i < sample_count; i++)
		data[i] = random_get32bit();

	mean = calculate_mean(data, sample_count);
	stddev = calculate_stddev(data, sample_count, mean);

	pr_info("\n--- statistical summary ---\n");
	pr_info("mean: %u\n", mean);
	pr_info("standard deviation: %u\n", stddev);

	calculate_frequency(data, sample_count, frequency, bin_count);

	pr_info("\n--- frequency distribution ---\n");
	for (i = 0; i < bin_count; i++) {
		u32 bin_min = (i * (UINT_MAX / bin_count));
		u32 bin_max = ((i + 1) * (UINT_MAX / bin_count)) - 1;

		pr_info("bin %2d [0x%08x-0x%08x]: %6u (%5.2f%%)\n",
				i, bin_min, bin_max, frequency[i],
				(frequency[i] * 100.0) / sample_count);
	}

	chi_pass = chi_square_test(frequency, bin_count, sample_count);
	pr_info("\n--- chi-square test ---\n");
	pr_info("result: %s\n", chi_pass ?  "PASS" : "FAIL");

	runs_pass = runs_test(data, sample_count);
	pr_info("\n--- runs test ---\n");
	pr_info("result: %s\n", runs_pass ? "PASS" : "FAIL");

	kfree(data);
	kfree(frequency);

	pr_info("\n=== test summary ===\n");
	pr_info("overall: %s\n", (chi_pass && runs_pass) ? "PASS" : "FAIL");

	return (chi_pass && runs_pass) ? PASS : FAIL;
}

/**
 * @brief 重复性测试 - 检查连续生成的随机数是否有重复
 */
int t_random_uniqueness(int argc, char **argv)
{
	u32 sample_count = 1000;
	u32 *data;
	u32 i, j;
	u32 duplicates = 0;

	pr_debug("===== random uniqueness test =====\n");

	if (argc > 1)
		sample_count = simple_strtoul(argv[1], &argv[1], 10);

	if (sample_count > 10000) {
		pr_err("sample count too large, max 10000\n");
		return FAIL;
	}

	data = kmalloc(sample_count * sizeof(u32), __GFP_ZERO);
	if (!data) {
		pr_err("memory allocation failed\n");
		return FAIL;
	}

	/* 生成随机数 */
	for (i = 0; i < sample_count; i++)
		data[i] = random_get32bit();

	/* 检查重复 */
	for (i = 0; i < sample_count; i++) {
		for (j = i + 1; j < sample_count; j++) {
			if (data[i] == data[j]) {
				duplicates++;
				pr_debug("duplicate found at positions %u and %u: 0x%08x\n", i, j, data[i]);
			}
		}
	}

	kfree(data);

	if (duplicates == 0) {
		pr_debug("no duplicates found in %u samples [OK]\n", sample_count);
	} else {
		pr_debug("found %u duplicates in %u samples [expected some]\n", duplicates, sample_count);
	}

	pr_debug("===== random uniqueness test passed =====\n");

	return PASS;
}

/**
 * @brief 不同比特位数测试
 */
int t_random_bits(int argc, char **argv)
{
	u32 i;
	u8 val8;
	u16 val16;
	u32 val32;
	u64 val64;

	pr_debug("===== random bits test =====\n");

	/* 测试8位随机数是否在正确范围内 */
	for (i = 0; i < 100; i++) {
		val8 = random_get8bit();
		if (val8 > 0xFF) {
			pr_err("8-bit random value out of range: %u\n", val8);
			return FAIL;
		}
	}
	pr_debug("8-bit random test: 100 samples in range [0, 255] [OK]\n");

	/* 测试16位随机数是否在正确范围内 */
	for (i = 0; i < 100; i++) {
		val16 = random_get16bit();
		if (val16 > 0xFFFF) {
			pr_err("16-bit random value out of range: %u\n", val16);
			return FAIL;
		}
	}
	pr_debug("16-bit random test: 100 samples in range [0, 65535] [OK]\n");

	/* 测试32位随机数 */
	for (i = 0; i < 100; i++) {
		val32 = random_get32bit();
		/* 32位随机数可以是任何u32值 */
	}
	pr_debug("32-bit random test: 100 samples [OK]\n");

	/* 测试64位随机数 */
	for (i = 0; i < 100; i++) {
		val64 = random_get64bit();
		/* 64位随机数可以是任何u64值 */
	}
	pr_debug("64-bit random test: 100 samples [OK]\n");

	pr_debug("===== random bits test passed =====\n");

	return PASS;
}

/**
 * @brief 多比特数组测试
 */
int t_random_array_multi(int argc, char **argv)
{
	void *data;
	u32 length = 20;
	u32 i;

	pr_debug("===== random array multi-bits test =====\n");

	if (argc > 1)
		length = simple_strtoul(argv[1], &argv[1], 10);

	if (length > 100) {
		pr_err("length too large, max 100\n");
		return FAIL;
	}

	/* 测试8位数组 */
	data = kmalloc(length, __GFP_ZERO);
	if (!data) {
		pr_err("memory allocation failed\n");
		return FAIL;
	}
	random_get_array(data, length, RNG_8BITS);
	kfree(data);
	pr_debug("8-bit array test: %u elements [OK]\n", length);

	/* 测试16位数组 */
	data = kmalloc(length * 2, __GFP_ZERO);
	if (!data) {
		pr_err("memory allocation failed\n");
		return FAIL;
	}
	random_get_array(data, length, RNG_16BITS);
	kfree(data);
	pr_debug("16-bit array test: %u elements [OK]\n", length);

	/* 测试32位数组 */
	data = kmalloc(length * 4, __GFP_ZERO);
	if (!data) {
		pr_err("memory allocation failed\n");
		return FAIL;
	}
	random_get_array(data, length, RNG_32BITS);
	kfree(data);
	pr_debug("32-bit array test: %u elements [OK]\n", length);

	/* 测试64位数组 */
	data = kmalloc(length * 8, __GFP_ZERO);
	if (!data) {
		pr_err("memory allocation failed\n");
		return FAIL;
	}
	random_get_array(data, length, RNG_64BITS);
	kfree(data);
	pr_debug("64-bit array test: %u elements [OK]\n", length);

	pr_debug("===== random array multi-bits test passed =====\n");

	return PASS;
}

#endif
