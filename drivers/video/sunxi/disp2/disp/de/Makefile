sub_dir =

ifeq ($(CONFIG_ARCH_SUN50IW1P1),y)
sub_dir = lowlevel_sun50iw1
endif

ifeq ($(CONFIG_ARCH_SUN8IW11),y)
sub_dir = lowlevel_sun8iw11
endif

ifneq ($(sub_dir),)
-include $(obj)/de/$(sub_dir)/Makefile
endif

