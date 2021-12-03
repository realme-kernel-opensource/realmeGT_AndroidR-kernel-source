# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2018-2020 Oplus. All rights reserved.

#ifdef OPLUS_FEATURE_MIDAS
oplus_midas-objs := midas_proc.o
obj-$(CONFIG_OPLUS_FEATURE_MIDAS) += oplus_midas.o
#endif /* OPLUS_FEATURE_MIDAS */

#ifdef OPLUS_FEATURE_MIDAS_DEVICE
oplus_midas_device-objs := midas_dev.o midas_ioctl.o
obj-$(CONFIG_OPLUS_FEATURE_MIDAS_DEVICE) += oplus_midas_device.o
#endif /* OPLUS_FEATURE_MIDAS_DEVICE */

oplus_binder_stats-objs := binder_stats_dev.o
obj-$(CONFIG_OPLUS_FEATURE_BINDER_STATS_ENABLE) += oplus_binder_stats.o
