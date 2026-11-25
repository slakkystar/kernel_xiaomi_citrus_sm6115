// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2021 Oplus. All rights reserved.
 */

#include <linux/sched.h>
#include <linux/reciprocal_div.h>
#include <../kernel/sched/sched.h>
#include <linux/special_opt/special_opt.h>

bool specopt_skip_balance(void)
{
	return (1 == sysctl_cpu_multi_thread) ? true : false;
}

bool is_heavy_load_task(struct task_struct *p)
{
	int cpu;
	unsigned long thresh_load;
	struct reciprocal_value spc_rdiv = reciprocal_value(100);

	if (!sysctl_cpu_multi_thread || !p)
		return false;

	cpu = task_cpu(p);
	thresh_load = capacity_orig_of(cpu) * HEAVY_LOAD_SCALE;
	if (task_util(p) > reciprocal_divide(thresh_load, spc_rdiv))
		return true;

	return false;
}
