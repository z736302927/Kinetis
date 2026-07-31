#ifndef __WIFI_CC_H__
#define __WIFI_CC_H__

#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <linux/printk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LWIP_PLATFORM_DIAG(x)    {printk x;}
#define LWIP_PLATFORM_ASSERT(x)  {pr_err("LWIP ASSERT: %s (%s:%d)\n", #x, __FILE__, __LINE__); while(1);}

typedef u8   u8_t;
typedef int8_t    s8_t;
typedef u16  u16_t;
typedef int16_t   s16_t;
typedef u32  u32_t;
typedef int32_t   s32_t;

typedef uintptr_t mem_ptr_t;

#define U16_F                 PRIu16
#define S16_F                 PRId16
#define X16_F                 PRIx16
#define U32_F                 PRIu32
#define S32_F                 PRId32
#define X32_F                 PRIx32

#define SZT_F                 "zu"

typedef int sys_prot_t;

#ifdef __GNUC__
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT    __attribute__ ((__packed__))
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x
#elif defined( __IAR_SYSTEMS_ICC__ )
#define PACK_STRUCT_BEGIN     __packed
#define PACK_STRUCT_STRUCT
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x
#else
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x
#endif

#ifndef BYTE_ORDER
#define BYTE_ORDER            LITTLE_ENDIAN
#endif

extern u32 host_platform_random(void);
#define LWIP_RAND() host_platform_random()

#ifdef __cplusplus
}
#endif

#endif /* __WIFI_CC_H__ */
