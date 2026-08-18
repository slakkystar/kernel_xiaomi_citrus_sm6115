/*
 * auto_cut_charge.c - Auto cut charge feature for battery protection
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 */

#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/autocut.h>
#include <linux/pmic-voter.h>

#include "../../drivers/power/supply/qcom/smb5-lib.h"

#define DEFAULT_CUT_THRESHOLD_SOC      100
#define DEFAULT_RECHECK_SOC            95
#define DEFAULT_MAX_CAPACITY           100
#define DEFAULT_LONGEVITY_MODE         0
#define FALLBACK_CHECK_INTERVAL_MS     3600000  /* 60 minutes backup */

int auto_cut_charge_enabled = 1;
int auto_cut_charge_threshold = DEFAULT_CUT_THRESHOLD_SOC;
int auto_cut_charge_recheck_soc = DEFAULT_RECHECK_SOC;
int auto_cut_charge_current_soc = 0;
bool auto_cut_charge_active = false;
int auto_cut_charge_max_capacity = DEFAULT_MAX_CAPACITY;
bool auto_cut_charge_longevity_mode = false;

static struct smb_charger *global_chg = NULL;
static struct delayed_work fallback_work;
static bool work_scheduled = false;
static struct kobject *auto_cut_kobj;

/* Statistics */
static int cut_count = 0;
static int last_cut_soc = 0;
static unsigned long last_cut_time = 0;
static int avg_drain_rate = 0;

/* Dynamic state */
static int last_soc = -1;
static unsigned long last_update_time = 0;
static int drain_rate_ma_per_s = 0;

module_param(auto_cut_charge_enabled, int, 0644);
module_param(auto_cut_charge_threshold, int, 0644);
module_param(auto_cut_charge_recheck_soc, int, 0644);
module_param(auto_cut_charge_max_capacity, int, 0644);
module_param(auto_cut_charge_longevity_mode, bool, 0644);
MODULE_PARM_DESC(auto_cut_charge_enabled, "Enable auto cut charge (0=off, 1=on)");
MODULE_PARM_DESC(auto_cut_charge_threshold, "Battery percentage to stop charging (default 100)");
MODULE_PARM_DESC(auto_cut_charge_recheck_soc, "Battery percentage to resume charging (default 95)");
MODULE_PARM_DESC(auto_cut_charge_max_capacity, "Maximum capacity limit (80-100) for longevity");
MODULE_PARM_DESC(auto_cut_charge_longevity_mode, "Enable longevity mode (limit to 80%)");

static void fallback_work_handler(struct work_struct *work);
static DECLARE_DELAYED_WORK(fallback_work, fallback_work_handler);

static int calculate_drain_rate(int new_soc, unsigned long now_ms)
{
	if (last_soc < 0 || last_update_time == 0)
		return 0;

	if (new_soc >= last_soc) /* Not discharging */
		return 0;

	unsigned long delta_ms = now_ms - last_update_time;
	if (delta_ms < 100) /* Too short, ignore */
		return 0;

	int soc_delta = last_soc - new_soc;
	/* drain rate in % per second */
	int rate = (soc_delta * 1000) / delta_ms;
	return rate;
}

void auto_cut_charge_update_soc(int soc)
{
	auto_cut_charge_current_soc = soc;
}

void auto_cut_charge_reset(void)
{
	if (auto_cut_charge_active) {
		auto_cut_charge_active = false;
		if (global_chg) {
			vote(global_chg->chg_disable_votable, AUTO_CUT_VOTER, false, 0);
			pr_info("Auto Cut Charge: Reset - charging re-enabled\n");
		}
	}
	cancel_delayed_work_sync(&fallback_work);
	work_scheduled = false;
	last_soc = -1;
	last_update_time = 0;
}

