# Makefile for the focaltech touchscreen drivers.
ccflags-y += $(DISABLE_LTO_CLANG) $(DISABLE_CFI_CLANG) -fno-data-sections -fno-function-sections

obj-$(CONFIG_TOUCHSCREEN_FT8719)	+=  focaltech_core.o
obj-$(CONFIG_TOUCHSCREEN_FT8719)	+=  focaltech_ex_fun.o
obj-$(CONFIG_TOUCHSCREEN_FT8719)	+=  focaltech_ex_mode.o
obj-$(CONFIG_TOUCHSCREEN_FT8719)	+=  focaltech_gesture.o
obj-$(CONFIG_TOUCHSCREEN_FT8719)	+=  focaltech_esdcheck.o
obj-$(CONFIG_TOUCHSCREEN_FT8719)   +=  focaltech_point_report_check.o
obj-$(CONFIG_TOUCHSCREEN_FT8719)   +=  focaltech_test/



obj-$(CONFIG_TOUCHSCREEN_FT8719)	+=  focaltech_flash.o




obj-$(CONFIG_TOUCHSCREEN_FT8719)   +=  focaltech_spi.o
