/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_CTYPE_H
#define _LINUX_CTYPE_H

/*
 * NOTE! This ctype does not handle EOF like the standard C
 * library is required to.
 */

#define _CTYPE_U	0x01	/* upper */
#define _CTYPE_L	0x02	/* lower */
#define _CTYPE_D	0x04	/* digit */
#define _CTYPE_C	0x08	/* cntrl */
#define _CTYPE_P	0x10	/* punct */
#define _CTYPE_S	0x20	/* white space (space/lf/tab) */
#define _CTYPE_X	0x40	/* hex digit */
#define _CTYPE_SP	0x80	/* hard space (0x20) */

extern const unsigned char _ctype[];

#define __ismask(x) (_ctype[(int)(unsigned char)(x)])

#define isalnum(c)	((__ismask(c)&(_CTYPE_U|_CTYPE_L|_CTYPE_D)) != 0)
#define isalpha(c)	((__ismask(c)&(_CTYPE_U|_CTYPE_L)) != 0)
#define iscntrl(c)	((__ismask(c)&(_CTYPE_C)) != 0)
static inline int isdigit(int c)
{
	return '0' <= c && c <= '9';
}
#define isgraph(c)	((__ismask(c)&(_CTYPE_P|_CTYPE_U|_CTYPE_L|_CTYPE_D)) != 0)
#define islower(c)	((__ismask(c)&(_CTYPE_L)) != 0)
#define isprint(c)	((__ismask(c)&(_CTYPE_P|_CTYPE_U|_CTYPE_L|_CTYPE_D|_CTYPE_SP)) != 0)
#define ispunct(c)	((__ismask(c)&(_CTYPE_P)) != 0)
/* Note: isspace() must return false for %NUL-terminator */
#define isspace(c)	((__ismask(c)&(_CTYPE_S)) != 0)
#define isupper(c)	((__ismask(c)&(_CTYPE_U)) != 0)
#define isxdigit(c)	((__ismask(c)&(_CTYPE_D|_CTYPE_X)) != 0)

#define isascii(c) (((unsigned char)(c))<=0x7f)
#define toascii(c) (((unsigned char)(c))&0x7f)

static inline unsigned char __tolower(unsigned char c)
{
	if (isupper(c))
		c -= 'A'-'a';
	return c;
}

static inline unsigned char __toupper(unsigned char c)
{
	if (islower(c))
		c -= 'a'-'A';
	return c;
}

#define tolower(c) __tolower(c)
#define toupper(c) __toupper(c)

/*
 * Fast implementation of tolower() for internal usage. Do not use in your
 * code.
 */
static inline char _tolower(const char c)
{
	return c | 0x20;
}

/* Fast check for octal digit */
static inline int isodigit(const char c)
{
	return c >= '0' && c <= '7';
}

#endif
