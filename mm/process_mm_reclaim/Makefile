# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2018-2021 Oplus. All rights reserved.

ifeq ($(OPLUS_FEATURE_PROCESS_RECLAIM), yes)
obj-y += process_mm_reclaim.o
else
obj-y += process_mm_reclaim_weak.o
endif
