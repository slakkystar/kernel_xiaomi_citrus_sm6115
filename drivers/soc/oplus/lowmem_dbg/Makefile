# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2018-2021 Oplus. All rights reserved.

# gki
CONFIG_KERVER_VERSION := $(VERSION).$(PATCHLEVEL)

ifeq ($(CONFIG_KERVER_VERSION),5.4)
$(info lowmem_dbg mtk_ion use kernel $(CONFIG_KERVER_VERSION))
obj-$(CONFIG_OPLUS_FEATURE_LOWMEM_DBG)   += lowmem_dbg_5.4.o
else
$(info lowmem_dbg mtk_ion use kernel $(CONFIG_KERVER_VERSION))
obj-$(CONFIG_OPLUS_FEATURE_LOWMEM_DBG)   += lowmem_dbg.o
endif


# mtk
ifeq ($(CONFIG_MTK_ION), y)
$(info lowmem_dbg mtk_ion use kernel $(CONFIG_KERVER_VERSION))
ifeq ($(CONFIG_KERVER_VERSION),4.19)
ccflags-$(CONFIG_MTK_ION)  += -I$(srctree)/drivers/staging/android/mtk_ion/
endif
ifeq ($(CONFIG_KERVER_VERSION),4.14)
ccflags-$(CONFIG_MTK_ION)  += -I$(srctree)/drivers/staging/android/ion/
endif
endif

# qcom
ccflags-$(CONFIG_QCOM_KGSL)  += -I$(srctree)/drivers/gpu/msm/
