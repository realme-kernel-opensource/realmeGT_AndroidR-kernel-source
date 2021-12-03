// SPDX-License-Identifier: GPL-2.0-only
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/wait.h>

#include "houston.h"

#ifndef pr_fmt
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#endif

// tag
#define HT_CTL_NODE "ht_ctl"
#define HT_TAG "ht_monitor: "

/*
 * log helper
 * lv == 0 -> verbose info warning error
 * lv == 1 -> info warning error
 * lv == 2 -> wraning error
 * lv >= 3 -> error
 */
#define ht_logv(fmt...) \
	do { \
		if (ht_log_lv < 1) \
			pr_info(HT_TAG fmt); \
	} while (0)

#define ht_logi(fmt...) \
	do { \
		if (ht_log_lv < 2) \
			pr_info(HT_TAG fmt); \
	} while (0)

#define ht_logw(fmt...) \
	do { \
		if (ht_log_lv < 3) \
			pr_warn(HT_TAG fmt); \
	} while (0)

#define ht_loge(fmt...) pr_err(HT_TAG fmt)
#define ht_logd(fmt...) pr_debug(HT_TAG fmt)

static int ht_log_lv = 1;
module_param_named(log_lv, ht_log_lv, int, 0664);

/* Need to align housotn.h HT_MONITOR_SIZE */
static const char *ht_monitor_case[HT_MONITOR_SIZE] = {
	"ts",
	"clus_0_min", "clus_0_cur", "clus_0_max", "clus_0_iso",
	"clus_1_min", "clus_1_cur", "clus_1_max", "clus_1_iso",
	"clus_2_min", "clus_2_cur", "clus_2_max", "clus_2_iso",
	"gpu_cur", "voltage_now", "current_now", "hw_instruction",
	"hw_cache_miss", "hw_cycle",
	"cpu-0-0-usr", "cpu-0-1-usr", "cpu-0-2-usr", "cpu-0-3-usr",
	"cpu-1-0-usr", "cpu-1-1-usr", "cpu-1-2-usr", "cpu-1-3-usr",
	"cpu-1-4-usr", "cpu-1-5-usr", "cpu-1-6-usr", "cpu-1-7-usr",
	"shell_front", "shell_frame", "shell_back",
	"util-0", "util-1", "util-2", "util-3", "util-4",
	"util-5", "util-6", "util-7",
	"process name", "layer name", "pid", "fps_align", "actualFps",
	"predictFps", "Vsync", "gameFps",
	"NULL", "NULL", "NULL",
	"NULL", "missedLayer", "render_pid", "render_util",
	"nt_rtg", "rtg_util_sum"
};

/*
 * houston monitor
 * data: sample data
 * layer: sample data for frame info
 * process: sample data for frame process info
 */
struct sample_data {
	u64 data[MAX_REPORT_PERIOD][HT_MONITOR_SIZE];
};

struct ht_monitor {
	struct power_supply *psy;
	struct thermal_zone_device* tzd[HT_MONITOR_SIZE];
	struct task_struct *thread;
	struct sample_data *buf;
} monitor = {
	.psy = NULL,
	.thread = NULL,
	.buf = NULL,
};

/* TODO parse instead */
/* cpu utils */
static struct ht_util_pol ht_utils[HT_CLUS_NUMS];

