// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */


#ifdef CONFIG_OPLUS_FEATURE_SCHED_ASSIST
#ifdef CONFIG_OPLUS_SYSTEM_KERNEL_QCOM

#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/sched_assist/sched_assist_workqueue.h>

int is_uxwork(struct work_struct *work)
{
	if (!sysctl_sched_assist_enabled)
		return 0;

	return work->ux_work;
}

bool set_uxwork(struct work_struct *work)
{
	if (!sysctl_sched_assist_enabled)
		return false;

	work->ux_work = 1;
	return true;
}

bool unset_uxwork(struct work_struct *work)
{
	if (!sysctl_sched_assist_enabled)
		return false;

	work->ux_work = 0;
	return true;
}

void set_ux_worker_task(struct task_struct *task)
{
	task->ux_state = 1;
}

void reset_ux_worker_task(struct task_struct *task)
{
	task->ux_state = 0;
}

#endif /* CONFIG_OPLUS_SYSTEM_KERNEL_QCOM */
#endif
