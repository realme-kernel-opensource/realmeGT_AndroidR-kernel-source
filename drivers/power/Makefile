# SPDX-License-Identifier: GPL-2.0-only
obj-$(CONFIG_POWER_AVS)		+= avs/
obj-$(CONFIG_POWER_RESET)	+= reset/
obj-$(CONFIG_POWER_SUPPLY)	+= supply/
#ifdef OPLUS_FEATURE_CHG_BASIC /* Jianchao.Shi@BSP.CHG.Basic, 2018/01/12, sjc Add for charging */

ifeq ($(strip $(BRAND_SHOW_FLAG)), oneplus)
	obj-$(CONFIG_OPLUS_SM8350_CHARGER)	+= oplus_chg/
else
	obj-$(CONFIG_OPLUS_SM8350_CHARGER)	+= oplus/
endif
#endif
