# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2018-2021 Oplus. All rights reserved.
#
# Makefile for the netfilter modules on top of IPv4.
#
KBUILD_OPTIONS += CONFIG_OPLUS_FEATURE_DATA_MODULE=m

KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
M ?= $(shell pwd)
modules modules_install clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M) $(KBUILD_OPTIONS) $(@)
