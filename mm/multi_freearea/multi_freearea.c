// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#include <linux/mm.h>
#include <linux/mmzone.h>
#include <linux/list.h>

#include "multi_freearea.h"

void list_sort_add(struct page *page, struct zone *zone, unsigned int order,
		   int mt)
{
	struct list_head *list = &zone->free_area[0][order].free_list[mt];
	unsigned long pfn = page_to_pfn(page);
	unsigned long segment = 0;
	int i;

	if (unlikely(pfn > zone->zone_label[FREE_AREA_COUNTS - 1].label)) {
		list = &zone->free_area[FREE_AREA_COUNTS - 1][order].free_list[mt];
		segment = zone->zone_label[FREE_AREA_COUNTS - 1].segment;
		goto add_page;
	}

	for (i = 0; i < FREE_AREA_COUNTS; i++) {
		if (pfn <= zone->zone_label[i].label) {
			list = &zone->free_area[i][order].free_list[mt];
			segment = zone->zone_label[i].segment;
			break;
		}
	}

add_page:
	if (pfn >= segment)
		list_add_tail(&page->lru, list);
	else
		list_add(&page->lru, list);
}

int page_to_flc(struct page *page)
{
	struct zone *zone = page_zone(page);
	unsigned long pfn = page_to_pfn(page);
	int flc;

	if (unlikely(pfn > zone->zone_label[FREE_AREA_COUNTS - 1].label))
		return FREE_AREA_COUNTS - 1;

	for (flc = 0; flc < FREE_AREA_COUNTS; flc++) {
		if (pfn <= zone->zone_label[flc].label)
			return flc;
	}

	return flc;
}

void ajust_zone_label(struct zone *zone)
{
	unsigned long prev_base;
	int i;

	for (i = 0; i < FREE_AREA_COUNTS; i++)
		zone->zone_label[i].label = zone->zone_start_pfn +
			zone->spanned_pages * (i + 1) / FREE_AREA_COUNTS;

	for (i = 0; i < FREE_AREA_COUNTS; i++) {
		if (i == 0)
			prev_base = zone->zone_start_pfn;
		else
			prev_base = zone->zone_label[i - 1].label;

		zone->zone_label[i].segment = prev_base +
			((zone->zone_label[i].label - prev_base) >> 1);
	}
}

unsigned int ajust_flc(unsigned int current_flc, unsigned int order)
{
	/*
	 * For large blocks allocate from the opposite end of the zone so
	 * that high-order free areas are not mixed with small allocations.
	 */
	if (order >= HIGH_ORDER_TO_FLC)
		return FREE_AREA_COUNTS - 1 - current_flc;

	return current_flc;
}
