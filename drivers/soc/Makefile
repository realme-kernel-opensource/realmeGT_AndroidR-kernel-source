# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Linux Kernel SOC specific device drivers.
#

obj-$(CONFIG_ARCH_ACTIONS)	+= actions/
obj-$(CONFIG_SOC_ASPEED)	+= aspeed/
obj-$(CONFIG_ARCH_AT91)		+= atmel/
obj-y				+= bcm/
obj-$(CONFIG_ARCH_DOVE)		+= dove/
obj-$(CONFIG_MACH_DOVE)		+= dove/
obj-y				+= fsl/
obj-$(CONFIG_ARCH_GEMINI)	+= gemini/
obj-$(CONFIG_ARCH_MXC)		+= imx/
obj-$(CONFIG_ARCH_IXP4XX)	+= ixp4xx/
obj-$(CONFIG_SOC_XWAY)		+= lantiq/
obj-y				+= mediatek/
obj-y				+= amlogic/
obj-y				+= qcom/
obj-y				+= renesas/
obj-$(CONFIG_ARCH_ROCKCHIP)	+= rockchip/
obj-$(CONFIG_SOC_SAMSUNG)	+= samsung/
obj-y				+= sunxi/
obj-$(CONFIG_ARCH_TEGRA)	+= tegra/
obj-y				+= ti/
obj-$(CONFIG_ARCH_U8500)	+= ux500/
obj-$(CONFIG_PLAT_VERSATILE)	+= versatile/
obj-y				+= xilinx/
obj-$(CONFIG_ARCH_ZX)		+= zte/
#ifdef VENDOR_EDIT
#wen.luo@BSP.Kernel.Stability 2020/03/25 add for oplus kernel system
obj-$(CONFIG_OPLUS_SYSTEM_KERNEL)	+= oplus/system/
#obj-y				+= oplus/device_info/
#ifdef OPLUS_FEATURE_SENSOR_SMEM
#ChenYajie@BSP.SENSOR, 2020/06/16, Add for sensor info
obj-y						+= oplus/sensor/
#endif
#endif /* VENDOR_EDIT */

#ifdef OPLUS_FEATURE_POWERINFO_STANDBY
#Yunqing.Zeng@BSP.Power.Basic, 2020/09/10, add for feature powerinfo standby.
obj-$(CONFIG_OPLUS_WAKELOCK_PROFILER) += oplus/oplus_wakelock/
#endif /* OPLUS_FEATURE_POWERINFO_STANDBY */

#ifdef OPLUS_FEATURE_TP_BASIC
#Qicai.gu@BSP.TP.Init, 2020/08/05, Add for device_info for GKI.
obj-$(CONFIG_OPLUS_DEVICE_IFNO)  +=  oplus/device_info/
#endif /* OPLUS_FEATURE_TP_BASIC */

#ifdef OPLUS_FEATURE_THERMAL_HORAE
obj-$(CONFIG_HORAE_THERMAL_SHELL) += oplus/thermal/
#endif /* OPLUS_FEATURE_THERMAL_HORAE */

#ifdef OPLUS_FEATURE_HOUSTON
#ted.lin@OPTIMIZATION, 2021/01/06, add for houston service
obj-$(CONFIG_OPLUS_FEATURE_HOUSTON) += oplus/houston/
#endif

#ifdef OPLUS_FEATURE_ESIM
#ChenGuoyao@NETWORK.IMS 2020/11/02 add for oplus kernel misc
obj-y += oplus/misc/
#endif /* OPLUS_FEATURE_ESIM */
#ifdef OPLUS_FEATURE_HEALTHINFO
#//wenbin.liu@PSW.Kernel.MM, 2018/05/23, Add for Healthinfo monitor
obj-$(CONFIG_OPLUS_HEALTHINFO)    +=  oplus/healthinfo/
#endif /* OPLUS_FEATURE_HEALTHINFO */
#ifdef OPLUS_FEATURE_MEMLEAK_DETECT
#Add for svelte logging
obj-$(CONFIG_SVELTE) +=  oplus/svelte/
#endif /* OPLUS_FEATURE_MEMLEAK_DETECT */
#ifdef OPLUS_FEATURE_LOWMEM_DBG
obj-$(CONFIG_OPLUS_FEATURE_LOWMEM_DBG) += oplus/lowmem_dbg/
#endif /* OPLUS_FEATURE_LOWMEM_DBG */

#ifdef OPLUS_FEATURE_IM
#ted.lin@optimization add for im module
obj-$(CONFIG_OPLUS_FEATURE_IM) += oplus/im/
#endif /* OPLUS_FEATURE_IM */

#ifdef OPLUS_FEATURE_MIDAS_DEVICE
obj-$(CONFIG_OPLUS_FEATURE_MIDAS_DEVICE)    += oplus/midas/
#endif

#ifdef OPLUS_FEATURE_CPUFREQ_BOUNCING
obj-$(CONFIG_OPLUS_FEATURE_CPUFREQ_BOUNCING) += oplus/cpufreq_bouncing/
#endif

#ifdef OPLUS_FEATURE_TPD
obj-$(CONFIG_OPLUS_FEATURE_TPD) += oplus/tpd/
#endif

#Add for NandSwap
obj-$(CONFIG_NANDSWAP)		+= oplus/oplus_nandswap/

