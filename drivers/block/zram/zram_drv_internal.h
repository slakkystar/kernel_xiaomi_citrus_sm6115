#ifndef _ZRAM_DRV_INTERNAL_H_
#define _ZRAM_DRV_INTERNAL_H_

#include "zram_drv.h"

#ifdef BIT
#undef BIT
#define BIT(nr)		(1lu << (nr))
#endif

#ifdef CONFIG_HYBRIDSWAP_ASYNC_COMPRESS
extern int async_compress_page(struct zram *zram, struct page* page);
extern void update_zram_index(struct zram *zram, u32 index, unsigned long page);
#endif
#endif /* _ZRAM_DRV_INTERNAL_H_ */