void auto_cut_charge_check(struct smb_charger *chg, int soc, int current_ma, int temp)
{
	if (!auto_cut_charge_enabled || !chg) {
		if (work_scheduled) {
			cancel_delayed_work_sync(&fallback_work);
			work_scheduled = false;
		}
		global_chg = NULL;
		return;
	}

	if (!global_chg)
		global_chg = chg;

	auto_cut_charge_current_soc = soc;

	/* Fail-safe: if battery below 5%, force resume and bypass logic */
	if (soc < 5) {
		if (auto_cut_charge_active) {
			vote(global_chg->chg_disable_votable, AUTO_CUT_VOTER, false, 0);
			auto_cut_charge_active = false;
			pr_info("Auto Cut Charge: Emergency resume at %d%%\n", soc);
		}
		goto schedule_fallback;
	}

	/* Calculate drain rate */
	unsigned long now = jiffies;
	int drain = calculate_drain_rate(soc, jiffies_to_msecs(now));
	if (drain > 0) {
		drain_rate_ma_per_s = drain;
		last_soc = soc;
		last_update_time = jiffies_to_msecs(now);
	} else if (soc != last_soc) {
		last_soc = soc;
		last_update_time = jiffies_to_msecs(now);
	}

	/* Determine effective limits based on temperature and mode */
	int user_limit = auto_cut_charge_longevity_mode ? 80 : auto_cut_charge_max_capacity;
	/* Thermal compensation: reduce limit if hot */
	int effective_limit = user_limit;
	if (temp > 420) /* >42°C */
		effective_limit = min(80, user_limit);
	else if (temp > 370) /* 37-42°C */
		effective_limit = min(90, user_limit);
	/* Respect user threshold (for cut) */
	if (auto_cut_charge_threshold < effective_limit)
		effective_limit = auto_cut_charge_threshold;

	int effective_recheck = auto_cut_charge_recheck_soc;
	/* Dynamic recheck based on drain rate */
	if (drain_rate_ma_per_s > 30) /* High drain (gaming) */
		effective_recheck = max(85, effective_recheck - 10);
	else if (drain_rate_ma_per_s > 10)
		effective_recheck = max(88, effective_recheck - 5);

	bool should_cut = false;
	bool should_resume = false;

	if (!auto_cut_charge_active) {
		/* Check for true full: SOC >= effective_limit AND current < 150mA */
		if (soc >= effective_limit && current_ma < 150) {
			should_cut = true;
		}
	} else {
		if (soc <= effective_recheck) {
			should_resume = true;
		}
	}

	if (should_cut) {
		vote(global_chg->chg_disable_votable, AUTO_CUT_VOTER, true, 0);
		auto_cut_charge_active = true;
		cut_count++;
		last_cut_soc = soc;
		last_cut_time = now;
		pr_info("Auto Cut Charge: CUT at %d%%, Temp=%d, I=%dmA, drain=%d%%/s\n",
			soc, temp, current_ma, drain_rate_ma_per_s);
	} else if (should_resume) {
		vote(global_chg->chg_disable_votable, AUTO_CUT_VOTER, false, 0);
		auto_cut_charge_active = false;
		pr_info("Auto Cut Charge: RESUME at %d%%, drain=%d%%/s\n",
			soc, drain_rate_ma_per_s);
	}

schedule_fallback:
	/* Schedule fallback work only if not already scheduled */
	if (!work_scheduled) {
		schedule_delayed_work(&fallback_work, msecs_to_jiffies(FALLBACK_CHECK_INTERVAL_MS));
		work_scheduled = true;
	}
}

static void fallback_work_handler(struct work_struct *work)
{
	if (!auto_cut_charge_enabled || !global_chg) {
		work_scheduled = false;
		return;
	}

	/*
	 * Fallback: re-evaluate using cached SOC and read current/temp if possible.
	 * We rely on the fact that bms_update_work will call us more often,
	 * so this is just a safety net.
	 */
	int soc = auto_cut_charge_current_soc;
	int current_ma = 0, temp = 250; /* default */
	union power_supply_propval val;
	int rc;

	if (global_chg->bms_psy) {
		rc = power_supply_get_property(global_chg->bms_psy,
					       POWER_SUPPLY_PROP_CURRENT_NOW, &val);
		if (!rc)
			current_ma = val.intval / 1000; /* uA to mA */
		rc = power_supply_get_property(global_chg->bms_psy,
					       POWER_SUPPLY_PROP_TEMP, &val);
		if (!rc)
			temp = val.intval;
	}

	auto_cut_charge_check(global_chg, soc, current_ma, temp);

	/* Reschedule if still enabled */
	if (auto_cut_charge_enabled && global_chg) {
		schedule_delayed_work(&fallback_work, msecs_to_jiffies(FALLBACK_CHECK_INTERVAL_MS));
	}
}

/* Sysfs attributes */
static ssize_t enabled_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", READ_ONCE(auto_cut_charge_enabled));
}

static ssize_t enabled_store(struct kobject *kobj, struct kobj_attribute *attr,
			    const char *buf, size_t count)
{
	int val;
	if (sscanf(buf, "%d", &val) != 1)
		return -EINVAL;
	if (val < 0 || val > 1)
		val = 0;

	WRITE_ONCE(auto_cut_charge_enabled, val);
	if (!val)
		auto_cut_charge_reset();

	return count;
}

static ssize_t threshold_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", READ_ONCE(auto_cut_charge_threshold));
}

static ssize_t threshold_store(struct kobject *kobj, struct kobj_attribute *attr,
			     const char *buf, size_t count)
{
	int val;
	if (sscanf(buf, "%d", &val) != 1)
		return -EINVAL;
	if (val < 50 || val > 100)
		val = DEFAULT_CUT_THRESHOLD_SOC;
	WRITE_ONCE(auto_cut_charge_threshold, val);
	return count;
}

