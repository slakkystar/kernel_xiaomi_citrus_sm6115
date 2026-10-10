/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Minimal replacement for the vendor oplus_project.h.
 *
 * The original implementation reads the project/CDT data from the
 * board specific shared memory (SMEM), which is device specific and is
 * therefore NOT ported. Only the engineering version API that is used by
 * the generic system features (phoenix, shutdown_detect, hung_task_enhance)
 * is kept; it always reports a release build.
 */
#ifndef _OPLUS_PROJECT_H_
#define _OPLUS_PROJECT_H_

enum OPPO_ENG_VERSION {
	OEM_RELEASE		= 0x00,
	AGING			= 0x01,
	CTA			= 0x02,
	PERFORMANCE		= 0x03,
	PREVERSION		= 0x04,
	ALL_NET_CMCC_TEST	= 0x05,
	ALL_NET_CMCC_FIELD	= 0x06,
	ALL_NET_CU_TEST		= 0x07,
	ALL_NET_CU_FIELD	= 0x08,
	ALL_NET_CT_TEST		= 0x09,
	ALL_NET_CT_FIELD	= 0x0A,
	HIGH_TEMP_AGING		= 0x0B,
	FACTORY			= 0x0C,
};

static inline unsigned int get_eng_version(void)
{
	return OEM_RELEASE;
}

#endif /* _OPLUS_PROJECT_H_ */