void ht_register_cpu_util(unsigned int cpu, unsigned int first_cpu,
		unsigned long *util, unsigned long *hi_util)
{
	struct ht_util_pol *hus;

	int i = 0;
	for (i = 0; i < 30; ++i)
		pr_err("ht register cpu %d util, first cpu %d\n", cpu, first_cpu);

	switch (first_cpu) {
#ifndef CONFIG_ARCH_LITO
	case 0: case 1: case 2: case 3: hus = &ht_utils[0]; break;
	case 4: case 5: case 6: hus = &ht_utils[1]; break;
#else
	case 0: case 1: case 2: case 3: case 4: case 5: hus = &ht_utils[0]; break;
	case 6: hus = &ht_utils[1]; break;
#endif
	case 7: hus = &ht_utils[2]; break;
	default:
		/* should not happen */
		ht_logw("wrnog first cpu utils idx %d\n", first_cpu);
		return;
	}

#ifndef CONFIG_ARCH_LITO
	if (cpu == CLUS_2_IDX)
#else
	if (cpu == CLUS_1_IDX || cpu == CLUS_2_IDX)
#endif
		cpu = 0;
	else
		cpu %= HT_CPUS_PER_CLUS;
	hus->utils[cpu] = util;
	hus->hi_util = hi_util;
	ht_logv("ht register cpu %d util, first cpu %d\n", cpu, first_cpu);
}

/*
 * for fps stabilizer interface
 * fps stable parameter, default -1 max
 */
static int efps_max = -1;
static int efps_pid = 0;
static int expectfps = -1;

#define EGL_BUF_MAX (128)
static char egl_buf[EGL_BUF_MAX] = {0};
static DEFINE_SPINLOCK(egl_buf_lock);

static int egl_buf_store(const char *buf, const struct kernel_param *kp)
{
	char _buf[EGL_BUF_MAX] = {0};

	if (strlen(buf) >= EGL_BUF_MAX)
		return 0;

	if (sscanf(buf, "%s\n", _buf) <= 0)
		return 0;

	spin_lock(&egl_buf_lock);
	memcpy(egl_buf, _buf, EGL_BUF_MAX);
	egl_buf[EGL_BUF_MAX - 1] = '\0';
	spin_unlock(&egl_buf_lock);

	return 0;
}

static int egl_buf_show(char *buf, const struct kernel_param *kp)
{
	char _buf[EGL_BUF_MAX] = {0};

	spin_lock(&egl_buf_lock);
	memcpy(_buf, egl_buf, EGL_BUF_MAX);
	_buf[EGL_BUF_MAX - 1] = '\0';
	spin_unlock(&egl_buf_lock);

	return snprintf(buf, PAGE_SIZE, "%s\n", _buf);
}

static struct kernel_param_ops egl_buf_ops = {
	.set = egl_buf_store,
	.get = egl_buf_show,
};
module_param_cb(egl_buf, &egl_buf_ops, NULL, 0664);

/* efps max used for debug puropse, check return value to analysis */
static int efps_max_store(const char *buf, const struct kernel_param *kp)
{
	int val, pid, eval, ret;

	ret = sscanf(buf, "%d,%d,%d\n", &pid, &val, &eval);
	if (ret < 2)
		return 0;

	efps_pid = pid;
	efps_max = val;
	expectfps = (ret == 3) ? eval : -1;
	return 0;
}

static int efps_max_show(char *buf, const struct kernel_param *kp)
{
	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n", efps_pid, efps_max, expectfps);
}

static struct kernel_param_ops efps_max_ops = {
	.set = efps_max_store,
	.get = efps_max_show,
};
module_param_cb(efps_max, &efps_max_ops, NULL, 0664);

/* fps stabilizer update & online config update */
static DECLARE_WAIT_QUEUE_HEAD(ht_fps_stabilizer_waitq);
static char ht_online_config_buf[PAGE_SIZE];
static bool ht_disable_fps_stabilizer_bat = true;
module_param_named(disable_fps_stabilizer_bat, ht_disable_fps_stabilizer_bat, bool, 0664);

static int ht_online_config_update_store(const char *buf, const struct kernel_param *kp)
{
	int ret;

	ret = sscanf(buf, "%s\n", ht_online_config_buf);
	ht_logi("fpsst: %d, %s\n", ret, ht_online_config_buf);
	wake_up(&ht_fps_stabilizer_waitq);
	return 0;
}

static int ht_online_config_update_show(char *buf, const struct kernel_param *kp)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", ht_online_config_buf);
}

