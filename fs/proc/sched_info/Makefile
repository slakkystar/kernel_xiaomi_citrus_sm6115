# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2018-2021 Oplus. All rights reserved.

LINUXINCLUDE += -I$(srctree)/

obj-$(CONFIG_OPLUS_FEATURE_CPU_JANKINFO) += oplus_schedinfo.o
oplus_schedinfo-y += oplus_sched_info.o
oplus_schedinfo-y += osi_base.o
oplus_schedinfo-y += osi_tasktrack.o
oplus_schedinfo-y += osi_topology.o
oplus_schedinfo-y += osi_hotthread.o
oplus_schedinfo-y += osi_loadindicator.o
oplus_schedinfo-y += osi_onlinecpu.o
oplus_schedinfo-y += osi_freq.o
oplus_schedinfo-y += osi_cpuload.o
oplus_schedinfo-y += osi_enable.o
oplus_schedinfo-y += osi_debug.o
oplus_schedinfo-y += osi_version.o
oplus_schedinfo-y += osi_cpuloadmonitor.o
oplus_schedinfo-y += osi_netlink.o
oplus_schedinfo-$(CONFIG_JANK_CPUSET) += osi_cpuset.o
