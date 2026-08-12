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

#include "../../drivers/power/supply/qcom/smb5-lib.h"

#define DEFAULT_CUT_THRESHOLD_SOC      100
#define DEFAULT_RECHECK_SOC            95
#define DEFAULT_CHECK_INTERVAL_MS      600000

int auto_cut_charge_enabled = 1;
int auto_cut_charge_threshold = DEFAULT_CUT_THRESHOLD_SOC;
int auto_cut_charge_recheck_soc = DEFAULT_RECHECK_SOC;
int auto_cut_charge_current_soc = 0;
bool auto_cut_charge_active = false;

static struct smb_charger *global_chg = NULL;
static struct delayed_work check_work;
static bool work_scheduled = false;
static struct kobject *auto_cut_kobj;

module_param(auto_cut_charge_enabled, int, 0644);
module_param(auto_cut_charge_threshold, int, 0644);
module_param(auto_cut_charge_recheck_soc, int, 0644);
MODULE_PARM_DESC(auto_cut_charge_enabled, "Enable auto cut charge (0=off, 1=on)");
MODULE_PARM_DESC(auto_cut_charge_threshold, "Battery percentage to stop charging (default 100)");
MODULE_PARM_DESC(auto_cut_charge_recheck_soc, "Battery percentage to resume charging (default 95)");

static void check_work_handler(struct work_struct *work);
static DECLARE_DELAYED_WORK(check_work, check_work_handler);

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
	cancel_delayed_work_sync(&check_work);
	work_scheduled = false;
}

void auto_cut_charge_check(struct smb_charger *chg)
{
	union power_supply_propval val;
	int soc = 0;
	int rc;
	bool should_cut = false;
	static int last_action = 0;

	if (!auto_cut_charge_enabled || !chg) {
		if (work_scheduled) {
			cancel_delayed_work_sync(&check_work);
			work_scheduled = false;
		}
		global_chg = NULL;
		return;
	}

	if (!global_chg)
		global_chg = chg;

	rc = smblib_get_prop_from_bms(chg, POWER_SUPPLY_PROP_CAPACITY, &val);
	if (rc < 0) {
		pr_err_ratelimited("Auto Cut: Failed to get battery SOC rc=%d\n", rc);
		return;
	}
	soc = val.intval;
	auto_cut_charge_current_soc = soc;

	if (!auto_cut_charge_active) {
		if (soc >= auto_cut_charge_threshold) {
			should_cut = true;
		}
	} else {
		if (soc <= auto_cut_charge_recheck_soc) {
			should_cut = false;
			auto_cut_charge_active = false;
			vote(global_chg->chg_disable_votable, AUTO_CUT_VOTER, false, 0);
			pr_info("Auto Cut Charge: Battery at %d%%, resuming charging\n", soc);
			last_action = 2;
		}
	}

	if (should_cut) {
		auto_cut_charge_active = true;
		vote(global_chg->chg_disable_votable, AUTO_CUT_VOTER, true, 0);
		pr_info("Auto Cut Charge: Battery at %d%%, charging stopped\n", soc);
		last_action = 1;
	}

	if (!work_scheduled) {
		schedule_delayed_work(&check_work, msecs_to_jiffies(DEFAULT_CHECK_INTERVAL_MS));
		work_scheduled = true;
	}
}

static void check_work_handler(struct work_struct *work)
{
	if (!auto_cut_charge_enabled || !global_chg) {
		work_scheduled = false;
		return;
	}

	auto_cut_charge_check(global_chg);

	if (auto_cut_charge_enabled && global_chg) {
		schedule_delayed_work(&check_work, msecs_to_jiffies(DEFAULT_CHECK_INTERVAL_MS));
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

static struct attribute *auto_cut_attrs[] = {
	&enabled_attr.attr,
	&threshold_attr.attr,
	&recheck_attr.attr,
	&active_attr.attr,
	&current_soc_attr.attr,
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

	pr_info("Auto Cut Charge initialized\n");
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
MODULE_DESCRIPTION("Auto cut charge - stops charging at 100%% battery");