static struct kernel_param_ops ht_online_config_update_ops = {
	.set = ht_online_config_update_store,
	.get = ht_online_config_update_show,
};
module_param_cb(ht_online_config_update, &ht_online_config_update_ops, NULL, 0664);


static inline int ht_mapping_tags(char *name)
{
	int i;

	for (i = 0; i < HT_MONITOR_SIZE; ++i)
		if (!strcmp(name, ht_monitor_case[i]))
			return i;

	return HT_MONITOR_SIZE;
}

static int ht_tzd_idx = HT_CPU_0;
void ht_register_thermal_zone_device(struct thermal_zone_device *tzd)
{
	int idx;
	int i = 0;

	/* tzd is guaranteed has value */
	ht_logi("tzd: %s id: %d\n", tzd->type, tzd->id);
	for (i = 0; i < 30; ++i)
		pr_err("tzd: %s id: %d\n", tzd->type, tzd->id);

	idx = ht_mapping_tags(tzd->type);

	if (idx > HT_THERM_2)
		return;
	if (ht_tzd_idx <= HT_THERM_2 && !monitor.tzd[idx]) {
		++ht_tzd_idx;
		monitor.tzd[idx] = tzd;
	}
}
EXPORT_SYMBOL(ht_register_thermal_zone_device);

static inline int ht_get_temp(int monitor_idx)
{
	int temp = 0;

	if (unlikely(!monitor.tzd[monitor_idx]))
		return 0;

	if (thermal_zone_get_temp(monitor.tzd[monitor_idx], &temp)) {
		ht_logv("failed to read out thermal zone with idx %d\n", monitor.tzd[monitor_idx]->id);
		return 0;
	}

	return temp;
}

/* report skin_temp to ais */
static unsigned int thermal_update_period_hz = 100;
module_param_named(thermal_update_period_hz, thermal_update_period_hz, uint, 0664);

static unsigned int ht_get_temp_delay(int idx)
{
	static unsigned long next[HT_MONITOR_SIZE] = {0};
	static unsigned int temps[HT_MONITOR_SIZE] = {0};

	/* only allow for reading sensor data */
	if (unlikely(idx < HT_CPU_0 || idx > HT_THERM_2))
		return 0;

	/* update */
	if (jiffies > next[idx] && jiffies - next[idx] > thermal_update_period_hz) {
		next[idx] = jiffies;
		temps[idx] = ht_get_temp(idx);
	}

	if (jiffies < next[idx]) {
		next[idx] = jiffies;
		temps[idx] = ht_get_temp(idx);
	}

	return temps[idx];
}

static inline const char* ht_ioctl_str(unsigned int cmd)
{
	switch (cmd) {
	case HT_IOC_FPS_STABILIZER_UPDATE: return "HT_IOC_FPS_STABILIZER_UPDATE";
	case HT_IOC_FPS_PARTIAL_SYS_INFO: return "HT_IOC_FPS_PARTIAL_SYS_INFO";
	}
	return "UNKNOWN";
}

