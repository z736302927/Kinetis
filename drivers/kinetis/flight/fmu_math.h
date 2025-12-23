#ifndef _FMU_MATH_H
#define _FMU_MATH_H

#include <linux/limits.h>

static inline unsigned int div32(unsigned int a, unsigned int b,
	unsigned int *r)
{
	if (b == 0) {
		*r = 0;
		return UINT_MAX;
	}
	*r = a % b;
	return a / b;
}

#endif /* _FMU_MATH_H */