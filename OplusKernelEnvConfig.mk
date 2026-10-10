# Copyright (C), 2008-2030, OPPO Mobile Comm Corp., Ltd
### All rights reserved.
###
### File: - OplusKernelEnvConfig.mk
### Description:
###     Global feature macros for the Oplus code that is present in this tree.
###     Reduced version of the vendor file: only the macros that are really
###     referenced by the ported code are defined here. Macros that belong to
###     features that were not ported (charger, touch, fingerprint, camera,
###     audio, ufs/mmc/sdcard, modem/pil, secure guard, filesystems, ...)
###     are intentionally NOT defined.
###
##################################################################################

ALLOWED_MCROS := \
OPLUS_FEATURE_PERFORMANCE \
OPLUS_FEATURE_TASK_CPUSTATS \
OPLUS_FEATURE_HANS_FREEZE \
OPLUS_FEATURE_SCHED_ASSIST \
OPLUS_FEATURE_IOMONITOR \
OPLUS_FEATURE_LOWMEM_DBG \
OPLUS_FEATURE_MULTI_KSWAPD \
OPLUS_FEATURE_WIFI_MTUDETECT \
OPLUS_FEATURE_SELINUX_CONTROL_LOG \
OPLUS_FEATURE_MULTI_FREEAREA \
OPLUS_FEATURE_VIRTUAL_RESERVE_MEMORY \
OPLUS_FEATURE_PROCESS_RECLAIM \
OPLUS_FEATURE_ZRAM_OPT \
OPLUS_FEATURE_MEMLEAK_DETECT \
OPLUS_BUG_STABILITY \
VENDOR_EDIT \
OPLUS_FEATURE_POWERINFO_FTM \
OPLUS_FEATURE_POWERINFO_STANDBY \
OPLUS_FEATURE_POWERINFO_STANDBY_DEBUG \
OPLUS_FEATURE_POWERINFO_RPMH \
OPLUS_FEATURE_HEALTHINFO

$(foreach myfeature,$(ALLOWED_MCROS),\
         $(eval KBUILD_CFLAGS += -D$(myfeature)) \
         $(eval KBUILD_CPPFLAGS += -D$(myfeature)) \
         $(eval CFLAGS_KERNEL += -D$(myfeature)) \
         $(eval CFLAGS_MODULE += -D$(myfeature)) \
)