static long ht_ctl_ioctl(struct file *file, unsigned int cmd, unsigned long __user arg)
{
	if (_IOC_TYPE(cmd) != HT_IOC_MAGIC) return 0;

	ht_logv("%s: cmd: %s %x, arg: %lu\n", __func__, ht_ioctl_str(cmd), cmd, arg);

	switch (cmd) {
	case HT_IOC_FPS_STABILIZER_UPDATE:
	{
		DEFINE_WAIT(wait);
		prepare_to_wait(&ht_fps_stabilizer_waitq, &wait, TASK_INTERRUPTIBLE);
		schedule();
		finish_wait(&ht_fps_stabilizer_waitq, &wait);

		ht_logi("fpsst: %s\n", ht_online_config_buf);

		if (ht_online_config_buf[0] == '\0') {
			// force invalid config
			char empty[] = "{}";
			copy_to_user((struct ht_fps_stabilizer_buf __user *) arg, &empty, strlen(empty));
		} else {
			copy_to_user((struct ht_fps_stabilizer_buf __user *) arg, &ht_online_config_buf, PAGE_SIZE);
		}
		break;
	}
	case HT_IOC_FPS_PARTIAL_SYS_INFO:
	{
		struct ht_partial_sys_info data;

		// fps stabilizer don't need battery info
		data.volt = data.curr = 0;

		// related to cpu cluster configuration
		// clus 0
		data.utils[0] = ht_utils[0].utils[0]? (u64) *(ht_utils[0].utils[0]): 0;
		data.utils[1] = ht_utils[0].utils[1]? (u64) *(ht_utils[0].utils[1]): 0;
		data.utils[2] = ht_utils[0].utils[2]? (u64) *(ht_utils[0].utils[2]): 0;
		data.utils[3] = ht_utils[0].utils[3]? (u64) *(ht_utils[0].utils[3]): 0;
		// clus 1
		data.utils[4] = ht_utils[1].utils[0]? (u64) *(ht_utils[1].utils[0]): 0;
		data.utils[5] = ht_utils[1].utils[1]? (u64) *(ht_utils[1].utils[1]): 0;
		data.utils[6] = ht_utils[1].utils[2]? (u64) *(ht_utils[1].utils[2]): 0;
		// clus 2
		data.utils[7] = ht_utils[2].utils[0]? (u64) *(ht_utils[2].utils[0]): 0;

		// pick highest temp
		data.skin_temp = max(ht_get_temp_delay(HT_THERM_0),
							max(ht_get_temp_delay(HT_THERM_1),
								ht_get_temp_delay(HT_THERM_2)));

		if (copy_to_user((struct ht_partial_sys_info __user *) arg, &data, sizeof(struct ht_partial_sys_info)))
			return 0;
		break;
	}
	default:
	{
		// handle unsupported ioctl cmd
		ht_logv("ioctl not support yet\n");
		return -1;
	}
	}
	return 0;
}

static const struct file_operations ht_ctl_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = ht_ctl_ioctl,
	.compat_ioctl = ht_ctl_ioctl,
};

static dev_t ht_ctl_dev;
static struct class *driver_class;
static struct cdev cdev;
static int ht_fops_init(void)
{
	int rc;
	struct device *class_dev;

	rc = alloc_chrdev_region(&ht_ctl_dev, 0, 1, HT_CTL_NODE);
	if (rc < 0) {
		ht_loge("alloc_chrdev_region failed %d\n", rc);
		goto exit;
	}

	driver_class = class_create(THIS_MODULE, HT_CTL_NODE);
	if (IS_ERR(driver_class)) {
		rc = -ENOMEM;
		ht_loge("class_create failed %d\n", rc);
		goto exit_unreg_chrdev_region;
	}

	class_dev = device_create(driver_class, NULL, ht_ctl_dev, NULL, HT_CTL_NODE);
	if (IS_ERR(class_dev)) {
		ht_loge("class_device_create failed %d\n", rc);
		rc = -ENOMEM;
		goto exit_destroy_class;
	}

	cdev_init(&cdev, &ht_ctl_fops);
	cdev.owner = THIS_MODULE;
	rc = cdev_add(&cdev, MKDEV(MAJOR(ht_ctl_dev), 0), 1);
	if (rc < 0) {
		ht_loge("cdev_add failed %d\n", rc);
		goto exit_destroy_device;
	}

	ht_logi("ht_fops_inited\n");

	return 0;

exit_destroy_device:
	device_destroy(driver_class, ht_ctl_dev);
exit_destroy_class:
	class_destroy(driver_class);
exit_unreg_chrdev_region:
	unregister_chrdev_region(ht_ctl_dev, 1);
exit:
	ht_logw("ht_fops_init failed\n");

	return -1;
}

static int ht_init(void)
{
	ht_logi("ht_init\n");
	ht_fops_init();
	return 0;
}
pure_initcall(ht_init);
