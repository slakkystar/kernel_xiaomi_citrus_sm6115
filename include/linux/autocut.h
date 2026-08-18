/*
 * auto_cut_charge.h - Auto cut charge feature for battery protection
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 */

#ifndef _LINUX_AUTO_CUT_CHARGE_H
#define _LINUX_AUTO_CUT_CHARGE_H

#ifdef CONFIG_AUTO_CUT_CHARGE

struct smb_charger;

extern int auto_cut_charge_enabled;
extern int auto_cut_charge_threshold;
extern int auto_cut_charge_recheck_soc;
extern int auto_cut_charge_current_soc;
extern bool auto_cut_charge_active;

void auto_cut_charge_check(struct smb_charger *chg, int soc, int current_ma, int temp);
void auto_cut_charge_update_soc(int soc);
void auto_cut_charge_reset(void);

#else /* CONFIG_AUTO_CUT_CHARGE */

static inline void auto_cut_charge_check(struct smb_charger *chg) { }
static inline void auto_cut_charge_update_soc(int soc) { }
static inline void auto_cut_charge_reset(void) { }

#endif /* CONFIG_AUTO_CUT_CHARGE */

#endif /* _LINUX_AUTO_CUT_CHARGE_H */
