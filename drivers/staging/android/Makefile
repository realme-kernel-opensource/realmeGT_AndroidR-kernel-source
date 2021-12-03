# SPDX-License-Identifier: GPL-2.0
ccflags-y += -I$(src)			# needed for trace events

obj-y					+= ion/

obj-$(CONFIG_ASHMEM)			+= ashmem.o
obj-$(CONFIG_ANDROID_VSOC)		+= vsoc.o

#ifdef OPLUS_FEATURE_HANS_FREEZE
#Kun.Zhou@ANDROID.RESCONTROL, 2019/09/23, add for hans freeze manager
obj-$(CONFIG_OPLUS_FEATURE_HANS) += hans.o
obj-$(CONFIG_OPLUS_FEATURE_HANS) += hans_netfilter.o
#endif /*OPLUS_FEATURE_HANS_FREEZE*/