static ssize_t recheck_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", READ_ONCE(auto_cut_charge_recheck_soc));
}

static ssize_t recheck_store(struct kobject *kobj, struct kobj_attribute *attr,
			    const char *buf, size_t count)
{
	int val;
	if (sscanf(buf, "%d", &val) != 1)
		return -EINVAL;
	if (val < 50 || val > 100)
		val = DEFAULT_RECHECK_SOC;
	WRITE_ONCE(auto_cut_charge_recheck_soc, val);
	return count;
}

static ssize_t active_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", auto_cut_charge_active ? 1 : 0);
}

static ssize_t current_soc_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", auto_cut_charge_current_soc);
}

static ssize_t max_capacity_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", READ_ONCE(auto_cut_charge_max_capacity));
}

static ssize_t max_capacity_store(struct kobject *kobj, struct kobj_attribute *attr,
				  const char *buf, size_t count)
{
	int val;
	if (sscanf(buf, "%d", &val) != 1)
		return -EINVAL;
	if (val < 80 || val > 100)
		val = DEFAULT_MAX_CAPACITY;
	WRITE_ONCE(auto_cut_charge_max_capacity, val);
	return count;
}

static ssize_t longevity_mode_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", auto_cut_charge_longevity_mode ? 1 : 0);
}

static ssize_t longevity_mode_store(struct kobject *kobj, struct kobj_attribute *attr,
				    const char *buf, size_t count)
{
	int val;
	if (sscanf(buf, "%d", &val) != 1)
		return -EINVAL;
	WRITE_ONCE(auto_cut_charge_longevity_mode, val ? true : false);
	return count;
}

static ssize_t cut_count_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", cut_count);
}

static ssize_t last_cut_soc_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", last_cut_soc);
}

static ssize_t last_cut_time_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	if (last_cut_time == 0)
		return sprintf(buf, "0\n");
	return sprintf(buf, "%lu\n", last_cut_time);
}

static ssize_t drain_rate_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", drain_rate_ma_per_s);
}

static struct kobj_attribute enabled_attr =
	__ATTR(enabled, 0664, enabled_show, enabled_store);

static struct kobj_attribute threshold_attr =
	__ATTR(threshold, 0664, threshold_show, threshold_store);

static struct kobj_attribute recheck_attr =
	__ATTR(recheck_soc, 0664, recheck_show, recheck_store);

static struct kobj_attribute active_attr =
	__ATTR(active, 0444, active_show, NULL);

static struct kobj_attribute current_soc_attr =
	__ATTR(current_soc, 0444, current_soc_show, NULL);

static struct kobj_attribute max_capacity_attr =
	__ATTR(max_capacity, 0664, max_capacity_show, max_capacity_store);

static struct kobj_attribute longevity_mode_attr =
	__ATTR(longevity_mode, 0664, longevity_mode_show, longevity_mode_store);

static struct kobj_attribute cut_count_attr =
	__ATTR(cut_count, 0444, cut_count_show, NULL);

static struct kobj_attribute last_cut_soc_attr =
	__ATTR(last_cut_soc, 0444, last_cut_soc_show, NULL);

static struct kobj_attribute last_cut_time_attr =
	__ATTR(last_cut_time, 0444, last_cut_time_show, NULL);

static struct kobj_attribute drain_rate_attr =
	__ATTR(drain_rate, 0444, drain_rate_show, NULL);

static struct attribute *auto_cut_attrs[] = {
	&enabled_attr.attr,
	&threshold_attr.attr,
	&recheck_attr.attr,
	&active_attr.attr,
	&current_soc_attr.attr,
	&max_capacity_attr.attr,
	&longevity_mode_attr.attr,
	&cut_count_attr.attr,
	&last_cut_soc_attr.attr,
	&last_cut_time_attr.attr,
	&drain_rate_attr.attr,
	NULL,
};

static struct attribute_group auto_cut_attr_group = {
	.attrs = auto_cut_attrs,
};

static int __init auto_cut_charge_init(void)
{
	int ret;

	auto_cut_kobj = kobject_create_and_add("auto_cut_charge", kernel_kobj);
	if (!auto_cut_kobj)
		return -ENOMEM;

	ret = sysfs_create_group(auto_cut_kobj, &auto_cut_attr_group);
	if (ret) {
		kobject_put(auto_cut_kobj);
		return ret;
	}

	pr_info("Auto Cut Charge initialized (Ultimate version)\n");
	return 0;
}

static void __exit auto_cut_charge_exit(void)
{
	auto_cut_charge_reset();
	kobject_put(auto_cut_kobj);
	pr_info("Auto Cut Charge exited\n");
}

module_init(auto_cut_charge_init);
module_exit(auto_cut_charge_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AnGgIt86");
MODULE_DESCRIPTION("Auto cut charge - stops charging at optimal level with smart features");
