// SPDX-License-Identifier: GPL-2.0-only
#ifndef __HOUSTON_CORE_H__
#define __HOUSTON_CORE_H__

#include <linux/thermal.h>

/* common config */
#define HT_PER_CLUS_MAX_CORES 8
#define HT_CLUS_NUMS 3
#define MAX_REPORT_PERIOD 18000

/* customize your monitor */
enum {
	HT_TS,
	HT_CLUS_FREQ_0_MIN,
	HT_CLUS_FREQ_0_CUR,
	HT_CLUS_FREQ_0_MAX,
	HT_ISO_0,
	HT_CLUS_FREQ_1_MIN,
	HT_CLUS_FREQ_1_CUR,
	HT_CLUS_FREQ_1_MAX,
	HT_ISO_1,
	HT_CLUS_FREQ_2_MIN,
	HT_CLUS_FREQ_2_CUR,
	HT_CLUS_FREQ_2_MAX,
	HT_ISO_2,
	HT_GPU_FREQ,
	HT_BAT_VOLT_NOW,
	HT_BAT_CURR_NOW,
	HT_HW_INSTRUCTION,
	HT_HW_CACHE_MISS,
	HT_HW_CYCLE,
	HT_CPU_0,
	HT_CPU_1,
	HT_CPU_2,
	HT_CPU_3,
	HT_CPU_4_0,
	HT_CPU_4_1,
	HT_CPU_5_0,
	HT_CPU_5_1,
	HT_CPU_6_0,
	HT_CPU_6_1,
	HT_CPU_7_0,
	HT_CPU_7_1,
	HT_THERM_0,
	HT_THERM_1,
	HT_THERM_2,
	HT_UTIL_0,
	HT_UTIL_1,
	HT_UTIL_2,
	HT_UTIL_3,
	HT_UTIL_4,
	HT_UTIL_5,
	HT_UTIL_6,
	HT_UTIL_7,
	HT_FPS_PROCESS,
	HT_FPS_LAYER,
	HT_FPS_PID,
	HT_FPS_ALIGN,
	HT_FPS_1,
	HT_FPS_2,
	HT_FPS_3,
	HT_FPS_4,
	HT_FPS_5,
	HT_FPS_6,
	HT_FPS_7,
	HT_FPS_8,
	HT_FPS_MISS_LAYER,
	HT_RENDER_PID,
	HT_RENDER_UTIL,
	HT_NT_RTG,
	HT_RTG_UTIL_SUM,
	HT_MONITOR_SIZE
};

/* arch dependent */
#ifndef CONFIG_ARCH_LITO
#define HT_CPUS_PER_CLUS 4
#else
#define HT_CPUS_PER_CLUS 6
#endif

#define CLUS_0_IDX 0

#ifndef CONFIG_ARCH_LITO
#define CLUS_1_IDX 4
#else
#define CLUS_1_IDX 6
#endif

#define CLUS_2_IDX 7

/* common struct */
struct ht_util_pol {
	unsigned long *utils[HT_PER_CLUS_MAX_CORES];
	unsigned long *hi_util;
};

/*
 * ioctl struct
 *
 * fps stabilizer related
 */
struct ht_fps_stabilizer_buf {
	char buf[PAGE_SIZE];
};

struct ht_partial_sys_info {
	u32 volt;
	u32 curr;
	u64 utils[8];
	u32 skin_temp;
};

/*
 * ioctl
 * porting fps stabilizer first
 */
#define HT_IOC_MAGIC 'k'
#define HT_IOC_FPS_STABILIZER_UPDATE _IOR(HT_IOC_MAGIC, 3, struct ht_fps_stabilizer_buf)
#define HT_IOC_FPS_PARTIAL_SYS_INFO _IOR(HT_IOC_MAGIC, 4, struct ht_partial_sys_info)

/* api */
#ifdef CONFIG_OPLUS_FEATURE_HOUSTON
void ht_register_cpu_util(unsigned int cpu, unsigned int first_cpu, unsigned long *util, unsigned long *hi_util);
void ht_register_thermal_zone_device(struct thermal_zone_device *tzd);
#else
static inline void ht_register_cpu_util(unsigned int cpu, unsigned int first_cpu, unsigned long *util, unsigned long *hi_util) {}
static inline void ht_register_thermal_zone_device(struct thermal_zone_device *tzd) {}
#endif

#endif // __HOUSTON_CORE_H__
